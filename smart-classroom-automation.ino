#include <WiFi.h>
#include "DHT.h"
#include "time.h"
#include <ESP_Mail_Client.h>

/* ---------- WiFi ---------- */
const char* ssid = "S24 Ultra";
const char* password = "1234567899";

/* ---------- Gmail SMTP ---------- */
#define ENABLE_EMAIL 1
#if ENABLE_EMAIL
const char* SMTP_SENDER_EMAIL    = "puspoha76@gmail.com";
const char* SMTP_SENDER_APP_PASS = "abcd efgh ijkl mnop";  // App password
const char* SMTP_RECIPIENT_EMAIL = "puspod00@gmail.com";
SMTPSession smtp;
#endif

/* ---------- Time ---------- */
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 6 * 3600;
const int   daylightOffset_sec = 0;
const int SEND_HOUR = 8;
const int SEND_MIN  = 0;

/* ---------- PIN SETUP ---------- */
#define PIR_PIN 13
#define DHTPIN 5
#define DHTTYPE DHT11
#define MQ2_AO 34
#define MQ2_DO 23
#define LIGHT_RELAY 26
#define FAN_RELAY 27
#define BUZZER 25
#define TRIG_PIN 12
#define ECHO_PIN 14

/* ---------- PARAMETERS ---------- */
#define OFF_DELAY 5000
#define SMOKE_SMOOTH_SAMPLES 30
#define CALIBRATION_SAMPLES 50
#define CALIBRATION_DELAY 50
#define THRESHOLD_FACTOR 2.5
#define WARMUP_MS 60000UL
const int U_ON_DISTANCE = 80;
const int U_OFF_DELAY   = 10000;

WiFiServer server(80);
DHT dht(DHTPIN, DHTTYPE);

/* ---------- STATES ---------- */
bool smokeAlert=false, buzzerActive=false, recalibrating=false;
bool projectorOn=false, emailSentToday=false, emailInfoPrintedToday=false;
bool lightOn=false, fanOn=false, motionDetected=false;
unsigned long buzzerStartTime=0, projectorLastSeen=0, lastMotionTime=0;
int SMOKE_THRESHOLD=400;

/* ---------- DECLARATIONS ---------- */
void handleSensors();
int  readSmoothMQ2();
void calibrateMQ2();
void handleWebRequests(WiFiClient &client);
void sendDashboard(WiFiClient &client);
void sendSchedulePage(WiFiClient &client);
void sendSensorData(WiFiClient &client);
long readDistanceCM();
void printNextEmailInfoIfNeeded();

#if ENABLE_EMAIL
void sendScheduleEmail();
void smtpCallback(SMTP_Status status);
#endif

