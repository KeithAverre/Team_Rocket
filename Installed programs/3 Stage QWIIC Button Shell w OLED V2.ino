/******************************************************************************
  File: OnboardProRFV6.ino
  Board: On-Board Pro-RF 
  Task 1: System state flow control
  Task 2: Recieve datastream from on-board Redboard Artemis via serial UART RX/TX
  Task 3: Send datastream to on-board Pro RF via serial UART RX/TX
  Version Info: v6.0 11/6/22 22:07
  Notes: Based on example server client 1 & 3 Stage QWIIC Button Shell On-Board w/ OLED V3
  Author: David Schull
  References:
  Fischer Moseley @ SparkFun Electronics
  David A. Mellis @ Arduino
  Paul Stoffregen @ Arduino
  Scott Fitzgerald @ Arduino
  Arturo Guadalupi @ Arduino
  Arduino IDE Built-In Example Files
  SparkFun Electronics Built-In Example Files
  Adafruit Built-In Example Files
******************************************************************************/

// LIBRARIES  /////////////////////////////////////////////////////////////////

// QWIIC LED Button
#include <SparkFun_Qwiic_Button.h>

// QWIIC OLED Display
/* DISABLE OLED
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
*/

// GLOBAL OBJECTS AND VARIABLES ///////////////////////////////////////////////////

// QWIIC BUTTON OBJECT AND VARIABLES
QwiicButton redButton; // Define QWIIC button object
uint8_t state0Brightness = 10; //The state 0 brightness for the LED. Can be between 0 (min) and 255 (max)
uint8_t state1Brightness = 100; //The state 1 brightness for the LED. Can be between 0 (min) and 255 (max)
uint8_t state2Brightness = 255; //The state 2 brightness for the LED. Can be between 0 (min) and 255 (max)
uint16_t state0CycleTime = 100; //The state 0 pulse time for the LED. Set to a bigger number for a slower pulse, or a smaller number for a faster pulse
uint16_t state0OffTime = 1000; //The state 0 off time. Set to 0 to be pulsing continuously.
uint16_t state1CycleTime = 1000; //The state 1 pulse time for the LED. Set to a bigger number for a slower pulse, or a smaller number for a faster pulse
uint16_t state1OffTime = 200; //The state 1 off time. Set to 0 to be pulsing continuously.

// STATUS LED VARIABLES
const int stopLoggingPin32 = 7; // Define the pin used to act as the OLA start/stop signal
const int greenLed1Pin = 8; // Define the pin used for our green status LED
int ledState = LOW; // ledState used to set the LED
unsigned long previousMillis = 0; // will store last time LED was updated
const long slowBlink = 1000; // interval at which to blink (milliseconds)
const long medBlink = 500; // interval at which to blink (milliseconds)
const long fastBlink = 100; // interval at which to blink (milliseconds)

// QWIIC OLED Display
/* DISABLE OLED
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels
#define OLED_RESET - 1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, & Wire, OLED_RESET);
*/

// OTHER VARIABLES

// displayHistory prevents the spamming of messages
int displayHistory = 0; // 0 Allows the next state message to display
int masterState = 0; // Set the master state to 0 (Idle) by default

// SETUP SCRIPT //////////////////////////////////////////////////////////////

void setup() {
  Serial.begin(115200); // Enable USB Serial Port at 115200
  startupMessage(); // Display the startup message on terminal screen
  Wire.begin(); // Join I2C bus for QWIIC button
  initiateRedButton(); // Check if the QWIIC red button is online
  redButton.LEDoff(); // Start with the red button LED off
  /* DISABLE OLED
  initiateOledDisplay(); // Check if the QWIIC OLED display us online
  */
  // Where is this pinMode used? From example? Review for removal.
  pinMode(LED_BUILTIN, OUTPUT); // Initilize the digital pin LED_BUILTIN as output
  stopLoggingAtStartup();
}

// MAIN LOOP  ////////////////////////////////////////////////////////////////

