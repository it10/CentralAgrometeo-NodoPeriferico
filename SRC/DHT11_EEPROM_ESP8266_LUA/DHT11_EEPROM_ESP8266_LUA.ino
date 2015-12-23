#include "DHT.h"              //cargamos la librería DHT
#define DHTPIN 2              //Seleccionamos el pin en el que se //conectará el sensor
#define DHTTYPE DHT11         //Se selecciona el DHT11 (hay //otros DHT)
DHT dht(DHTPIN, DHTTYPE);     //Se inicia una variable que será usada por Arduino para comunicarse con el sensor

#include <SoftwareSerial.h>
#include <stdlib.h>

SoftwareSerial ser(10, 11);
//asignacion de pines:
// RXDATA    10               // SoftwareSerial pin de recepción.
// TXDATA    11               // SoftwareSerial pin de transmisión.
#define HRESET    8           // Pin para hardware Reset del módulo ESP8266

#include <EEPROM.h>
int addr = 0;


volatile float HUMEDAD;
volatile float TEMPERATURA ;
volatile byte val;
volatile byte value;
int aux =0;
float temp;
float hum;
String stringTemp;

void setup() {
  Serial.begin(9600);              //Se inicia la comunicación serial 
  dht.begin();                      //Se inicia el sensor
  ser.begin(9600);                 //Se inicia el puerto serie por sofware
  pinMode(HRESET, OUTPUT);          //Declaro un pin como salida para hacer el hardware reset del ESP
  digitalWrite(HRESET,LOW); 
  delay(300);
  digitalWrite(HRESET,HIGH);
  delay(500);
}

void loop() {
HUMEDAD = dht.readHumidity();                   //Se lee la humedad
TEMPERATURA = dht.readTemperature();            //Se lee la temperatura
if((HUMEDAD == NAN) || (TEMPERATURA == NAN))    //si la lectura falla el modulo devuelve NAN
{
  Serial.println("Reinicio del modulo DHT11");
  dht.begin();                                  //Se inicia el sensor
  HUMEDAD = dht.readHumidity();                 //Se lee la humedad
  TEMPERATURA = dht.readTemperature();          //Se lee la temperatura
}
while((HUMEDAD <= 1) || (TEMPERATURA <= 1))    //si la lectura falla el modulo devuelve NAN
{
  Serial.println("Reinicio del modulo DHT11");
  dht.begin();                                  //Se inicia el sensor
  delay(300);
  HUMEDAD = dht.readHumidity();                 //Se lee la humedad
  TEMPERATURA = dht.readTemperature();          //Se lee la temperatura
}
val = (byte) HUMEDAD;                           //casteo para poder escribir en EEPROM
EEPROM.write(addr, val);                        //escribe en la direccion addr el entero val (t casteado a int)

val = (byte) TEMPERATURA;
EEPROM.write(addr+1, val);                      //escribe en la direccion addr el entero val (h casteado a int)

//Se imprimen las variables (previamente guardadas en la EEPROM)
Serial.println("Humedad: "); 
value = EEPROM.read(addr);
aux = (int) value;
Serial.print(aux, DEC);
Serial.println();
hum = (int) value;

Serial.println("Temperatura: ");
value = EEPROM.read(addr+1);
aux = (int) value;
Serial.print(aux, DEC);
Serial.println();


// Conversion a string de la variable que voy a enviar

  temp = aux;
  char buf[16];
  String strTemp = dtostrf(temp, 3, 0, buf);//  Esta función recibe la variable que queremos convertir (variableFlotante "temp"), el tamaño mínimo de la cadena resultante
                                            //  (tamañoMinimo) incluyendo el punto decimal y el signo negativo en caso necesario, el número de dígitos tras el punto decimal (presicion) 
                                            //  y una cadena con suficiente espacio para guardar. Regresa un apuntador a la cadena Donde Guardar "buf".
if(temp < 100)
{
String strTemp = dtostrf(temp, 2, 0, buf);
 stringTemp = "str = \"tmp0" +strTemp + "\"";
}
else
{
  String strTemp = dtostrf(temp, 3, 0, buf);
  stringTemp = "str = \"tmp";
  stringTemp += strTemp;
  stringTemp += "\"";
}

String stringHum;
if( hum < 100)
{
String strHum = dtostrf(hum, 2, 0, buf);
stringHum = "str = \"hum0" +strHum + "\"";
}
else
{
String strHum = dtostrf(hum, 3, 0, buf);
stringHum = "str = \"hum" +strHum + "\"";
}


//    -- en arduino se deberá enviar por puerto serie:
//    -- 1) conn=net.createConnection(net.TCP, 0)
//    -- 2) conn:connect(8888, "192.168.4.1")
//    -- 3) str = "dato que se quiere enviar"
//    -- 4) conn:send("#" ..str.. "\r\n")
ser.println("conn=net.createConnection(net.TCP, 0)");
delay(500);
ser.println("conn:connect(8888, \"192.168.4.1\")");
delay(500);
Serial.println(stringTemp);
ser.println(stringTemp);
delay(500);
ser.println("conn:send(\"#\" ..str)");

delay(17000); //thingspeak recibe datos con una tasa de refresco de 15 seg

ser.println("conn=net.createConnection(net.TCP, 0)");
delay(500);
ser.println("conn:connect(8888, \"192.168.4.1\")");
delay(500);
Serial.println(stringHum);
ser.println(stringHum);
delay(500);
ser.println("conn:send(\"#\" ..str)");
/*if (ser.available()) { //si recibo algo por soft serial lo envio por el puerto serie a la PC
  //int inByte = ser.read();
  //Serial.write(inByte);
  Serial.print(ser.read());
}*/
delay(17000); //thingspeak recibe datos con una tasa de refresco de 15 seg
}
