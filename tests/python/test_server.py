import asyncio
import importlib
import json
import os
import sys
import types
import unittest
from pathlib import Path
from tempfile import TemporaryDirectory


def _print_response(request_name, response):
    print(f"{request_name} response: {json.dumps(response, sort_keys=True)}", flush=True)


def _print_http_error(request_name, exc):
    response = {"status_code": exc.status_code, "detail": exc.detail}
    _print_response(request_name, response)


def _install_fastapi_stubs():
    fastapi_module = types.ModuleType("fastapi")

    class HTTPException(Exception):
        def __init__(self, status_code, detail):
            super().__init__(detail)
            self.status_code = status_code
            self.detail = detail

    class Request:
        pass

    class FastAPI:
        def get(self, *_args, **_kwargs):
            def decorator(func):
                return func

            return decorator

        def post(self, *_args, **_kwargs):
            def decorator(func):
                return func

            return decorator

    responses_module = types.ModuleType("fastapi.responses")

    class StreamingResponse:
        def __init__(self, body_iterator, media_type=None):
            self.body_iterator = body_iterator
            self.media_type = media_type

    fastapi_module.FastAPI = FastAPI
    fastapi_module.HTTPException = HTTPException
    fastapi_module.Request = Request
    responses_module.StreamingResponse = StreamingResponse

    sys.modules["fastapi"] = fastapi_module
    sys.modules["fastapi.responses"] = responses_module


def _install_uvicorn_stub():
    uvicorn_module = types.ModuleType("uvicorn")
    uvicorn_module.run = lambda *_args, **_kwargs: None
    sys.modules["uvicorn"] = uvicorn_module


