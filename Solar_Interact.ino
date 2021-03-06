#include <Arduino.h>
#include <SoftwareSerial.h>

//New
#include <Encoder.h>

Encoder knobLeft(2,3);
//New ^

#define TX 52 //Bluetooth Transmit
#define RX 53 //Bluetooth Recieve

#define START_EASY 110 //Start game message to tablet
#define START_MEDIUM 120 //Start game message to tablet
#define START_HARD 130 //Start game message to tablet
#define ENDGAME 255 //End game message from tablet

//#define MAX_POWER 25 // Maximum power generated by either solar panel or crank
#define BTrefreshTime 2000 // (ms)

#define LED1SOLAR 44 // PWM input solar pin led1
#define LED2SOLAR 45 // PWM input solar pin led2
#define LED3SOLAR 4 // PWM input solar pin led3
#define LED4SOLAR 5 // PWM input solar pin led4
#define LED5SOLAR 6 // PWM input solar pin led5

#define LED1USER 9 // PWM input user pin led1
#define LED2USER 10 // PWM input user pin led2
#define LED3USER 11 // PWM input user pin led3
#define LED4USER 12 // PWM input user pin led4
#define LED5USER 13 // PWM input user pin led5

const int LEDUserPins[5] = {LED1USER, LED2USER, LED3USER, LED4USER, LED5USER}; // array of user pins
const int LEDSolarPins[5] = {LED1SOLAR, LED2SOLAR, LED3SOLAR, LED4SOLAR, LED5SOLAR}; // array of solar pins
const int LEDAllPins[10] = {LED1USER, LED2USER, LED3USER, LED4USER, LED5USER, LED1SOLAR, LED2SOLAR, LED3SOLAR, LED4SOLAR, LED5SOLAR}; // array of all pins
const int maxPower = 20; // Maximum power generated by either solar panel or crank
const int numberFloors = 5; // number of floors in model

int solarPower; // stores solar panel power level

String incomingData = ""; // undecoded data from bluetooth
int receivedData = 0; // received data from bluetooth
int gieselLightPower = 0;
SoftwareSerial BTserial(TX, RX); // TX | RX (On BT module)

// Sends data to tablet
void send(int data){
  static unsigned long timeKeeper = 0;
 /* if (data >= 10) {
    BTserial.print('T');
  }
  else {
    BTserial.print('F');
  }*/
  BTserial.print(data);

  Serial.print("Data to send: ");
  Serial.println(data);
}

// Recieves data from tablet
void receiveData(){
  if(BTserial.available()){
        incomingData = ""; //No repeats
      Serial.print("---- receiveData() "); Serial.println(millis());
      Serial.print("Receving: ");
      while(BTserial.available()){ // While there is more to be read, keep reading.
        incomingData += BTserial.read(); // before 
      }

//      decipher(incomingData.toInt());
    receivedData = incomingData.toInt();
    Serial.println(receivedData);
  }
}

// sets up difficulty level for solar panel depending on received data 
void difficultyLevel(int difficulty) {
  while (true) {
    if (receivedData == START_EASY) {
      solarPower = 11; //11 Watts
      break; }
    if (receivedData == START_MEDIUM) {
      solarPower = 15; //15 Watts
      break; }
    if (receivedData == START_HARD) {
      solarPower = 22; // 22 Watts
      break; }
  }

}

// Runs the entire game in this method
void gameMode() {
  int encoderReading, encoderPower;
  difficultyLevel(receivedData);
  receivedData = 0;
  
  while (true) {
    receiveData();
    if (receivedData == ENDGAME) { // checks end game condition
      Serial.println("Game end");
      break;
    }
    encoderReading = readEncoder(); // reads power from generator
    encoderPower = convertEncoderToPower(encoderReading); // converts reading to power
    send(encoderPower); // sends power to tablet
    lightGeisel(gieselLightPower, 0); // lights right side of Geisel
    lightGeisel(solarPower, 1); // lights left side of Geisel
    delay(100);
  }
}

