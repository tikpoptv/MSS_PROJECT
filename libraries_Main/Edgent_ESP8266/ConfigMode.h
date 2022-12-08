#include <TridentTD_LineNotify.h>
#include <FS.h>
#include <ESP8266HTTPClient.h>
#include <BlynkSimpleEsp8266.h>

/* include : time */
#include <NTPClient.h>
#include <WiFiUdp.h>

//sensor DHT
#include <DHT.h>               
#define DHTPIN  D2              
#define DHTTYPE DHT22      
DHT dht(DHTPIN, DHTTYPE);   
WidgetLED LED_DHT22_ONLINE(V7);
WidgetLED LED_DHT22_OFFLINE(V8);
float humn; //humidity
float temp; //temperature

//sensor PIR
int PIR = D1;
int pir_val = 0;
int pir_on = 0;
WidgetLCD LCD(V0); 
WidgetLED LED_PIR(V1); // LED การเคลื่อนไหว
WidgetLED LED_PIR_ONLINE(V3);
WidgetLED LED_PIR_OFFLINE(V4);
float PIR_Read;

//BUZZER
int Buzzer = D8;

//line not
#define TOKENCOUNT  1  //How many token that you want to send? (Size of array below)
#define LINE_TOKEN "wmM1qDFQ8FyVmFHdTKuj2x1bneSsPEpXDV29jB5zuBl"
int send_once = 0;




String IPCAM_IP  =  "172.20.10.11:8888";
bool ipCameraEnabled = true; 

//blynk
#define BLYNK_PRINT Serial
#define BLYNK_AUTH_TOKEN "DyflnmKxcEFHX3YF5YN1orJVrnTB_5EK" //Enter your blynk auth token
char auth[] = BLYNK_AUTH_TOKEN;
char ssid[] = "Deetawee8888_2G";
char pass[] = "88888888";
BlynkTimer timer;

//pin
//int led_alert_DHT = D4;
int led_alert_PIR = D0;



//send notify
bool send_notify = true;

//send notify
bool pir_on_once = true;

//delay
int delay_loop = 1000;

/* time */
const long offsetTime = 25200;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 25200 );
int hourNow, minuteNow, secondNow;

// TIME CONTROL
// DHT //
const int DHT_START_hour = 1;
const int DHT_START_minute = 56;

// PIR //
const int PIR_START_hour_PM = 1;
const int PIR_START_hour_AM = 5;
const int PIR_START_minute = 50;
int Automatic_PIR = 1;

//Buzzer 

int Buzzer_Status = 0;

void setup() { //Start Setup
  
  //Serial port
  Serial.begin(115200);

  //connect internet
  Blynk.begin(auth, ssid, pass, "blynk.cloud", 80);

  //line not
  LINE.setToken(LINE_TOKEN);

  //camera setup
  chk_camera();

  //dht
  dht.begin();
  //timer.setInterval(2500, Sensor);

  //pir
  pinMode(PIR, INPUT);

  //Buzzer 
  pinMode(Buzzer, OUTPUT);
  digitalWrite(Buzzer, LOW);

  //pin setup
 // pinMode(led_alert_DHT, OUTPUT);
 // digitalWrite(led_alert_DHT, LOW);

  pinMode(led_alert_PIR, OUTPUT);
  digitalWrite(led_alert_PIR, HIGH);

  //time
  timeClient.begin();
  timeClient.update();

  //Blynk
  Blynk.virtualWrite(V10, 0);
} //END Setup

