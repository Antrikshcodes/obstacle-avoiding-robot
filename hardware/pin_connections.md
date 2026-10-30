# Hardware Pin Connections and Wiring Specification

This document provides the exhaustive hardware pin mapping, electrical interface rules, and signal conditioning details for the **Autonomous Obstacle Avoiding Robot**.

---

## 1. Master Pin Connection Table

| Subsystem / Peripheral | Peripheral Pin | ESP32 GPIO Pin | Signal Type | Electrical Level | Functional Role |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **L298N Motor Driver** | ENA | `GPIO 25` | Output (PWM) | 3.3V Logic | Left Motor Speed Control (LEDC PWM) |
| | IN1 | `GPIO 26` | Output (Digital) | 3.3V Logic | Left Motor Direction Input 1 |
| | IN2 | `GPIO 27` | Output (Digital) | 3.3V Logic | Left Motor Direction Input 2 |
| | IN3 | `GPIO 32` | Output (Digital) | 3.3V Logic | Right Motor Direction Input 3 |
| | IN4 | `GPIO 33` | Output (Digital) | 3.3V Logic | Right Motor Direction Input 4 |
| | ENB | `GPIO 19` | Output (PWM) | 3.3V Logic | Right Motor Speed Control (LEDC PWM) |
| | GND | `GND` | Power Reference | 0V | Common Ground reference |
| **HC-SR04 Sonar** | VCC | `5V` (Ext / L298N) | Power Input | 5.0V DC | Primary operating voltage |
| | Trig | `GPIO 5` | Output (Digital) | 3.3V Logic | $10\,\mu\text{s}$ Trigger pulse |
| | Echo | `GPIO 18` | Input (Digital) | 3.3V via Divider | Echo duration proportional to distance |
| | GND | `GND` | Power Reference | 0V | Common Ground reference |
| **SG90 Micro Servo** | Signal (Orange/Yellow) | `GPIO 13` | Output (PWM) | 3.3V Logic | 50 Hz PWM positioning signal |
| | VCC (Red) | `5V` (Ext / L298N) | Power Input | 5.0V DC | Motor drive power rail |
| | GND (Brown/Black) | `GND` | Power Reference | 0V | Common Ground reference |
| **Power Distribution** | Battery (+) 7.4V–8.4V | L298N `12V` Terminal | Power Input | 7.4V – 8.4V | Primary battery drive supply |
| | Battery (-) GND | L298N `GND` Terminal | Power Reference | 0V | System ground return |
| | L298N 5V Out | ESP32 `VIN` / `5V` | Regulated DC | 5.0V DC | Powers ESP32 internal 3.3V LDO |

---

## 2. ESP32 GPIO Selection & Validation Rationale

The ESP32 possesses diverse pin multiplexing configurations and critical hardware constraints. The pins for this project were selected according to the following engineering criteria:

1. **Avoiding Boot Strapping Pins:**
   * `GPIO 0`: Bootloader mode pin. Pulling low during reset forces UART download mode.
   * `GPIO 2`: Strapping pin; must be left floating or pulled low during flashing.
   * `GPIO 12` (MTDI): Flash voltage select (VDD_SDIO). Pulling HIGH at boot forces internal flash regulator to 1.8V instead of 3.3V, causing ESP32 boot failure.
   * `GPIO 15`: Outputs PWM debug signal at boot and affects silent boot logging.
   * *Outcome:* None of the motor control, ultrasonic, or servo lines connect to GPIO 0, 2, 12, or 15.

2. **Avoiding Input-Only Pins:**
   * `GPIO 34, 35, 36, 39` (Sensor VN/VP) lack internal pull-up and pull-down circuitry and cannot output signals.
   * *Outcome:* All motor direction controls and PWM channels are strictly routed to output-capable pins (GPIO 19, 25, 26, 27, 32, 33, 13, 5).

3. **LEDC PWM Routing:**
   * GPIO 25 and GPIO 19 are routed directly through ESP32's internal LEDC matrix, allowing full 8-bit hardware PWM resolution at 1 kHz without CPU overhead.

---

## 3. HC-SR04 Echo Level Shifter (Voltage Divider)

The HC-SR04 ultrasonic sensor operates at a nominal supply voltage of $5.0\text{ V}$. Its `ECHO` output pin drives high to approximately $5.0\text{ V}$ TTL logic. Because the ESP32 input pins are rated for a maximum input voltage of $V_{DD} + 0.3\text{V} \approx 3.6\text{ V}$, a direct connection can degrade or damage the ESP32 GPIO over time.

### Voltage Divider Topology

```
HC-SR04 Echo (5V)
       │
       ┌┴┐
       │ │ R1 = 1.0 kΩ
       └┬┘
        ├───→ Connected to ESP32 GPIO 18 (Measured: ~3.33V)
       ┌┴┐
       │ │ R2 = 2.0 kΩ (or 2.2 kΩ)
       └┬┘
       │
      GND (0V Common)
```

### Voltage Calculation

Using the standard voltage division equation:

$$V_{\text{ESP32\_IN}} = V_{\text{ECHO}} \times \left( \frac{R_2}{R_1 + R_2} \right)$$

Substituting nominal component values:

$$V_{\text{ESP32\_IN}} = 5.0\text{ V} \times \left( \frac{2000\,\Omega}{1000\,\Omega + 2000\,\Omega} \right) = 5.0\text{ V} \times \frac{2}{3} \approx 3.333\text{ V}$$

* **Tolerance check:** If using a standard $2.2\,\text{k}\Omega$ resistor:
  $$V_{\text{ESP32\_IN}} = 5.0\text{ V} \times \left( \frac{2200}{1000 + 2200} \right) = 5.0 \times 0.6875 \approx 3.437\text{ V}$$
  Both configurations remain safely below the $3.6\text{ V}$ absolute maximum rating while satisfying the $V_{IH}$ (high-level input voltage threshold of $0.75 \times V_{DD} \approx 2.475\text{ V}$).

---

## 4. Power & Common Ground Rules

1. **Common Ground Connection:**
   The `GND` of the ESP32 board, the `GND` of the L298N driver, the `GND` of the ultrasonic sensor, the `GND` of the SG90 servo, and the negative terminal (`-`) of the battery pack must all be connected to a single reference ground node. Without a shared ground, digital signals will float, causing random motor triggering and invalid distance measurements.

2. **Decoupling from Logic Rail:**
   Never power the SG90 servo motor from the ESP32 `3V3` pin. Servos can pull peak currents exceeding $500\text{ mA}$ under stall or rapid direction reversal, which will immediately drop the ESP32 core voltage below the $2.7\text{ V}$ brownout threshold and cause continuous system reboots.
