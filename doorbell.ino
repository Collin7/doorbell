//#include <NTPClient.h>
//#include <WiFiUdp.h>
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
String motionStatus;
int noActivityTimer;
long motionCooldown = 20000;  // 20 sec before PIR reads motion again
long lastMotionTriggerTime = 0;

bool residentsAway = false;

String strTopic;
String strPayload;

const byte rowsLCD = 2;
const byte columnsLCD = 16;

int successFailTone[] = { 261, 277, 294, 311, 330, 349, 370, 392, 415, 440 };
int normalKeypressTone = 1760;

//Initialize Objects
WiFiClient espDoorbellController;
PubSubClient client(espDoorbellController);
SimpleTimer timer;
EasyButton doorbellButton(DOORBELL_BUTTON);
//WiFiUDP ntpUDP;
//NTPClient timeClient(ntpUDP, "za.pool.ntp.org", 7200, 60000);
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
}

void buttonPressHandler() {
  doorbellButton.read();
}

void doorbellButtonPressed() {
  //Logic here if doorbell pressed
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

  if (pirValue == LOW && pirStatus != 1) {
    motionStatus = "no motion";
    pirStatus = 1;

  } else if (pirValue == HIGH && pirStatus != 2) {
    if (millis() - lastMotionTriggerTime >= motionCooldown) {
      lastMotionTriggerTime = millis();
      motionStatus = "motion detected";
      if (residentsAway) {
        showAwayMessage();
      } else {
        showWelcomeMessage();
      }
      //sendState();
      pirStatus = 2;
      for (int i = 0; i <= 2; i++) {
        singleBeepSoundEffect();
        lcd.backlight();
        delay(500);
        lcd.noBacklight();
        delay(500);
        lcd.backlight();
      }
    }
    timer.restartTimer(noActivityTimer);
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
  strTopic = String((char*)topic);
  // Serial.println("Topic " + strTopic);
  String command = String((char*)payload);
  // Serial.println("Command " + command);
  if (strTopic == restartTopic) {
    restartESP();
  } else if (strTopic == residentsAwayTopic) {
    residentsAway = parseBool(command);
    if (residentsAway) {
      showAwayMessage();
    } else {
      showWelcomeMessage();
    }
  }
}

bool parseBool(String value) {
  value.toLowerCase();
  return value.equals("true");
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