void loop() { //Start loop


  
  
  ////////////////////////////////////////////////////////////////
  /* read sensor and start blynk*/
  get_dht();   //read humidity and temperature
  get_pir();   //motion sensor
  Blynk.run(); //start blynk 
  ////////////////////////////////////////////////////////////////

  
  ////////////////////////////////////////////////////////////////
  /* PIR SENSOR */
  if(pir_on){
    digitalWrite(led_alert_PIR, HIGH);
    LED_PIR_OFFLINE.off();
    LED_PIR_ONLINE.on();    
    if(pir_val == 1){ //ถ้ามีความเคลื่อนไหวใหทำอะไรใส่ในนี้
      LED_PIR.on();
      if(Buzzer_Status == 1){
        digitalWrite(Buzzer, HIGH);
        Blynk.virtualWrite(V11, "Get_Buzzer ทำงาน");
      }
      else{
        Blynk.virtualWrite(V11, "Buzzer Mode is Offline");
      }
      sendLineNotify();
      
    }
    else{ //ถ้าไม่มีความเคลื่อนไหวให้ทำอะไร ถ้าไม่ต้องทำอะไรก็ไม่ต้องใส่ หรือลบ else ออกได้เลย
      digitalWrite(Buzzer, LOW);
      LED_PIR.off(); 
    }
  } 
  else {
    digitalWrite(led_alert_PIR, LOW);
    LED_PIR_OFFLINE.on();
    LED_PIR_ONLINE.off();
  }
  ////////////////////////////////////////////////////////////////


  timeClient.update();

  Serial.println(timeClient.getFormattedTime());

  if(timeClient.isTimeSet()) {
    if (DHT_START_hour == timeClient.getHours() && DHT_START_minute <= timeClient.getMinutes()) {
        get_DHT_LineNotify();
    } 
  }


  //////////////////////////////////////////////////////////////////
  if(Automatic_PIR == 1){
    get_Automatic_PIR();
  }
  else {
        if(send_notify == true){
           LINE.notify("อัตโนมัติ \n PIR โดนบังคับปิด ");
          send_notify = false;
        }
        pir_on = 0;
        Blynk.virtualWrite(V2, 0);
        Serial.println("ระบบอัตโนมัติโดนบังคับปิด");
    }
  
 
  
  
  /* delay */ 
  delay(delay_loop);
} //END LOOP

//////////////////////////////////////////////////////////////////
/* function camera download */
void downloadAndSaveFile(String fileName, String  url){
  
  HTTPClient http;

  Serial.println("[HTTP] begin...\n");
  Serial.println(fileName);
  Serial.println(url);
  http.begin(url);
  
  Serial.printf("[HTTP] GET...\n", url.c_str());
  // start connection and send HTTP header
  int httpCode = http.GET();
  if(httpCode > 0) {
      // HTTP header has been send and Server response header has been handled
      Serial.printf("[HTTP] GET... code: %d\n", httpCode);
      Serial.printf("[FILE] open file for writing %d\n", fileName.c_str());
      
      File file = SPIFFS.open(fileName, "w");

      // file found at server
      if(httpCode == HTTP_CODE_OK) {

          // get lenght of document (is -1 when Server sends no Content-Length header)
          int len = http.getSize();

          // create buffer for read
          uint8_t buff[128] = { 0 };

          // get tcp stream
          WiFiClient * stream = http.getStreamPtr();

          // read all data from server
          while(http.connected() && (len > 0 || len == -1)) {
              // get available data size
              size_t size = stream->available();
              if(size) {
                  // read up to 128 byte
                  int c = stream->readBytes(buff, ((size > sizeof(buff)) ? sizeof(buff) : size));
                  // write it to Serial
                  //Serial.write(buff, c);
                  file.write(buff, c);
                  if(len > 0) {
                      len -= c;
                  }
              }
              delay(1);
          }

          Serial.println();
          Serial.println("[HTTP] connection closed or file end.\n");
          Serial.println("[FILE] closing file\n");
          file.close();
          
      }
      
  }
  http.end();
}
//////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////
/* camera */
void listFiles(void) {
  Serial.println();
  Serial.println("SPIFFS files found:");

  Dir dir = SPIFFS.openDir("/"); // Root directory
  String  line = "=====================================";

  Serial.println(line);
  Serial.println("  File name               Size");
  Serial.println(line);

  while (dir.next()) {
    String fileName = dir.fileName();
    Serial.print(fileName);
    int spaces = 25 - fileName.length(); // Tabulate nicely
    while (spaces--) Serial.print(" ");
    File f = dir.openFile("r");
    Serial.print(f.size()); Serial.println(" bytes");
  }

  Serial.println(line);
  Serial.println();
  delay(1000);
}
//////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////
void chk_camera(){
  if(ipCameraEnabled){
    //Initialize File System
    if(SPIFFS.begin()){
      Serial.println("SPIFFS Initialize....ok");
    }else{
      Serial.println("SPIFFS Initialization...failed");
    }
   
    //Format File System
    if(SPIFFS.format()){
      Serial.println("File System Formated");
    }else{
      Serial.println("File System Formatting Error");
    }
  }
}
//func send image
void sendLineNotify(){
  if(ipCameraEnabled){
     downloadAndSaveFile("/snapshot.jpg","http://"+ IPCAM_IP +"/out.jpg");
     Serial.println("Send message");
      LINE.notifyPicture("\n [อัตโนมัติ] \n ตรวจพบการเคลื่อนไหว \n \n ห้องหมายเลข : 18054  \n เวลา : "+String(timeClient.getFormattedTime())+" ", SPIFFS, "/snapshot.jpg");  
  }
}
//////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////
/* function DHT */
void get_dht(){
  //read dht
  humn = dht.readHumidity();
  temp = dht.readTemperature();
  //chk dht
  if(isnan(humn) || isnan(temp)){
    Serial.println("DHT ERROR");
   // digitalWrite(led_alert_DHT, LOW);
    LED_DHT22_ONLINE.off();
    LED_DHT22_OFFLINE.on();
    humn = 0;
    temp = 0;
    //delay(1500);
    //return;
  }
  else{
    Serial.print("HUM : ");
    Serial.print(humn);
    Serial.println(" %");
    Serial.println("");
    Serial.print("Temp : ");
    Serial.print(temp);
    Serial.println(" C");

   // digitalWrite(led_alert_DHT, HIGH);
    LED_DHT22_ONLINE.on();
    LED_DHT22_OFFLINE.off();
    
  }
  Blynk.virtualWrite(V5, humn);
  Blynk.virtualWrite(V6, temp);
}
//////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////
//button pir
BLYNK_WRITE(V2){
  if(param.asInt()){
      pir_on = 1;
      
  }
  else{
      pir_on = 0;
  }
}
//////////////////////////////////////////////////////////////////
BLYNK_WRITE(V10){
  if(param.asInt()){
    Blynk.virtualWrite(V11, "ระบบอัตโนมัติโดนบังคับปิด");
    Serial.println("ระบบอัตโนมัติโดนบังคับปิด");
    Automatic_PIR = 0;
    
    //LINE.notify("test");
  }
  else{
     Automatic_PIR = 1;
     Blynk.virtualWrite(V11, "ระบบอัตโนมัติทำงานโดยอัตโนมัติ");
     Serial.println("ระบบอัตโนมัติทำงานโดยอัตโนมัติ");
     send_notify = true;
  }
}


