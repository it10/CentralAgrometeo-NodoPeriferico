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

//Parametros de red y direccion IP del servidor donde enviamos los datos:
#define mySSID        "IT10"            //    "labsenales"          "electronica"       //WiFi SSID.
#define myPASS        "coopit10kit"     //    "esunsecreto"         "laboratorio"       //Password.
#define myCHL     "6"                   //numero de canal
#define myECN     "4"                   //ecn: (0 OPEN) (1 WEP) (2 WPA_PSK) (3 WPA2_PSK) (4 WPA_WPA2_PSK)
//AT+CWSAP=mySSID,myPASS,myCHL,myECN

#define serverIP  "184.106.153.149"     //IP de ThingSpeak.com
#define ChannKEY  "66L8D19CBNDFLPDA"    //ThingSpeak channel key.

#define updField  "1"                   //Updating Field( 1,2,3,4,5...) ThingSpeak
#define SPERIOD   30000                 //Periodo de muestreo (milisegundos).
#define MAXFAILS  100                   //Numero de fallas para RESET del ESP8266.

//Concatenamos los string messages:
#define soft_RST  "AT+RST\r\n"
#define mode_MSG  "AT+CWMODE=3\r\n"                                     // (1 Estación), (2 Punto de acceso AP), (3 Ambos)
#define join_MSG  "AT+CWJAP=\"" mySSID "\",\"" myPASS "\"\r\n"          // enviar datos para vincularse al AP
//#define join_MSG  "AT+CWSAP=\"" mySSID "\",\"" myPASS "\",\"" myCHL "\",\"" myECN "\"\r\n" //enviar datos para vincularse al AP AT+CWSAP=mySSID,myPASS,myCHL,myECN
#define conn_MSG  "AT+CIPSTART=\"TCP\",\"" serverIP "\",80\r\n"         // TCP, ThingSpeak, puerto 80.
unsigned long interval=SPERIOD;                                         // El tiempo que necesitamos esperar
unsigned long previousMillis=0;                                         // millis() retorna un unsigned long.

String apiKey = "66L8D19CBNDFLPDA";                                     //key de mi canal en thingspeak


int i,j;
int fails=101;                  // cargar failures>MAXFAILS para generar un RESET al inicio.
String newVal;//="00.0";        // Four characters (fixed format number).
String cmd;
float temp;
String getStr;
int contador = 0;


//########################################################################################################################


void setup() {
  Serial.begin(19200);              //Se inicia la comunicación serial 
  dht.begin();                      //Se inicia el sensor
  ser.begin(19200);                 //Se inicia el puerto serie por sofware
  pinMode(HRESET, OUTPUT);          //Declaro un pin como salida para hacer el hardware reset del ESP
  digitalWrite(HRESET,HIGH);        
  initESP();                        // ESP8266 init.
}


