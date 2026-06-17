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

// =========================
// POWER BOOST SETTINGS
// =========================
int minPower = 120;     // Power minimal agar motor bisa start di karpet
int maxPower = 255;     // Power maksimum

// =========================
// WiFi CONNECTION
// =========================
void setup_wifi() {
  Serial.println();
  Serial.print("Connecting WiFi ");
  
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println();
  Serial.println("WiFi Connected");
  Serial.print("IP : ");
  Serial.println(WiFi.localIP());
}

// =========================
// MOTOR CONTROL (SESUAI KODE AWAL)
// =========================
void setMotor(int pwmPin, int in1, int in2, int speedVal) {
  // Boost power untuk karpet
  if (speedVal > 0 && speedVal < minPower) {
    speedVal = minPower;
  }
  if (speedVal < 0 && speedVal > -minPower) {
    speedVal = -minPower;
  }
  
  speedVal = constrain(speedVal, -maxPower, maxPower);
  
  if(speedVal > 0) {
    digitalWrite(in1, HIGH);
    digitalWrite(in2, LOW);
    ledcWrite(pwmPin, speedVal);
    Serial.printf("  PWM %d: %d FORWARD\n", pwmPin, speedVal);
  }
  else if(speedVal < 0) {
    digitalWrite(in1, LOW);
    digitalWrite(in2, HIGH);
    ledcWrite(pwmPin, abs(speedVal));
    Serial.printf("  PWM %d: %d BACKWARD\n", pwmPin, abs(speedVal));
  }
  else {
    digitalWrite(in1, LOW);
    digitalWrite(in2, LOW);
    ledcWrite(pwmPin, 0);
    Serial.printf("  PWM %d: STOP\n", pwmPin);
  }
}

// =========================
// MOTOR COMMAND LANGSUNG (KIWI DRIVE TAPI KITA UBAH)
// =========================
void moveMotors(int motorKiri, int motorKanan, int motorBelakang) {
  Serial.println("=================================");
  Serial.printf("MOTOR CMD: Kiri=%d, Kanan=%d, Belakang=%d\n", 
                motorKiri, motorKanan, motorBelakang);
  
  // Motor 1 = Depan Kiri (ENA1, IN1, IN2)
  setMotor(ENA1, IN1, IN2, motorKiri);
  
  // Motor 2 = Depan Kanan (ENB1, IN3, IN4)
  setMotor(ENB1, IN3, IN4, motorKanan);
  
  // Motor 3 = Belakang (ENA2, IN5, IN6)
  setMotor(ENA2, IN5, IN6, motorBelakang);
  
  Serial.println("=================================\n");
}

// =========================
// GERAKAN DASAR
// =========================
void maju(int speed) {
  int s = constrain(speed, 0, maxPower);
  moveMotors(s, s, s);
  Serial.printf(">> MAJU (%d)\n", s);
}

void mundur(int speed) {
  int s = constrain(speed, 0, maxPower);
  moveMotors(-s, -s, -s);
  Serial.printf(">> MUNDUR (%d)\n", s);
}

void geserKiri(int speed) {
  int s = constrain(speed, 0, maxPower);
  moveMotors(-s, s, 0);
  Serial.printf(">> GESER KIRI (%d)\n", s);
}

void geserKanan(int speed) {
  int s = constrain(speed, 0, maxPower);
  moveMotors(s, -s, 0);
  Serial.printf(">> GESER KANAN (%d)\n", s);
}

void rotasiKiri(int speed) {
  int s = constrain(speed, 0, maxPower);
  moveMotors(-s, s, -s);
  Serial.printf(">> ROTASI KIRI (%d)\n", s);
}

void rotasiKanan(int speed) {
  int s = constrain(speed, 0, maxPower);
  moveMotors(s, -s, s);
  Serial.printf(">> ROTASI KANAN (%d)\n", s);
}

void stopRobot() {
  moveMotors(0, 0, 0);
  Serial.println(">> STOP");
}

// =========================
// SOLENOID / KICKER
// =========================
void kick() {
  Serial.println(">> KICK / SHOOT!");
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
  
  Serial.print("Topic : ");
  Serial.println(topic);
  Serial.print("Payload : ");
  Serial.println(data);
  
  if(String(topic) == "robot/cmd") {
    StaticJsonDocument<128> doc;
    DeserializationError err = deserializeJson(doc, data);
    
    if(!err) {
      String action = doc["action"];
      int speed = doc["speed"] | 150;
      
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
      
      client.subscribe("robot/cmd");
      Serial.println("Subscribed to: robot/cmd");
    }
    else {
      Serial.print("Failed, rc=");
      Serial.println(client.state());
      delay(2000);
    }
  }
}

// =========================
// TEST MOTOR SATU PERSATU
// =========================
void testAllMotors() {
  Serial.println("\n========== MOTOR TEST ==========");
  
  Serial.println("\n1. Test Motor KIRI (Depan Kiri) - Maju");
  moveMotors(200, 0, 0);
  delay(1500);
  stopRobot();
  delay(500);
  
  Serial.println("\n2. Test Motor KANAN (Depan Kanan) - Maju");
  moveMotors(0, 200, 0);
  delay(1500);
  stopRobot();
  delay(500);
  
  Serial.println("\n3. Test Motor BELAKANG - Maju");
  moveMotors(0, 0, 200);
  delay(1500);
  stopRobot();
  delay(500);
  
  Serial.println("\n4. Test SEMUA MOTOR Maju");
  maju(200);
  delay(1500);
  stopRobot();
  delay(500);
  
  Serial.println("\n5. Test SEMUA MOTOR Mundur");
  mundur(200);
  delay(1500);
  stopRobot();
  
  Serial.println("\n========== TEST SELESAI ==========");
}

// =========================
// SETUP
// =========================
void setup() {
  Serial.begin(115200);
  Serial.println("\n==================================");
  Serial.println("ROBO-3 MECANUM 3-WHEEL CONTROLLER");
  Serial.println("WITH POWER BOOST FOR CARPET");
  Serial.println("==================================\n");
  
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
  
  // Setup PWM (CARA YANG SUDAH TERBUKTI BEKERJA)
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
  
  Serial.println("\nSystem Ready!");
  Serial.println("Waiting for MQTT connection...\n");
  
  // JALANKAN TEST MOTOR (UNCOMMENT UNTUK TESTING)
  delay(2000);
  testAllMotors();
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
  if(millis() - lastCommandTime > SAFETY_TIMEOUT) {
    static bool wasStopped = false;
    if(!wasStopped) {
      stopRobot();
      wasStopped = true;
      Serial.println("SAFETY: Auto-stop (no command)");
    }
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