BLYNK_WRITE(V9){
  if(param.asInt()){
    Serial.println("BUZZER TEST!!!!");
    digitalWrite(Buzzer, HIGH);
  }
  else{
    digitalWrite(Buzzer, LOW);
    
  }
}

BLYNK_WRITE(V12){
  if(param.asInt()){
    Buzzer_Status = 1;
  }
  else{
    Buzzer_Status = 0;
  }
}

//////////////////////////////////////////////////////////////////
/* pir senesor */
void get_pir(){

  PIR_Read = digitalRead(PIR);
  if(isnan(PIR_Read)){
    Serial.println("PIR ERROR");
    Blynk.virtualWrite(V2, 0);
    pir_val = 0;
  }
  else{
    pir_val = digitalRead(PIR);
  }
    
}
//////////////////////////////////////////////////////////////////

void get_DHT_LineNotify(){
  if(temp < 30){ //ถ้าอุณหภูมิมากกว่า 26 ให้ทำอะไร
    send_notify = true;
    LINE.notify("อัตโนมัติ \n มีโอกาสที่ห้องไม่ได้ปิดแอร์ \n อุณหภูมิตอนนี้ :  "+String(temp)+" C \n เวลา : "+String(timeClient.getFormattedTime())+"");
    delay(300000);
  }
  else{
    if(send_notify == true){
      LINE.notify("อัตโนมัติ \n ห้องอุณหภูมิปกติ \n อุณหภูมิตอนนี้ : "+String(temp)+" C \n เวลา : "+String(timeClient.getFormattedTime())+"");
      send_notify = false;
    }
  }
}

void get_Automatic_PIR(){
  Serial.println("get_Automatic_PIR_ONLINE");
  if(timeClient.isTimeSet()) {
    if (PIR_START_hour_PM <= timeClient.getHours() && PIR_START_minute == timeClient.getMinutes()) {
        if(send_notify == true){
          LINE.notify("อัตโนมัติ \n PIR กำลังทำงานโดยระบบอัตโนมัติ ");
          send_notify = false;
        }
        Blynk.virtualWrite(V2, 0);
        pir_on = 1;
        pir_on_once = true;

    } 
    else if(PIR_START_hour_AM >= timeClient.getHours() && PIR_START_minute == timeClient.getMinutes()) {

        if(send_notify == true){
          LINE.notify("อัตโนมัติ \n PIR กำลังทำงานโดยระบบอัตโนมัติ ");
          send_notify = false;
        }
        Blynk.virtualWrite(V11, "ระบบอัตโนมัติ 00:01 - 05:30 ทำงาน");
        Blynk.virtualWrite(V2, 0);
        pir_on = 1;
        pir_on_once = true;

    }
    else {
        if(pir_on_once == true){
          pir_on = 0;
          pir_on_once = false;
        }
       
    }
  }

  //(DHT_START_hour == timeClient.getHours() && DHT_START_minute <= timeClient.getMinutes())

}