void loop() {

  while (masterState == 0) { // Idle Mode (State 0)
    if (displayHistory != 1) { // Show the message, only once
      messageState0(); // Display via serial console
      /* DISABLE OLED
      drawState0Display(); //Display via OLED
      */
      displayHistory = 1; // Allow the next state message to display
      // ENTER STATE 0 RUN ONCE CODE BELOW
      redButton.LEDconfig(state0Brightness, state0CycleTime, state0OffTime); // Blink the red button LED dim
      // ENTER STATE 0 RUN ONCE CODE ABOVE
    }
    //delay(20); // Reduce stress on the I2C bus
    // ENTER STATE 0 MAIN CODE BELOW
    digitalWrite(greenLed1Pin, HIGH); // Turn on the LED solid
    // ENTER STATE 0 MAIN CODE ABOVE
    buttonPressState0(); // Watch for red button press 
  }

  while (masterState == 1) { //Pre-Flight Mode (State 1)
    if (displayHistory != 1) { // Show the message, only once
      messageState1(); // Display via serial console
      /* DISABLE OLED
      drawState1Display(); //Display via OLED
      */
      displayHistory = 1; // Prevent message spamming
      // ENTER STATE 1 RUN ONCE CODE BELOW
    redButton.LEDconfig(state1Brightness, state1CycleTime, state1OffTime); // Blink the red button LED medium
      // ENTER STATE 1 RUN ONCE CODE ABOVE
    }
    digitalWrite(stopLoggingPin32, HIGH);
    // delay(20); // Reduce stress on the I2C bus
    // ENTER STATE 1 MAIN CODE BELOW
    greenLedBlink(medBlink); // Blink the LED 
    // ENTER STATE 1 MAIN CODE ABOVE
    buttonPressState1(); // Watch for red button press 
  }

  while (masterState == 2) { // Flight Mode (State 2)
    if (displayHistory != 1) { // Show the message, only once
      messageState2(); // Display via serial console
      /* DISABLE OLED
      drawState2Display(); //Display via OLED
      */
      displayHistory = 1; // Prevent message spamming
      // ENTER STATE 2 RUN ONCE CODE BELOW
    redButton.LEDon(state2Brightness); // Solid red button LED full brightness
      // ENTER STATE 2 RUN ONCE CODE ABOVE
    }
    // delay(20); // Reduce stress on the I2C bus
    // ENTER STATE 2 MAIN CODE BELOW
    greenLedBlink(fastBlink); // Blink the LED 
    // ENTER STATE 2 MAIN CODE ABOVE
    buttonPressState2(); // Watch for red button press 
  }
}


// FUNCTIONS ////////////////////////////////////////////////////////////////

// QWIIC Device Initiations
/* DISABLE OLED

void initiateOledDisplay() {
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("ERROR: OLED offline! Freezing Operation."));
    for (;;)
    ; // Don't proceed, loop forever
  }
  Serial.println("Test Passed: QWIIC OLED online.");
  display.clearDisplay();
}
*/
void initiateRedButton() {
  //check if button will acknowledge over I2C
  if (redButton.begin() == false) {
    Serial.println("ERROR: Red button offline! Freezing Operation.");
    while (1);
  }

  Serial.println("Test Passed: QWIIC red button online.");
  for (int i = 1; i < 12; ++i) {
    // Quickly blinks with increasing brightness
    uint8_t startupBrightness = (20 * i);
    redButton.LEDon(startupBrightness);
    delay(20);
    redButton.LEDoff();
    delay(65);
  }
}

// System Message Functions ////////////////////////////////////////////////////////////////

void flightEndProcedure(int endSource) {

  // Show flight end message on OLED
  /* DISABLE OLED
  display.clearDisplay();
  display.setTextSize(2); // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.setCursor(0, 0); // Start at top-left corner
  display.println("FLIGHT");
  display.println("ENDED");
  display.display();
  */
  // Display alert to terminal
  Serial.println("ALERT: Flight mode manually ended!");
  // Function takes in the source of flight ending for logging purposes (maybe more than one added, perhaps a time based, etc.)
  if (endSource == 1) {
    // Red Button = "1"
    Serial.println("Flight mode ended via red button.");
  }  
  
  digitalWrite(stopLoggingPin32, LOW); // Turn OLA logging off

  // Flash red button LED after ending flight
  for (int i = 1; i < 12; ++i) {
    // Quickly blinks with increasing brightness
    uint8_t startupBrightness = (20 * i);
    redButton.LEDon(startupBrightness);
    delay(20);
    redButton.LEDoff();
    delay(65);
  }
  delay(500); // Delay one second to keep OLED message and give the system a moment
}

