# Wrens Nest Protocol

## Version 1 Scope

Protocol version 1 supports monitoring only. It allows the C++ client to request
host metrics from a Go agent through the Python proxy.

The version 1 data flow is:

1. The client encrypts a monitoring request and posts it to the proxy.
2. The proxy queues the opaque encrypted packet for one agent.
3. The agent receives and decrypts the request.
4. The agent validates the request and collects the selected metrics.
5. The agent encrypts a response and posts it to the proxy.
6. The client receives, decrypts, validates, and displays the response.

Arbitrary command execution, service control, log streaming, file transfer, and
direct client-to-agent monitoring are outside the version 1 protocol.

## Responsibilities

### Client

- Generate a unique `request_id` for each request.
- Select metrics with the version 1 metric bitmask.
- Encrypt requests with the server-specific key.
- Match responses by `request_id`.
- Enforce a response timeout.
- Validate decrypted responses before displaying them.

### Proxy

- Validate client and server identifiers.
- Queue requests and responses for the correct client/server pair.
- Treat encrypted packets as opaque data.
- Never receive or store server encryption keys.
- Return transport errors without inventing monitoring results.

### Agent

- Poll or stream pending requests from the proxy.
- Decrypt and validate each request.
- Reject unsupported versions, types, and metric bits.
- Collect metrics without executing request-provided shell commands.
- Return exactly one final response for each accepted request.

## Identity

The protocol uses three identifiers:

- `client_id`: identifies one Wrens Nest client installation.
- `server_id`: identifies one managed server belonging to that client.
- `request_id`: uniquely identifies one monitoring request and correlates it
  with its response.

All identifiers are JSON strings. A `request_id` must be unique within a client
installation for at least as long as responses may remain queued. UUIDs are
recommended.

## Metric Selection

The `metrics` request field is an unsigned integer bitmask. JSON carries it as a
decimal number, although implementations may define their constants in
hexadecimal.

| Bit | Hex value | Decimal value | Metric |
|---:|---:|---:|---|
| 0 | `0x01` | 1 | Uptime |
| 1 | `0x02` | 2 | CPU |
| 2 | `0x04` | 4 | Memory |
| 3 | `0x08` | 8 | Disk |

Values are combined with bitwise OR. For example, uptime and memory use
`0x01 | 0x04`, transmitted as `5`. All version 1 metrics use `0x0F`,
transmitted as `15`.

`metrics` must be between `1` and `15`. Zero and values containing unknown bits
are invalid. Version 1 implementations must not silently ignore unknown bits.

## Decrypted Monitor Request

```json
{
  "protocol_version": 1,
  "type": "monitor_request",
  "request_id": "550e8400-e29b-41d4-a716-446655440000",
  "metrics": 15
}
```

Required fields:

- `protocol_version`: integer; must equal `1`.
- `type`: string; must equal `monitor_request`.
- `request_id`: non-empty string.
- `metrics`: integer containing only defined version 1 metric bits.

Additional fields are ignored in version 1 so compatible fields can be added
later. A field with the wrong JSON type makes the request invalid.

## Decrypted Monitor Response

```json
{
  "protocol_version": 1,
  "type": "monitor_response",
  "request_id": "550e8400-e29b-41d4-a716-446655440000",
  "status": "completed",
  "collected_at": "2026-07-30T18:42:12Z",
  "metrics": {
    "uptime_seconds": 86400,
    "cpu": {
      "utilization_percent": 23.5,
      "sample_seconds": 1
    },
    "memory": {
      "used_bytes": 2147483648,
      "total_bytes": 4294967296
    },
    "disks": [
      {
        "mount_point": "/",
        "used_bytes": 10737418240,
        "total_bytes": 21474836480
      }
    ]
  },
  "error": null
}
```

Response rules:

- `request_id` must exactly match the request.
- `status` must be either `completed` or `failed`.
- `collected_at` is a UTC RFC 3339 timestamp.
- A completed response includes every requested metric and no fabricated
  values.
- Metrics that were not requested may be omitted.
- `error` is `null` for a completed response.
- A client must reject a response with a mismatched version, type, or
  `request_id`.

## Metric Schemas

### Uptime

`uptime_seconds` is a non-negative integer representing elapsed seconds since
the host booted.

### CPU

`cpu.utilization_percent` is a number from `0` through `100`, measured across
all logical CPUs during `cpu.sample_seconds`. It is not an instantaneous
counter.

`cpu.sample_seconds` is a positive number describing the measurement interval.

### Memory

`memory.used_bytes` and `memory.total_bytes` are non-negative integers.
`used_bytes` must not exceed `total_bytes`.

Version 1 defines used memory as memory unavailable for new applications
without swapping. Agent implementations must document any operating-system
specific approximation.

### Disk

`disks` is an array because a host may expose multiple monitored filesystems.
Each entry contains:

