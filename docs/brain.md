# Line Follower Robot - System Architecture & Knowledge Base ("The Brain")

> **Document Status:** Baseline Archive & Reverse-Engineering Reference  
> **Source Baseline:** Firmware V6.1 ("The Golden Restoration")  
> **Hardware Platform:** ESP32 DOIT DevKit V1 + TB6612FNG Driver + 8-Channel IR Array

---

## 1. System Overview

This autonomous line follower robot is engineered for competition-grade track following, 90-degree intersection handling, maze routing (Left-Hand Rule bias), and checkpoint reporting.

The robot combines high-frequency **Proportional-Derivative (PD) closed-loop steering** with a stateful **Zero-Delay Intersection Memory Engine** and **Wireless OTA (Over-The-Air) telemetry**.

---

## 2. Hardware Architecture & Pin Mapping

### 2.1 Microcontroller: ESP32 DOIT DevKit V1
* **Core Frequency:** 240 MHz dual-core Xtensa LX6
* **Flash:** 4MB
* **Logic Voltage:** 3.3V (IR array & TB6612 inputs accept 3.3V logic)

### 2.2 Pinout Table

| Function | Pin Name | ESP32 GPIO | Direction | Description |
| :--- | :--- | :--- | :--- | :--- |
| **Left Motor Speed** | `PWMA` | `GPIO 25` | Output (PWM) | Motor A PWM speed control (0–255) |
| **Left Motor Dir 1** | `AIN1` | `GPIO 26` | Output (Digital) | Motor A Direction Bit 1 |
| **Left Motor Dir 2** | `AIN2` | `GPIO 27` | Output (Digital) | Motor A Direction Bit 2 |
| **Right Motor Speed** | `PWMB` | `GPIO 16` | Output (PWM) | Motor B PWM speed control (0–255) |
| **Right Motor Dir 1** | `BIN1` | `GPIO 17` | Output (Digital) | Motor B Direction Bit 1 |
| **Right Motor Dir 2** | `BIN2` | `GPIO 18` | Output (Digital) | Motor B Direction Bit 2 |
| **IR Sensor 1 (Far Right)** | `D1` | `GPIO 36` (VP) | Input (Digital) | Outer Right Sensor (Index 0) |
| **IR Sensor 2** | `D2` | `GPIO 39` (VN) | Input (Digital) | Mid-Right Sensor (Index 1) |
| **IR Sensor 3** | `D3` | `GPIO 34` | Input (Digital) | Inner Right Sensor (Index 2) |
| **IR Sensor 4 (Center Right)** | `D4` | `GPIO 35` | Input (Digital) | Center-Right Guidance Sensor (Index 3) |
| **IR Sensor 5 (Center Left)** | `D5` | `GPIO 32` | Input (Digital) | Center-Left Guidance Sensor (Index 4) |
| **IR Sensor 6** | `D6` | `GPIO 33` | Input (Digital) | Inner Left Sensor (Index 5) |
| **IR Sensor 7** | `D7` | `GPIO 22` | Input (Digital) | Mid-Left Sensor (Index 6) |
| **IR Sensor 8 (Far Left)** | `D8` | `GPIO 23` | Input (Digital) | Outer Left Sensor (Index 7) |
| **Status LED** | `BONUS_LED` | `GPIO 4` | Output (Digital) | Visual state indicator |
| **Acoustic Alert** | `BUZZER_PIN` | `GPIO 5` | Output (Digital) | Non-blocking checkpoint beeper |

> [!IMPORTANT]
> **Sensor Array Orientation:**  
> `sensorPins[0]` = GPIO 36 is on the **FAR RIGHT** of the robot.  
> `sensorPins[7]` = GPIO 23 is on the **FAR LEFT** of the robot.  
> Digital Read value of `1` indicates line detected (black line on white floor).

---

## 3. Mathematical & Control Models

### 3.1 Sensor Weighting & Error Calculation
Error is calculated as a weighted sum across all 8 sensors:

$$\text{weights} = [-40, -30, -20, -10, 10, 20, 30, 40]$$

$$\text{Error} = \sum_{i=0}^{7} (\text{digitalRead}(sensorPins[i]) == \text{LINE}) \times \text{weights}[i]$$

* **Negative Error ($< 0$):** Line is under the right side $\rightarrow$ Robot steers right.
* **Positive Error ($> 0$):** Line is under the left side $\rightarrow$ Robot steers left.
* **Zero Error ($= 0$):** Robot is perfectly centered on the line.

### 3.2 PD Controller
The steering correction is computed using Proportional and Derivative terms:

$$P = \text{Error}$$
$$D = \text{Error} - \text{lastError}$$
$$\text{Correction} = (K_p \times P) + (K_d \times D)$$

**Current Proven Parameters (V6.1):**
* $K_p = 2.0$ (Proportional response for line tracking)
* $K_d = 14.0$ (Derivative damping to prevent oscillation)
* $\text{baseSpeed} = 130$ (Normal forward cruising speed out of 255)

### 3.3 Motor Compensation (Drift Balancing)
Physical DC motors do not have identical RPM curves. The calibration factors balance them:
* `leftCalibration = 1.00`
* `rightCalibration = 0.98` (Right motor was mechanically ~2% faster)

