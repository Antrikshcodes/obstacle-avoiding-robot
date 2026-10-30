/**
 * ============================================================================
 * Project: Autonomous Obstacle Avoiding Robot
 * File: motor_control.h
 * Description: Modular motor control driver for L298N Dual H-Bridge on ESP32.
 * Target Hardware: ESP32 Dev Module with L298N Motor Driver
 * ============================================================================
 */

#ifndef MOTOR_CONTROL_H
#define MOTOR_CONTROL_H

#include <Arduino.h>
#include "config.h"

// Check for ESP32 Arduino core version compatibility
#if defined(ESP_ARDUINO_VERSION_MAJOR) && (ESP_ARDUINO_VERSION_MAJOR >= 3)
  #define USE_ESP32_CORE_V3 1
#else
  #define USE_ESP32_CORE_V3 0
#endif

class MotorController {
public:
    MotorController() : _leftSpeed(SPEED_DEFAULT), _rightSpeed(SPEED_DEFAULT) {}

    /**
     * @brief Configures GPIO modes and initializes ESP32 PWM channels.
     */
    void begin() {
        // Configure direction control pins as outputs
        pinMode(PIN_MOTOR_IN1, OUTPUT);
        pinMode(PIN_MOTOR_IN2, OUTPUT);
        pinMode(PIN_MOTOR_IN3, OUTPUT);
        pinMode(PIN_MOTOR_IN4, OUTPUT);

        // Configure PWM pins
        #if USE_ESP32_CORE_V3
            // Core v3.0+ API: direct pin binding
            ledcAttach(PIN_MOTOR_ENA, PWM_FREQUENCY, PWM_RESOLUTION_BITS);
            ledcAttach(PIN_MOTOR_ENB, PWM_FREQUENCY, PWM_RESOLUTION_BITS);
        #else
            // Core v2.x API: channel-based configuration
            ledcSetup(PWM_CHANNEL_LEFT, PWM_FREQUENCY, PWM_RESOLUTION_BITS);
            ledcAttachPin(PIN_MOTOR_ENA, PWM_CHANNEL_LEFT);

            ledcSetup(PWM_CHANNEL_RIGHT, PWM_FREQUENCY, PWM_RESOLUTION_BITS);
            ledcAttachPin(PIN_MOTOR_ENB, PWM_CHANNEL_RIGHT);
        #endif

        // Ensure motors are initially powered down
        stop();
        DEBUG_PRINTLN("[MOTOR] Controller initialized. Motors halted.");
    }

    /**
     * @brief Sets custom independent speeds for left and right tracks/wheels.
     * @param left PWM duty cycle (0 - 255)
     * @param right PWM duty cycle (0 - 255)
     */
    void setSpeeds(uint8_t left, uint8_t right) {
        _leftSpeed = left;
        _rightSpeed = right;
    }

    /**
     * @brief Drives both motors forward at the specified PWM duty cycle.
     * @param speed PWM speed value (0 - 255). Defaults to SPEED_DEFAULT if 0.
     */
    void moveForward(uint8_t speed = SPEED_DEFAULT) {
        applyPwm(speed, speed);
        // Left Motor Forward: IN1 HIGH, IN2 LOW
        digitalWrite(PIN_MOTOR_IN1, HIGH);
        digitalWrite(PIN_MOTOR_IN2, LOW);
        // Right Motor Forward: IN3 HIGH, IN4 LOW
        digitalWrite(PIN_MOTOR_IN3, HIGH);
        digitalWrite(PIN_MOTOR_IN4, LOW);
        DEBUG_PRINTF("[MOTOR] Forward @ PWM %u\n", speed);
    }

    /**
     * @brief Drives both motors in reverse at the specified PWM duty cycle.
     * @param speed PWM speed value (0 - 255). Defaults to SPEED_REVERSE if 0.
     */
    void moveBackward(uint8_t speed = SPEED_REVERSE) {
        applyPwm(speed, speed);
        // Left Motor Reverse: IN1 LOW, IN2 HIGH
        digitalWrite(PIN_MOTOR_IN1, LOW);
        digitalWrite(PIN_MOTOR_IN2, HIGH);
        // Right Motor Reverse: IN3 LOW, IN4 HIGH
        digitalWrite(PIN_MOTOR_IN3, LOW);
        digitalWrite(PIN_MOTOR_IN4, HIGH);
        DEBUG_PRINTF("[MOTOR] Backward @ PWM %u\n", speed);
    }

    /**
     * @brief Executes an in-place pivot turn to the left.
     * Left wheel reverses, right wheel moves forward.
     * @param speed PWM speed value (0 - 255). Defaults to SPEED_TURN if 0.
     */
    void turnLeft(uint8_t speed = SPEED_TURN) {
        applyPwm(speed, speed);
        // Left Motor Reverse
        digitalWrite(PIN_MOTOR_IN1, LOW);
        digitalWrite(PIN_MOTOR_IN2, HIGH);
        // Right Motor Forward
        digitalWrite(PIN_MOTOR_IN3, HIGH);
        digitalWrite(PIN_MOTOR_IN4, LOW);
        DEBUG_PRINTF("[MOTOR] Pivot Left @ PWM %u\n", speed);
    }

    /**
     * @brief Executes an in-place pivot turn to the right.
     * Left wheel moves forward, right wheel reverses.
     * @param speed PWM speed value (0 - 255). Defaults to SPEED_TURN if 0.
     */
    void turnRight(uint8_t speed = SPEED_TURN) {
        applyPwm(speed, speed);
        // Left Motor Forward
        digitalWrite(PIN_MOTOR_IN1, HIGH);
        digitalWrite(PIN_MOTOR_IN2, LOW);
        // Right Motor Reverse
        digitalWrite(PIN_MOTOR_IN3, LOW);
        digitalWrite(PIN_MOTOR_IN4, HIGH);
        DEBUG_PRINTF("[MOTOR] Pivot Right @ PWM %u\n", speed);
    }

    /**
     * @brief Cuts PWM power to all motors and sets H-bridge to floating/brake state.
     */
    void stop() {
        applyPwm(0, 0);
        digitalWrite(PIN_MOTOR_IN1, LOW);
        digitalWrite(PIN_MOTOR_IN2, LOW);
        digitalWrite(PIN_MOTOR_IN3, LOW);
        digitalWrite(PIN_MOTOR_IN4, LOW);
        DEBUG_PRINTLN("[MOTOR] Stopped.");
    }

private:
    uint8_t _leftSpeed;
    uint8_t _rightSpeed;

    /**
     * @brief Applies PWM signal to Enable A and Enable B channels.
     */
    void applyPwm(uint8_t left, uint8_t right) {
        #if USE_ESP32_CORE_V3
            ledcWrite(PIN_MOTOR_ENA, left);
            ledcWrite(PIN_MOTOR_ENB, right);
        #else
            ledcWrite(PWM_CHANNEL_LEFT, left);
            ledcWrite(PWM_CHANNEL_RIGHT, right);
        #endif
    }
};

#endif // MOTOR_CONTROL_H
