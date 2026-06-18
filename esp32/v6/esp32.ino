#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// =========================
// WIFI CONFIG
// =========================
const char* ssid = "Redmi 13";
const char* password = "iyapunyarael";

// =========================
// MQTT CONFIG
// =========================
const char* mqtt_server = "broker.hivemq.com";
const int mqtt_port = 1883;

WiFiClient espClient;
PubSubClient client(espClient);

// =========================
// MOTOR PINS (SESUAI KODE AWAL)
// =========================
#define ENA1 25
#define IN1 26
#define IN2 27

#define ENB1 14
#define IN3 12
#define IN4 13

#define ENA2 33
#define IN5 32
#define IN6 15

// =========================
// MOTOR CALIBRATION V6 (Advanced Mapping)
// =========================
// 1. MIN_POWER (Deadband) : Berapa PWM minimum agar motor KUAT mulai berputar?
const int MIN_POWER_KIRI = 90;
const int MIN_POWER_KANAN = 150; // Diberi 150 agar langsung kuat mendobrak beban baterai di kanan
const int MIN_POWER_BELAKANG = 120;

// 2. MAX_POWER (Top Speed) : Berapa batas atas kecepatan motor?
const int MAX_POWER_KIRI = 190;  // Dibatasi ke 190 agar kecepatan tertingginya tidak mendahului kanan
const int MAX_POWER_KANAN = 255; // Bebas mentok sampai 255
const int MAX_POWER_BELAKANG = 255;

// =========================
// SOLENOID (SHOOTER)
// =========================
#define SOLENOID_PIN 4

// =========================
// ULTRASONIC
// =========================
#define TRIG_PIN 23
#define ECHO_PIN 22

// =========================
// BUZZER
// =========================
#define BUZZER_PIN 21

// =========================
// SAFETY TIMER
// =========================
unsigned long lastCommandTime = 0;
const unsigned long SAFETY_TIMEOUT = 500;
bool isStopped = true;

// =========================
// WiFi CONNECTION
// =========================
void setup_wifi() {
  Serial.println("\nConnecting WiFi...");
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("\nWiFi Connected");
  Serial.print("IP : ");
  Serial.println(WiFi.localIP());
}

// =========================
// MOTOR CONTROL
// =========================
void setMotor(int pwmPin, int in1, int in2, int speedVal) {
  speedVal = constrain(speedVal, -255, 255);
  
  if(speedVal > 0) {
    digitalWrite(in1, HIGH);
    digitalWrite(in2, LOW);
    ledcWrite(pwmPin, speedVal);
  }
  else if(speedVal < 0) {
    digitalWrite(in1, LOW);
    digitalWrite(in2, HIGH);
    ledcWrite(pwmPin, abs(speedVal));
  }
  else {
    digitalWrite(in1, LOW);
    digitalWrite(in2, LOW);
    ledcWrite(pwmPin, 0);
  }
}

// =========================
// MOTOR COMMAND LANGSUNG (V6 ALGORITHM)
// =========================
void moveMotors(int motorKiri, int motorKanan, int motorBelakang) {
  
  // Lambda function untuk menghitung Advanced Mapping
  auto mapSpeed = [](int inputSpeed, int minPwr, int maxPwr) {
    if (inputSpeed == 0) return 0;
    int absSpeed = abs(inputSpeed);
    
    // Asumsikan inputSpeed dari controller antara 1 s/d 255
    // Kita petakan ke rentang minPwr s/d maxPwr yang spesifik per roda
    int mapped = map(absSpeed, 1, 255, minPwr, maxPwr);
    
    // Kembalikan arahnya (maju/mundur)
    return (inputSpeed > 0) ? mapped : -mapped;
  };
  
  // Terapkan mapping ke masing-masing roda
  int pwmKiri = mapSpeed(motorKiri, MIN_POWER_KIRI, MAX_POWER_KIRI);
  int pwmKanan = mapSpeed(motorKanan, MIN_POWER_KANAN, MAX_POWER_KANAN);
  int pwmBelakang = mapSpeed(motorBelakang, MIN_POWER_BELAKANG, MAX_POWER_BELAKANG);

  setMotor(ENA1, IN1, IN2, pwmKiri);
  setMotor(ENB1, IN3, IN4, pwmKanan);
  setMotor(ENA2, IN5, IN6, pwmBelakang);
  
  isStopped = false; // Tandai bahwa robot sedang bergerak
}

