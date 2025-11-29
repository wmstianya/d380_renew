## 1. Specification Alignment
- [x] 1.1 Review `openspec/specs/modbus-unified/spec.md` and cross-check existing timing clauses to avoid conflict.
- [x] 1.2 Finalize the new timing requirements in `specs/modbus-unified/spec.md`.

## 2. Firmware Implementation
- [x] 2.1 Add configuration knobs (poll interval / response delay) to `modbus_uart4.c` and expose safe defaults.
- [x] 2.2 Update `modbusMasterScheduler` to block progression until either a response arrives or the configured timeout elapses.
- [x] 2.3 Ensure UART4 diagnostics log the applied timings whenever the scheduler advances.

## 3. Verification
- [ ] 3.1 Bench test with at least one slave by varying poll intervals (fast/slow) and confirm UART4 captures replies.
- [ ] 3.2 Document the recommended settings and capture sample logs showing the improved behavior.