/* ---------- SETUP ---------- */
void setup() {
  Serial.begin(115200);
  dht.begin();

  pinMode(PIR_PIN, INPUT);
  pinMode(MQ2_AO, INPUT);
  pinMode(MQ2_DO, INPUT);
  pinMode(LIGHT_RELAY, OUTPUT);
  pinMode(FAN_RELAY, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  digitalWrite(LIGHT_RELAY, HIGH);
  digitalWrite(FAN_RELAY, HIGH);
  digitalWrite(BUZZER, LOW);

  Serial.println("🟡 Warming up MQ2...");
  delay(WARMUP_MS);
  calibrateMQ2();

  WiFi.begin(ssid, password);
  Serial.print("Connecting WiFi");
  while(WiFi.status()!=WL_CONNECTED){delay(300);Serial.print(".");}
  Serial.println("\n✅ WiFi Connected!");
  Serial.println(WiFi.localIP());
  server.begin();

  Serial.println("⏱️ Syncing NTP time...");
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  struct tm timeinfo;
  while(!getLocalTime(&timeinfo)){delay(500);Serial.print(".");}
  Serial.println("\n✅ Time synchronized!");

#if ENABLE_EMAIL
  smtp.callback(smtpCallback);
#endif
  printNextEmailInfoIfNeeded();
}

/* ---------- LOOP ---------- */
void loop() {
  handleSensors();

  struct tm timeinfo;
  if(getLocalTime(&timeinfo)){
    int hourNow=timeinfo.tm_hour, minuteNow=timeinfo.tm_min;
    int nowMin=hourNow*60+minuteNow, sendMin=SEND_HOUR*60+SEND_MIN;

#if ENABLE_EMAIL
    if(nowMin==sendMin && !emailSentToday){
      sendScheduleEmail();
      emailSentToday=true;
      emailInfoPrintedToday=true;
    }
    if(!emailSentToday && !emailInfoPrintedToday){
      printNextEmailInfoIfNeeded();
    }
#endif

    if(hourNow==0 && minuteNow==0){
      emailSentToday=false;
      emailInfoPrintedToday=false;
    }
  }

  WiFiClient client=server.available();
  if(client) handleWebRequests(client);
}

/* ---------- SENSOR LOGIC ---------- */
void handleSensors(){
  int motion = digitalRead(PIR_PIN);
  int smokeAnalog=readSmoothMQ2();
  int smokeDigital=digitalRead(MQ2_DO);
  static int confirm=0;

  // 🔥 Smoke Detection
  if(!recalibrating){
    if(smokeAnalog>SMOKE_THRESHOLD || smokeDigital==HIGH){
      if(++confirm>=5 && !smokeAlert){
        smokeAlert=true;
        buzzerActive=true; buzzerStartTime=millis();
        digitalWrite(BUZZER,HIGH);
        digitalWrite(LIGHT_RELAY,HIGH);
        digitalWrite(FAN_RELAY,HIGH);
        Serial.println("🚨 Smoke detected! Light & Fan OFF. Alarm ON.");
      }
    } else {
      if(confirm>0) confirm--;
      if(confirm==0 && smokeAlert){
        smokeAlert=false;
        Serial.println("✅ Smoke cleared.");
      }
    }
  }

  if(buzzerActive && millis()-buzzerStartTime>5000){
    digitalWrite(BUZZER,LOW); buzzerActive=false;
  }

  // 🧍 Motion-based Light/Fan Control
  if(!smokeAlert && !recalibrating){
    if(motion==HIGH){
      motionDetected=true;
      lastMotionTime=millis();
      digitalWrite(LIGHT_RELAY,LOW);
      digitalWrite(FAN_RELAY,LOW);
      if(!lightOn || !fanOn) Serial.println("🧍 Motion Detected → Light & Fan ON");
      lightOn=fanOn=true;
    } else {
      if(motionDetected && millis()-lastMotionTime>=OFF_DELAY){
        digitalWrite(LIGHT_RELAY,HIGH);
        digitalWrite(FAN_RELAY,HIGH);
        if(lightOn || fanOn) Serial.println("No Motion → Light & Fan OFF");
        lightOn=fanOn=false;
        motionDetected=false;
      }
    }
  }

  // 🎥 Ultrasonic Projector
  long dist=readDistanceCM();
  if(dist>0 && dist<U_ON_DISTANCE){
    projectorLastSeen=millis();
    if(!projectorOn){projectorOn=true;Serial.println("🎥 Projector ON");}
  } else if(projectorOn && millis()-projectorLastSeen>U_OFF_DELAY){
    projectorOn=false;Serial.println("🛑 Projector OFF");
  }
}

/* ---------- MQ2 ---------- */
int readSmoothMQ2(){
  static float ema=0;const float alpha=0.15;long sum=0;
  for(int i=0;i<SMOKE_SMOOTH_SAMPLES;i++){sum+=analogRead(MQ2_AO);delayMicroseconds(500);}
  int avg=sum/SMOKE_SMOOTH_SAMPLES;if(ema==0)ema=avg;
  ema=ema*(1-alpha)+avg*alpha;return(int)ema;
}

/* ---------- CALIBRATION ---------- */
void calibrateMQ2(){
  recalibrating=true;long baseSum=0;int valid=0;
  for(int i=0;i<CALIBRATION_SAMPLES;i++){
    int v=analogRead(MQ2_AO);
    if(v>20&&v<4000){baseSum+=v;valid++;}
    delay(CALIBRATION_DELAY);
  }
  int baseLevel=(valid>0)?(baseSum/valid):350;
  SMOKE_THRESHOLD=(int)(baseLevel*THRESHOLD_FACTOR);
  recalibrating=false;
  Serial.printf("✅ MQ2 Base=%d | Threshold=%d\n",baseLevel,SMOKE_THRESHOLD);
}

/* ---------- ULTRASONIC ---------- */
long readDistanceCM(){
  digitalWrite(TRIG_PIN,LOW);delayMicroseconds(2);
  digitalWrite(TRIG_PIN,HIGH);delayMicroseconds(10);
  digitalWrite(TRIG_PIN,LOW);
  long dur=pulseIn(ECHO_PIN,HIGH,30000);
  if(dur==0)return-1;
  return dur*0.034/2;
}

/* ---------- EMAIL INFO ---------- */
void printNextEmailInfoIfNeeded(){
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo))return;
  int nowMin=timeinfo.tm_hour*60+timeinfo.tm_min;
  int sendMin=SEND_HOUR*60+SEND_MIN;
  if(nowMin<sendMin){
    Serial.println("📨 Email will send automatically at 08:00 AM");
    emailInfoPrintedToday=true;
  }else{
    Serial.println("📨 Email will send automatically tomorrow at 08:00 AM");
    emailInfoPrintedToday=true;
  }
}

