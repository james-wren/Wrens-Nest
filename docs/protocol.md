# Wrens Nest Proxy Protocol

[Created by Codex]

## Purpose

This document defines the JSON contract for the C++ client, Python proxy, and Go agent. The proxy is the required coordination point for command execution. The client and agent should build around the proxy contract instead of assuming direct client-to-agent communication.

The proxy is responsible for:

- Registering clients and assigning stable client IDs.
- Tracking short-lived client reachability windows.
- Queuing command requests from clients for agents.
- Queuing command responses and output events from agents for clients.
- Preserving enough state that a proxy restart does not lose registered clients or in-flight command metadata.

The proxy is not responsible for:

- Interpreting command semantics.
- Running commands locally.
- Decrypting encrypted command bodies.
- Owning server-specific secrets.

## Identity Model

The proxy protocol uses explicit client, server, and command identifiers.

`client_id` identifies a Wrens Nest client installation.

`server_id` identifies one managed server registered under that client.

`command_id` identifies one command execution request from creation through final response.

IDs are represented as strings in JSON payloads. Existing numeric IDs can still be serialized as strings so future UUID-style IDs do not require another protocol break.

## Existing Coordination Endpoints

### `GET /register`

Registers a new client and returns its proxy identity.

Response:

```json
{
  "client_id": "0"
}
```

Compatibility note: the current proxy returns `uid`. New client work should normalize that value to `client_id`.

### `GET /open/{client_id}`

Marks a client as reachable for a short window. The proxy records the caller IP and an expiration timestamp.

Response:

```json
{
  "status": "open",
  "client_id": "0",
  "expires_at": "2026-07-03T12:34:56"
}
```

Compatibility note: the current proxy returns `0`. New work should move to the structured response above.

### `GET /wait/{client_id}`

Streams client reachability updates for an agent waiting to coordinate with a client.

Stream event when the client is reachable:

```json
{
  "type": "client_reachable",
  "client_id": "0",
  "client_ip": "203.0.113.10",
  "expires_at": "2026-07-03T12:34:56"
}
```

Stream heartbeat when no client is currently reachable:

```json
{
  "type": "heartbeat"
}
```

Compatibility note: the current proxy streams a raw IP line or a blank line. New work should use newline-delimited JSON events.

## Command Request Flow

Command execution is proxy-required:

1. Client opens a reachable session with the proxy.
2. Client submits a command request for a specific server.
3. Agent polls or streams pending commands for its server.
4. Agent executes the command according to its local allowlist and runtime rules.
5. Agent posts output events and a final command result to the proxy.
6. Client reads output events and the final result from the proxy.

## Command Request Payload

Command requests must include identity, command text or argv, and enough metadata to make retries and UI updates deterministic.

```json
{
  "command_id": "cmd_20260703_000001",
  "client_id": "client_0",
  "server_id": "server_0",
  "command": {
    "mode": "argv",
    "argv": ["systemctl", "status", "nginx"],
    "text": null
  },
  "created_at": "2026-07-03T12:00:00Z",
  "timeout_seconds": 30
}
```

Rules:

- `mode` is either `argv` or `text`.
- `argv` is preferred for commands that can avoid shell parsing.
- `text` is allowed only for predefined agent-side command templates or explicit shell-mode operations.
- The proxy stores and forwards the request but does not validate whether the command is allowed.
- The agent is responsible for rejecting unsupported command modes, unsafe commands, malformed payloads, and missing fields.

## Streaming Output Event

Streaming output is represented as ordered newline-delimited JSON events. The first implementation may store and return only final output, but the event shape is still defined now so the protocol can grow without changing clients.

```json
{
  "type": "output",
  "command_id": "cmd_20260703_000001",
  "client_id": "client_0",
  "server_id": "server_0",
  "sequence": 1,
  "stream": "stdout",
  "data": "nginx.service - A high performance web server\n",
  "created_at": "2026-07-03T12:00:01Z"
}
```

Rules:

- `stream` is `stdout` or `stderr`.
- `sequence` starts at `1` for each command and increases by one per output event.
- `data` is a UTF-8 string. Binary output is out of scope for the first protocol version.

## Final Command Response

Every command must eventually produce one final response.

```json
{
  "type": "result",
  "command_id": "cmd_20260703_000001",
  "client_id": "client_0",
  "server_id": "server_0",
  "status": "completed",
  "stdout": "nginx is running\n",
  "stderr": "",
  "exit_code": 0,
  "error": null,
  "started_at": "2026-07-03T12:00:00Z",
  "finished_at": "2026-07-03T12:00:02Z"
}
```

Failure response:

```json
{
  "type": "result",
  "command_id": "cmd_20260703_000002",
  "client_id": "client_0",
  "server_id": "server_0",
  "status": "failed",
  "stdout": "",
  "stderr": "Unit not found\n",
  "exit_code": 5,
  "error": {
    "code": "command_failed",
    "message": "Command exited with a non-zero status",
    "details": {
      "exit_code": 5
    }
  },
  "started_at": "2026-07-03T12:05:00Z",
  "finished_at": "2026-07-03T12:05:01Z"
}
```

Allowed `status` values:

- `queued`
- `running`
- `completed`
- `failed`
- `rejected`
- `timeout`
- `cancelled`

## Error Object

Protocol errors use a consistent object shape.

```json
{
  "code": "invalid_request",
  "message": "server_id is required",
  "details": {
    "field": "server_id"
  }
}
```

Rules:

- `code` is stable and intended for programmatic handling.
- `message` is human-readable.
- `details` is optional and may contain structured debugging context.
- Secrets, decrypted payloads, and private key material must never appear in `message` or `details`.

## Persistence Requirements

The proxy must persist:

- Registered clients.
- Server registrations or server ownership mappings once implemented.
- Pending command requests.
- Final command responses until the client acknowledges receipt.

The proxy may discard:

- Expired reachability windows.
- Heartbeat events.
- Streaming output events after a final response is stored, if the final response contains aggregated `stdout` and `stderr`.

## Versioning

Protocol payloads should include `protocol_version` once the command endpoints are implemented.

Initial command endpoint work should use:

```json
{
  "protocol_version": 1
}
```

Breaking changes require a new integer version.
