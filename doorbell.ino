#include <NTPClient.h>
#include <WiFiUdp.h>
#include <EasyButton.h>
#include <Credentials.h>
#include <ESP8266WiFi.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>
#include <ESP8266SSDP.h>
#include <SimpleTimer.h>
#include <LiquidCrystal_I2C.h>

const char* DEVICE_NAME = "Doorbell Controller";

#define doorbellTopic "doorbell/ring"
#define doorbellRestartTopic "doorbell/restart"

//Define Pins
#define DOORBELL_BUTTON D5
#define PIRPIN D6
#define SPEAKER_PIN D8

int pirValue;
int pirStatus;
String motionStatus;
int noActivityTimer;
long motionCooldown = 20000;
long lastMotionTriggerTime = 0;


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
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "za.pool.ntp.org", 7200, 60000);
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

  noActivityTimer = timer.setInterval(10000, noActivityAction);  //5 min = 300000

  setupWiFi();
  setupMQTT();
  setupOTA();
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
  client.publish("doorbell/pressed", "ding-dong", true);
  timer.restartTimer(noActivityTimer);
  buttonPressedSoundEffect();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(1, 0);
  lcd.print("RESIDENTS HAVE");
  lcd.setCursor(1, 1);
  lcd.print("BEEN NOTIFIED");
}

void buttonPressedSoundEffect() {
  for (int note : successFailTone) {
    tone(SPEAKER_PIN, note);
    delay(50);
  }
  noTone(SPEAKER_PIN);
}

void presenceSoundEffect() {
  tone(SPEAKER_PIN, normalKeypressTone);
  delay(100);
  noTone(SPEAKER_PIN);
}

void checkPirSensor() {
  pirValue = digitalRead(PIRPIN);  //read state of the PIR

  if (pirValue == LOW && pirStatus != 1) {
    motionStatus = "no motion";
    //sendState();
    pirStatus = 1;

  } else if (pirValue == HIGH && pirStatus != 2) {
    if (millis() - lastMotionTriggerTime >=  motionCooldown) {
      lastMotionTriggerTime = millis();
      motionStatus = "motion detected";
      presenceSoundEffect();
      showWelcomeMessage();
      //sendState();
      pirStatus = 2;
      for (int i = 0; i <= 2; i++) {
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

void showWelcomeMessage() {
  lcd.clear();
  lcd.setCursor(4, 0);
  lcd.print("WELCOME!");
  lcd.setCursor(0, 1);
  lcd.print("PLEASE RING BELL");
}

void showAwayMessage() {
  lcd.clear();
  lcd.setCursor(6, 0);
  lcd.print("SORRY!");
  lcd.setCursor(0, 1);
  lcd.print("WE NOT AT HOME!!");
}

void setupWiFi() {

  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");

    // Timeout if connection takes to long (15 seconds)
    if (millis() - startTime > 15000) {
      restartESP();
    }
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println("MAC: ");
  Serial.println(WiFi.macAddress());
}

void setupMQTT() {
  client.setServer(MQTT_SERVER, MQTT_PORT);
  client.setCallback(callback);  // callback is the function that gets called for a topic subscription

  Serial.println("Attempting MQTT connection...");
  if (client.connect(DEVICE_NAME, MQTT_USERNAME, MQTT_PASSWORD)) {
    Serial.println("Connected to MQTT broker");
    subscribeMQTTTopics();
  } else {
    Serial.print("Failed to connect to MQTT broker. RC=");
    Serial.println(client.state());
    Serial.println("Restarting ESP...");
    restartESP();
  }
}

void subscribeMQTTTopics() {
  client.subscribe("doorbell/#");
}

void callback(char* topic, byte* payload, unsigned int length) {
  //Handle mqtt messages received by this device
  payload[length] = '\0';
  strTopic = String((char*)topic);
  Serial.println("Topic " + strTopic);
  String command = String((char*)payload);
  Serial.println("Command " + command);
  // if (strTopic ==  restart_topic) {
  //   restartESP();
  // }
  //  else if (strTopic == dateTimeTopic) {
  //   // dateTime = command;
  //   // if (!menuActive) {
  //   //   drawHomeScreen();
  //   // }
  // }
}

void setupOTA() {
  ArduinoOTA.setHostname(DEVICE_NAME);
  ArduinoOTA.begin();
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
  ESP.restart();
}