/* ---------- WEB ---------- */
void handleWebRequests(WiFiClient &client){
  String req=client.readStringUntil('\r');client.flush();

  if(req.indexOf("GET /schedule")!=-1){sendSchedulePage(client);return;}
  if(req.indexOf("/data")!=-1){sendSensorData(client);return;}
  if(req.indexOf("/recalibrate")!=-1){calibrateMQ2();sendDashboard(client);return;}

  if(req.indexOf("/lightOn")!=-1){digitalWrite(LIGHT_RELAY,LOW);lightOn=true;}
  if(req.indexOf("/lightOff")!=-1){digitalWrite(LIGHT_RELAY,HIGH);lightOn=false;}
  if(req.indexOf("/fanOn")!=-1){digitalWrite(FAN_RELAY,LOW);fanOn=true;}
  if(req.indexOf("/fanOff")!=-1){digitalWrite(FAN_RELAY,HIGH);fanOn=false;}
  if(req.indexOf("/buzzerOn")!=-1){digitalWrite(BUZZER,HIGH);}
  if(req.indexOf("/buzzerOff")!=-1){digitalWrite(BUZZER,LOW);}
  if(req.indexOf("/projectorOn")!=-1){projectorOn=true;}
  if(req.indexOf("/projectorOff")!=-1){projectorOn=false;}

  sendDashboard(client);
}

/* ---------- DASHBOARD ---------- */
void sendDashboard(WiFiClient &c){
  c.println("HTTP/1.1 200 OK\nContent-Type:text/html\n\n<!DOCTYPE html><html><head>");
  c.println("<meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'>");
  c.println("<title>ClassShield v7.1</title>");
  c.println("<style>body{font-family:Segoe UI;background:#0b0c10;color:#fff;text-align:center;margin:0;} .card{background:rgba(255,255,255,0.1);margin:15px;padding:15px;border-radius:15px;} .btn{padding:10px 20px;margin:5px;border:none;border-radius:10px;font-weight:bold;} .on{background:#00ff88;color:#000;} .off{background:#ff3333;color:#fff;}</style>");
  c.println("<script>setInterval(()=>{fetch('/data').then(r=>r.json()).then(d=>{['temp','hum','smoke','light','fan','buzzer','projector','motion'].forEach(k=>{if(document.getElementById(k))document.getElementById(k).innerText=d[k];});});},1000);</script></head><body>");
  c.println("<h1>ClassShield Dashboard</h1>");
  c.println("<div class='card'><h3>🌡️ Temp: <span id='temp'>--</span> °C</h3><h3>💧 Humidity: <span id='hum'>--</span> %</h3><h3>🔥 Smoke: <span id='smoke'>--</span></h3><h3>🧍 Motion: <span id='motion'>--</span></h3></div>");
  c.println("<div class='card'><h3>💡 Light: <span id='light'>--</span></h3><a href='/lightOn'><button class='btn on'>ON</button></a><a href='/lightOff'><button class='btn off'>OFF</button></a></div>");
  c.println("<div class='card'><h3>🌀 Fan: <span id='fan'>--</span></h3><a href='/fanOn'><button class='btn on'>ON</button></a><a href='/fanOff'><button class='btn off'>OFF</button></a></div>");
  c.println("<div class='card'><h3>🔊 Buzzer: <span id='buzzer'>--</span></h3><a href='/buzzerOn'><button class='btn on'>ON</button></a><a href='/buzzerOff'><button class='btn off'>OFF</button></a></div>");
  c.println("<div class='card'><h3>🎥 Projector: <span id='projector'>--</span></h3><a href='/projectorOn'><button class='btn on'>ON</button></a><a href='/projectorOff'><button class='btn off'>OFF</button></a></div>");
  c.println("<div class='card'><a href='/schedule'><button class='btn on'>📅 Class Routine</button></a><a href='/recalibrate'><button class='btn on'>🔄 Recalibrate</button></a></div></body></html>");
}

