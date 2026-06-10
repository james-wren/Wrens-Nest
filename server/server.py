import json
from fastapi import FastAPI, Request, HTTPException
from fastapi.responses import StreamingResponse
from datetime import datetime, timedelta
import uvicorn
import asyncio

with open("clients.json", "r") as file:
    clients = json.load(file)

app = FastAPI()
@app.get("/register")
async def register():
    uid = len(clients)

    clients[str(uid)] = {"status": 1} #status 1 is ofline, 0 is online

    return {"uid": uid}

@app.get("/open/{uid}")
async def open_session(uid: int, request: Request):
    if str(uid) not in clients:
        raise HTTPException(status_code=404, detail="uid not found")
    ip = request.client.host
    clients[str(uid)]["ip"] = ip

    expire = (datetime.now() + timedelta(minutes=5)).isoformat()
    clients[str(uid)]["expi"] = expire

    clients[str(uid)]["status"] = 0

@app.get("/wait/{uid}")
async def wait(uid: int):
    async def stream():
        while True:
            entry = clients.get(str(uid))
            if entry["status"] == 0:
                if datetime.fromisoformat(clients[str(uid)]["expi"]) > datetime.now():
                    yield entry["ip"]
                break
            
            yield "\n"
            await asyncio.sleep(1)


uvicorn.run(app, host="0.0.0.0", port=1690)