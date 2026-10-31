# PicoPulse — Embedded Optical Heart Rate Monitor

> **An optical pulse-monitoring prototype built with Raspberry Pi Pico 2 W, MAX30102 and a 1.8-inch ST7735S TFT display.**  
> Embedded C · I²C · SPI · PPG signal processing · Hardware integration

PicoPulse acquires red and infrared photoplethysmography (PPG) data from a fingertip, estimates heart rate in beats per minute (BPM), and displays the result locally. The firmware also computes an **experimental RED/IR optical ratio (R)** as a foundation for studying pulse oximetry.

**Status:** Functional engineering prototype. Heart-rate estimates have been demonstrated during stationary tests, but accuracy has not been clinically validated. **R is not a calibrated SpO₂ percentage.**

---

## The Engineering Challenge

Optical pulse sensing is straightforward in principle but less predictable in practice. The sensor detects small changes in reflected light associated with pulsatile blood volume; however, finger placement, contact pressure, movement and slowly drifting light levels can distort the measured signal. Meanwhile, the microcontroller must continue collecting samples while processing data and updating a display.

The primary design goals were to:

1. Read continuous RED and IR samples from the MAX30102 over I²C.
2. Estimate BPM from the infrared signal without presenting an answer when the signal is unreliable.
3. Monitor acquisition timing and report sensor FIFO overflow events.
4. Present measurements on a small SPI-connected screen without a connected computer.
5. Keep the embedded C firmware modular and understandable.

## The Solution — Local Sensing, Processing and Display

Rather than transferring measurements to a computer for processing, PicoPulse performs acquisition, analysis and visualization on the **Raspberry Pi Pico 2 W**. The design deliberately uses a small number of off-the-shelf modules and separates hardware drivers from signal-processing logic.

| Engineering decision | Implementation | Reason |
|---|---|---|
| Optical sensing | MAX30102, RED + IR channels | Access to raw PPG signals |
| Data acquisition | I²C0 and sensor FIFO | Continuous digital sample collection |
| Heart-rate processing | Embedded C on RP2350 | On-device estimation without a host computer |
| Sample history | 500-sample circular buffer | Approximately five seconds at the configured 100 Hz |
| Measurement quality | Contact, movement, rate and peak-interval checks | Suppress unreliable BPM estimates |
| Local interface | ST7735S TFT over SPI0 | Immediate live feedback |
| Diagnostics | USB serial output and FIFO event counter | Observable behavior during testing |

## Hardware Stack

| Part | Role |
|---|---|
| **Raspberry Pi Pico 2 W (RP2350)** | Microcontroller running the C firmware |
| **MAX30102 breakout** | Red and infrared optical sensor |
| **ST7735S 1.8-inch TFT (128 × 160)** | Live BPM, R and status output |
| Breadboard, jumper wires and USB cable | Prototype interconnection and power |

**Connections used in the prototype:**

| Device | Signal | Raspberry Pi Pico 2 W |
|---|---|---|
| MAX30102 | SDA / SCL | GP4 / GP5 (I²C0) |
| MAX30102 | VIN / GND | 3V3(OUT) / common GND |
| ST7735S | SCK / MOSI | GP18 / GP19 (SPI0) |
| ST7735S | CS / DC / RST | GP17 / GP20 / GP21 |
| ST7735S | VCC / GND | 3V3(OUT) / common GND |

The TFT backlight connection depends on the actual module and its current-limiting circuitry; verify the module specification before reproducing the wiring.

## System Architecture

The embedded firmware is divided into sensor acquisition, measurement storage, BPM processing, RED/IR ratio processing, and display/UI modules.

![PicoPulse system architecture](images/system_architecture.png)

**Source modules** (kept in the repository root to match the working local CMake project):

| File | Responsibility |
|---|---|
| `main.c` | Initialization, acquisition loop and result coordination |
| `max30102.c/.h` | Sensor setup, I²C registers and FIFO samples |
| `measurement.c/.h` | Sample buffer and signal statistics |
| `pulse.c/.h` | Filtering, peak detection and BPM estimation |
| `spo2.c/.h` | RED/IR AC/DC ratio computation (experimental) |
| `display.c/.h` | ST7735S SPI driver, drawing and bitmap text |
| `ui.c/.h` | Screen layout and status visualization |

## Signal Processing — From PPG Samples to BPM