/* ---------- SCHEDULE PAGE ---------- */
void sendSchedulePage(WiFiClient &c){
  c.println("HTTP/1.1 200 OK\nContent-Type:text/html\n\n<!DOCTYPE html><html><body style='font-family:Segoe UI;color:#fff;background:#001f3f;text-align:center;'>");
  c.println("<h2>Today's Class Routine</h2><table border='1' style='margin:auto;'>");
  c.println("<tr><th>Time</th><th>Subject</th><th>Instructor</th></tr>");
  c.println("<tr><td>09:00</td><td>English</td><td>Mr. Rahman</td></tr>");
  c.println("<tr><td>10:00</td><td>Mathematics</td><td>Ms. Ayesha</td></tr>");
  c.println("<tr><td>11:00</td><td>Physics</td><td>Dr. Karim</td></tr>");
  c.println("<tr><td>12:00</td><td>Lunch</td><td>-</td></tr>");
  c.println("<tr><td>01:00</td><td>ICT</td><td>Mr. Nayan</td></tr>");
  c.println("</table><br><a href='/'><button>⬅️ Back</button></a></body></html>");
}

/* ---------- JSON DATA ---------- */
void sendSensorData(WiFiClient &c){
  int motion=digitalRead(PIR_PIN);
  float temp=dht.readTemperature(), hum=dht.readHumidity();
  int smoke=readSmoothMQ2();
  String j="{";
  j+="\"temp\":"+String(temp)+",\"hum\":"+String(hum)+",\"smoke\":"+String(smoke)+",";
  j+="\"motion\":\""+String(motion==HIGH?"Human Detected":"No Human")+"\",";
  j+="\"light\":\""+String(digitalRead(LIGHT_RELAY)==LOW?"ON":"OFF")+"\",";
  j+="\"fan\":\""+String(digitalRead(FAN_RELAY)==LOW?"ON":"OFF")+"\",";
  j+="\"buzzer\":\""+String(digitalRead(BUZZER)==HIGH?"ON":"OFF")+"\",";
  j+="\"projector\":\""+String(projectorOn?"ON":"OFF")+"\"}";
  c.println("HTTP/1.1 200 OK\nContent-Type:application/json\n\n"+j);
}

/* ---------- EMAIL ---------- */
#if ENABLE_EMAIL
void smtpCallback(SMTP_Status status){Serial.println(status.info());}
void sendScheduleEmail(){
  Serial.println("📧 Sending daily class schedule email...");
  Session_Config cfg;
  cfg.server.host_name="smtp.gmail.com"; cfg.server.port=465;
  cfg.login.email=SMTP_SENDER_EMAIL; cfg.login.password=SMTP_SENDER_APP_PASS;
  cfg.time.ntp_server="pool.ntp.org,time.nist.gov"; cfg.time.gmt_offset=6*3600;
  SMTP_Message msg;
  msg.sender.name="ClassShield"; msg.sender.email=SMTP_SENDER_EMAIL;
  msg.subject="Today's Class Schedule - ClassShield";
  msg.addRecipient("Leon",SMTP_RECIPIENT_EMAIL);
  msg.html.content="<h2>Today's Class Routine</h2><ul><li>09:00 - English</li><li>10:00 - Math</li><li>11:00 - Physics</li><li>12:00 - Lunch</li><li>01:00 - ICT</li></ul>";
  if(!smtp.connect(&cfg)){Serial.println("❌ SMTP connect failed!");return;}
  if(!MailClient.sendMail(&smtp,&msg))Serial.printf("❌ Send fail: %s\n",smtp.errorReason().c_str());
  else Serial.println("✅ Email sent successfully!");
}
#endif