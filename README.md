# Smart Classroom Automation and Monitoring System Using IoT and ESP32

## 📌 Project Overview

This project presents an IoT-based Smart Classroom Automation and Monitoring System designed to improve classroom automation, safety, energy efficiency, and environmental monitoring using ESP32 technology.

The system automatically controls classroom devices such as lights, fans, and projectors based on human presence and environmental conditions. It also monitors smoke levels, temperature, and humidity in real time while providing a web-based dashboard for remote monitoring and manual control.

Additionally, the system includes an automated email notification service that sends the daily classroom routine using Gmail SMTP.



# 🚀 Features

✅ Automatic Light Control using PIR Motion Sensor
✅ Automatic Fan Control
✅ Ultrasonic-Based Projector Automation
✅ Real-Time Temperature & Humidity Monitoring
✅ Smoke and Gas Detection using MQ-2 Sensor
✅ Buzzer Alarm for Safety Alerts
✅ Web-Based IoT Dashboard
✅ Manual Device Control from Browser
✅ Live Sensor Data Monitoring
✅ Daily Automated Email Notifications
✅ NTP Time Synchronization
✅ Dynamic MQ2 Calibration System



# 🛠️ Hardware Components

| Component                   | Quantity    | Purpose                           |
| --------------------------- | ----------- | --------------------------------- |
| ESP32 Development Board     | 1           | Main microcontroller              |
| DHT11 Sensor                | 1           | Temperature & humidity monitoring |
| PIR Motion Sensor           | 1           | Human motion detection            |
| MQ-2 Gas Sensor             | 1           | Smoke and gas detection           |
| Ultrasonic Sensor (HC-SR04) | 1           | Projector automation              |
| Relay Module                | 2/4 Channel | Device switching                  |
| Buzzer                      | 1           | Alarm notification                |
| LED/Light Bulb              | 1           | Smart lighting                    |
| Fan                         | 1           | Smart cooling system              |
| Jumper Wires                | Several     | Circuit connections               |
| Breadboard                  | 1           | Prototyping                       |
| WiFi Network                | 1           | IoT connectivity                  |


# 🔌 Hardware Connections

## 📍 DHT11 Sensor

| DHT11 Pin | ESP32 Pin |
| --------- | --------- |
| VCC       | 3.3V      |
| GND       | GND       |
| DATA      | GPIO 5    |


## 📍 PIR Motion Sensor

| PIR Pin | ESP32 Pin |
| ------- | --------- |
| VCC     | 5V        |
| GND     | GND       |
| OUT     | GPIO 13   |


## 📍 MQ-2 Gas Sensor

| MQ2 Pin | ESP32 Pin |
| ------- | --------- |
| VCC     | 5V        |
| GND     | GND       |
| AO      | GPIO 34   |
| DO      | GPIO 23   |


## 📍 Ultrasonic Sensor (HC-SR04)

| Ultrasonic Pin | ESP32 Pin |
| -------------- | --------- |
| VCC            | 5V        |
| GND            | GND       |
| TRIG           | GPIO 12   |
| ECHO           | GPIO 14   |


## 📍 Relay Connections

| Device      | ESP32 GPIO |
| ----------- | ---------- |
| Light Relay | GPIO 26    |
| Fan Relay   | GPIO 27    |


## 📍 Buzzer Connection

| Buzzer Pin | ESP32 Pin |
| ---------- | --------- |
| Positive   | GPIO 25   |
| Negative   | GND       |



# ⚙️ System Workflow

1. ESP32 reads sensor data continuously.
2. PIR sensor detects human movement.
3. Lights and fans turn ON automatically when motion is detected.
4. Ultrasonic sensor controls projector activity.
5. MQ-2 sensor monitors smoke and harmful gas levels.
6. If smoke is detected:

   * Buzzer alarm activates
   * Electrical devices turn OFF automatically
7. Web dashboard displays live classroom data.
8. Users can manually control devices from the dashboard.
9. Automated emails send the daily class routine at scheduled times.



# 🌐 Web Dashboard Features

The ESP32 hosts a local web server that provides:

* Real-time sensor monitoring
* Light ON/OFF control
* Fan ON/OFF control
* Buzzer control
* Projector control
* Classroom routine page
* Sensor calibration option


# 📧 Email Notification System

The system automatically sends daily classroom schedules using Gmail SMTP integration.

Features:

* Scheduled email sending
* Real-time environmental status
* Classroom routine delivery



# 📊 Sensors Used

| Sensor            | Function                          |
| ----------------- | --------------------------------- |
| DHT11             | Temperature & humidity monitoring |
| PIR Sensor        | Human detection                   |
| MQ-2              | Smoke/gas detection               |
| Ultrasonic Sensor | Distance/projector detection      |



# 🧠 Automation Logic

| Condition              | Action                 |
| ---------------------- | ---------------------- |
| Motion Detected        | Light & Fan ON         |
| No Motion              | Light & Fan OFF        |
| Smoke Detected         | Alarm ON + Devices OFF |
| Object within Distance | Projector ON           |
| No Object Detected     | Projector OFF          |



# 💻 Technologies Used

* ESP32
* Embedded C++
* Arduino IDE
* WiFi Networking
* HTML/CSS Dashboard
* JSON API
* SMTP Email Service
* IoT Automation



# 📷 Project Output

The system provides:

* Live sensor monitoring dashboard
* Automated classroom appliance control
* Smoke detection alert system
* Daily classroom schedule email service


# 🔮 Future Improvements

* Cloud database integration
* Mobile application support
* Face recognition attendance system
* AI-based energy optimization




# 📚 Project Title

Smart Classroom Automation and Monitoring System Using IoT and ESP32



# 👨‍💻 Authors

KUMARY PUSPO RANI
Department of computer science and engineering .

# 📄 License

This project is developed for educational and research purposes.
