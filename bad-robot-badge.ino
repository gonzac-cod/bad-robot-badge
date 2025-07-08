#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>
#include <Adafruit_NeoPixel.h>
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

// NRF24L01
#define CE_PIN 9
#define CSN_PIN 10
RF24 radio(CE_PIN, CSN_PIN);

// Neopixel
#define PIN 8
#define LED_COUNT 1
Adafruit_NeoPixel leds = Adafruit_NeoPixel(LED_COUNT, PIN, NEO_GRB + NEO_KHZ800);

// Display Nokia 5110
#define RST 6
#define CE 5
#define DC 4
#define DIN 3
#define CLK 2
Adafruit_PCD8544 display = Adafruit_PCD8544(CLK, DIN, DC, CE, RST);

const int baudRate = 9600;

int arrayLengthArcSmile = 11;
int arcSmileX[11] = {7,7,7,6,6,5,4,3,2,1,0};
int arcSmileY[11] = {6,5,4,3,2,1,0,0,0,0,0};

unsigned long previousMillis = 0;
const long interval = 1000;
int lastAnimation = 0;

const int scanButton = A0;
bool scanState = LOW;
bool lastScanState = LOW;
bool scanning = false;

int enableAttackPin = 7;
int enableAttackState = LOW;
const int attackButton = A1;
int attackState = LOW;
bool lastAttackState = LOW;
bool attacking = false;

const int channel1Pin = A2;
const int channel6Pin = A3;
const int channel11Pin = A4;

int selectedChannel = 0;
const int CHANNEL_1 = 1;
const int CHANNEL_6 = 6;
const int CHANNEL_11 = 11;

bool channel1State = LOW;
bool channel6State = LOW;
bool channel11State = LOW;

const int START_BAND_1 = 1;
const int END_BAND_1 = 23;

const int START_BAND_6 = 26;
const int END_BAND_6 = 48;

const int START_BAND_11 = 51;
const int END_BAND_11 = 73;

int selectedStartBand = 0;
int selectedEndBand = 0;

void setup() {
  display.begin();
  display.setContrast(60);
  display.clearDisplay();
 
  leds.begin();
  leds.show();
  
  pinMode(enableAttackPin, INPUT); 
  pinMode(scanButton, INPUT); 
  pinMode(attackButton, INPUT); 
  pinMode(channel1Pin, INPUT); 
  pinMode(channel6Pin, INPUT); 
  pinMode(channel11Pin, INPUT);  

  Serial.begin(baudRate);

  display.setTextSize(1);
  display.setTextColor(BLACK);
  display.setCursor(0, 0);
  display.println("Starting...");
  display.display();

  Serial.println("AT+CWMODE=1");
  delay(2000);
  display.clearDisplay();
  
  radio.begin();
  radio.setPALevel(RF24_PA_HIGH);
  radio.stopListening();
}

void loop() {
  enableAttackState = digitalRead(enableAttackPin);
  /////////////////////////////
  channel1State = digitalRead(channel1Pin);
  channel6State = digitalRead(channel6Pin);
  channel11State = digitalRead(channel11Pin);
  if (channel1State == HIGH) {
    selectedChannel = CHANNEL_1;
  }
  if (channel6State == HIGH) {
    selectedChannel = CHANNEL_6;
  }
  if (channel11State == HIGH) {
    selectedChannel = CHANNEL_11;
  }
  checkSelectedChannel();
  //////////////////////////////

  if (!attacking) {
    scanState = digitalRead(scanButton);
  }
  if (scanState == HIGH && lastScanState == LOW) {
    delay(50);

    scanning = !scanning;
    if (scanning) {
      display.clearDisplay();
      display.setCursor(0, 0);
      display.setTextSize(1);
      display.setTextColor(BLACK);
      display.println("Scanning...");
      display.display();
    } else {
      display.clearDisplay();
      display.setCursor(0, 0);
      display.setTextSize(1);
      display.setTextColor(BLACK);
      display.println("Scanning stopped!");
      display.display();
      delay(1000);
      display.clearDisplay();
    }
  }
  
  lastScanState = scanState;
  if (scanning) {
    scan();
  }
  //////////////////////////////////////
  if (!scanning && !attacking) {
    leds.setPixelColor(0, 3, 187, 133);
    leds.show();
    showHappyFace();
  }
  //////////////////////////////////////
  if (!scanning) {
    attackState = digitalRead(attackButton);
  }
  if (attackState == HIGH && lastAttackState == LOW) {
    delay(50);

    attacking = !attacking;
  }
  
  lastAttackState = attackState;
  if (attacking) {
    leds.setPixelColor(0, 255, 0, 0);
    leds.show();
    showEvilFace();
    attack();
  }
}

