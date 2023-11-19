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

void setupOTA() {
  ArduinoOTA.setHostname(DEVICE_NAME);
  ArduinoOTA.begin();
}