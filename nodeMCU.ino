/*
   Sketch se desarrolla para sensar las redes inalambricas, se hace uso de pantalla OLED I2C
*/

#include "ESP8266WiFi.h"
#include <NTPClient.h>
#include <WiFiUdp.h>
#include "ESP8266Ping.h"
#include <Wire.h>  // Only needed for Arduino 1.6.5 and earlier
#include "SSD1306Wire.h" // legacy include: `#include "SSD1306.h"`
#include "credentials.h"
#include<SoftwareSerial.h> 
#include <DHT11.h>
#include "FirebaseESP8266.h"
#include <ArduinoJson.h>

//OLED initialization
// Initialize the OLED display using Wire library
SSD1306Wire  display(0x3c, D2, D1);  //D2=SDK  D1=SCK  As per labeling on NodeMCU

//Initialize Software serial object
SoftwareSerial s(3,1);
String data;

//JSON to receive data from Linkit One
 //********************** JASON CONSTANTS ***************************************
 const int capacity = JSON_OBJECT_SIZE(30);             //
 StaticJsonDocument<capacity> doc;                                            //
 //******************************************************************************

/*device id*/
byte mac[6];                    //
String MAC_Adress;              //
//---------------------------------
int pingTime;
int rssi; //WiFi power in dbm
const char* remoteHost="www.google.com";

//DHT sensor definition
int pin = 2;
DHT11 dht11(pin);
float hum =0; //humidity
float temp = 0;

//Linkit One variables
float lat = 0;
float lon = 0;
int bat = 0;
boolean state = false;
const int ID = 1;
const String KEY = "-M6HNlBA9OvTtHQd_66y";

FirebaseData firebaseData;

//Network Time Protocol definition
 const long utcOffsetSeconds = 3600*(-5);//Colombian time
 WiFiUDP ntpUDP;
 NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetSeconds);
 unsigned long epochTime;


//******************************************** SETUP **************************************
void setup() {
  s.begin(9600);
  display.init();
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);

  //********************** DEVICE COMUNICATION CONSTANTS DEFINITION ***********************
  const char ssid[] = WIFI_SSID;                                                         //
  const char pass[] = WIFI_PASS;                                                         //
  const char fire_host[] = FIREBASE_HOST;                                                //
  const char fire_auth[] = FIREBASE_AUTH;                                                //  
  //***************************************************************************************


 //NTP initialization
 timeClient.begin();
 
  MAC_Adress=obtenerMac();
  WiFi.begin(ssid,pass);
  display.clear();
  drawFontScreen("Conectando...", 0, 0);//(String, xpos,ypos)
  pinMode(16, OUTPUT);
  while(WiFi.status() != WL_CONNECTED){
    blink(7);   
    delay(1000);
  }
  drawFontScreen("Conectado, IP->", 0, 10);
  String ipStr = ip2Str(WiFi.localIP());
  drawFontScreen(ipStr, 0, 20);
  drawFontScreen("MAC_ADRESS: ", 0, 30);
  drawFontScreen(MAC_Adress,0,40);  
  delay(5000);
  Firebase.begin(fire_host, fire_auth);
} 

//********************************************** MAIN PROGRAM **********************************
void loop() {
   
  display.clear();
  scanSerial();//Se recibe el JSON desde Linkit One 
  int str_len = data.length() + 1;
  char  input[str_len];
  data.toCharArray(input,str_len);
  DeserializationError err = deserializeJson(doc, data);
  if(err) {
    display.clear();
    drawFontScreen("Fallo en desearilzacion JSON", 0, 0);
    drawFontScreen(err.c_str(), 0, 10);  
  } else {
    lat = doc["lat"];
    lon = doc["lon"];
    bat = doc["bat"];
    state = doc["estado"];
  }
  
  display.clear();
  drawFontScreen("id: "+ID, 0, 0);
  drawFontScreen("Estado: "+(String)state, 0, 10);
  drawFontScreen("Batery: "+(String)bat+" %", 0, 20);
  drawFontScreen("Latitude: "+(String)lat, 0, 30);
  drawFontScreen("Longitude: "+(String)lon, 0, 40);
 
   
  //Read DHT11 sensor
  delay(2000);
  testDHT11();  
  delay(2000);
  display.clear();
  drawFontScreen("Potencia de señal", 0, 0);
  //Latencia de la red
  rssi = WiFi.RSSI()+10;
  drawFontScreen(WiFi.SSID(), 0, 10);drawFontScreen("Potencia-> ", 0, 20);drawFontScreen((String)rssi+"_dbm", 55, 20);
  Ping.ping(remoteHost,10);
  pingTime = Ping.averageTime();
  drawFontScreen("Ping: ", 0, 30);drawFontScreen((String)pingTime+"_ms", 30, 30);
  
  //rastreo de redes existentes
  rastreaWiFi();
  //reconnect wifi if the network crashes
  Firebase.reconnectWiFi(true);

  //Envio de datoa a firebase
  //ID inside ID tag
  uploadInt("id", ID);
  delay(2000);
  //Latitud
  uploadFloat("lat", lat);
  delay(2000);
  uploadFloat("lon", lon);
  delay(2000);
  uploadFloat("hum", hum);
  delay(2000);
  uploadFloat("temp", temp);
  delay(2000);
  uploadInt("pot", rssi);
  delay(2000);
  uploadInt("ping", pingTime);
  delay(2000);
   uploadInt("bat", bat);
  delay(2000);
   uploadInt("state", state); 
  delay(2000);
  timeClient.update();
  epochTime = timeClient.getEpochTime();
  uploadInt("time", epochTime); 
}

