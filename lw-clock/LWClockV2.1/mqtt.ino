#include "config.h" // Include the global configuration file
#include <ArduinoJson.h> // Ensure ArduinoJson is included

// Ensure all required variables and types are declared
extern uint8_t brightd;
extern int8_t timezone;
extern bool isDayLightSaving;
extern String MQTTLine; // Declare MQTTLine variable
extern unsigned long ReconnectTime;
extern const unsigned long MQTT_CONNECT;
extern PubSubClient mqttClient;
extern bool mqttOn;
extern String mqtt_name;
extern String mqtt_user;
extern String mqtt_pass;
extern String mqtt_server;
extern int mqtt_port;
extern String mqtt_sub_crline;
extern void displayShowText(String message); // Declare the function from LWClockV2.1.ino


void publishHomeAssistantDeviceDiscovery() {
    char topic_config[128];
    StaticJsonDocument<512> root; // Reduced size to fit within optimized payload
    char buffer[512];

    // Device block (abbreviated as dev)
    JsonObject dev = root.createNestedObject("dev");
    dev["ids"] = mqtt_name.c_str();
    dev["name"] = mqtt_name.c_str();
    dev["mf"] = "Peik Industries";
    dev["mdl"] = "LWClock v2.1";
    dev["sw"] = "2.1";

    // Origin block (abbreviated as o)
    JsonObject o = root.createNestedObject("o");
    o["name"] = "LWClock MQTT";
    o["sw"] = "2.1";

    // Components block
    JsonObject cmps = root.createNestedObject("cmps");

    // Brightness component
    JsonObject brightness = cmps.createNestedObject("brightness");
    brightness["p"] = "light";
    brightness["name"] = "Brightness";
    brightness["unique_id"] = "brightness";
    brightness["command_topic"] = "homeassistant/brightness/set";
    brightness["state_topic"] = "homeassistant/brightness";
    brightness["brightness"] = true;
    brightness["supported_color_modes"] = "brightness";
    brightness["brightness_scale"] = 23;
    brightness["ic"] = "mdi:view-comfy";

    // Creeping line component
    JsonObject creepingLine = cmps.createNestedObject("MQTTLine");
    creepingLine["p"] = "text";
    creepingLine["name"] = "MQTTLine";
    creepingLine["unique_id"] = "MQTTLine";
    creepingLine["command_topic"] = "homeassistant/MQTTLine/set";
    creepingLine["state_topic"] = "homeassistant/MQTTLine";

    // Debug: Print constructed JSON
    serializeJsonPretty(root, Serial);

    // Publish the device discovery payload
    sprintf(topic_config, "homeassistant/device/%s/config", mqtt_name.c_str());
    serializeJson(root, buffer);
    mqttClient.publish(topic_config, buffer, true);

    Serial.println("Published Home Assistant device discovery configuration.");
}

void init_mqtt() {
  mqttClient.setServer(mqtt_server.c_str(), mqtt_port);
  mqttClient.setCallback(callback);
  mqttClient.setKeepAlive(60); // Increase keep-alive interval to 60 seconds
}

boolean reconnect() {
  Serial.print("Attempting MQTT connection...");
  if (mqttClient.connect(mqtt_name.c_str(), mqtt_user.c_str(), mqtt_pass.c_str())) {
    Serial.println("connected");
    mqttClient.publish("ClockTopic", "Hello world");
    mqttClient.subscribe("homeassistant/brightness/set"); // Updated to include device name in the topic
    mqttClient.subscribe(("homeassistant/" + mqtt_name + "/timezone/set").c_str());
    mqttClient.subscribe(("homeassistant/" + mqtt_name + "/daylight_savings/set").c_str());
    mqttClient.subscribe(("homeassistant/MQTTLine/set"));
    mqttClient.subscribe("showtext");
    publishHomeAssistantDeviceDiscovery();
  } else {
    Serial.print("failed, rc=");
    Serial.print(mqttClient.state());
    Serial.println(" try again in 5 seconds");
  }
  return mqttClient.connected();
}

