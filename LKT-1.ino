//Se cambia el sketch debido al daño de su modulo GSM se cambia activacion del dispositivo por medio de la lectura de su 
//entrada de potencia eléctrica, se hace comunicacion por puerto de hardware serial con libreria ArduinoJSON 5.0
#include <ArduinoJson.h>
#include <LGPS.h>
#include <LBattery.h>

const String id = "001";

//Led Indicador de estado
const int ledVerde=13;

//Variables para testear encendido dispositivo
boolean state;
int sat_num;//Rastrea el numero de satelites para validar geolocalización
gpsSentenceInfoStruct info;
double latitud=0.0;
double longitud=0.0;

//Definicion de objeto JSON
const int capacity = JSON_OBJECT_SIZE(10);
StaticJsonBuffer<capacity> jb;
JsonObject& obj = jb.createObject();

void setup() {
 //se activa transmision Serial por el puerto fisico serial (D0, D1)
 Serial1.begin(9600);
 pinMode(ledVerde,OUTPUT);
 state=false;
 LGPS.powerOn();
 delay(1000);
 digitalWrite(ledVerde,LOW);
}

void loop() {
   getData(&info);//Toma datos del gps
   //Lectura de Bateria interna
   Serial.print("Nivel de bateria interna-> "); Serial.println(LBattery.level());
   Serial.print("La bateria esta cargando-> ");Serial.println(LBattery.isCharging());

   state = LBattery.isCharging()? true: false; 
   //El dispositivo se encuentra encendido
   obj["lat"] = latitud;
   obj["lon"] = longitud;
   obj["bat"] =  LBattery.level();
   obj["id"] = id;
   obj["estado"] = state;
   obj.printTo(Serial1);
   blink(ledVerde,10);
   delay(1000);
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++ ZONA DE DEFINICION DE FUNCIONES +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//Función para parpadear los leds
void blink(unsigned int ledPin, int n)//n-> nuemro de veces que repite parpadeo
{
   for(int i=0;i<=n;i++){
    digitalWrite(ledPin,HIGH);
    delay(75);
    digitalWrite(ledPin,LOW);
    delay(75);
   }
}

//Funcion para obtener y procesar estructura de datos de geolocalización
void getData(gpsSentenceInfoStruct* info)
{
  LGPS.getData(info);
  String str = (char*)(info->GPGGA);; //se captura toda la trama que devuelve el GPS
  Serial.println(str);
  for(int i=0;i<2;i++){
    str = str.substring(str.indexOf(',')+1);//Se corta la cadena dos veces en pos donde esta ',' para llegar a latitud
   }
  //Se convierte la trama de la latitud a un double 
  latitud = convert(str.substring(0, str.indexOf(',')), str.charAt(str.indexOf(',') + 1) == 'S'); 
  for(int i=0;i<2;i++){
    str = str.substring(str.indexOf(',')+1);//Se corta la cadena dos veces en posicion donde esta ',' para llegar a longitud
   }
  //Se convierte la longitud de string a formato tipo double 
  longitud=convert(str.substring(0, str.indexOf(',')), str.charAt(str.indexOf(',') + 1) == 'W');
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++  
  //Se obtiene el numero de satelites conectados
   for(int i=0;i<3;i++){
    str = str.substring(str.indexOf(',')+1);//Se corta la cadena 3 veces en pos donde esta ',' para llegar al numero se satélites
   }
  sat_num=str.substring(0,str.indexOf(',')).toInt();

 }
 
//Convierte formato string de la estructua info a un flotante para ser procesado con GOOGLE MAPS
//pasa el formato de ddmm.mmmm a dd.mmmm ademas agrega el signo de direccon N-S. E-O
float convert(String str, boolean dir)
{
  double mm, dd;
  int point = str.indexOf('.');
  dd = str.substring(0, (point - 2)).toFloat();
  mm = str.substring(point - 2).toFloat() / 60.00;
  return (dir ? -1 : 1) * (dd + mm);
} 

//Funcion para escalar el pin Analogico A0 a valor de voltaje
float fmap(float x,float in_min, float in_max, float out_min, float out_max){
  return (x-in_min)*(out_max - out_min) / (in_max -in_min)+ out_min;
}
