# Embedded Telemetry & State Machine Firmware

A modular, hardware-decoupled firmware implementation developed for the **STM32 Nucleo-C031C6** development board and a custom peripheral sensor shield.

The system implements continuous analog telemetry streaming, digital filtering, non-blocking input debouncing, interactive serial command-line processing, and an event-driven Finite State Machine (FSM) utilizing `libfsm-sw`. Complete unit testing is implemented natively via PlatformIO and the Unity test framework using hardware abstraction stubs and the Fake Function Framework (`fff.h`).

---

## System Architecture & File Structure

The project strictly follows modular embedded C guidelines, separating hardware-specific peripheral management (STM32 HAL / CMSIS) from hardware-agnostic application logic:

<p align="center">
  <img width="723" height="648" alt="System Architecture" src="https://github.com/user-attachments/assets/14a96f9b-160c-4178-98d8-9e62990bc059" />
</p>

---

## Hardware Pinout & Wiring

<p align="center">
  <img width="1005" height="557" alt="Hardware Pinout" src="https://github.com/user-attachments/assets/3fc06b9c-d5f7-4b89-b3aa-25def3873e22" />
</p>

---

## Module Overview & Design Choices

### 1. Telemetry (`recruiting/telemetry`)

- **DMA Circular Acquisition:** The ADC continuously samples `PA0` and `PA1` using hardware DMA in circular mode, writing 16-bit counts directly into `telemetry_adc_buffer[2]` without CPU intervention.
- **Conversion Mathematics:**
  - **Potentiometer:** Linear scaling:
    ```text
    T_POT = 0.0 + (raw / 4095.0) * 50.0 °C
    ```
  - **NTC Thermistor:** Calculated via the Beta equation using nominal reference values (`R_0 = 10 kΩ`, `T_0 = 298.15 K`, `β = 3950 K`):
    ```text
    1 / T = (1 / T_0) + (1 / β) * ln(R_NTC / R_0)
    T_Celsius = T_Kelvin - 273.15
    ```
    The raw ADC input is clamped to `[1, 4094]` to prevent zero division or logarithmic domain errors (`NaN` / `Inf`).
- **150-Element Circular Buffer Moving Average:** Implements a rolling accumulator window (`TELEMETRY_WINDOW_SIZE = 150`). During startup, dynamic divisor calculation averages only acquired samples until the 150-element boundary is satisfied.
- **Noise Simulation:** Adds bounded pseudorandom variation (`±50` counts) clamped

### 2. User Button (`recruiting/button`)

- **Non-Blocking Debounce:** Hardware interrupts (`HAL_GPIO_EXTI_Falling_Callback`) assert a volatile flag. The main loop polishes the event via `button_state_update()` using `HAL_GetTick()`, enforcing a 200 ms debounce lockout before latching the press.
- **Single-Shot Latch Consumption:** Calling `button_was_pressed()` returns `true` once and immediately resets the state to `false`, preventing re-triggering across FSM iterations.

### 3. Command Line Interface (`recruiting/cli`)

- **Non-Blocking Interrupt Reception:** Characters arriving over USART2 trigger `HAL_UART_RxCpltCallback` into a 32-byte circular accumulation buffer.
- **Grammar Parsing:** Commands terminate on carriage return (`\r`) or line feed (`\n`).
- **Supported Commands:**
  - `raw`: Sets telemetry mode to un-filtered samples.
  - `moving average`: Enables the 150-sample moving average filter.
  - `random noise`: Toggles simulated noise injection.
  - `run`: Latches an execution flag to transition the system to `STATE_LISTENING`.

### 4. Finite State Machine (`recruiting/fsm`)

Driven by the `libfsm-sw` event library, the system routes through six states:

<p align="center">
  <img width="725" height="500" alt="FSM State Diagram" src="https://github.com/user-attachments/assets/cd368bdc-9de8-41e1-a114-52c0fb89c7d4" />
</p>

- **`STATE_POST`:** Delays 100 ms to allow ADC DMA stabilization. Validates sensors (`0 <= POT < 4085`, `100 < NTC < 4000`). Routes to `STATE_WAITING` on success or `STATE_FATAL` on out-of-bounds readings.
- **`STATE_WAITING`:** CLI active, sensor streaming paused. LD4 flashes (500 ms period, 15% duty cycle). External LED breathes via PWM (2 s period). Transition to `STATE_LISTENING` triggered by button press or CLI command `run`.
- **`STATE_LISTENING`:** CLI inactive, telemetry streams formatted packets every 100 ms. LD4 blinks (200 ms period, 80% duty cycle). External LED PWM duty cycle indicates raw differential:
  ```text
  Duty Cycle = (max(0, NTC - POT) / WARNING_THRESHOLD) * 100%
  ```