$$\text{leftMotorPWM} = (\text{baseSpeed} - \text{Correction}) \times \text{leftCalibration}$$
$$\text{rightMotorPWM} = (\text{baseSpeed} + \text{Correction}) \times \text{rightCalibration}$$

---

## 4. Key Algorithms & State Logic

### 4.1 Zero-Delay "Wait for White" Turn Engine
Traditional line followers use blocking `delay()` calls when hitting a 90° corner, causing loss of momentum and sensor blindness. 

**V6.1 Zero-Delay Logic:**
1. **Intersection Detection (The Vault):**
   * If $\ge 3$ left sensors activate: Lock `turnMemory = 50` (Left Turn).
   * If $\ge 3$ right sensors activate: Lock `turnMemory = -50` (Right Turn).
   * **Left-First Bias:** Left turns are evaluated before right turns to guarantee Left-Hand-Rule maze exit.
2. **Intersection Blindness:**
   * While passing across the intersection, force `error = 0` to drive straight through the junction until all sensors clear the tape.
3. **White Space Trigger:**
   * When `sensorsOnLine == 0` (robot clears the tape), it enters `isRecovering = true` and executes an in-place tank spin (`spinSpeed = 110`) in the direction recorded in `turnMemory`.
4. **Recovery Lock:**
   * The robot continues spinning until center sensors (pin 3 or 4) re-acquire the black line, then immediately switches back to normal PD tracking with zero time wasted.

### 4.2 Non-Blocking Checkpoint Audio Engine
* Checkpoints are wide horizontal transverse lines where $\ge 6$ sensors detect black simultaneously.
* Rather than calling `delay(150)` (which freezes motor PID updates), V6.1 uses an asynchronous timestamp:
  ```cpp
  if (sensorsOnLine >= 6) {
      beepUntil = millis() + 150;
  }
  if (millis() < beepUntil) {
      digitalWrite(BUZZER_PIN, HIGH);
      digitalWrite(BONUS_LED, HIGH);
  } else {
      digitalWrite(BUZZER_PIN, LOW);
      digitalWrite(BONUS_LED, LOW);
  }
  ```

### 4.3 OTA Wireless Emergency Safety Brake
When uploading firmware wirelessly over WiFi via ArduinoOTA:
* The `ArduinoOTA.onStart()` callback immediately forces `analogWrite(PWMA, 0)` and `analogWrite(PWMB, 0)`.
* This prevents the robot from driving off the table/track while new code is transmitting.

---

## 5. Evolutionary History & Lessons Learned

| Iteration | Key Changes | Outcome / Retrospective |
| :--- | :--- | :--- |
| **V1 – V3** | Initial prototyping & basic thresholding | Heavy overshoot on curves; required memory state. |
| **V4** | Zero-delay "wait for white" introduced | First rock-solid run. Handled 90° turns without stopping. |
| **V5** | Added buzzer pin (GPIO 5) & checkpoint detector | Buzzer chirped intermittently; right-turn bias caused maze loops. |
| **V6** | Gauntlet Solver (Straight bulldozer push, in-air safety, color invert) | **Failed experiment:** Excessive `delay()` calls caused jerky movement and line loss. |
| **V7 LFR** | Barebones Left-First Rule experiment | **Bugged:** Sensor pin index was accidentally inverted (`i == 0 \|\| i == 1` marked as left). Stripped OTA. |
| **V6.1** | **The Golden Restoration** | Combined V4 smooth physics + non-blocking buzzer + Left-First bias. Cleanest and fastest version. |
| **V6.2** | Instant Whip experiment | Attempted tank-turn `while()` loops with inverted weights; unverified and omitted checkpoints. |

---

## 6. Architecture Blueprint for Ground-Up Rewrite

When rebuilding from scratch (clean OOP architecture), implement the following component separation:

```
src/
├── main.cpp                 # Loop orchestration & state machine runner
├── config/
│   └── RobotConfig.h        # Pinouts, speed limits, PID constants, WiFi credentials
├── drivers/
│   ├── MotorController.h/cpp# TB6612FNG driver (PWM, braking, calibration scaling)
│   └── SensorArray.h/cpp    # RLS-08 8-channel reader, binary masking, raw/weighted error
├── control/
│   ├── PIDController.h/cpp  # Standard PID with anti-windup and derivative filtering
│   └── Navigator.h/cpp      # State machine (FOLLOWING, INTERSECTION, SPINNING, CHECKPOINT)
└── network/
    └── OTAHandler.h/cpp     # WiFi connection management & OTA emergency brake
```

### Key Improvements Planned for the Rewrite:
1. **True OOP Abstraction:** Remove all global mutable variables into structured classes.
2. **Configurable Drive Modes:** State machine pattern (`DriveMode::NORMAL_PID`, `DriveMode::INTERSECTION_ACQUIRE`, `DriveMode::EMERGENCY_STOP`).
3. **Dynamic Calibration Mode:** Power-on routine where the robot pivots 360° to automatically calculate min/max analog thresholds for each IR sensor.
4. **Telemetry/Serial Dash:** Output live error, loop frequency (Hz), and PID contributions over serial/Bluetooth for tuning on the fly.
