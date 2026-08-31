# Wrens Nest v1 API Documentation
This is the official documentation for the Wrens Nest client-to-agent communication api. All non-routing information using this API should be AES encrypted in order to ensure security.

## Endpoints
The top level endpoint is ```/api/wn/v1/```, all endpoints not related to user registration and management will be at ```/api/wn/v1/{uid}``` with uid being the unique code given from the registration endpoint.

### Registration and Management
These are the endpoints focused on registration and API key management

<details>
<summary><code>GET</code> <code>/api/wn/v1/register</code></summary>

This is the registration endpoint to get a UID and API key

#### Request
Request parameters
| Name | type | required | description |
| --- | --- | --- | --- |
| None | N/A | N/A | N/A

```
GET /api/wn/v1/register
Accept: application/json
```

#### Response
Response parameters
| Name | type | required | description |
| --- | --- | --- | --- |
| uid | integer | true | A unique identifier for each client and its servers, they are incremented server side and cannot be recovered if lost.|
| api-key | base64 | true | An HTTP safe base64 string used for uid validation, a uid will not be accessible if this key is lost.

```
HTTP/1.1 200 OK
Content-Type: application/json

{
    "uid": {uid},
    "api-key": {api-key}
}
```

</details>

<details>
<summary><code>GET</code> <code>/api/wn/v1/clients/{uid}/register</code></summary>

This is the registration endpoint to get a server-uid and its API key requests are made with a streaming response of json packets including a heartbeat packet to keep the stream alive.

#### Request
Request parameters
| Name | type | required | description |
| --- | --- | --- | --- |
| uid | integer | true | The clients unique identifier used for server side identification|
| api-key | base64 | true | The corresponding api key for the uid |
```
GET /api/wn/v1/clients/{uid}/register
Authorization: Bearer {api-key}
Accept: application/json
```

#### Response
Response parameters
| Name | type | required | description |
| --- | --- | --- | --- |
| server-uid | integer | true | A unique identifier for each server under a client, they are incremented server side and must be regenerated if lost.|
| server-api-key | base64 | true | An HTTP safe base64 string used for server validation, needed for a server to listen to the command endpoint and respond. |
```
HTTP/1.1 200 OK
Content-Type: application/json

{
    "server-uid": {server-uid},
    "server-api-key": {server-api-key}
}
```

</details>

### Commands
These are the endpoints for uploading and receiving server instructions

<details>
<summary><code>POST</code> <code>/api/wn/v1/{uid}/servers/{server-uid}/requests</code></summary>



#### Request
Request parameters
| Name | type | required | description |
| --- | --- | --- | --- |
| uid | integer | true | The clients unique identifier used for server side identification|
|server-uid | integer | true | The server that the command should be forwarded to |
| api-key | base64 | true | The corresponding api key for the uid |
```
POST /api/wn/v1/{uid}/servers/{server-uid}/requests
Authorization: Bearer {api-key}
Content-Type: application/json

{
    "type": {request-type},
    "packet": {
        {AES-packet}
    }
}
```

#### Response
Response parameters
| Name | type | required | description |
| --- | --- | --- | --- |
| success | boolean | true | returns true on success, false otherwise |
| command-id | base64 | true | a client unique base64 string used for identifying responses |
```
HTTP/1.1 200 OK
Content-Type: application/json

{
    "success": {success},
    "id": {command-id}
}
```

</details>

<details>
<summary><code>GET</code> <code>/api/wn/v1/{uid}/servers/{server-uid}/requests</code></summary>



#### Request
Request parameters
| Name | type | required | description |
| --- | --- | --- | --- |
| uid | integer | true | The clients unique identifier used for server side identification|
|server-uid | integer | true | the server that the command should be forwarded to |
| server-api-key | base64 | true | The corresponding api key for the servers-uid |
```
GET /api/wn/v1/{uid}/servers/{server-uid}/requests
Authorization: Bearer {server-api-key}
Connection: keep-alive
```

#### Response
Response parameters
| Name | type | required | description |
| --- | --- | --- | --- |
| command-type | string | true | brief categorization info  |
| command-id | base64 | true | a client unique base64 string used for identifying responses |
| aes-packet | base64 | true | The AES encrypted instructions that the agent will read |
```
HTTP/1.1 200 OK
Content-Type: application/x-ndjson
Connection: keep-alive

{"heartbeat": true}
{"heartbeat": true}
{"type": {command-type}, "id": {command-id},"packet": {aes-packet}}
```

</details>

### Responses
These are the endpoints for streaming and receiving responses.

<details>
<summary><code>POST</code> <code>/api/wn/v1/{uid}/servers/{server-uid}/responses</code></summary>

The endpoint for servers to send their responses to commands.

#### Request
Request parameters
| Name | type | required | description |
| --- | --- | --- | --- |
| uid | integer | true | The clients unique identifier used for server side identification|
|server-uid | integer | true | The server that the command should be forwarded to |
| server-api-key | base64 | true | The corresponding api key for the uid |
| response-type | string | true | A string giving the type of command response, used for prioritization|
| command-id | base64 | true | the command id used for matching requests to responses.|
```
POST /api/wn/v1/{uid}/servers/{server-uid}/responses
Authorization: Bearer {server-api-key}
Content-Type: application/json

{
    "type": {response-type},
    "id": {command-id},
    "packet": {
        {AES-packet}
    }
}
```

#### Response
Response parameters
| Name | type | required | description |
| --- | --- | --- | --- |
| success | boolean | true | returns true on success, false otherwise |
| command-id | base64 | true | the same command id provided, used for success validation. |
```
HTTP/1.1 200 OK
Content-Type: application/json

{
    "success": {success},
    "id": {command-id}
}
```

</details>


<details>
<summary><code>GET</code> <code>/api/wn/v1/{uid}/responses</code></summary>

The endpoint for clients to receive responses to commands.

#### Request
Request parameters
| Name | type | required | description |
| --- | --- | --- | --- |
| uid | integer | true | The clients unique identifier used for server side identification |
| api-key | base64 | true | The clients api validation key |
```
GET /api/wn/v1/{uid}/responses
Authorization: Bearer {api-key}
Connection: keep-alive
```

#### Response
Response parameters
| Name | type | required | description |
| --- | --- | --- | --- |
| response-type | string | true | brief categorization info  |
| command-id | base64 | true | a client unique base64 string used for identifying responses |
| aes-packet | base64 | true | The AES encrypted response from the agent |
```
HTTP/1.1 200 OK
Content-Type: application/x-ndjson
Connection: keep-alive

{"heartbeat": true}
{"heartbeat": true}
{"type": {response-type}, "id": {command-id},"packet": {aes-packet}}
```

</details>