- **`STATE_PAUSED`:** CLI active, telemetry streaming stopped. LD4 blinks (2000 ms period, 50% duty cycle). External LED breathes via PWM (2 s period). Button press or CLI `run` returns to `STATE_LISTENING`.
- **`STATE_WARNING`:** Both LEDs disabled. Broadcasts `"WARNING\r\n"` over UART every 200 ms. Holding the user button for $\ge 2\text{ seconds}$ returns system to `STATE_WAITING`. Remaining in Warning for $> 10\text{ seconds}$ escalates to `STATE_FATAL`.
- **`STATE_FATAL`:** Trap state; sensor reading and CLI disabled. LD4 flashes rapidly (40 ms period, 50% duty cycle). Broadcasts `"ERROR: <description>\r\n"` over UART every 100 ms. Exited only via hardware reset button or software reset triggered by button press (`NVIC_SystemReset()`).

### 5. Host Telemetry GUI & Real-Time Audio Synthesizer (`gui/`)

A cross-platform host monitoring application developed in Python, featuring real-time plotting and dynamic acoustic modulation[cite: 13, 16]:

- **Decoupled Concurrency Pipeline:**
  - **Serial Ingestion Worker (`threading.Thread`):** Operates on a dedicated daemon thread to ingest incoming frames from `/dev/ttyACM0` at 115200 baud without blocking the user interface. It handles line decoding, validates packet structures, filters out CLI/state banners, and posts parsed data dictionaries to a thread-safe `queue.Queue`[cite: 13].
  - **GUI Event Loop (`PyQt5` & `PyQtGraph`):** Consumes queued data points on a 30 ms timer tick (~33 FPS), updating rolling history buffers (`deque(maxlen=200)`) for both potentiometer and thermistor temperatures without unbounded memory growth[cite: 13].
- **Dynamic Sound Synthesis (`sounddevice` & `NumPy`):**
  - Synthesizes audio using an asynchronous real-time callback stream running at 44.1 kHz via PortAudio[cite: 13].
  - Translates the active temperature difference into a target synthesizer frequency between a 220 Hz baseline and a 1200 Hz ceiling[cite: 13]:
    ```text
    ΔT = max(0.0, T_NTC - T_POT)
    ```
  - Applies single-pole low-pass filtering to frequency transitions for a smooth glissando/portamento effect[cite: 13].
  - Implements a continuous phase accumulator to eliminate buffer-boundary clicking artifacts[cite: 13]:
    ```text
    φ_n = (φ_{n-1} + Δφ) mod 2π
    ```

---

## Build & Run Instructions

### Prerequisites

- [PlatformIO Core (CLI)](https://platformio.org/) or VS Code with the PlatformIO IDE extension.
- ST-LINK USB drivers (for flashing and serial communication).
- GCC compiler suite on host system (for native unit tests).
- Python 3.10+ with python3-devel, python3-pip, and portaudio-devel system headers.
- [Doxygen](https://www.doxygen.nl/) and [Graphviz](https://graphviz.org/) (optional, for documentation generation).

### 1. Build and Flash Firmware (Target Hardware)

Connect the Nucleo board via USB and execute:

```bash
# Build firmware for STM32C031C6 and flash onto target board
pio run -e release

pio run -e release -t upload
```

### 2. Launch Serial Monitor

Open the serial interface at 115200 baud:

```bash
  pio device monitor -b 115200
```

### 3 Launch Host Telemetry GUI & Audio Synthesizer

Open a terminal in the gui/ directory and activate the virtual environment:

```bash
cd gui
python3 -m venv .venv
source .venv/bin/activate

# Install required host dependencies
pip install pyserial pyqtgraph PyQt5 numpy sounddevice

# Run the real-time viewer and audio generator
python main.py

```

### 4 Unit Testing (Native Host Environment)

All custom modules are isolated from target hardware using preprocessor switches (#ifndef UNIT_TEST), mock stubs, and weak linking fallback symbols.
Tests run natively on host x86_64 CPU hardware via the Unity test framework.

Running Tests
Execute all suites across the project:

```bash
  pio test -e tests
```

Execute an individual module test suite:

```bash
  pio test -e tests -f test_telemetry
  pio test -e tests -f test_cli
  pio test -e tests -f test_button
  pio test -e tests -f test_fsm
```

Test Coverage Breakdown

test_telemetry:
Linear potentiometer conversions, NTC logarithmic Steinhart-Hart conversions, boundary rail-clamping (0 ... 4095), moving average accumulator mathematics, and circular buffer rollover behavior.

test_cli:
ASCII token parsing, single-shot latch clearing for the run command, unknown command rejection, and 32-byte input overflow protection.

test_button:
Falling-edge interrupt handling, single-shot latch clearing, 200 ms non-blocking debounce timing (using FFF to mock HAL_GetTick), and foreign GPIO pin filtering.

test_fsm:
Power-On Self-Test (POST) sensor boundary routing, button and CLI transitions, 5-second continuous threshold timing, 2-second button hold detection, and 10-second warning timeout escalation.

API Documentation Generation
All functions in Core/Inc/recruiting/ are documented using Qt-style Doxygen format tags (/_! \brief ... \param ... \return _/).

Generate the standalone HTML documentation website:

# Generate documentation from project root

      doxygen Doxyfile

# View documentation in default browser

      xdg-open docs/html/index.html
