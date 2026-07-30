import os
from pathlib import Path
import json
from fastapi import FastAPI, Request, HTTPException
from fastapi.responses import StreamingResponse
from datetime import datetime, timedelta
import uvicorn
import asyncio

COMMAND_HEARTBEAT_SECONDS = 10

default_clients_file = Path(__file__).with_name("clients.json")
clients_file = Path(
    os.environ.get("WRENS_NEST_CLIENTS_FILE", default_clients_file)
)

with clients_file.open("r", encoding="utf-8") as file:
    clients = json.load(file)

class RuntimeSession:
    def __init__(self):
        self.command_queue = asyncio.Queue()
        self.response_queue = asyncio.Queue()

class ProxyState:
    def __init__(self, clients_file, clients):
        self.clients = clients
        self.clients_file = clients_file
        self.runtime_sessions = {}
        self.close_tasks = {}
        self.command_close_delay_seconds = 10

    def _save(self):
        temp_file = self.clients_file.with_suffix(".tmp")

        with temp_file.open("w", encoding="utf-8") as file:
            json.dump(self.clients, file, indent=4)
            file.write("\n")

        temp_file.replace(self.clients_file)

    async def register(self):
        nxt_uid = (
            max(int(uid) for uid in self.clients.keys()) + 1
            if self.clients
            else 0
        )

        timestamp = datetime.now().isoformat()
        self.clients[str(nxt_uid)] = {
            "status": 1,
            "servers": {},
            "created_at": timestamp,
            "updated_at": timestamp
        }

        self._save()
        return nxt_uid

    async def register_server(self, uid):
        client_id = str(uid)
        if client_id not in self.clients:
            raise HTTPException(status_code=404, detail="uid not found")

        servers = self.clients[client_id]["servers"]
        server_uid = (
            max(int(server_uid) for server_uid in servers.keys()) + 1
            if servers
            else 0
        )

        timestamp = datetime.now().isoformat()
        servers[str(server_uid)] = {
            "status": 1,
            "created_at": timestamp,
            "updated_at": timestamp
        }

        self.clients[str(uid)]["updated_at"] = timestamp

        self._save()
        return server_uid

    def locate_server(self, uid, server_num):
        client_uid = str(uid)
        server_uid = str(server_num)

        if client_uid not in self.clients:
            raise HTTPException(status_code=404, detail="uid not found")

        if server_uid not in self.clients[client_uid]["servers"]:
            raise HTTPException(status_code=404, detail="server not found")

        return self.clients[client_uid], self.clients[client_uid]["servers"][server_uid]

    async def enqueue_command(self, uid, server_num, payload):
        if "packet" not in payload:
            raise HTTPException(
                status_code=400,
                detail="missing encrypted packet"
            )

        client, server = self.locate_server(uid, server_num)

        session_key = f"{uid}:{server_num}"
        session = self.runtime_sessions.get(session_key)

        if session is None:
            session = RuntimeSession()
            self.runtime_sessions[session_key] = session

        command = dict(payload)
        command["server_id"] = server_num

        await session.command_queue.put(command)

        client["status"] = 0
        server["status"] = 0

        timestamp = datetime.now().isoformat()
        client["updated_at"] = timestamp
        server["updated_at"] = timestamp

        prev_task = self.close_tasks.get(session_key)

        if prev_task is not None and not prev_task.done():
            prev_task.cancel()

        close_task = asyncio.create_task(
            self._close_after_delay(uid, server_num)
        )

        self.close_tasks[session_key] = close_task

        return command

    async def enqueue_response(self, uid, server_num, payload):
        if "command_id" not in payload:
            raise HTTPException(status_code=400, detail="missing command_id")

        if "packet" not in payload:
            raise HTTPException(
                status_code=400,
                detail="missing encrypted packet"
            )

        self.locate_server(uid, server_num)
        session_key = f"{uid}:{server_num}"

        session = self.runtime_sessions.get(session_key)

        if session is None:
            session = RuntimeSession()
            self.runtime_sessions[session_key] = session

        response = dict(payload)
        response["server_id"] = server_num

        await session.response_queue.put(response)

        return response

    async def stream_commands(self, uid, server_num):
        self.locate_server(uid, server_num)

        session_key = f"{uid}:{server_num}"
        session = self.runtime_sessions.get(session_key)

        if session is None:
            session = RuntimeSession()
            self.runtime_sessions[session_key] = session

        async def stream():
            while True:
                try:
                    command = await asyncio.wait_for(
                        session.command_queue.get(),
                        timeout=COMMAND_HEARTBEAT_SECONDS
                    )

                except asyncio.TimeoutError:
                    yield "\n"

                else:
                    yield json.dumps(command) + "\n"

        return StreamingResponse(stream(), media_type="text/plain")


    async def _close_after_delay(self, uid, server_num):
        await asyncio.sleep(self.command_close_delay_seconds)
        client, server = self.locate_server(uid, server_num)
        client["status"] = 1
        server["status"] = 1

state = ProxyState(clients_file, clients)

app = FastAPI()
@app.get("/register")
async def register():
    return {"uid": await state.register()}

@app.get("/register/{uid}")
async def register_server(uid: int):
    return {"server_uid": await state.register_server(uid)}

@app.get("/open/{uid}")
async def open_session(uid: int, request: Request):
    if str(uid) not in clients:
        raise HTTPException(status_code=404, detail="uid not found")
    ip = request.client.host
    clients[str(uid)]["ip"] = ip

    expire = (datetime.now() + timedelta(minutes=5)).isoformat()
    clients[str(uid)]["expi"] = expire

    clients[str(uid)]["status"] = 0

    return 0

@app.get("/wait/{uid}")
async def wait(uid: int):
    async def stream():
        connected = False
        while not connected:
            for i in range(12):
                entry = clients.get(str(uid))
                if entry["status"] == 0:
                    if datetime.fromisoformat(entry["expi"]) > datetime.now():
                        yield entry["ip"]
                        connected = True
                        break
                    else:
                        entry["status"] = 1
                if not connected:
                    await asyncio.sleep(5)
                    break
            if not connected:
                yield "\n"

    return StreamingResponse(stream(), media_type="text/plain")

if __name__ == "__main__":
    uvicorn.run(app, host="0.0.0.0", port=1690)
