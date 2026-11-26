### Communication Protocol Specification  
The transaction follows a request-response-acknowledge handshake with timeout recovery:
| Packet | Source | Format | Description |
| :--- | :--- | :--- | :--- |
| **Request (RST)** | Terminal $\rightarrow$ STM32 | `!RST#` | Initiates sensory data request. |
| **Sensory Data (ADC)** | STM32 $\rightarrow$ Terminal | `!ADC=xxxx#\r\n` | Returns 12-bit ADC value (0 – 4095). Retransmitted every 3s until ACK. |
| **Acknowledge (OK)** | Terminal $\rightarrow$ STM32 | `!OK#` | Confirms receipt and terminates the transmission session. |
* **ADC Voltage Calculation:**  
  With Proteus reference voltage $V_{REF} = 5.0\text{ V}$ and potentiometer supply at $3.3\text{ V}$ (50% position $\rightarrow V_{IN} = 1.65\text{ V}$):
  $$\text{ADC} = \frac{V_{IN}}{V_{REF}} \times 4095 = \frac{1.65\text{ V}}{5.0\text{ V}} \times 4095 \approx 1351$$
* **ARQ Retransmission Policy:**  
  If `!OK#` is not returned within 3000 ms, the STM32 re-transmits the identical `!ADC=xxxx#\r\n` packet without re-sampling the analog pin, ensuring strict data consistency across retries.
---
### Finite State Machine (FSM) Architecture  
#### 1. Command Parser FSM (`command_parser_fsm`)
- **`CMD_WAIT_START`:** Listens to the circular buffer for the start delimiter `'!'`. Discards non-delimiter characters.
- **`CMD_RECEIVING`:** Accumulates alphanumeric command characters into an internal buffer until the termination delimiter `'#'` is detected:
  - If payload equals `"RST"`, sets `command_flag = 1` and `command_data = CMD_RST`.
  - If payload equals `"OK"`, sets `command_flag = 1` and `command_data = CMD_OK`.
  - Reverts to `CMD_WAIT_START` upon packet completion, re-encountering `'!'`, or buffer overrun.
#### 2. UART Communication FSM (`uart_communiation_fsm`)
- **`COMM_WAIT_RST`:** Idle state waiting for `CMD_RST`. Upon trigger:
  - Reads current analog sensor data: `adc_value = HAL_ADC_GetValue(&hadc1)`.
  - Formats packet: `sprintf(tx_msg, "!ADC=%lu#\r\n", adc_value)`.
  - Emits leading newline and transmits `!ADC=xxxx#\r\n`.
  - Starts 3-second software timer (`timeout_timer = HAL_GetTick()`) and enters `COMM_WAIT_OK`.
- **`COMM_WAIT_OK`:** Waits for confirmation:
  - If `CMD_OK` is received: clears flags, emits trailing newline, and returns to `COMM_WAIT_RST`.
  - If timeout expires (`HAL_GetTick() - timeout_timer >= 3000` ms): re-transmits the previous `tx_msg` payload and resets `timeout_timer`.
---
### Pin Mapping  
| Component | MCU Pin | Type | Function |
| :--- | :--- | :--- | :--- |
| **ADC0 (IN0)** | `PA0` | Analog Input | ADC Channel 0 input from OPAMP voltage follower |
| **USART2_TX** | `PA2` | Alternate Function Push-Pull | Serial TX line to Virtual Terminal `RXD` (9600 baud) |
| **USART2_RX** | `PA3` | Input Floating | Serial RX line from Virtual Terminal `TXD` (Interrupt enabled) |
| **LED_RED (D1)** | `PA5` | GPIO Output (Push-Pull) | System heartbeat indicator (500 ms toggle rate) |
| **RV1 (POT-HG)** | External | Analog Transducer | 1 kΩ linear potentiometer powered between +3.3V and GND |
| **U2 (OPAMP)** | External | Signal Conditioner | Unity-gain buffer (Voltage Follower) driving `PA0` |
---
### How to Run the Simulation  
1. Open the project in **STM32CubeIDE** and build (`Ctrl + B`) to produce the binary `.hex` target file under the `Debug/` directory. *(Ensure "Convert to Intel Hex file" is enabled in Project Properties)*.  
2. Open the Proteus design file (`.pdsprj`) in **Proteus 8 Professional**.  
3. Double-click the `STM32F103C6` (or `STM32F103C8`) MCU:  
   - Set **Program File** to the compiled `Lab5.hex` file.  
   - Verify that **Crystal Frequency** is set to `8MHz`.  
4. Configure the **Virtual Terminal**:  
   - Baud Rate: `9600`, Data Bits: `8`, Parity: `NONE`, Stop Bits: `1`.  
   - Right-click inside the terminal window during simulation and check **Echo Typed Characters** to display outgoing commands locally.  
5. Click **Play (Run Simulation)**:  
   - Observe `LED_RED` on `PA5` toggling periodically at 1 Hz.  
   - Type `!RST#` into the Virtual Terminal: the MCU immediately transmits the formatted sensor string (e.g., `!ADC=1351#`).  
   - Pause for more than 3 seconds without typing: confirm that the identical `!ADC=1351#` packet is automatically re-transmitted every 3 seconds.  
   - Type `!OK#`: verify that the MCU halts retransmission and successfully returns to the idle state.