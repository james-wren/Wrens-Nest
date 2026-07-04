# Wrens Nest Project Definition

## Summary

Wrens Nest is a terminal-based server management application for developers who operate multiple small servers and want one local tool for status checks, deployment support, and routine operational tasks.

The project has three cooperating parts:

- A C++ client application named `wn`.
- A Go agent that is deployed to managed servers.
- A Python FastAPI proxy service used for client and agent coordination.

The current codebase is early-stage. The primary direction is to turn the existing prototype into a reliable local operations tool with a clear architecture, predictable setup flow, and safe server command execution.

## Project Goals

1. Provide a single TUI for managing many servers.
2. Reduce repeated SSH/SCP work for common operational tasks.
3. Support fast server status polling through a lightweight remote agent.
4. Keep the tool self-contained enough that setup is simple.
5. Use the project as a practical learning platform for C++, Go, networking, encryption, and systems design.

## Non-Goals

Wrens Nest is not trying to be a full replacement for Kubernetes, Ansible, Terraform, Prometheus, or a commercial monitoring suite.

It should stay focused on personal or small-team server management:

- Add and configure servers.
- Start, stop, and inspect services.
- View status and logs.
- Transfer small updates or helper files.
- Run constrained remote actions safely.

## Intended Users

The first user is the project owner: a developer managing several personal servers.

Future users may include developers with similar setups, but the project should first optimize for one real workflow instead of broad generic infrastructure management.

## Current Architecture

### C++ Client

The C++ application is the main user-facing program.

Responsibilities:

- Install itself as `wn`.
- Store local configuration under `~/.wrens_nest/`.
- Register with the proxy service.
- Add servers to local config.
- Generate per-server AES keys.
- Deploy the Go agent and agent config through SSH/SCP.
- Render the terminal UI through FTXUI.
- Send command and monitoring requests through the proxy protocol.

Important files:

- `src/main.cpp`
- `src/agents.cpp`
- `src/monitor.cpp`
- `src/tui.cpp`
- `src/encryption.cpp`
- `include/*.h`

### Go Agent

The Go agent runs on managed servers.

Responsibilities:

- Read its transferred config.
- Coordinate with the proxy service.
- Receive command or status requests through the proxy service.
- Decrypt requests using the server-specific AES key.
- Return command output or status data.

Important files:

- `agent/src/agent.go`
- `agent/agent`
- `agent/agent_64`
- `src/embedded/*_go.h`

### Python Proxy

The Python proxy coordinates connections between the local client and remote agents.

Responsibilities:

- Register clients and assign user IDs.
- Track online/offline status.
- Hold command and response queues.
- Provide streaming endpoints for connection coordination.
- Serve as the required message path between clients and agents.

Important files:

- `server/server.py`
- `server/clients.json`

Protocol reference:

- `docs/protocol.md` [Created by Codex]

## Data Flow

### First Run

1. User runs `wn`.
2. Client checks whether `/usr/local/bin/wn` exists.
3. If missing, client performs setup.
4. Client registers with the proxy service.
5. Client creates `~/.wrens_nest/`.
6. Client writes local metadata and server config files.
7. Client copies itself to `/usr/local/bin/wn`.

### Add Server

1. User runs `wn -m add [name] [user] [ip] [key]`.
2. Client creates `~/.wrens_nest/` on the remote server through SSH.
3. Client generates a per-server AES-256 key.
4. Client saves server metadata locally.
5. Client writes a temporary agent config.
6. Client writes the embedded Go agent to a temporary file.
7. Client transfers the config and agent to the remote server through SCP.
8. Client removes local temporary transfer files.

### Monitoring Request

1. Client reads server metadata from `~/.wrens_nest/data/servers.json`.
2. Client builds a proxy protocol command request.
3. Client submits the request to the proxy for a specific server.
4. Agent receives the queued request from the proxy.
5. Agent validates and runs the requested status action.
6. Agent posts output events and a final response back to the proxy.
7. Client reads the final response from the proxy and updates the TUI.

Command and monitoring payloads are defined in `docs/protocol.md`. [Created by Codex]

## Security Model

Wrens Nest should assume all server management operations are sensitive.

Security expectations:

- Each server gets its own AES key.
- Long-term secrets should not be printed in normal logs.
- SSH keys should be referenced by path, not copied into Wrens Nest config.
- Agent commands should be explicit and constrained.
- The agent should reject malformed or unauthenticated requests.
- Remote command execution should avoid shell injection risks.
- Local config files should use restrictive permissions.

Current implementation risks to address:

- Some shell commands are assembled with raw string concatenation.
- The command API is not fully implemented.
- Agent request authentication needs to be made explicit.
- Local config file permissions are not yet enforced.
- Proxy persistence and lifecycle behavior are not finalized.

## Development Principles

1. Make behavior explicit before adding features.
2. Prefer small vertical slices over large rewrites.
3. Keep the C++ client responsible for local UX and orchestration.
4. Keep the Go agent small, auditable, and boring.
5. Keep the Python proxy focused on coordination, not business logic.
6. Treat SSH/SCP as bootstrap and fallback mechanisms.
7. Treat the agent protocol as the long-term fast path.
8. Add tests around parsing, encryption, config handling, and command safety as the code stabilizes.

## Near-Term Roadmap

### Milestone 1: Stabilize Project Shape

- Document architecture and current assumptions.
- Decide the exact role of the proxy service.
- Define local config file schemas.
- Define the agent request and response protocol.
- Make build and run steps repeatable.

### Milestone 2: Make Setup Reliable

- Add argument validation for CLI commands.
- Make first-run setup idempotent.
- Handle missing proxy service clearly.
- Avoid overwriting existing config unexpectedly.
- Add clear error messages for SSH, SCP, and registration failures.

### Milestone 3: Implement Safe Agent Communication

- Define supported request types.
- Add request authentication.
- Add agent-side request validation.
- Return structured JSON responses.
- Handle timeouts and offline servers.

### Milestone 4: Build the TUI Workflow

- Show configured servers.
- Show online/offline state.
- Show basic CPU, memory, disk, and uptime data.
- Add a selected-server detail panel.
- Add a terminal/log panel once command handling is safe.

### Milestone 5: Operational Features

- Start and stop configured services.
- Tail logs.
- Transfer update files.
- Run predefined maintenance commands.
- Export or backup configuration.

## Open Design Questions

- Should the proxy be required, optional, or only used for NAT traversal?
- Should the agent listen directly on each server or only connect outward?
- How should agents be started and kept alive on remote servers?
- Should service management target systemd first?
- What data should be stored locally versus remotely?
- What is the minimum supported OS for client and server machines?

## Definition of Done

A feature is done when:

- It has a clear user-facing workflow.
- It handles expected failure cases.
- It does not expose secrets in normal output.
- It keeps local and remote config consistent.
- It is documented when it changes setup, architecture, or workflow.
- It can be manually verified with a repeatable command or checklist.