// =========================
// GERAKAN DASAR
// =========================
void maju(int speed) {
  int s = constrain(speed, 0, 255);
  moveMotors(s, s, 0); 
}

void mundur(int speed) {
  int s = constrain(speed, 0, 255);
  moveMotors(-s, -s, 0);
}

void geserKiri(int speed) {
  int s = constrain(speed, 0, 255);
  moveMotors(s/2, -s/2, -s);
}

void geserKanan(int speed) {
  int s = constrain(speed, 0, 255);
  moveMotors(-s/2, s/2, s);
}

void rotasiKiri(int speed) {
  int s = constrain(speed, 0, 255);
  moveMotors(s, -s, s);
}

void rotasiKanan(int speed) {
  int s = constrain(speed, 0, 255);
  moveMotors(-s, s, -s);
}

void stopRobot() {
  setMotor(ENA1, IN1, IN2, 0);
  setMotor(ENB1, IN3, IN4, 0);
  setMotor(ENA2, IN5, IN6, 0);
  isStopped = true;
}

// =========================
// SOLENOID / KICKER
// =========================
void kick() {
  digitalWrite(SOLENOID_PIN, HIGH);
  delay(200);
  digitalWrite(SOLENOID_PIN, LOW);
}

// =========================
// ULTRASONIC SENSOR
// =========================
float bacaJarak() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  
  long duration = pulseIn(ECHO_PIN, HIGH, 30000);
  
  if(duration == 0) return -1;
  
  return duration * 0.0343 / 2.0;
}

// =========================
// MQTT CALLBACK
// =========================
void callback(char* topic, byte* payload, unsigned int length) {
  String data;
  for(unsigned int i = 0; i < length; i++) {
    data += (char)payload[i];
  }
  
  if(String(topic) == "rafly/krsbi_iot/cmd") {
    StaticJsonDocument<256> doc; 
    DeserializationError err = deserializeJson(doc, data);
    
    if(!err) {
      String action = doc["action"];
      int speed = doc["speed"] | 150;
      
      Serial.printf("=> Terima JSON: %s (%d)\n", action.c_str(), speed);
      
      lastCommandTime = millis();
      
      if(action == "maju") {
        maju(speed);
      }
      else if(action == "mundur") {
        mundur(speed);
      }
      else if(action == "geserKiri") {
        geserKiri(speed);
      }
      else if(action == "geserKanan") {
        geserKanan(speed);
      }
      else if(action == "rotasiKiri") {
        rotasiKiri(speed);
      }
      else if(action == "rotasiKanan") {
        rotasiKanan(speed);
      }
      else if(action == "stop") {
        stopRobot();
      }
      else if(action == "kick") {
        kick();
      }
    } else {
      Serial.print("JSON Error: ");
      Serial.println(err.c_str());
    }
  }
}

// =========================
// MQTT RECONNECT
// =========================
void reconnect() {
  while(!client.connected()) {
    Serial.println("MQTT Connecting...");
    if(client.connect("ESP32_KIWI")) {
      Serial.println("MQTT Connected");
      client.subscribe("rafly/krsbi_iot/cmd");
    } else {
      delay(2000);
    }
  }
}

// =========================
// SETUP
// =========================
void setup() {
  Serial.begin(115200);
  Serial.println("\nSystem Ready...");
  
  // Setup motor pins
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(IN5, OUTPUT);
  pinMode(IN6, OUTPUT);
  
  // Setup solenoid
  pinMode(SOLENOID_PIN, OUTPUT);
  digitalWrite(SOLENOID_PIN, LOW);
  
  // Setup ultrasonic
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  
  // Setup buzzer
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  
  // Setup PWM
  ledcAttach(ENA1, 1000, 8);
  ledcAttach(ENB1, 1000, 8);
  ledcAttach(ENA2, 1000, 8);
  
  // Stop all motors
  stopRobot();
  
  // Connect to WiFi
  setup_wifi();
  
  // Setup MQTT
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  
  lastCommandTime = millis();
}

// =========================
// LOOP
// =========================
void loop() {
  if(!client.connected()) {
    reconnect();
  }
  client.loop();
  
  // Safety stop
  if(!isStopped && (millis() - lastCommandTime > SAFETY_TIMEOUT)) {
    stopRobot();
  }
  
  // Ultrasonic untuk obstacle
  float jarak = bacaJarak();
  if(jarak > 0 && jarak <= 10) {
    digitalWrite(BUZZER_PIN, HIGH);
  } else {
    digitalWrite(BUZZER_PIN, LOW);
  }
  
  delay(20);
}
