## MODIFIED Requirements

### Requirement: UART4 Master Response Detection

The UART4 ModBus master SHALL correctly detect and process slave responses within the configured timeout window.

#### Scenario: Slave responds within timeout
- **GIVEN** UART4 is configured as ModBus master with `respTimeoutMs = 500`
- **AND** a slave device is connected and powered on
- **WHEN** the master sends a read request (FC03) to a valid slave address
- **AND** the slave responds within 500ms
- **THEN** `modbusHasRxData()` SHALL return TRUE
- **AND** `masterProcessResponse()` SHALL be called
- **AND** the `onResponse` callback SHALL be invoked with the slave data
- **AND** the scheduler log SHALL show `result=resp`

#### Scenario: DMA IDLE interrupt triggers correctly
- **GIVEN** UART4 DMA RX is configured with IDLE line detection
- **WHEN** a complete ModBus frame is received
- **AND** the bus goes idle (no more bytes for 1 character time)
- **THEN** the UART4 IDLE interrupt SHALL fire
- **AND** `uartIdleIrqHandler(&uartUnionHandle)` SHALL be called
- **AND** `rxReady` flag SHALL be set to TRUE
- **AND** `rxLen` SHALL contain the actual received byte count

#### Scenario: Receive buffer correctly passed to ModBus layer
- **GIVEN** UART4 has received a valid ModBus response frame
- **AND** `uartIsRxReady(&uartUnionHandle)` returns TRUE
- **WHEN** `modbusHasRxData()` is called
- **THEN** the frame data SHALL be copied to `ModbusHandle.rxBuf`
- **AND** `ModbusHandle.rxLen` SHALL match the received length
- **AND** the function SHALL return TRUE

