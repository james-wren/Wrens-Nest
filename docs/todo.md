# Wrens Nest Todo

## Immediate Next Steps

1. Define the project contract in more detail.
   - Lock down the exact responsibility of the client, agent, and proxy.
   - Decide which parts are required for the app to function and which are optional.
   - Write the request/response shapes for the agent protocol.

2. Stabilize first-run setup.
   - Make the install flow idempotent.
   - Replace fragile shell construction with safer command handling.
   - Validate that `~/.wrens_nest/` and its subfolders exist before writing files.
   - Set explicit file permissions for config and secret material.

3. Clean up configuration storage.
   - Define a stable schema for `servers.json` and `info.json`.
   - Separate user-editable settings from internal metadata.
   - Decide what should be local-only versus transferred to remote machines.

4. Finish the proxy service behavior.
   - Implement the missing command and response paths in `server/server.py`.
   - Decide whether the proxy is a control plane, a bootstrap helper, or both.
   - Add persistence so client state survives proxy restarts.

5. Complete agent deployment.
   - Confirm the embedded agent build process is reproducible.
   - Verify the config transfer step matches the agent's runtime expectations.
   - Add checks for failed SCP, failed SSH, and partial deploys.

6. Make server management commands real.
   - Replace placeholder start/stop logic.
   - Define how services are started and stopped on the remote host.
   - Add a clear path for service discovery and status reporting.

7. Build out monitoring.
   - Return structured status data from the agent.
   - Add CPU, memory, disk, and uptime reporting.
   - Make the TUI show real server state instead of placeholder content.

8. Harden encryption and transport.
   - Add explicit authentication for encrypted requests.
   - Make decryption failures and malformed payloads visible.
   - Review the key lifecycle for generation, storage, and rotation.

9. Add tests around the risky parts.
   - Cover config parsing and serialization.
   - Cover encryption helpers.
   - Cover command building and protocol parsing.
   - Cover first-run and add-server workflows.

10. Improve the developer loop.
    - Document build steps for the C++ client, Go agent, and Python proxy.
    - Document how the embedded agent artifacts are produced.
    - Add a short verification checklist for local changes.

## Later Work

- Add log viewing and tailing.
- Add update transfer workflows.
- Add server grouping or tags.
- Add better error reporting in the TUI.
- Add backup/export of local configuration.
- Review whether direct SSH fallback should remain in the long-term design.

