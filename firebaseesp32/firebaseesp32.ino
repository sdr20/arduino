#include <WiFi.h>
#include <FirebaseESP32.h>
#include <DHT.h>

// Replace with your network credentials
const char* ssid = "simonrhain";
const char* password = "Sheila*082574";

// Replace with your Firebase project credentials
#define FIREBASE_HOST "https://itlog-database-default-rtdb.asia-southeast1.firebasedatabase.app"
#define FIREBASE_AUTH "AIzaSyCr9bO9Hxao0VIstBR1nK0UWchlCIQSNhU"

// DHT sensor settings
#define DHTPIN 4        // Digital pin connected to the DHT sensor
#define DHTTYPE DHT22   // DHT 22 (AM2302)

// Initialize DHT sensor
DHT dht(DHTPIN, DHTTYPE);

FirebaseData firebaseData;
FirebaseConfig firebaseConfig;
FirebaseAuth firebaseAuth;

bool fanOn = false;
float maxHumidity = 50.0;
float maxTemperature = 25.0;

void setup() {
  Serial.begin(115200);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  // Configure Firebase
  firebaseConfig.host = FIREBASE_HOST;
  firebaseConfig.signer.tokens.legacy_token = FIREBASE_AUTH;
  Firebase.begin(&firebaseConfig, &firebaseAuth);
  Firebase.reconnectWiFi(true);

  // Initialize DHT sensor
  dht.begin();

  // Set initial values to Firebase
  Firebase.setFloat(firebaseData, "/settings/maxHumidity", maxHumidity);
  Firebase.setFloat(firebaseData, "/settings/maxTemperature", maxTemperature);
  Firebase.setBool(firebaseData, "/control/fanOn", fanOn);

  // Setup listener for control and settings
  Firebase.setStreamCallback(firebaseData, streamCallback, streamTimeoutCallback);
  Firebase.beginStream(firebaseData, "/control");
  Firebase.beginStream(firebaseData, "/settings");
}

void loop() {
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();

  // Check if any reads failed and exit early (to try again).
  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  // Send sensor data to Firebase
  Firebase.setFloat(firebaseData, "/sensor_data/humidity", humidity);
  Firebase.setFloat(firebaseData, "/sensor_data/temperature", temperature);

  // Check and update fan status
  if (humidity > maxHumidity || temperature > maxTemperature) {
    if (!fanOn) {
      fanOn = true;
      Firebase.setBool(firebaseData, "/control/fanOn", fanOn);
      Serial.println("Fan turned ON");
    }
  } else {
    if (fanOn) {
      fanOn = false;
      Firebase.setBool(firebaseData, "/control/fanOn", fanOn);
      Serial.println("Fan turned OFF");
    }
  }

  delay(2000); // Delay between readings
}

// Callback function for changes in the stream
void streamCallback(StreamData data) {
  if (data.dataPath() == "/control/fanOn") {
    fanOn = data.boolData();
    Serial.printf("Fan status changed: %s\n", fanOn ? "ON" : "OFF");
  } else if (data.dataPath() == "/settings/maxHumidity") {
    maxHumidity = data.floatData();
    Serial.printf("Max Humidity changed: %.2f\n", maxHumidity);
  } else if (data.dataPath() == "/settings/maxTemperature") {
    maxTemperature = data.floatData();
    Serial.printf("Max Temperature changed: %.2f\n", maxTemperature);
  }
}

// Callback function for stream timeout
void streamTimeoutCallback(bool timeout) {
  if (timeout) {
    Serial.println("Stream timeout, reconnecting...");
    Firebase.beginStream(firebaseData, "/control");
    Firebase.beginStream(firebaseData, "/settings");
  }
}