- `mount_point`: non-empty string.
- `used_bytes`: non-negative integer.
- `total_bytes`: non-negative integer.

`used_bytes` must not exceed `total_bytes`. Version 1 must include the root
filesystem (`/`) when it is available. Pseudo-filesystems and temporary
in-memory filesystems should be excluded.

## Failed Monitor Response

An agent returns a failed response when it can decrypt the request and recover
a usable `request_id`, but cannot validate or fulfill it.

```json
{
  "protocol_version": 1,
  "type": "monitor_response",
  "request_id": "550e8400-e29b-41d4-a716-446655440000",
  "status": "failed",
  "collected_at": "2026-07-30T18:42:12Z",
  "metrics": {},
  "error": {
    "code": "invalid_metrics",
    "message": "The metric selection contains unsupported bits"
  }
}
```

Version 1 error codes:

| Code | Meaning |
|---|---|
| `invalid_request` | Required data is missing or has the wrong type. |
| `unsupported_version` | `protocol_version` is not supported. |
| `unsupported_type` | `type` is not `monitor_request`. |
| `invalid_metrics` | The metric mask is zero or contains unknown bits. |
| `collection_failed` | The agent could not collect a requested metric. |
| `decrypt_failed` | The encrypted packet could not be authenticated or decrypted. |
| `timeout` | The request did not complete before the client deadline. |
| `agent_offline` | The proxy could not deliver the request to an active agent. |

`message` is human-readable and must not contain keys, plaintext packets,
ciphertext, or other secrets. Clients must make decisions using `code`, not by
matching `message`.

If the agent cannot decrypt enough of a packet to recover a trustworthy
`request_id`, it must not construct a correlated application response. That
failure should be logged without sensitive data and handled as a timeout or
delivery failure by the client/proxy.

## Encrypted Transport Envelope

The proxy-visible request and response body is:

```json
{
  "protocol_version": 1,
  "request_id": "550e8400-e29b-41d4-a716-446655440000",
  "packet": {
    "iv": "base64-encoded-value",
    "text": "base64-encoded-value"
  }
}
```

The outer `request_id` is routing metadata and must match the identifier inside
the encrypted payload. The receiving client or agent must reject a mismatch.
The proxy may use the outer identifier for queue lookup but must not interpret
the encrypted body.

`iv` and `text` are standard padded Base64 strings. Empty or invalid Base64
values are rejected.

The current AES-CBC packet format provides confidentiality but does not prove
integrity or authenticity. It must be replaced with an authenticated-encryption
format, such as AES-GCM, before version 1 is considered safe for release. The
final authenticated envelope must include an authentication tag and bind the
protocol version, client ID, server ID, and request ID as authenticated
metadata.

## Proxy HTTP Interface

The version 1 proxy exposes these logical operations:

| Method | Path | Caller | Purpose |
|---|---|---|---|
| `POST` | `/clients/{client_id}/servers/{server_id}/requests` | Client | Queue an encrypted monitor request. |
| `GET` | `/clients/{client_id}/servers/{server_id}/requests` | Agent | Stream pending requests and heartbeats. |
| `POST` | `/clients/{client_id}/servers/{server_id}/responses` | Agent | Queue an encrypted monitor response. |
| `GET` | `/clients/{client_id}/servers/{server_id}/responses/{request_id}` | Client | Wait for the correlated response. |

Successful streaming endpoints use newline-delimited JSON. An idle heartbeat is:

```json
{"type":"heartbeat"}
```

A heartbeat is transport metadata, is not encrypted, and is not a monitoring
response.

HTTP errors use:

```json
{
  "error": {
    "code": "server_not_found",
    "message": "The requested server is not registered"
  }
}
```

Expected transport codes include:

- `400` for malformed envelopes.
- `404` for unknown clients, servers, or expired response identifiers.
- `409` for a duplicate `request_id`.
- `413` for a packet exceeding the configured size limit.
- `429` when queue or rate limits are exceeded.

## Timeouts, Retries, and Duplicates

- The client owns the overall monitoring deadline.
- The first implementation should use a configurable deadline with a
  10-second default.
- Retrying a request must reuse the same `request_id`.
- The proxy and agent must treat repeated requests with the same `request_id`
  as duplicates rather than collecting the metric twice.
- A response is final. The agent must not post more than one final response for
  a request.
- The proxy retains a final response until the client acknowledges it or the
  configured retention period expires.
- Queue persistence across proxy restarts is required before release if the
  proxy is the only supported transport.

## Versioning

Every application payload and transport envelope includes
`protocol_version`. A receiver must reject unsupported versions explicitly.

New optional fields and newly assigned metric bits may be added without
changing the version only when version 1 receivers continue to reject unknown
metric bits and safely ignore unknown object fields. Changes to field meanings,
units, encryption, or required fields require a new protocol version or an
explicitly negotiated capability.