void callback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  if (String(topic) == ("homeassistant/brightness/set")) {
    brightd = message.toInt();
    mqttClient.publish("homeassistant/brightness", String(brightd).c_str(), true);
    Serial.println("Brightness set to: " + String(brightd));
  } else if (String(topic) == "LWClock/timezone/set") {
    timezone = message.toInt();
    mqttClient.publish("LWClock/timezone", String(timezone).c_str(), true);
    Serial.println("Timezone set to: " + String(timezone));
  } else if (String(topic) == "LWClock/daylight_savings/set") {
    isDayLightSaving = (message == "true");
    mqttClient.publish("LWClock/daylight_savings", isDayLightSaving ? "true" : "false", true);
    Serial.println("Daylight savings set to: " + String(isDayLightSaving));
  } else if (String(topic) == "showtext") { // Handle showtext topic
    Serial.println("Received showtext: " + message);
    displayShowText(message); // Call the function in LWClockV2.1.ino
  } else if (String(topic) == mqtt_sub_crline) {
    Serial.println("Received MQTTLine: " + message);
    MQTTLine = message; // Update MQTTLine with the received message
    mqttClient.publish("LWClock/MQTTLine", MQTTLine.c_str(), true); // Publish the updated MQTTLine
// Add payload to serial output for MQTTLine/set
  } else if (String(topic) == "LWClock/MQTTLine/set") {
    Serial.println("Received topic: " + String(topic));
    Serial.println("Payload: " + message); // Print the payload
    MQTTLine = message; // Update MQTTLine with the received message
    mqttClient.publish("LWClock/MQTTLine", MQTTLine.c_str(), true); // Publish the updated MQTTLine
  } else {
    Serial.println("Unknown topic: " + String(topic));
  }
}

#if (USE_BME280 == true || USE_DHT == true)
void sendMQTT() {
  #if USE_DHT == true
  String tempDHT;
  String humDHT;
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  if (isnan(t) || isnan(h)) {
    Serial.println("Failed to read from DHT");
    tempDHT = "tErr";
    humDHT = "hErr";
  } else {
    float hic = dht.computeHeatIndex(t, h, false);
    tempDHT = String(hic, 1);
    humDHT = String(h, 1);
    Serial.print("Temperature: ");
    Serial.print(tempDHT);
    Serial.print("°C");
    Serial.print(". Humidity: ");
    Serial.print(humDHT);
    Serial.println("%");
  }
  tempDHT.toCharArray(mqttData, (tempDHT.length() + 1));
  mqttClient.publish(mqtt_pub_temp.c_str(), mqttData, true);
  humDHT.toCharArray(mqttData, (humDHT.length() + 1));
  mqttClient.publish(mqtt_pub_hum.c_str(), mqttData, true);
  #endif // USE_DHT

  #if USE_BME280 == true
  String tempBME = getTempBME280(dataCorrect, 2);
  String humBME = getHumBME280(dataCorrect, 2);
  String pressBME = getPressBME280(hpa, 2);
  Serial.print("Temperature: ");
  Serial.print(tempBME);
  Serial.print("°C");
  Serial.print(". Humidity: ");
  Serial.print(humBME);
  Serial.print("%");
  Serial.print(". Pressure: ");
  Serial.print(pressBME);
  Serial.println((hpa ? " hPa" : " mm"));
  tempBME.toCharArray(mqttData, (tempBME.length() + 1));
  mqttClient.publish(mqtt_pub_temp.c_str(), mqttData, true);
  humBME.toCharArray(mqttData, (humBME.length() + 1));
  mqttClient.publish(mqtt_pub_hum.c_str(), mqttData, true);
  pressBME.toCharArray(mqttData, (pressBME.length() + 1));
  mqttClient.publish(mqtt_pub_press.c_str(), mqttData, true);
  #endif // USE_BME280
}
#endif // (USE_BME280 == true || USE_DHT == true)
