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
// MOTOR CALIBRATION
// =========================
const float MOTOR_COMP_KIRI = 0.75; 
const float MOTOR_COMP_KANAN = 1.00;
const float MOTOR_COMP_BELAKANG = 1.00;

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
bool isStopped = true; // Menyimpan status stop

// =========================
// POWER BOOST SETTINGS
// =========================
int minPower = 120;     // Power minimal agar motor bisa start di karpet
int maxPower = 255;     // Power maksimum

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
  speedVal = constrain(speedVal, -maxPower, maxPower);
  
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
// MOTOR COMMAND LANGSUNG
// =========================
void moveMotors(int motorKiri, int motorKanan, int motorBelakang) {
  // Boost power (minPower) dipindah ke sini agar tidak membatalkan kompensasi
  if (motorKiri > 0 && motorKiri < minPower) motorKiri = minPower;
  if (motorKiri < 0 && motorKiri > -minPower) motorKiri = -minPower;
  
  if (motorKanan > 0 && motorKanan < minPower) motorKanan = minPower;
  if (motorKanan < 0 && motorKanan > -minPower) motorKanan = -minPower;
  
  if (motorBelakang > 0 && motorBelakang < minPower) motorBelakang = minPower;
  if (motorBelakang < 0 && motorBelakang > -minPower) motorBelakang = -minPower;

  setMotor(ENA1, IN1, IN2, (int)(motorKiri * MOTOR_COMP_KIRI));
  setMotor(ENB1, IN3, IN4, (int)(motorKanan * MOTOR_COMP_KANAN));
  setMotor(ENA2, IN5, IN6, (int)(motorBelakang * MOTOR_COMP_BELAKANG));
  isStopped = false; // Tandai bahwa robot sedang bergerak
}

// =========================
// GERAKAN DASAR
// =========================
void maju(int speed) {
  int s = constrain(speed, 0, maxPower);
  moveMotors(s, s, 0); // Motor belakang (0) agar tidak mendorong ke samping
}

void mundur(int speed) {
  int s = constrain(speed, 0, maxPower);
  moveMotors(-s, -s, 0);
}

void geserKiri(int speed) {
  int s = constrain(speed, 0, maxPower);
  // Gerak kepiting ke Kiri tanpa rotasi
  moveMotors(-s/2, s/2, s);
}

void geserKanan(int speed) {
  int s = constrain(speed, 0, maxPower);
  // Gerak kepiting ke Kanan tanpa rotasi
  moveMotors(s/2, -s/2, -s);
}

void rotasiKiri(int speed) {
  int s = constrain(speed, 0, maxPower);
  moveMotors(-s, s, -s);
}

void rotasiKanan(int speed) {
  int s = constrain(speed, 0, maxPower);
  moveMotors(s, -s, s);
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
    StaticJsonDocument<256> doc; // Memperbesar kapasitas memory parsing JSON jaga-jaga
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
  
  // Safety stop: Otomatis berhenti jika tidak ada perintah > 500ms
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