//**************************************************** FUNCTION DEFINITION ZONE ****************************************************************

void uploadFloat(String tag, float data) {
  display.clear();
  if(Firebase.setFloat(firebaseData,"stations/"+KEY + "/"+tag, data))
  {
    //Success
     drawFontScreen("Se envia a firebase",0 , 0);
     drawFontScreen(tag,0 , 10);

  }else{
    //Failed?, get the error reason from firebaseData
    drawFontScreen("Fallo en envio de datos", 0, 0);
    //Serial.println(firebaseData.errorReason());
  }
  delay(1000);
}
void uploadInt(String tag, int data) {
  display.clear();
  if(Firebase.setInt(firebaseData,"stations/"+KEY + "/"+tag, data))
  {
    //Success
     drawFontScreen("Se envia a firebase",0 , 0);
     drawFontScreen(tag,0 , 10);

  }else{
    //Failed?, get the error reason from firebaseData
    drawFontScreen("Fallo en envio de datos", 0, 0);
    drawFontScreen(tag,0 , 10);
    //Serial.println(firebaseData.errorReason());
  }
  delay(1000);
}
//Firebse funtion to ulpload string value
void uploadString(String tag, String data) {
  display.clear();
  if(Firebase.setString(firebaseData,"stations/"+KEY + "/"+tag, data))
  {
    //Success
     drawFontScreen("Se envia a firebase",0 , 0);
     drawFontScreen(tag,0 , 10);

  }else{
    //Failed?, get the error reason from firebaseData
    drawFontScreen("Fallo en envio de datos", 0, 0);
    drawFontScreen(tag,0 , 10);
    //Serial.println(firebaseData.errorReason());
  }
  delay(1000);
}

void drawFontScreen(String datos, int x, int y) {
  // clear display before draw data stream;
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.setFont(ArialMT_Plain_10);
    display.drawString(x, y, datos);
    // write the buffer to the display
    display.display();
    delay(500);
 }

void rastreaWiFi(){
  // Scan WiFi networks
  int n = WiFi.scanNetworks();
  display.clear();
  drawFontScreen("Escaneo realizado...", 0, 0);
  if (n == 0) {
    drawFontScreen("No se encuentran redes", 0, 10);
  } else {
    drawFontScreen((String)n+" Redes encontradas", 0, 10);
    delay(2000);
    display.clear();
    int ypos = 0;  
    for (int i = 0; i < n; ++i) {
      // Se imprime los SSID y la potencia RSSI de cada potencia
      ;
      drawFontScreen((String)(i + 1), 0, ypos);
      drawFontScreen(": ", 10, ypos);
      drawFontScreen((String)WiFi.SSID(i), 20, ypos);
      //Serial.print(WiFi.RSSI(i)); este dato se debe enviar a firebase
      ypos+=10;
      delay(100);
    }
  }
  blink(6);
  //TODO: Implement JSON parser and send to MQTT broker
}

void blink(int repeat){
  for(int i=0;i<=repeat;i++){
    digitalWrite(16, HIGH);
    delay(350);
    digitalWrite(16, LOW);
    delay(350);
  }
}

/**********Funcion para obtener la mac del dispositivo**********************/
String obtenerMac()
{
  // Obtenemos la MAC del dispositivo
  WiFi.macAddress(mac);
 
  // Convertimos la MAC a String
  String keyMac = "";
  for (int i = 0; i < 6; i++)
  {
    String pos = String((uint8_t)mac[i], HEX);
    if (mac[i] <= 0xF)
      pos = "0" + pos;
    pos.toUpperCase();
    keyMac += pos;
    if (i < 5)
      keyMac += ":";
  }
 
  // Devolvemos la MAC en String
  return keyMac;
}

/********************Funcion para parsear la ip a string **********************/
String ip2Str(IPAddress ip){
  String s="";
  for (int i=0; i<4; i++) {
    s += i  ? ". " + String(ip[i]) : String(ip[i]);
  }
  return s;
}

/******************* Funcion para escanear puerto serial ************************/
void scanSerial() {
  do {
    drawFontScreen("Esperando com. Serial", 0, 0);
    data = s.readStringUntil('\n');
  }while(s.available());
  drawFontScreen("Llego com. serial", 0, 10);
  drawFontScreen(data, 0, 20);
  delay(5000);
}

/***************** Funcipn para teetear Humedad y Temperatura DHT11*****************/
void testDHT11(){
  display.clear();
  if(dht11.read(hum, temp) == 0) {
    drawFontScreen("temperatura= ",0 , 0);
    drawFontScreen((String)temp,65 , 0);
    drawFontScreen("_°C",95 , 0);
    drawFontScreen("Humedad= ",0 , 10);
    drawFontScreen((String)hum,55 , 10);
     drawFontScreen("_%",85 , 10);
   }
   else {
    drawFontScreen("Error al leer DHT11 ",0 , 0);
   }
} 
