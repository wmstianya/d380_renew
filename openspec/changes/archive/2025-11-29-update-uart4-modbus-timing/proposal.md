# Change: Update UART4 ModBus Master Timing

## Why
Current UART4 ModBus master polls the downstream slaves with a fixed 100 ms cadence and a 1 s timeout. Field observation shows the master sends the next request before the previous slave has completed its DMA transfer, so the bus never captures valid replies. We need explicit timing requirements so firmware can defer the next poll until the previous response window ends and allow integrators to tune the poll/timeout balance.

## What Changes
- Specify configurable poll interval/response window requirements for the UART4 ModBus master role.
- Require the scheduler to wait for `respTimeoutMs` (or early response) before advancing to the next slave.
- Document the default timing expectations and allow project-level overrides for slow slaves.
- Update firmware tasks to honor the new timing rules and add instrumentation for diagnostics.

## Impact
- Affected specs: `modbus-unified`
- Affected code: `HARDWARE/modbus/modbus_uart4.c`, `HARDWARE/modbus/modbus_master.c`, `USER/main.c` (timing/logging)

