/* Project: Blooming Boundaries */

#include <Adafruit_CircuitPlayground.h>
#include <Servo.h>

// --- PIN DEFINITIONS ---
const int PIN_PRESSURE_DEFS = A3; 
const int PIN_PRESSURE_SOC  = A1; 
const int PIN_LIGHT         = A7; 
const int PIN_SERVO         = A2; 

// --- CONFIGURATION & THRESHOLDS ---
const int ANGLE_OPEN       = 0;   
const int ANGLE_CLOSED     = 90;  
const int BREATH_MIN       = 0;   
const int BREATH_MAX       = 60; // For breathing effect 

const int THRESHOLD_DEFS   = 750; 
const int THRESHOLD_SOC    = 170; 
const int THRESHOLD_LIGHT  = 300; 
const int HYSTERESIS       = 25;  
const int LIGHT_HYST       = 40; // Specific buffer for light to prevent shaking

// --- GLOBAL VARIABLES ---
// Citation: Servo object management based on Arduino Servo Library (https://www.arduino.cc/en/reference/servo)
Servo myServos; 

float sm_p_defs = 0; 
float sm_p_soc  = 0; 
float sm_light  = 0; 

bool isHandshakeActive    = false; 
bool isExposedActive      = false; // Track light state to prevent flickering
unsigned long lastBreath  = 0;
unsigned long lastRainbow = 0; 
int currentBreathAngle    = BREATH_MIN;
int breathDirection       = 1; 
int currentStaticAngle    = -1;

// I used these tones for a happy startup sound. Startup Melody: C5, E5, G5, C6
int melody[]    = {523, 659, 783, 1046};  
int durations[] = {150, 150, 150, 400}; 

void setup() {
  Serial.begin(9600);
  // I started the CPX and cleared the lights
  CircuitPlayground.begin();
  CircuitPlayground.strip.clear();
  CircuitPlayground.strip.show();
  
  // I attached the servo to pin A2
  myServo.attach(PIN_SERVO);
  
  // Citation: Pin mode configuration (https://www.arduino.cc/en/Tutorial/AnalogInput)
  // I set the pins to read input
  pinMode(PIN_PRESSURE_DEFS, INPUT);
  pinMode(PIN_PRESSURE_SOC,  INPUT);
  pinMode(PIN_LIGHT,         INPUT);
  
  setServoAngle(ANGLE_OPEN);
}

void loop() {
  // I smoothed the sensor data by mixing 20% new and 80% old value, this helps reduce sensor noise and makes the result more stable.
  sm_p_defs = (analogRead(PIN_PRESSURE_DEFS) * 0.2) + (sm_p_defs * 0.8);
  sm_p_soc  = (analogRead(PIN_PRESSURE_SOC)  * 0.2) + (sm_p_soc  * 0.8);
  sm_light  = (analogRead(PIN_LIGHT)         * 0.2) + (sm_light  * 0.8);

  bool isDefensive = (sm_p_defs > THRESHOLD_DEFS);
  // I first used a normal threshold method.
  // Then I added hysteresis with a small buffer.
  // This prevents fast switching when the sensor value is close to the limit.
  if (sm_p_soc > (THRESHOLD_SOC + HYSTERESIS)) {
    if (!isHandshakeActive) {
      isHandshakeActive = true;
      playHandshakeMelody(); 
    }
  } else if (sm_p_soc < (THRESHOLD_SOC - HYSTERESIS)) {
    if (isHandshakeActive) {
      isHandshakeActive = false;
      CircuitPlayground.strip.clear();
      CircuitPlayground.strip.show();
    }
  }
  // --- Light sensor hysteresis logic to prevent shaking ---
  if (sm_light > (THRESHOLD_LIGHT + LIGHT_HYST)) {
    isExposedActive = true;
  } else if (sm_light < (THRESHOLD_LIGHT - LIGHT_HYST)) {
    isExposedActive = false;
  }

  // --- Priority: Defensive > Social/Light ---
  if (isDefensive) {
    setServoAngle(ANGLE_CLOSED);
    if (isHandshakeActive) {
       isHandshakeActive = false; 
    CircuitPlayground.strip.clear();
    CircuitPlayground.strip.show();
  } 
  } else {
    if (isHandshakeActive) {
      drawRainbow(100); 
    }

    if (isExposedActive) {
      runBreathingEffect();
    } else {
      setServoAngle(ANGLE_OPEN);
    }
  }
  
  delay(15);
}

// Citation: Servo angle control & Dynamic motion(https://docs.arduino.cc/tutorials/generic/basic-servo-control/)
void setServoAngle(int angle) {
  if (currentStaticAngle != angle) {
    myServos.write(angle);
    currentStaticAngle = angle;
  }
}

// Citation: Breathing-style servo motion (https://learn.adafruit.com/multi-tasking-the-arduino-part-1/a-clean-sweep?)
void runBreathingEffect() {
  unsigned long now = millis(); // I used millis() so the motion does not block the rest of the program.
  if (now - lastBreath > 35) {
    lastBreath = now;
    currentBreathAngle += breathDirection;
    
    if (currentBreathAngle >= BREATH_MAX) {
      currentBreathAngle = BREATH_MAX;
      breathDirection = -1;
    } else if (currentBreathAngle <= BREATH_MIN) {
      currentBreathAngle = BREATH_MIN;
      breathDirection = 1;
    }
    
    myServos.write(currentBreathAngle);
    currentStaticAngle = -1; 
  }
}

// Citation: Sound generation (https://docs.arduino.cc/built-in-examples/digital/toneMelody/),(https://learn.adafruit.com/circuit-playground-hot-potato/playing-a-melody?)
void playHandshakeMelody() {
  for (int i=0; i<4; i++) {
    CircuitPlayground.playTone(melody[i], durations[i]); 
  }
  lastRainbow = millis(); 
}

// Citation: NeoPixel animation (https://learn.adafruit.com/circuit-playground-kaleidoscope/inside?)
void drawRainbow(int interval) {
  unsigned long now = millis();
  if (now - lastRainbow > interval) {
    lastRainbow = now;
    uint32_t offset = millis() / 15; 
    for (int i=0; i<10; i++) {
      CircuitPlayground.strip.setPixelColor(i, CircuitPlayground.colorWheel(((i * 256 / 10) + offset) & 255));
    }
    CircuitPlayground.strip.show();
  }
}