// Serial Message Functions ////////////////////////////////////////////////////////////////

void startupMessage() {
  Serial.println("<-------------- SYSTEM ONLINE --------------> ");
  Serial.println("Redboard Artemis Online.");
}

void messageState0() {
  Serial.println("Current system state is: idle (State 0) ");
  Serial.println("Press the red button to enter pre-flight mode");
}

void messageState1() {
  Serial.println("Current system state is: pre-flight mode (State 1)");
  Serial.println("Press the red button to enter flight mode");
}

void messageState2() {
  Serial.println("Current system state is: flight mode (State 2) ");
  Serial.println("Press the red button to end and enter idle mode");
}

// OLED Screen Functions ////////////////////////////////////////////////////////////////
/* DISABLE OLED
void drawState0Display(void) {
  display.clearDisplay();
  display.setTextSize(1); // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.setCursor(0, 0); // Start at top-left corner
  display.println("CURRENT STATE");
  display.println("Idle (0)\n");
  display.println("NEXT: Pre-Flight (1)");
  display.display();
}

void drawState1Display(void) {
  display.clearDisplay();
  display.setTextSize(1); // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.setCursor(0, 0); // Start at top-left corner
  display.println("CURRENT STATE");
  display.println("Pre-Flight (1)\n");
  display.println("NEXT: Flight Mode (2)");
  display.display();
}

void drawState2Display(void) {
  display.clearDisplay();
  display.setTextSize(1); // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.setCursor(0, 0); // Start at top-left corner
  display.println("CURRENT STATE");
  display.println("Flight Mode (2)\n");
  display.println("PRESS TO END FLIGHT");
  display.display();
}
*/
// Button Press Functions ////////////////////////////////////////////////////////////////

void buttonPressState0(){
  if (redButton.isPressed() == true) {
    redButton.LEDon(state2Brightness); // Illuminate button when pressed
    masterState = 1; // Set the master state to 1 (Pre-Flight)
    displayHistory = 0; // Allow the next state message to display
    while (redButton.isPressed() == true) {
      delay(10); // Wait for user to stop pressing
    }
  }  
}

void buttonPressState1(){
  if (redButton.isPressed() == true) {
    redButton.LEDon(state2Brightness); // Illuminate button when pressed
    masterState = 2; // Set the master state to 2 (Flight Mode)
    displayHistory = 0; // Allow the next state message to display
    while (redButton.isPressed() == true) {
      delay(10); // Wait for user to stop pressing
    }
  }    
}

void buttonPressState2(){
  if (redButton.isPressed() == true) {
    redButton.LEDon(state2Brightness);
    flightEndProcedure(1);
    masterState = 0; // Set the master state to 0 (Idle)
    displayHistory = 0; // Allow the next state message to display
    while (redButton.isPressed() == true) {
      delay(10); // Wait for user to stop pressing
    }
  }
}

void stopLoggingAtStartup(){
  // Cycle pin 32 for about 3 seconds to attempt to stop the OLA at startup
  for (int i = 0; i < 60; i++){
    digitalWrite(stopLoggingPin32, HIGH); // Turn OLA logging off
    redButton.LEDon(state0Brightness);
    delay(50);
    digitalWrite(stopLoggingPin32, LOW); // Start with logging off on the OLA
    redButton.LEDon(state2Brightness);
  }
  Serial.println("OLA stop attempted");  
}
// LED Functions ////////////////////////////////////////////////////////////////

void greenLedBlink(int blinkSpeed) {

  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= blinkSpeed) {
    // save the last time you blinked the LED
    previousMillis = currentMillis;

    // if the LED is off turn it on and vice-versa:
    if (ledState == LOW) {
      ledState = HIGH;
    } else {
      ledState = LOW;
    }

    // set the LED with the ledState of the variable:
    digitalWrite(greenLed1Pin, ledState);
  }
}