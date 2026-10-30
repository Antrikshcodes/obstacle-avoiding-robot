/**
 * ============================================================================
 * Project: Autonomous Obstacle Avoiding Robot
 * File: config.h
 * Description: Global configuration, pin definitions, thresholds, and timings.
 * Target Hardware: ESP32 Dev Module (WROOM-32)
 * ============================================================================
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ============================================================================
// SERIAL DEBUGGING CONFIGURATION
// ============================================================================
#define DEBUG_ENABLED           1               // Set to 1 to enable serial telemetry, 0 to disable
#define SERIAL_BAUD_RATE        115200          // Serial monitor baud rate

#if DEBUG_ENABLED
  #define DEBUG_PRINT(x)        Serial.print(x)
  #define DEBUG_PRINTLN(x)      Serial.println(x)
  #define DEBUG_PRINTF(...)     Serial.printf(__VA_ARGS__)
#else
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINTLN(x)
  #define DEBUG_PRINTF(...)
#endif

// ============================================================================
// ESP32 GPIO PIN ASSIGNMENTS
// ============================================================================
// Note: Selected pins avoid strapping conflicts (GPIO 0, 2, 12, 15) and input-only pins (34-39).

// L298N Dual H-Bridge Motor Driver
#define PIN_MOTOR_ENA           25      // PWM Enable pin for Motor A (Left)
#define PIN_MOTOR_IN1           26      // Direction input 1 for Motor A (Left)
#define PIN_MOTOR_IN2           27      // Direction input 2 for Motor A (Left)
#define PIN_MOTOR_IN3           32      // Direction input 1 for Motor B (Right)
#define PIN_MOTOR_IN4           33      // Direction input 2 for Motor B (Right)
#define PIN_MOTOR_ENB           19      // PWM Enable pin for Motor B (Right)

// Ultrasonic Sensor (HC-SR04)
// CAUTION: HC-SR04 Echo output is 5V TTL logic. ESP32 pins are 3.3V tolerant.
// An external voltage divider (e.g., 1kΩ / 2kΩ) MUST be used on the Echo pin.
#define PIN_ULTRASONIC_TRIG     5       // Output trigger pulse
#define PIN_ULTRASONIC_ECHO     18      // Input echo pulse (through voltage divider)

// SG90 Micro Servo
#define PIN_SERVO               13      // PWM control signal for servo turret

// ============================================================================
// ESP32 PWM CONFIGURATION (LEDC PERIPHERAL)
// ============================================================================
#define PWM_FREQUENCY           1000    // PWM frequency in Hz (optimal for L298N bipolar switches)
#define PWM_RESOLUTION_BITS     8       // 8-bit resolution (duty cycle range: 0 - 255)
#define PWM_CHANNEL_LEFT        0       // LEDC channel allocated for Left Motor
#define PWM_CHANNEL_RIGHT       1       // LEDC channel allocated for Right Motor

// ============================================================================
// MOTOR SPEED PRESETS (0 to 255)
// ============================================================================
// Tune these values based on battery voltage (e.g., 7.4V vs 9V) and chassis friction.
#define SPEED_DEFAULT           180     // Cruising forward speed
#define SPEED_TURN              165     // Differential turn rotation speed
#define SPEED_REVERSE           150     // Backing-up speed
#define SPEED_MIN_THRESHOLD     120     // Minimum PWM value required to overcome motor deadband

// ============================================================================
// ULTRASONIC SENSOR & DETECTION PARAMETERS
// ============================================================================
#define OBSTACLE_STOP_DISTANCE_CM       25.0f   // Distance threshold to trigger obstacle avoidance (cm)
#define OBSTACLE_CRITICAL_DISTANCE_CM   12.0f   // Critical threshold requiring immediate reverse (cm)
#define MAX_SENSOR_DISTANCE_CM          300.0f  // Upper valid distance limit for HC-SR04
#define MIN_SENSOR_DISTANCE_CM          2.0f    // Lower valid distance limit for HC-SR04
#define SENSOR_TIMEOUT_US               18000   // pulseIn timeout in µs (~300 cm round-trip)
#define SENSOR_SAMPLE_COUNT             3       // Number of samples for median filtering

// ============================================================================
// SERVO SCANNING ANGLES (Degrees)
// ============================================================================
#define SERVO_ANGLE_CENTER              90      // Facing directly forward
#define SERVO_ANGLE_RIGHT               20      // Facing right (avoid 0° to prevent mechanical strain)
#define SERVO_ANGLE_LEFT                160     // Facing left (avoid 180° to prevent mechanical strain)
#define SERVO_SETTLING_DELAY_MS         280     // Delay to let SG90 reach position and settle vibrations

// ============================================================================
// MANEUVER TIMING & RECOVERY (Milliseconds)
// ============================================================================
#define DURATION_BACKWARD_MS            320     // Time to reverse when clearing tight spaces
#define DURATION_TURN_MS                380     // Time for ~90° pivot turn on high-grip surfaces
#define DURATION_U_TURN_MS              760     // Time for ~180° turn when blocked on both sides
#define SAFETY_POST_MANEUVER_DELAY_MS   150     // Motor stabilization delay between maneuvers
#define STARTUP_SAFETY_DELAY_MS         2000    // Delay after boot before robot starts moving

#endif // CONFIG_H