class ProxyStoreTests(unittest.IsolatedAsyncioTestCase):
    def setUp(self):
        self.tempdir = TemporaryDirectory()
        self.addCleanup(self.tempdir.cleanup)
        self.addCleanup(lambda: os.environ.pop("WRENS_NEST_CLIENTS_FILE", None))
        self.addCleanup(lambda: sys.modules.pop("server.server", None))
        self.addCleanup(lambda: sys.modules.pop("fastapi", None))
        self.addCleanup(lambda: sys.modules.pop("fastapi.responses", None))
        self.addCleanup(lambda: sys.modules.pop("uvicorn", None))

        self.clients_file = Path(self.tempdir.name) / "clients.json"
        self.clients_file.write_text("{}\n", encoding="utf-8")
        os.environ["WRENS_NEST_CLIENTS_FILE"] = str(self.clients_file)

        _install_fastapi_stubs()
        _install_uvicorn_stub()

        import server.server as server_module

        self.server = importlib.reload(server_module)
        self.server.state.clients = {}
        self.server.state.runtime_sessions = {}
        self.server.state.close_tasks = {}
        self.server.state.command_close_delay_seconds = 0.01

    async def asyncTearDown(self):
        for task in self.server.state.close_tasks.values():
            task.cancel()
        await asyncio.gather(*self.server.state.close_tasks.values(), return_exceptions=True)

    async def test_register_allocates_zero_and_persists(self):
        uid = await self.server.state.register()
        _print_response("register", {"uid": uid})

        self.assertEqual(uid, 0)
        saved = json.loads(self.clients_file.read_text(encoding="utf-8"))
        self.assertEqual(saved["0"]["status"], 1)
        self.assertEqual(saved["0"]["servers"], {})
        self.assertIn("created_at", saved["0"])
        self.assertIn("updated_at", saved["0"])

    async def test_register_skips_existing_registered_uids(self):
        self.server.state.clients["0"] = {"status": 0, "created_at": "a", "updated_at": "b"}

        uid = await self.server.state.register()
        _print_response("register with existing uid", {"uid": uid})

        self.assertEqual(uid, 1)
        self.assertIn("1", self.server.state.clients)

    async def test_register_server_allocates_ids_within_uid(self):
        await self.server.state.register()

        first_server_id = await self.server.state.register_server(0)
        second_server_id = await self.server.state.register_server(0)
        _print_response(
            "register servers",
            {"uid": 0, "server_ids": [first_server_id, second_server_id]},
        )

        self.assertEqual(first_server_id, 0)
        self.assertEqual(second_server_id, 1)
        self.assertEqual(self.server.state.clients["0"]["servers"]["0"]["status"], 1)
        self.assertEqual(self.server.state.clients["0"]["servers"]["1"]["status"], 1)

    async def test_command_queues_opaque_packet_and_marks_uid_online(self):
        await self.server.state.register()
        await self.server.state.register_server(0)

        command = await self.server.state.enqueue_command(
            0,
            0,
            {
                "command_id": "cmd-1",
                "metadata": {"source": "ui"},
                "packet": {"iv": "abc", "text": "ciphertext"},
            },
        )
        _print_response("enqueue command", command)

        self.assertEqual(command["command_id"], "cmd-1")
        self.assertEqual(command["server_id"], 0)
        self.assertEqual(self.server.state.clients["0"]["status"], 0)
        self.assertEqual(self.server.state.clients["0"]["servers"]["0"]["status"], 0)
        queued_command = self.server.state.runtime_sessions["0:0"].command_queue.get_nowait()
        self.assertEqual(queued_command["packet"], {"iv": "abc", "text": "ciphertext"})
        self.assertEqual(queued_command["metadata"], {"source": "ui"})

    async def test_command_queues_are_isolated_by_server_id(self):
        await self.server.state.register()
        await self.server.state.register_server(0)
        await self.server.state.register_server(0)

        await self.server.state.enqueue_command(
            0,
            1,
            {"command_id": "cmd-2", "packet": {"iv": "def", "text": "second"}},
        )

        self.assertNotIn("0:0", self.server.state.runtime_sessions)
        queued_command = self.server.state.runtime_sessions["0:1"].command_queue.get_nowait()
        self.assertEqual(queued_command["server_id"], 1)
        self.assertEqual(queued_command["packet"], {"iv": "def", "text": "second"})

    async def test_command_closes_server_after_last_posted_command_timeout(self):
        await self.server.state.register()
        await self.server.state.register_server(0)

        await self.server.state.enqueue_command(
            0,
            0,
            {"command_id": "cmd-1", "packet": {"iv": "abc", "text": "first"}},
        )
        await self.server.state.enqueue_command(
            0,
            0,
            {"command_id": "cmd-2", "packet": {"iv": "def", "text": "second"}},
        )

        self.assertEqual(self.server.state.clients["0"]["status"], 0)
        self.assertEqual(self.server.state.clients["0"]["servers"]["0"]["status"], 0)
        await asyncio.sleep(0.03)
        self.assertEqual(self.server.state.clients["0"]["status"], 1)
        self.assertEqual(self.server.state.clients["0"]["servers"]["0"]["status"], 1)

    async def test_response_queues_opaque_packet(self):
        await self.server.state.register()
        await self.server.state.register_server(0)

        response = await self.server.state.enqueue_response(
            0,
            0,
            {
                "command_id": "cmd-1",
                "metadata": {"source": "agent"},
                "packet": {"iv": "xyz", "text": "encrypted-result"},
            },
        )
        _print_response("enqueue response", response)

        self.assertEqual(response["packet"], {"iv": "xyz", "text": "encrypted-result"})
        self.assertEqual(response["server_id"], 0)
        queued_response = self.server.state.runtime_sessions["0:0"].response_queue.get_nowait()
        self.assertEqual(queued_response["command_id"], "cmd-1")
        self.assertEqual(queued_response["packet"], {"iv": "xyz", "text": "encrypted-result"})

    async def test_command_requires_encrypted_packet(self):
        await self.server.state.register()
        await self.server.state.register_server(0)

        with self.assertRaises(self.server.HTTPException) as exc:
            await self.server.state.enqueue_command(0, 0, {"metadata": {"source": "ui"}})

        _print_http_error("enqueue command missing payload", exc.exception)
        self.assertEqual(exc.exception.status_code, 400)
        self.assertEqual(exc.exception.detail, "missing encrypted packet")

    async def test_response_requires_command_id(self):
        await self.server.state.register()
        await self.server.state.register_server(0)

        with self.assertRaises(self.server.HTTPException) as exc:
            await self.server.state.enqueue_response(0, 0, {"stdout": "missing id"})

        _print_http_error("enqueue response missing command_id", exc.exception)
        self.assertEqual(exc.exception.status_code, 400)
        self.assertEqual(exc.exception.detail, "missing command_id")

    async def test_response_requires_encrypted_packet(self):
        await self.server.state.register()
        await self.server.state.register_server(0)

        with self.assertRaises(self.server.HTTPException) as exc:
            await self.server.state.enqueue_response(0, 0, {"command_id": "cmd-1"})

        _print_http_error("enqueue response missing payload", exc.exception)
        self.assertEqual(exc.exception.status_code, 400)
        self.assertEqual(exc.exception.detail, "missing encrypted packet")

    async def test_command_stream_sends_idle_heartbeats_without_marking_online(self):
        await self.server.state.register()
        await self.server.state.register_server(0)
        self.server.COMMAND_HEARTBEAT_SECONDS = 0.01

        response = await self.server.state.stream_commands(0, 0)
        stream = response.body_iterator.__aiter__()
        heartbeat = await asyncio.wait_for(stream.__anext__(), timeout=1)

        self.assertEqual(heartbeat, "\n")
        self.assertEqual(self.server.state.clients["0"]["status"], 1)
        self.assertEqual(self.server.state.clients["0"]["servers"]["0"]["status"], 1)
        await stream.aclose()


if __name__ == "__main__":
    unittest.main()
