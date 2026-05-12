#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#define CT1_PIN 0  
#define CT2_PIN 1  

const char* WIFI_SSID = "NITDA-ICT-HUB";
const char* WIFI_PASS = "6666.2524#";
const char* TB_SERVER = "demo.thingsboard.io"; 
const char* TB_TOKEN = "a9H2tWfiKLMbxwVZ1aFE";    

// ---------------------------------------------------------
// [FEATURE 3: Dynamic Cloud Configuration] 
// These are DEFAULTS. They get overwritten by ThingsBoard over WiFi.
// ---------------------------------------------------------
float V_ASSUMED = 230.0;         
float CURRENT_CALIBRATION = 30.0; 
float TARIFF_RATE = 200.0; // [FEATURE 8: Real-Time Tariff] Cost per kWh in Naira

WiFiClient espClient;
PubSubClient mqttClient(espClient);

struct MeterData {
  float ct1_current;
  float ct2_current;
  float total_power;
  double energy_wh;
  String active_source;
};

MeterData liveData = {0, 0, 0, 0, "UNKNOWN"};
SemaphoreHandle_t dataMutex; 

// ---------------------------------------------------------
// [FEATURE 3: Dynamic Cloud Configuration] - The Listener
// This function triggers automatically when you change a setting on the website.
// ---------------------------------------------------------
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  StaticJsonDocument<256> doc;
  deserializeJson(doc, payload, length);

  // If the cloud sends a new voltage, update it instantly!
  if (doc.containsKey("V_ASSUMED")) {
    V_ASSUMED = doc["V_ASSUMED"];
    Serial.println("Cloud Update: Voltage is now " + String(V_ASSUMED));
  }
  if (doc.containsKey("CURRENT_CALIBRATION")) {
    CURRENT_CALIBRATION = doc["CURRENT_CALIBRATION"];
  }
  if (doc.containsKey("TARIFF_RATE")) {
    TARIFF_RATE = doc["TARIFF_RATE"];
  }
}

// ---------------------------------------------------------
// [FEATURE 1: Moving Average Filters]
// ---------------------------------------------------------
float getFilteredRMS(int pin, float &previous_filtered_value) {
  unsigned long sum_sq = 0;
  int sample_count = 1000;
  int offset = 2048; 

  // Fast ADC Sampling
  for (int i = 0; i < sample_count; i++) {
    int centered = analogRead(pin) - offset;
    sum_sq += (centered * centered);
    delayMicroseconds(50); 
  }

  float rms_adc = sqrt((float)sum_sq / sample_count);
  float raw_current = rms_adc * (3.3 / 4095.0) * CURRENT_CALIBRATION;

  // [FEATURE 1 LINE] The EMA Filter Math: Smooths out the jumps
  float smoothed_current = (0.2 * raw_current) + (0.8 * previous_filtered_value);
  if(smoothed_current < 0.15) smoothed_current = 0.0; // Cut off noise

  previous_filtered_value = smoothed_current;
  return smoothed_current;
}

// ---------------------------------------------------------
// THE SENSOR TASK (Runs on Core 0)
// ---------------------------------------------------------
void Task_SampleSensors(void *pvParameters) {
  float prev_ct1 = 0, prev_ct2 = 0;
  unsigned long last_time = millis();

  for (;;) { 
    float c1 = getFilteredRMS(CT1_PIN, prev_ct1);
    float c2 = getFilteredRMS(CT2_PIN, prev_ct2);

    float hours_passed = (millis() - last_time) / 3600000.0;
    last_time = millis();

    // ---------------------------------------------------------
    // [FEATURE 4: Dual-Source Power Profiling]
    // ---------------------------------------------------------
    String source = "IDLE";
    if (c1 > 0.5 && c2 < 0.5) source = "GRID_ACTIVE";
    else if (c2 > 0.5 && c1 < 0.5) source = "INVERTER_ACTIVE";

    // ---------------------------------------------------------
    // [FEATURE 5: Overload & Load Priority Alarms]
    // ---------------------------------------------------------
    bool overload_warning = false;
    // If running on inverter and load exceeds 15 Amps, trigger an alarm!
    if (source == "INVERTER_ACTIVE" && c2 > 15.0) {
      overload_warning = true; 
    }

    if (xSemaphoreTake(dataMutex, portMAX_DELAY)) {
      liveData.ct1_current = c1;
      liveData.ct2_current = c2;
      liveData.total_power = (c1 + c2) * V_ASSUMED;
      liveData.energy_wh += liveData.total_power * hours_passed;
      liveData.active_source = source;
      xSemaphoreGive(dataMutex);
    }
    
    // [FEATURE 5 LINE] Instantly push an urgent alarm topic if overloaded
    if (overload_warning && mqttClient.connected()) {
       mqttClient.publish("v1/devices/me/telemetry", "{\"ALARM\":\"INVERTER OVERLOAD\"}");
    }

    vTaskDelay(100 / portTICK_PERIOD_MS); 
  }
}

// ---------------------------------------------------------
// THE MQTT TASK (Runs every 1 second)
// ---------------------------------------------------------
void Task_MQTT(void *pvParameters) {
  for (;;) {
    if (WiFi.status() == WL_CONNECTED) {
      if (!mqttClient.connected()) {
        mqttClient.connect("AMI-V0.1", TB_TOKEN, NULL);
        
        // [FEATURE 3 LINE] Subscribe to Cloud Configuration changes upon connection
        mqttClient.subscribe("v1/devices/me/attributes"); 
      } else {
        mqttClient.loop(); // Keeps the listener active

        MeterData tempCopy;
        if (xSemaphoreTake(dataMutex, portMAX_DELAY)) {
          tempCopy = liveData;
          xSemaphoreGive(dataMutex);
        }

        // ---------------------------------------------------------
        // [FEATURE 8: Real-Time Tariff Tracking]
        // ---------------------------------------------------------
        float cost_incurred = (tempCopy.energy_wh / 1000.0) * TARIFF_RATE;

        StaticJsonDocument<256> doc;
        doc["ct1_current"] = String(tempCopy.ct1_current, 2);
        doc["ct2_current"] = String(tempCopy.ct2_current, 2);
        doc["total_power"] = String(tempCopy.total_power, 2);
        doc["energy_wh"] = String(tempCopy.energy_wh, 2);
        doc["active_source"] = tempCopy.active_source;
        doc["real_time_cost"] = String(cost_incurred, 2); // [FEATURE 8 LINE]

        char buffer[256];
        serializeJson(doc, buffer);
        mqttClient.publish("v1/devices/me/telemetry", buffer);
      }
    }
    vTaskDelay(1000 / portTICK_PERIOD_MS); 
  }
}

void setup() {
  Serial.begin(115200);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) delay(500);

  mqttClient.setServer(TB_SERVER, 1883);
  mqttClient.setCallback(mqttCallback); // Link the listener function

  dataMutex = xSemaphoreCreateMutex();
  xTaskCreate(Task_SampleSensors, "Sensor", 4096, NULL, 2, NULL); 
  xTaskCreate(Task_MQTT, "MQTT", 4096, NULL, 1, NULL); 
}

void loop() {}