// Reads a speed from the encoder
int readEncoder(){
  static long encoderPosition  = -999;

  static unsigned long dt = 0;
  static unsigned long lastTimeStamp = 0;
  static int rotSpeed = 0;

  long newPosition;
  newPosition = knobLeft.read();

  dt = (unsigned long)(millis() - lastTimeStamp);
  dt = dt/6;
  lastTimeStamp = millis(); 
  rotSpeed = abs(newPosition - encoderPosition)/dt; 
  rotSpeed = rotSpeed == 65535 ? 0 : rotSpeed; //Remove outliers 
  if (newPosition == encoderPosition) {
    rotSpeed = 0;
  }
  if (newPosition != encoderPosition) {
    Serial.print("Speed = ");
    Serial.println(rotSpeed);

    
    encoderPosition = newPosition;
  }
    gieselLightPower = rotSpeed;
    rotSpeed = rotSpeed-6;
    Serial.print("Position = ");
    Serial.println(newPosition);
    Serial.print("Old Speed = ");
    Serial.println(rotSpeed);
  if (rotSpeed > 9) {
    rotSpeed = 9;
  }
  if (rotSpeed < 0) {
    rotSpeed = 0;
  }
  Serial.print("New Speed = ");
  Serial.println(rotSpeed);
  
  return rotSpeed; //for now, this is just a value
}

/* Should map from 0 to 25 Watts, can only be an Int! */
int convertEncoderToPower(int encoderValue) {
  return encoderValue;
}

// Lights the Geisel model
void lightGeisel(int geiselPower, int lightLeft) {
  int LEDPins[5]; // defines the pins to light up
  int maxFloorThreshold = maxPower/numberFloors; // obtains a threshold for turning on floors
  int index = geiselPower/maxFloorThreshold; // obtains the index to turn which LEDs completely on
  int brightness = (geiselPower-(index*5)); // obtains a proportional value of how much to light up the last level
  int brightnessMap = map(brightness, 0, 5, 0, 9); // maps brightness to a value between 0 and 10

// if set to one, lights up solar side
  if(lightLeft == 1) {
    for(int i = 0; i < numberFloors; i++) {
      LEDPins[i] = LEDSolarPins[i];
    }
  }

// if set to zero, lights up user side
  if(lightLeft == 0) {
    for(int i = 0; i < numberFloors; i++) {
      LEDPins[i] = LEDUserPins[i];
    }
  }

// lights up all the lights that need to be fully lit up, 10 is quite bright, 1023 is blinding
  for(int i = 0; i<index; i++){
    analogWrite(LEDPins[i], 10);
  }
  analogWrite(LEDPins[index], brightnessMap); // lights up the last level partially 

  // turn off all the other floors
  for(int i = index+1; i<numberFloors; i++){
    analogWrite(LEDPins[i], 0);
  }

  // debugging message for seeing brightness
  Serial.print("Brightness is set at " );
  Serial.println(brightness);
}

// resets the lights by turning them all off and on again repeatively
void resetLights() {
  
  for(int i = 0; i < 2*numberFloors;i++){
    analogWrite(LEDAllPins[i], 8);
  }
  delay(1500);
  for(int i = 0; i < 2*numberFloors;i++){
    analogWrite(LEDAllPins[i], 0);
  }
  delay(500);
}


//New ^

void setup() {
  Serial.begin(9600);
  Serial.println("System running.");
 
  BTserial.begin(9600);
  pinMode(TX, INPUT_PULLUP); // only needed for JY-MCUY v1.06x
  BTserial.listen();

  BTserial.print("BT GO.."); BTserial.println(millis());
  resetLights();
}


void loop() {
  
    receiveData();
    if (receivedData == START_EASY || receivedData == START_MEDIUM || receivedData == START_HARD) {
        Serial.println("Game start");
        gameMode();
        delay(1000);
        resetLights();
    }
  delay(3000);
  

//readEncoder();
}
