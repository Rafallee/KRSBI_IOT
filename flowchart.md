# Flowchart for esp32/v2/esp32.ino

This Mermaid flowchart documents the main program flow and MQTT callback actions.

```mermaid
flowchart TD
  Start([Start])
  Setup[/>Setup() - init Serial, pins, PWM, WiFi, MQTT, stopRobot/]
  Loop["Loop()"]
  CheckMQTT{client.connected()?}
  Reconnect["reconnect() -> connect and subscribe robot/cmd"]
  ClientLoop["client.loop()"]
  SafetyCheck{millis() - lastCommandTime &gt; SAFETY_TIMEOUT?}
  AutoStop["stopRobot(); wasStopped = true\n(Serial: SAFETY Auto-stop)"]
  ReadUltra["jarak = bacaJarak()"]
  Obstacle{jarak > 0 and jarak <= 10?}
  BuzzerOn["digitalWrite(BUZZER_PIN, HIGH)"]
  BuzzerOff["digitalWrite(BUZZER_PIN, LOW)"]
  Delay["delay(20)"]
  LoopBack --> Loop

  Start --> Setup --> Loop
  Loop --> CheckMQTT
  CheckMQTT -- No --> Reconnect --> CheckMQTT
  CheckMQTT -- Yes --> ClientLoop
  ClientLoop --> SafetyCheck
  SafetyCheck -- Yes and !wasStopped --> AutoStop
  SafetyCheck -- No --> ReadUltra
  AutoStop --> ReadUltra
  ReadUltra --> Obstacle
  Obstacle -- Yes --> BuzzerOn --> Delay
  Obstacle -- No --> BuzzerOff --> Delay
  Delay --> LoopBack

  subgraph MQTT_Callback [MQTT callback on topic "robot/cmd"]
    CB_In["Receive JSON payload"]
    ParseOK{deserializeJson OK?}
    Extract["action, speed (default 150)\nlastCommandTime = millis()"]
    ActionChoice{action == ?}
    Maju["maju(speed)"]
    Mundur["mundur(speed)"]
    GeserKiri["geserKiri(speed)"]
    GeserKanan["geserKanan(speed)"]
    RotasiKiri["rotasiKiri(speed)"]
    RotasiKanan["rotasiKanan(speed)"]
    StopCmd["stopRobot()"]
    KickCmd["kick() -> solenoid pulse"]
    ParseErr["Serial: JSON Error"]
  end

  ClientLoop -->|incoming msg| CB_In
  CB_In --> ParseOK
  ParseOK -- yes --> Extract --> ActionChoice
  ActionChoice -->|maju| Maju
  ActionChoice -->|mundur| Mundur
  ActionChoice -->|geserKiri| GeserKiri
  ActionChoice -->|geserKanan| GeserKanan
  ActionChoice -->|rotasiKiri| RotasiKiri
  ActionChoice -->|rotasiKanan| RotasiKanan
  ActionChoice -->|stop| StopCmd
  ActionChoice -->|kick| KickCmd
  ParseOK -- no --> ParseErr
```
