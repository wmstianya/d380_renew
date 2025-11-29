## ADDED Requirements

### Requirement: UART4 ModBus Master Timing Control
The UART4 ModBus master role SHALL respect a configurable poll cadence and response window so that downstream slaves have sufficient time to place data on the RS485 bus before the next query is transmitted.

#### Scenario: Response window enforcement
- **WHEN** the UART4 master sends a frame to a slave
- **THEN** it SHALL remain in `MASTER_WAIT` until either a valid response completes or the configured `respTimeoutMs` elapses
- **AND** it SHALL NOT dispatch the next slave request earlier than that response window.

#### Scenario: Configurable defaults
- **WHEN** the project boots with factory settings
- **THEN** the UART4 master SHALL default to a poll interval ≥ 250 ms and a timeout ≥ 500 ms
- **AND** integrators SHALL be able to raise or lower both values (within 50–2000 ms) via `ModbusMasterCfg` so deployments with slower slaves can extend the window.

#### Scenario: Diagnostics
- **WHEN** the UART4 master advances to the next slave
- **THEN** it SHALL record the measured turnaround time (response vs timeout) in its statistics/log output so field engineers can verify timing behavior without instruments.

