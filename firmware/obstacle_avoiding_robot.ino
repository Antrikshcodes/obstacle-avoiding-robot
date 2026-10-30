/**
 * ============================================================================
 * Project: Autonomous Obstacle Avoiding Robot
 * File: obstacle_avoiding_robot.ino
 * Description: Main firmware entry point, autonomous state machine, ultrasonic
 *              sensing, servo scanning, and decision algorithm for ESP32.
 * Target Board: ESP32 Dev Module (WROOM-32)
 * Framework: Arduino ESP32 Core
 * ============================================================================
 */

#include <Arduino.h>
#include <ESP32Servo.h>
#include "config.h"
#include "motor_control.h"

// ============================================================================
// GLOBAL INSTANCES AND STATE
// ============================================================================
MotorController motors;
Servo scanningServo;

// Consecutive turn tracking to prevent oscillation in tight corners
enum TurnDirection { NONE, LEFT, RIGHT };
TurnDirection lastTurnDirection = NONE;
uint8_t consecutiveTurnCount = 0;
const uint8_t MAX_CONSECUTIVE_TURNS = 3;

// Timing tracking for non-blocking forward distance sampling
unsigned long lastSampleTime = 0;
const unsigned long SAMPLE_INTERVAL_MS = 60;

// ============================================================================
// ULTRASONIC SENSOR FUNCTIONS
// ============================================================================

/**
 * @brief Performs a single ultrasonic distance measurement.
 * @return Measured distance in centimeters, or MAX_SENSOR_DISTANCE_CM on timeout.
 */
float readSingleDistance() {
    // Clear trigger pin
    digitalWrite(PIN_ULTRASONIC_TRIG, LOW);
    delayMicroseconds(2);

    // Send a 10 microsecond HIGH pulse to initiate ultrasonic burst
    digitalWrite(PIN_ULTRASONIC_TRIG, HIGH);
    delayMicroseconds(10);
    digitalWrite(PIN_ULTRASONIC_TRIG, LOW);

    // Measure the duration of the HIGH pulse on Echo pin (with timeout)
    unsigned long durationUs = pulseIn(PIN_ULTRASONIC_ECHO, HIGH, SENSOR_TIMEOUT_US);

    // If duration is 0, echo timed out (no obstacle within range)
    if (durationUs == 0) {
        return MAX_SENSOR_DISTANCE_CM;
    }

    // Formula: Distance (cm) = (Duration (µs) * Speed of Sound (0.0343 cm/µs)) / 2
    // Simplified: Distance (cm) = Duration / 58.3
    float distanceCm = (float)durationUs * 0.0343f / 2.0f;

    // Constrain to physically valid sensor limits
    if (distanceCm < MIN_SENSOR_DISTANCE_CM) {
        distanceCm = MIN_SENSOR_DISTANCE_CM;
    } else if (distanceCm > MAX_SENSOR_DISTANCE_CM) {
        distanceCm = MAX_SENSOR_DISTANCE_CM;
    }

    return distanceCm;
}

/**
 * @brief Reads distance with a 3-point median filter to reject acoustic glitches.
 * @return Filtered distance in centimeters.
 */
float getFilteredDistance() {
    float samples[SENSOR_SAMPLE_COUNT];

    for (uint8_t i = 0; i < SENSOR_SAMPLE_COUNT; i++) {
        samples[i] = readSingleDistance();
        if (i < SENSOR_SAMPLE_COUNT - 1) {
            delayMicroseconds(3000); // 3ms separation to avoid acoustic multipath echoes
        }
    }

    // Simple 3-element sorting network to extract median
    if (samples[0] > samples[1]) { float tmp = samples[0]; samples[0] = samples[1]; samples[1] = tmp; }
    if (samples[1] > samples[2]) { float tmp = samples[1]; samples[1] = samples[2]; samples[2] = tmp; }
    if (samples[0] > samples[1]) { float tmp = samples[0]; samples[0] = samples[1]; samples[1] = tmp; }

    return samples[1]; // Median value
}

// ============================================================================
// SERVO SCANNING & ENVIRONMENT PROFILING
// ============================================================================

/**
 * @brief Sweeps the ultrasonic sensor left and right to evaluate spatial clearance.
 * @param[out] distanceLeft Clearance measured on the left side in cm.
 * @param[out] distanceRight Clearance measured on the right side in cm.
 */
void scanEnvironment(float &distanceLeft, float &distanceRight) {
    DEBUG_PRINTLN("[SCAN] Commencing spatial clearance scan...");

    // 1. Scan Right
    scanningServo.write(SERVO_ANGLE_RIGHT);
    delay(SERVO_SETTLING_DELAY_MS);
    distanceRight = getFilteredDistance();
    DEBUG_PRINTF("[SCAN] Right Clearance: %.1f cm\n", distanceRight);

    // 2. Scan Left
    scanningServo.write(SERVO_ANGLE_LEFT);
    delay(SERVO_SETTLING_DELAY_MS);
    distanceLeft = getFilteredDistance();
    DEBUG_PRINTF("[SCAN] Left Clearance:  %.1f cm\n", distanceLeft);

    // 3. Return to Center
    scanningServo.write(SERVO_ANGLE_CENTER);
    delay(SERVO_SETTLING_DELAY_MS);
    DEBUG_PRINTLN("[SCAN] Turret restored to center heading.");
}

// ============================================================================
// OBSTACLE AVOIDANCE DECISION LOGIC
// ============================================================================

/**
 * @brief Executes intelligent avoidance maneuver when path is obstructed.
 * @param frontDistance The obstacle distance detected in front.
 */
