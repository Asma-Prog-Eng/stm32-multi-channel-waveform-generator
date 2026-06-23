# STM32 Multi-Channel Independent Waveform Generator

A high-performance embedded application showcasing concurrent multi-frequency signal generation on an **ARM Cortex-M3 (STM32F103RB)** microcontroller using a single 16-bit hardware timer peripheral.

## 🚀 Architectural Overview

Instead of dedicating multiple hardware timers or burning CPU cycles on blocking software delays, this project configures **Timer 2** to generate up to four distinct square-wave frequencies simultaneously across four independent channels. 

The master counter runs continuously up to `0xFFFF`. Every time a specific channel meets its comparison threshold, the micro toggles the GPIO pin *directly in hardware* and drops into an interrupt context. The firmware then dynamically slides that specific channel's compare window forward by an isolated phase tick value.

### Key Performance Specifications
* **System Core Clock:** 48 MHz
* **Timer Timebase:** 24 MHz clock (Providing a clock tick resolution of **41.66 nanoseconds**)
* **Channels Used:** 4 Concurrent Outputs

---

## 📌 Hardware Pin Mapping & Remap Strategy

On the Nucleo-64 (STM32F103RB) board, standard `TIM2` pins `PA2` and `PA3` conflict directly with the onboard ST-LINK USART1 telemetry link. To bypass this, the **AFIO Partial Remap 2** matrix was clocked and activated:

| Timer Channel | Default Pin | Remapped Pin | Target Frequency | Phase Shift (Half-Period Ticks) |
| :--- | :---: | :---: | :---: | :---: |
| **TIM2_CH1** | PA0 | **PA0** | 500 Hz | 24,000 Ticks (1.00 ms) |
| **TIM2_CH2** | PA1 | **PA1** | 1.00 kHz | 12,000 Ticks (0.50 ms) |
| **TIM2_CH3** | PA2 | **PB10** | 2.00 kHz | 6,000 Ticks (0.25 ms) |
| **TIM2_CH4** | PA3 | **PB11** | 4.00 kHz | 3,000 Ticks (0.125 ms) |

---

## 🛠️ Lessons Learned & Debugging Milestones

### 1. The Auto-Reload Register (ARR) Trap
* **Symptom:** Setting a static pulse value of `26000` while setting the timer period (`ARR`) to `0xFFFF` resulted in a constant output frequency of **198 Hz** on the logic analyzer regardless of math tweaks.
* **Resolution:** Discovered the timer was counting through its full 16-bit range (`65536` total ticks) rather than resetting at the edge phase. Transitioned the architecture from standard static comparison to a **dynamic phase-shifting ISR engine**.

### 2. The AFIO Clock Gating Gotcha
* **Symptom:** Setting up Port B (`PB10`/`PB11`) resulted in perfectly dead/flat voltage lines on Channels 3 and 4.
* **Resolution:** Identified that the Alternate Function I/O module requires explicit bus clocking. Un-commenting `__HAL_RCC_AFIO_CLK_ENABLE()` successfully unfroze the `AFIO->MAPR` register to lock in the routing.
---

## 📂 Core Firmware Components
* `main_app.c`: Configures clock trees, declares frequency targets, and coordinates application initialization.
* `msp.c`: Activates low-level peripheral clock gates (`TIM2`, `GPIOA`, `GPIOB`, `AFIO`), maps alternate push-pull functions, and configures Nested Vector Interrupt priorities (`TIM2_IRQn`).
* `it.c`: Implements the high-speed Interrupt Service Routines (`TIM2_IRQHandler`) and handles real-time dynamic pointer manipulation.

---

## 💻 How To Inspect
1. Compile and flash using STM32CubeIDE or your preferred toolchain.
2. Connect a digital logic analyzer to pins **PA0, PA1, PB10, and PB11**.
<img width="1861" height="971" alt="image" src="https://github.com/user-attachments/assets/1fb883fb-8b38-412f-896a-63c428c58d26" />