void checkSelectedChannel() {
  switch (selectedChannel) {
    case CHANNEL_1:
      selectedStartBand = START_BAND_1;
      selectedEndBand = END_BAND_1;
      break;
    case CHANNEL_6:
      selectedStartBand = START_BAND_6;
      selectedEndBand = END_BAND_6;
      break;
    case CHANNEL_11:
      selectedStartBand = START_BAND_11;
      selectedEndBand = END_BAND_11;
      break;
  }
}

void attack() {
  if (enableAttackState == HIGH) {
    for(int i=selectedStartBand; i<=selectedEndBand; i++){
      radio.setChannel(i);
      const char message[] = "NOISE";
      radio.write(&message, sizeof(message));
    }
  } else {
      display.setCursor(66, 40);
      display.println("OFF");
      display.display();
  }
}

void scan() {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextSize(1);
  display.setTextColor(BLACK);
  display.println("Scanning...");

  Serial.flush();
  Serial.println("AT+CWLAP");
  delay(2000);

  while (Serial.available()) {
    String wifiLine = Serial.readStringUntil('\n');

    if (wifiLine.indexOf("+CWLAP:") != -1 && wifiLine.indexOf('"') != -1) {
      int firstQuote = wifiLine.indexOf('"');         
      int secondQuote = wifiLine.indexOf('"', firstQuote + 1);
      if (firstQuote == -1 || secondQuote == -1) continue;
      
      String ssid = wifiLine.substring(firstQuote + 1, secondQuote);

      // RSSI
      int rssiStart = secondQuote + 2;
      int rssiEnd = wifiLine.indexOf(',', rssiStart);
      if (rssiStart == -1 || rssiEnd == -1) continue;  
      String rssi = wifiLine.substring(rssiStart, rssiEnd);

      // Channel
      int macStart = wifiLine.indexOf('"', rssiEnd) + 1;  
      int macEnd = wifiLine.indexOf('"', macStart);
      int channelStart = macEnd + 2;  
      int channelEnd = wifiLine.indexOf(',', channelStart);
      if (channelEnd == -1) channelEnd = wifiLine.length();
      String channel = wifiLine.substring(channelStart, channelEnd);

      display.clearDisplay();
      display.println(ssid);
      display.println(rssi + " dBm");
      display.println("Ch: " + channel);
      display.display();  
      delay(1000);
      break;
    }
  }
}

void showHappyFace() {
  drawHappyFace();
  blinkHappyFace();
}

void showEvilFace() {
  unsigned long currentMillis = millis();

  drawEvilFace(lastAnimation);

  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    if (lastAnimation == 0){
      lastAnimation = 2;
    } else {
      lastAnimation = 0;
    }
  }
}

void drawHappyFace() {
  display.clearDisplay();
  display.fillCircle(15, 18, 3, BLACK);
  display.fillCircle(69, 18, 3, BLACK); 
  drawArc(42, 27);
  display.display();
  delay(1000);
}

void drawArc(int centerX, int centerY) {
  for (int i=0; i < (arrayLengthArcSmile * 2); i++) {
    if (i < arrayLengthArcSmile) {
      display.drawPixel(centerX-arcSmileX[i], centerY-arcSmileY[i], BLACK);
    } else {
      display.drawPixel(centerX+arcSmileX[i-arrayLengthArcSmile], centerY-arcSmileY[i-arrayLengthArcSmile], BLACK);
    }
  }
}

void blinkHappyFace() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(BLACK);
  display.setCursor(12, 15);
  display.println(">");
  display.setCursor(66, 15);
  display.println("<");
  drawArc(42, 27);
  display.display();
  delay(1000);
}

void drawEvilFace(int offsetX) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(BLACK);
  display.setCursor(14-offsetX, 9);
  display.println("\\");
  display.setCursor(66-offsetX, 9);
  display.println("/");
  display.fillCircle(15-offsetX, 18, 3, BLACK);
  display.fillCircle(69-offsetX, 18, 3, BLACK); 
  drawArc(42-offsetX, 27);
  display.display();
}
