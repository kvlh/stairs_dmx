
// - - - - -
// ESPDMX - A Arduino library for sending and receiving DMX using the builtin serial hardware port.
//
// Copyright (C) 2015  Rick <ricardogg95@gmail.com>
// This work is licensed under a GNU style license.
//
// Last change: Musti <https://github.com/IRNAS> (edited by Musti)
//
// Documentation and samples are available at https://github.com/Rickgg/ESP-Dmx
// Connect GPIO02 - TDX1 to MAX3485 or other driver chip to interface devices
// Pin is defined in library
// - - - - -
//OTA
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Update.h>

const char* host = "esp32";
const char* ssid = "NES";
const char* password = "segavita90";
unsigned long time_now = 0;
WebServer server(80);

/*
 * Login page
 */

const char* loginIndex =
 "<form name='loginForm'>"
    "<table width='20%' bgcolor='A09F9F' align='center'>"
        "<tr>"
            "<td colspan=2>"
                "<center><font size=4><b>ESP32 Login Page</b></font></center>"
                "<br>"
            "</td>"
            "<br>"
            "<br>"
        "</tr>"
        "<tr>"
             "<td>Username:</td>"
             "<td><input type='text' size=25 name='userid'><br></td>"
        "</tr>"
        "<br>"
        "<br>"
        "<tr>"
            "<td>Password:</td>"
            "<td><input type='Password' size=25 name='pwd'><br></td>"
            "<br>"
            "<br>"
        "</tr>"
        "<tr>"
            "<td><input type='submit' onclick='check(this.form)' value='Login'></td>"
        "</tr>"
    "</table>"
"</form>"
"<script>"
    "function check(form)"
    "{"
    "if(form.userid.value=='admin' && form.pwd.value=='admin')"
    "{"
    "window.open('/serverIndex')"
    "}"
    "else"
    "{"
    " alert('Error Password or Username')/*displays error message*/"
    "}"
    "}"
"</script>";

/*
 * Server Index Page
 */

const char* serverIndex =
"<script src='https://ajax.googleapis.com/ajax/libs/jquery/3.2.1/jquery.min.js'></script>"
"<form method='POST' action='#' enctype='multipart/form-data' id='upload_form'>"
   "<input type='file' name='update'>"
        "<input type='submit' value='Update'>"
    "</form>"
 "<div id='prg'>progress: 0%</div>"
 "<script>"
  "$('form').submit(function(e){"
  "e.preventDefault();"
  "var form = $('#upload_form')[0];"
  "var data = new FormData(form);"
  " $.ajax({"
  "url: '/update',"
  "type: 'POST',"
  "data: data,"
  "contentType: false,"
  "processData:false,"
  "xhr: function() {"
  "var xhr = new window.XMLHttpRequest();"
  "xhr.upload.addEventListener('progress', function(evt) {"
  "if (evt.lengthComputable) {"
  "var per = evt.loaded / evt.total;"
  "$('#prg').html('progress: ' + Math.round(per*100) + '%');"
  "}"
  "}, false);"
  "return xhr;"
  "},"
  "success:function(d, s) {"
  "console.log('success!')"
 "},"
 "error: function (a, b, c) {"
 "}"
 "});"
 "});"
 "</script>";

/*
 * setup function
 */
/////////////////////////////////////////////////////////////////////////////////////////////////
// schody
#include "ESPDMX.h"
#define ECHO1 13
#define TRIG1 12
#define ECHO2 26
#define TRIG2 25
#define ECHO3 33
#define TRIG3 32
#define ECHO4 21
#define TRIG4 19
#define ECHO5 23
#define TRIG5 22
DMXESPSerial dmx;

int Delay=0;
int max_distance=80;
int max_distance_d1=90;
int max_distance_d3=55;
int delay_between=20000;
int max_bright=120;

void FC (int x, int y);
void FC_double(int S1, int E1,int S2,int E2);
int ultr(int echo, int trig);