void loop() {
HUMEDAD = dht.readHumidity();                   //Se lee la humedad
TEMPERATURA = dht.readTemperature();            //Se lee la temperatura
if((HUMEDAD == NAN) || (TEMPERATURA == NAN))    //si la lectura falla el modulo devuelve NAN
{
  Serial.print("Reinicio del modulo DHT11");
  dht.begin();                                  //Se inicia el sensor
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

Serial.println("Temperatura: ");
value = EEPROM.read(addr+1);
aux = (int) value;
Serial.print(aux, DEC);
Serial.println();


// Conversion a string de la variable que voy a enviar
  temp = aux;
  char buf[16];
  String strTemp = dtostrf(temp, 4, 1, buf);//  Esta función recibe la variable que queremos convertir (variableFlotante "temp"), el tamaño mínimo de la cadena resultante
                                            //  (tamañoMinimo) incluyendo el punto decimal y el signo negativo en caso necesario, el número de dígitos tras el punto decimal (presicion) 
                                            //  y una cadena con suficiente espacio para guardar. Regresa un apuntador a la cadena Donde Guardar "buf".
  

// Inicia una conexión con un servicio TCP.

  Conexion_TCP();


// Preparo GET string

  getStr = "GET /update?api_key=";
  getStr += apiKey;
  getStr +="&field2=";
  getStr += String(strTemp);
  getStr += "\r\n\r\n";

//envia el largo del string, y luego envia el string a thingspeak

  Enviar_largo_dato();   
  


delay(16000); //Se espera 16 segundos para seguir leyendo //datos
}


//################# funciones #################


//________________________________________________________________________________________________________________________
bool waitChar(char Wchar, int duration)
{
  for(i=duration;i>0;i--)
  {
    
    delay(1000);
    
    //Serial.println(ser.read());
    
    if(ser.read() == Wchar)
     {
        return true;
     }
  }
  return false;
}
//________________________________________________________________________________________________________________________
void sendDebug(String cmd)
{
  Serial.print("SEND: ");
  Serial.println(cmd);
  ser.println(cmd);
  //Serial.print(Serial.read());
  Serial.print(ser.read());
  
}
//________________________________________________________________________________________________________________________
void initESP(){                     //con esta funcion se resetea el modulo, se configura como estación y se conecta a un AP.
  while(1){
    if (fails > MAXFAILS)           //------ reset ESP8266. 
    {
      Serial.println("Hardware RESET");
      digitalWrite(HRESET,LOW);
      
      delay(3000);
      digitalWrite(HRESET,HIGH);
      
      delay(3000);
      
      sendDebug("AT");
      delay(2000);      
      while (!ser.find("OK"))
      {
        Serial.print(".");
        delay(300);
        contador++;
        if(contador >= 10)
        {
          contador=0;
          sendDebug("AT");
        }
      };
      Serial.println("OK");   //es lo que responderia el ESP (lo simulo para verlo en la consola)
      
           
      Serial.println(soft_RST);
      ser.print(soft_RST);
      Serial.println(ser.read());
      if(waitChar('K',10000))
      {
        Serial.println("OK");
        //return;
      } //Esperar por 'K' de "OK". para salir del setup                  Reemplace ok por k
      delay(2000);
      fails=0;
    }


    sendDebug("AT");
    delay(1000);
    if(ser.find("OK"))
    {
      Serial.println("OK");
      Serial.println("Preparando para configurar MODE_MSG y join_MSG");
      delay(1000);
      
      sendDebug("AT+CWMODE=3");
      delay(1000);
      if(ser.find("no change"))
      {
        Serial.println("OK, no change");
      }
      else
      {
        Serial.println("no OK de mode");
      }
      delay(1000);
 
    bool flag = 1; 
    while(flag == 1)
    {
      sendDebug("AT+CWJAP=\"" mySSID "\",\"" myPASS "\"\r\n");
      delay(5000); //con 3000 funciona despues de varios intentos
      if(ser.find("OK"))
      {
        Serial.println("OK");
        flag = 0;
      }
      else
      {
        Serial.println("no OK de join");
      }
      delay(1000);
     }

      sendDebug("AT+CIPMUX=1");
      delay(1000);
      if(ser.find("OK"))
      {
        Serial.println("OK");
        //return;
      }
      else
      {
        Serial.println("no OK de CIPMUX");
      }
      delay(1000);

//-----------------------------------------------------------------------------
      sendDebug("AT");            //luego de configurarlo es necesario un reseteo
      delay(2000);
      while (!ser.find("OK"))
      {
        Serial.print(".");
        delay(300);
      };
      Serial.println("OK");   //es lo que responderia el ESP (lo simulo para verlo en la consola)
      
      Serial.print("SEND: ");     
      Serial.println(soft_RST);
      ser.print(soft_RST);
      Serial.println(ser.read());
      if(waitChar('K',10000))
      {
        Serial.println("OK");
        fails=101; // la proxima vez que envie a configurar el ESP hará un hardware reset
        return;
      } //Esperar por 'K' de "OK". para salir del setup                  Reemplace ok por k
      delay(2000);
//-----------------------------------------------------------------------------      
    }
    fails++;
  }
  }


//________________________________________________________________________________________________________________________
  void Conexion_TCP()
  {
  cmd = "AT+CIPSTART=\"TCP\",\"";              //Es necesario indicar el tipo de conexión (TCP/UDP) la dirección IP y el puerto al que se realiza la conexión
                                                      //con el formato AT+CIPSTART={id},{TCP|UDP},{IP},{puerto} Siendo {id} el número de identificador de la conexión
  cmd += "184.106.153.149"; // api.thingspeak.com
  cmd += "\",80";
  ser.println(cmd);
   
  if(ser.find("Error"))
  {
    Serial.println("AT+CIPSTART error");
    return;
  }

  
  }
//________________________________________________________________________________________________________________________
void Enviar_largo_dato()
{
  cmd = "AT+CIPSEND=";            //Prepara el envío de datos. Cuando esté listo devolverá el código ">" como inductor para el comienzo del envío
  cmd += String(getStr.length());
  Serial.println(cmd);
  ser.println(cmd);

  if(ser.find(">"))
  {
    Serial.print("> ");
    Serial.println(getStr);
    ser.print(getStr);
  }
  else
  {
    ser.println("AT+CIPCLOSE");
    // alert user
    Serial.println("AT+CIPCLOSE");
    //-----------------------------------------------------------------------------
    // reinicio el modulo
   // initESP();
    Serial.println("Se reiniciara el modulo");

      Serial.println("Hardware RESET");
      digitalWrite(HRESET,LOW);
      delay(3000);
      digitalWrite(HRESET,HIGH);
      
      delay(3000);
      sendDebug("AT");
      delay(2000);      
      while (!ser.find("OK"))
      {
        Serial.print(".");
        delay(300);
        contador++;
        if(contador >= 10)
        {
          contador=0;
          sendDebug("AT");
        }
      };
      Serial.println("OK");   //es lo que responderia el ESP (lo simulo para verlo en la consola)
      
           
      Serial.println(soft_RST);
      ser.print(soft_RST);
      Serial.println(ser.read());
      if(waitChar('K',10000))
      {
        Serial.println("OK");
        return;
      }
      delay(2000);
 /*     */
//-----------------------------------------------------------------------------
  }
}