The MAX30102 is configured for **100 samples per second**. The firmware maintains a 500-sample circular buffer and evaluates an approximately five-second window. The processing chain smooths short-term noise, removes a local baseline, identifies candidate pulse peaks, and evaluates whether the intervals are sufficiently regular.

![PicoPulse signal processing](images/signal_processing.png)

The BPM estimate is calculated from the mean spacing between accepted peaks:

\[
\mathrm{BPM}=\frac{60 f_s}{\overline{N}}
\]

Here, \(f_s\) is the estimated sample reception rate in Hz and \(\overline{N}\) is the mean peak-to-peak distance in samples. The reception rate is measured from Pico timestamps; it is **not** an independent guarantee that every sensor sample was preserved.

### Experimental RED/IR Analysis

For pulse oximetry research, the firmware computes:

\[
R=\frac{AC_{RED}/DC_{RED}}{AC_{IR}/DC_{IR}}
\]

The current implementation derives relative AC amplitudes using a baseline-removed RMS measurement and DC levels from the mean signal. **The resulting R value is an optical feature, not a validated oxygen-saturation percentage.** A device-specific calibration and appropriate validation would be required to convert it into clinically trustworthy SpO₂.

## User Interface and Hardware Debugging

The ST7735S screen presents **BPM, the experimental R ratio, signal status and detected FIFO overflow count**. Invalid BPM measurements are replaced by a placeholder instead of being displayed as current readings.

Two particularly important issues emerged during integration:

- **Pressure and motion sensitivity:** Varying finger pressure caused strong optical disturbances and irregular peak detection. Supporting the hand on a table and keeping pressure steady improved stability.
- **White TFT screen:** The backlight worked, but the panel received no meaningful graphics because the firmware selected **SPI1** while the chosen GPIO pins were configured for **SPI0**. Correcting the peripheral selection restored the color test and live interface.

*Prototype and display photographs will be added to this section.*

## Experimental Validation and Results

The tests below are observations from development sessions, **not clinical performance claims**.

![PicoPulse experimental results](images/experimental_results.png)

| Check | Recorded observation | Interpretation |
|---|---|---|
| Sensor identification | Device ID `0x15` | Basic I²C communication confirmed |
| Finger contact | Large RED/IR increase when finger placed | Optical sensor responds to contact |
| Sample reception | Approximately **100.3–100.4 samples/s** | Stable observed reception during tests |
| FIFO | **0 reported overflow events** | No overflows reported in logged sessions |
| BPM estimation | Repeated estimates around **88–90 BPM** in one stationary session | Algorithm produced plausible estimates |
| Manual pulse check | **88–90 BPM**, measured afterwards | Plausibility comparison only; not simultaneous |
| Experimental R ratio | Approximately **0.426–0.462** in one session | Relatively gradual variation under stationary contact |
| Display | RGB color test and live UI worked | SPI output and rendering demonstrated |

**Interpretation:** The prototype demonstrates acquisition, processing and local display. More controlled, simultaneous reference measurements are needed to quantify heart-rate accuracy; the optical ratio has not been calibrated to SpO₂.

## Build and Run

**Prerequisites:** Raspberry Pi Pico SDK, an ARM embedded toolchain, CMake, a supported build tool and Raspberry Pi Pico 2 W. The committed CMake configuration targets `pico2_w` and enables USB serial output.

```bash
cmake -S . -B build
cmake --build build
```

With the Pico connected in **BOOTSEL** mode, the firmware can be copied on macOS using:

```bash
cp build/pulsmatare.uf2 /Volumes/RP2350/
```

The USB serial port can be opened in a serial monitor (115200 baud is used in the development setup) to inspect acquisition rate, BPM, R and FIFO diagnostics.

## Limitations and Scope

This is a breadboard-based **educational engineering prototype**, not a certified medical device. Signal quality depends on placement and finger pressure; the BPM estimator has not undergone controlled reference validation; and the optical RED/IR ratio is **not a calibrated SpO₂ measurement**. It must not be used for diagnosis, patient monitoring or medical decisions.

## References

- [MAX30102 — Analog Devices](https://www.analog.com/en/products/max30102.html)
- [MAX30102 datasheet](https://www.analog.com/media/en/technical-documentation/data-sheets/MAX30102.pdf)
- [Raspberry Pi Pico-series documentation](https://www.raspberrypi.com/documentation/microcontrollers/pico-series.html)
- [Raspberry Pi Pico SDK](https://github.com/raspberrypi/pico-sdk)