void setup() {
  Serial.begin(115200);
  pinMode(TRIG1, OUTPUT); //OUTPUT
  pinMode(ECHO1, INPUT); 
  pinMode(TRIG2, OUTPUT); 
  pinMode(ECHO2, INPUT); 
  pinMode(TRIG3, OUTPUT); 
  pinMode(ECHO3, INPUT); 
  pinMode(TRIG4, OUTPUT); 
  pinMode(ECHO4, INPUT); 
  pinMode(TRIG5, OUTPUT); 
  pinMode(ECHO5, INPUT); 

  Serial.println("starting...");

  dmx.init(24, 4);           // initialization for complete bus

  Serial.println("initialized...");
  
  for (int i = 0; i < 24; i++)
    {
      dmx.write(i, 0);
    }
  dmx.update();  

  //OTA/////////////
  //////////////////////////////////////////////////////////////////////////////////////////////////
  // Connect to WiFi network
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay2(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  /*use mdns for host name resolution*/
  if (!MDNS.begin(host)) { //http://esp32.local
    Serial.println("Error setting up MDNS responder!");
    while (1) {
      delay2(1000);
    }
  }
  Serial.println("mDNS responder started");
  /*return index page which is stored in serverIndex */
  server.on("/", HTTP_GET, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", loginIndex);
  });
  server.on("/serverIndex", HTTP_GET, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", serverIndex);
  });
  /*handling uploading firmware file */
  server.on("/update", HTTP_POST, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
    ESP.restart();
  }, []() {
    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
      Serial.printf("Update: %s\n", upload.filename.c_str());
      if (!Update.begin(UPDATE_SIZE_UNKNOWN)) { //start with max available size
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      /* flashing firmware to ESP*/
      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_END) {
      if (Update.end(true)) { //true to set the size to the current progress
        Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
      } else {
        Update.printError(Serial);
      }
    }
  });
  server.begin();
  dmx.write(1, 254);
  dmx.update();
  delay2(200);
  dmx.write(1, 0);
  dmx.update();
  delay2(200);
  dmx.write(1, 254);
  dmx.update();
  delay2(200);
  dmx.write(1, 0);
  dmx.update();
}


void loop() {
  ///OTA///////////////////
  ///////////////////////////////////////////////////////////////////////////////////////
  server.handleClient();
  //other
  int d1=ultr(ECHO1,TRIG1);
  int d2=ultr(ECHO2,TRIG2);
  int d3=ultr(ECHO3,TRIG3);
//  int d4=ultr(ECHO5,TRIG5);
  if(d1<=max_distance_d1){
    Serial.print("D1: ");
    Serial.println(d1);  
    FC(1,17);
  }
  if(d2<=max_distance){
    Serial.print("D2: ");
    Serial.println(d2);  
    FC_double(13,17,12,1); //Start1Up_shorter,End1Up,Start2Down_longer,End2Down
  }  
  if(d3<=max_distance_d3){
    Serial.print("D3: ");
    Serial.println(d3);  
    FC(17,1);
  }



}


void delay2(int x){
  time_now = millis();
  while(millis() < time_now + x){
  }
}


void FC (int x, int y){ //Start,End
  if(x<y) {
    for (int i=x; i<=y;i++)
    {
      for (int v=0; v<=max_bright; v++)
      {
        dmx.write(i, v);
        dmx.update();
        //delay2(Delay);
      }
    }
    delay2(delay_between);
    for (int i=x; i<=y;i++)
    {
      for (int v=max_bright; v>=0; v--)
      {
        dmx.write(i, v);
        dmx.update();
        //delay2(Delay);
      }
    }
  }
  else if (x>y){
    for (int i=x; i>=y;i--)
    {
      for (int v=0; v<=max_bright; v++)
      {
        dmx.write(i, v);
        dmx.update();
        //delay2(Delay);
      }
    }
    delay2(delay_between);
    for (int i=x; i>=y;i--)
    {
      for (int v=max_bright; v>=0; v--)
      {
        dmx.write(i, v);
        dmx.update();
        //delay2(Delay);
      }
    }
  }
}


void FC_double (int S1, int E1,int S2,int E2)  //Start1Up_shorter,End1Up,Start2Down_longer,End2Down
{ 
  if(S1<=E1 && S2>=E2 ){
    int j=S1;
    for (int i=S2; i>=E2;i--)
    {
      for (int v=0; v<=max_bright; v++)
      {
        dmx.write(i, v);
        if(j!=0) dmx.write(j,v);
        dmx.update();
        //delay2(Delay);
      }
      if(j<E1 && j!=0) j++;
      else j=0;
    }
    delay2(delay_between);
    j=S1;
    for (int i=S2; i>=E2;i--)
    {
      for (int v=max_bright; v>=0; v--)
      {
        dmx.write(i, v);
        if(j!=0) dmx.write(j,v);
        dmx.update();
        //delay2(Delay);
      }
      if(j<E1 && j!=0) j++;
      else j=0;
    }
  }
  else {
    Serial.print("error schody sie nie zgadzaja");
  }
}


int ultr(int echo, int trig){
  long duration; 
  int distance;
  digitalWrite(trig, LOW);
  delayMicroseconds(2);
  // Sets the trigPin HIGH (ACTIVE) for 10 microseconds
  digitalWrite(trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig, LOW);
  // Reads the echoPin, returns the sound wave travel time in microseconds
  duration = pulseIn(echo, HIGH);
  // Calculating the distance
  distance = duration * 0.034 / 2; // Speed of sound wave divided by 2 (go and back)
  // Displays the distance on the Serial Monitor
  return distance;
}
