/******************************************************************************
  File: OnboardRedboardv4.ino
  Board: On-Board Redboard Artemis 
  Task 1: System state flow control
  Task 2: Recieve datastream from on-board Openlog Artemis via serial UART RX/TX
  Task 3: Send datastream to on-board Pro RF via serial UART RX/TX  
  Version Info: v4.0 11/8/22 19:27
  Notes: Based on example client radio 2 & 3 Stage QWIIC Button Shell On-Board w/ OLED V3
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

// Datastream packet:
//r tcDate,rtcTime,Q6_1,Q6_2,Q6_3,RawAX,RawAY,RawAZ,RawGX,RawGY,RawGZ,RawMX,RawMY,RawMZ,gps_Lat,gps_Long,gps_Alt,gps_GroundSpeed,gps_Heading,degC,pressure_hPa,pressure_Pa,altitude_m,output_Hz,count,
// <01/01/2000,01:50:46.24,0.00947,0.00877,-0.00213,-626,820,-8020,12,14,-6,14,201,-85,1852138818,1935755378,1866886735,1667852663,1349743938,25.1875,1023.87,98664.05,223.90,9.091,1,>


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
const int blueLed1Pin = 9; // Define the pin used for our blue status LED
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
char receivedChars[251]; //Char array to store serial data from OLA
int ndxLength; // Stores the legth of the recived flight packet
boolean newData = false; // Flow control
int packetCounter = 0; // Counts how many datastream packets are recieved


// SETUP SCRIPT //////////////////////////////////////////////////////////////

void setup() {
  Serial.begin(115200); // Enable USB Serial Port at 115200
  Serial1.begin(115200); // open UART RX port, set baud rate to 115200 bps
  startupMessage(); // Display the startup message on terminal screen
  Wire.begin(); // Join I2C bus for QWIIC button
  initiateRedButton(); // Check if the QWIIC red button is online
  redButton.LEDoff(); // Start with the red button LED off
  /* DISABLE OLED
  initiateOledDisplay(); // Check if the QWIIC OLED display us online
  */
  // Where is this pinMode used? From example? Review for removal.
  pinMode(LED_BUILTIN, OUTPUT); // Initilize the digital pin LED_BUILTIN as output
  stopOpenLogArtemis(); // Stop logging at startup - required for syncrynozation
  masterState = 1; // Set system state to pre-flight after startup procedure
}

// MAIN LOOP  ////////////////////////////////////////////////////////////////

void loop() {


  while (masterState == 0) { // Pre-Flight Mode (State 1)
    if (displayHistory != 1) { // Show the message, but only once
      messageState0(); // Display state via serial console
      displayHistory = 1; // Allow the next state message to display
      digitalWrite(blueLed1Pin, LOW); // Turn off blue status LED 
    }
    greenLedBlink(slowBlink); // Blink the green status LED 
  }
  while (masterState == 1) { // Pre-Flight Mode (State 1)
    if (displayHistory != 1) { // Show the message, only once
      messageState1(); // Display via serial console
      displayHistory = 1; // Allow the next state message to display
      redButton.LEDconfig(state1Brightness, state1CycleTime, state1OffTime); // Blink the red button LED dim
    }
    greenLedBlink(medBlink); // Blink the LED 
    buttonPressState1(); // Watch for red button press
  }

  while (masterState == 2) { // Flight Mode (State 2)
    if (Serial1.available() > 0){
      digitalWrite(blueLed1Pin, HIGH); // Turn blue status LED on
    }
    if (displayHistory != 1) { // Show the message, only once
      messageState2(); // Display via serial console
      displayHistory = 1; // Allow the next state message to display
      redButton.LEDon(state2Brightness); // Solid red button LED full brightness
    }
    greenLedBlink(fastBlink); // Blink the LED 
    buttonPressState2(); // Watch for red button press
    recordFlightDataPacket(); // Intake datastream via serial
    showFlightDataPacket(); // Show the datastream to console
    sendFlightDataPacket(); // Send the datastream via serial

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
  // Flash red button LED after ending flight
  for (int i = 1; i < 12; ++i) {
    // Quickly blinks with increasing brightness
    uint8_t startupBrightness = (20 * i);
    redButton.LEDon(startupBrightness);
    delay(20);
    redButton.LEDoff();
    delay(65);
  }
  stopOpenLogArtemis(); // Turn OLA logging off
  digitalWrite(blueLed1Pin, LOW); // Turn blue status LED off
  delay(500); // Delay one second to keep OLED message and give the system a moment
}

// Serial Message Functions ////////////////////////////////////////////////////////////////

void startupMessage() {
  Serial.println("<-------------- SYSTEM ONLINE --------------> ");
  Serial.println("Redboard Artemis Online.");
}

void messageState0() {
  Serial.println("Current system state is: idle (State 0) ");
  // Serial.println("Press the red button to enter pre-flight mode");
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
    //while (redButton.isPressed() == true) {
      //delay(10); // Wait for user to stop pressing
    //}
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

void stopOpenLogArtemis(){

  digitalWrite(blueLed1Pin, HIGH); // Turn blue status LED on

  // Cycle pin 32 for a few seconds to attempt to stop the OLA at startup
  for (int i = 0; i < 50; i++){
    digitalWrite(stopLoggingPin32, HIGH); // Turn OLA logging off
    redButton.LEDon(state1Brightness); // Flash the red button LED low
    delay(50); // Breif delay
    digitalWrite(stopLoggingPin32, LOW); // Start with logging off on the OLA
    redButton.LEDon(state2Brightness); // Flash the red button LED high
  }

  digitalWrite(blueLed1Pin, LOW); // Turn blue status LED off
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

// Data Packet Functions ///////////////////////////////////////////////////////

void recordFlightDataPacket() {
    static int ndx = 1;
    char comma = ',';
    char endMarker = '\n';
    char rc;
    receivedChars[0] = '<'; // start packet marker

    while (Serial1.available() > 0 && newData == false) {
        rc = Serial1.read();
        if (rc != endMarker) {
            receivedChars[ndx] = rc;
            ndx++;
        }
        else {
            receivedChars[ndx -2] = '>'; // end packet marker
            receivedChars[ndx -1] = '\0'; // terminate the string
            ndxLength = ndx;
            ndx = 1;
            newData = true;
            packetCounter++;
            
        }
    }
}

void showFlightDataPacket() {
    if (newData == true) {

        Serial.print("Datastream from OLA: ");
        Serial.println(receivedChars);

    }
}

void sendFlightDataPacket() {
    if (newData == true) {
        // Send a packet only every 0.50 seconds or so
        if (packetCounter % 5 == 0){
          Serial1.println(receivedChars);
        }
        clear_arr(receivedChars);
        newData = false;
    }
}

void clear_arr(char* arr){
  memset(arr, 0, ndxLength);
}