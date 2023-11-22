#include <EasyButton.h>
#include <Credentials.h>
#include <ESP8266WiFi.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>
#include <ESP8266SSDP.h>
#include <SimpleTimer.h>
#include <LiquidCrystal_I2C.h>

const char* DEVICE_NAME = "Doorbell Controller";

//Define MQTT Topics
const char* doorbellPressedTopic = "doorbell/pressed";
const char* restartTopic = "doorbell/restart";
const char* residentsAwayTopic = "doorbell/residents/away";

//Define Pins
#define DOORBELL_BUTTON D5
#define PIRPIN D6
#define SPEAKER_PIN D8


//Define Global Variables
int pirValue;
int pirStatus;
int noActivityTimer;
long motionCooldown = 30000;  // 30 sec before PIR reads motion again
long lastMotionTriggerTime = 0;

bool residentsAway = false;

const byte rowsLCD = 2;
const byte columnsLCD = 16;

const int flashDuration = 500;  // Time in milliseconds for each flash
const int flashes = 4;          // Number of flashes
bool flashLCD = false;
unsigned long previousMillis = 0;
int flashCount = 0;


int successFailTone[] = { 261, 277, 294, 311, 330, 349, 370, 392, 415, 440 };
int normalKeypressTone = 1760;

//Initialize Objects
WiFiClient espDoorbellController;
PubSubClient client(espDoorbellController);
SimpleTimer timer;
EasyButton doorbellButton(DOORBELL_BUTTON);
LiquidCrystal_I2C lcd(0x3F, columnsLCD, rowsLCD);

void setup() {
  Serial.begin(115200);

  pinMode(DOORBELL_BUTTON, INPUT);
  pinMode(PIRPIN, INPUT);

  lcd.init();
  lcd.backlight();

  showWelcomeMessage();

  doorbellButton.begin();
  doorbellButton.onPressed(doorbellButtonPressed);

  setupWiFi();
  setupMQTT();
  setupOTA();

  noActivityTimer = timer.setInterval(300000, noActivityAction);  //5 min = 300000
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  timer.run();
  client.loop();  //the mqtt function that processes MQTT messages
  ArduinoOTA.handle();
  buttonPressHandler();
  checkPirSensor();
  flashLcdOnMotion();
}

void flashLcdOnMotion() {
  unsigned long currentMillis = millis();

  if (flashLCD) {

    if (currentMillis - previousMillis >= flashDuration) {
      previousMillis = currentMillis;

      if (flashCount % 2 == 0) {
        singleBeepSoundEffect();
        lcd.backlight();
      } else {
        lcd.noBacklight();
      }
      // Increment the flash count
      flashCount++;
      // Check if the flashing sequence is complete
      if (flashCount >= flashes * 2) {
        // set flash to false
        lcd.backlight();
        flashLCD = false;
        flashCount = 0;
      }
    }
  }
}

void flashLCDOnMotionDetection() {
  unsigned long currentMillis = millis();
}

void buttonPressHandler() {
  doorbellButton.read();
}

void doorbellButtonPressed() {
  lcd.backlight();  // in case button was pressed during the off flash state
  flashLCD = false;
  client.publish(doorbellPressedTopic, "ding-dong", false);
  timer.restartTimer(noActivityTimer);
  if (!residentsAway) {
    showResidentsNotifiedMessage();
    buttonPressedHomeSoundEffect();
  } else {
    showResidentsNotifiedNotHomeMessage();
    buttonPressedAwaySoundEffect();
  }
}

void checkPirSensor() {
  pirValue = digitalRead(PIRPIN);

  if (pirValue == HIGH && pirStatus != 2) {
    //Motion detected
    if (millis() - lastMotionTriggerTime >= motionCooldown) {
      pirStatus = 2;
      flashLCD = true;
      lastMotionTriggerTime = millis();
      if (residentsAway) {
        showAwayMessage();
      } else {
        showWelcomeMessage();
      }
    }
    timer.restartTimer(noActivityTimer);

  } else if (pirValue == LOW && pirStatus != 1) {
    // no motion
    pirStatus = 1;
  }
}

void noActivityAction() {
  lcd.noBacklight();
}

void subscribeMQTTTopics() {
  client.subscribe("doorbell/#");
}

void callback(char* topic, byte* payload, unsigned int length) {
  //Handle mqtt messages received by this device
  payload[length] = '\0';

  if (strcmp(topic, restartTopic) == 0) {
    restartESP();
  } else if (strcmp(topic, residentsAwayTopic) == 0) {
    residentsAway = parseBool((const char*)payload);
    if (residentsAway) {
      showAwayMessage();
    } else {
      showWelcomeMessage();
    }
  }
}

bool parseBool(const char* value) {
  char lowerCaseValue[strlen(value) + 1];  // Create a buffer for the lowercase string
  strcpy(lowerCaseValue, value);           // Copy the original string to the buffer
  strlwr(lowerCaseValue);                  // Convert the buffer to lowercase

  return (strcmp(lowerCaseValue, "true") == 0);
}

void reconnect() {
  // Reconnect to WiFi and MQTT
  if (WiFi.status() != WL_CONNECTED) {
    setupWiFi();
  }
  // Check if MQTT connection is not established
  if (!client.connected()) {
    setupMQTT();
  }
}

void restartESP() {

  for (int i = 0; i <= 2; i++) {
    singleBeepSoundEffect();
    delay(100);
    showRestartingMessage();
  }
  delay(1000);
  ESP.restart();
}