void executeAvoidanceManeuver(float frontDistance) {
    // 1. Immediate Safety Stop
    motors.stop();
    DEBUG_PRINTF("[ALERT] Obstacle detected at %.1f cm! Stopping motors.\n", frontDistance);
    delay(SAFETY_POST_MANEUVER_DELAY_MS);

    // 2. Critical Distance Recovery (if obstacle is too close to pivot safely)
    if (frontDistance < OBSTACLE_CRITICAL_DISTANCE_CM) {
        DEBUG_PRINTLN("[RECOVERY] Within critical threshold. Reversing...");
        motors.moveBackward(SPEED_REVERSE);
        delay(DURATION_BACKWARD_MS);
        motors.stop();
        delay(SAFETY_POST_MANEUVER_DELAY_MS);
    }

    // 3. Scan Left and Right
    float distanceLeft = 0.0f;
    float distanceRight = 0.0f;
    scanEnvironment(distanceLeft, distanceRight);

    // 4. Direction Arbitration & Oscillation Prevention
    bool leftOpen = (distanceLeft >= OBSTACLE_STOP_DISTANCE_CM);
    bool rightOpen = (distanceRight >= OBSTACLE_STOP_DISTANCE_CM);

    if (!leftOpen && !rightOpen) {
        // Dead-end / cul-de-sac detected: Perform 180° U-turn
        DEBUG_PRINTLN("[DECISION] Both flanks blocked. Executing 180° escape turn.");
        motors.turnRight(SPEED_TURN);
        delay(DURATION_U_TURN_MS);
        lastTurnDirection = RIGHT;
        consecutiveTurnCount = 0;
    } else if (distanceLeft > distanceRight) {
        // Left has superior clearance
        if (lastTurnDirection == LEFT) {
            consecutiveTurnCount++;
        } else {
            lastTurnDirection = LEFT;
            consecutiveTurnCount = 1;
        }

        // Loop breaker: if stuck pivoting left repeatedly, force alternative
        if (consecutiveTurnCount >= MAX_CONSECUTIVE_TURNS) {
            DEBUG_PRINTLN("[LOOP BREAKER] Exceeded consecutive left turns. Forcing U-turn.");
            motors.turnRight(SPEED_TURN);
            delay(DURATION_U_TURN_MS);
            consecutiveTurnCount = 0;
        } else {
            DEBUG_PRINTLN("[DECISION] Left path is clearer. Turning Left.");
            motors.turnLeft(SPEED_TURN);
            delay(DURATION_TURN_MS);
        }
    } else {
        // Right has superior or equal clearance
        if (lastTurnDirection == RIGHT) {
            consecutiveTurnCount++;
        } else {
            lastTurnDirection = RIGHT;
            consecutiveTurnCount = 1;
        }

        // Loop breaker: if stuck pivoting right repeatedly, force alternative
        if (consecutiveTurnCount >= MAX_CONSECUTIVE_TURNS) {
            DEBUG_PRINTLN("[LOOP BREAKER] Exceeded consecutive right turns. Forcing U-turn.");
            motors.turnLeft(SPEED_TURN);
            delay(DURATION_U_TURN_MS);
            consecutiveTurnCount = 0;
        } else {
            DEBUG_PRINTLN("[DECISION] Right path is clearer. Turning Right.");
            motors.turnRight(SPEED_TURN);
            delay(DURATION_TURN_MS);
        }
    }

    // 5. Post-Turn Stabilization
    motors.stop();
    delay(SAFETY_POST_MANEUVER_DELAY_MS);
    DEBUG_PRINTLN("[NAV] Maneuver complete. Resuming autonomous cruising.");
}

// ============================================================================
// SYSTEM SETUP
// ============================================================================
void setup() {
    #if DEBUG_ENABLED
        Serial.begin(SERIAL_BAUD_RATE);
        delay(500);
        DEBUG_PRINTLN("==================================================");
        DEBUG_PRINTLN("  Autonomous Obstacle Avoiding Robot - ESP32     ");
        DEBUG_PRINTLN("==================================================");
    #endif

    // Initialize ultrasonic sensor GPIOs
    pinMode(PIN_ULTRASONIC_TRIG, OUTPUT);
    pinMode(PIN_ULTRASONIC_ECHO, INPUT);
    digitalWrite(PIN_ULTRASONIC_TRIG, LOW);

    // Initialize motor driver
    motors.begin();

    // Initialize SG90 servo motor
    // Standard SG90 pulse widths: 500µs (0°) to 2400µs (180°)
    scanningServo.setPeriodHertz(50); // 50Hz standard servo frequency
    scanningServo.attach(PIN_SERVO, 500, 2400);
    scanningServo.write(SERVO_ANGLE_CENTER);
    DEBUG_PRINTLN("[SERVO] Initialized and centered at 90 degrees.");

    // Startup countdown delay to allow operator to place robot safely
    DEBUG_PRINTLN("[SYSTEM] Commencing startup countdown (2 seconds)...");
    delay(STARTUP_SAFETY_DELAY_MS);
    DEBUG_PRINTLN("[SYSTEM] Autonomous navigation active.");
}

// ============================================================================
// MAIN AUTONOMOUS NAVIGATION LOOP
// ============================================================================
void loop() {
    unsigned long currentMillis = millis();

    // Sample distance at configured polling interval
    if (currentMillis - lastSampleTime >= SAMPLE_INTERVAL_MS) {
        lastSampleTime = currentMillis;

        float distanceAhead = getFilteredDistance();
        DEBUG_PRINTF("[STATUS] Ahead: %.1f cm\n", distanceAhead);

        // Check against collision avoidance threshold
        if (distanceAhead <= OBSTACLE_STOP_DISTANCE_CM) {
            executeAvoidanceManeuver(distanceAhead);
        } else {
            // Path is clear; cruise forward
            motors.moveForward(SPEED_DEFAULT);
        }
    }
}
