// Se desactiva el envio de estado de ambas bombas al servidor MQTT durante la reconeccion al servidor.
// Se agrega señal digital de producción de planta 3.
/*Se corrige error en  envio de señal de tanque lleno, y se agrega funcion que todos los dias a las 00:00
envia el estado de todas las señales*/ 

#include "OTA.h"
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#define mySSID "BIOENER_SMANDO"
#define myPASSWORD "Bioener2013"


//-----------------------Dominio del Broker MQTT, nombre de usuario, contraseña e ID------------
const char* mqtt_server = "node02.myqtthub.com";
const char* User = "Bioener";
const char* CodePass = "Bioener.2017";
//Id en hub de Sala de mandos:
const char* Id = "plantas_deAgua.";
//Id en hub de Leonardo:
//const char* Id = "plantas.de.agua";
String mensaje2;

WiFiClient espClient;

PubSubClient client(espClient);

//-------------------------Configuro las entradas ------------------------------------------

const int pinDI0 = D5; //Tanque lleno 
const int pinDI = D6; //Produccion Planta 1
const int pinDI2 = D7; //Produccion Planta 2
const int pinDI3 = D1; //Produccion Planta 3

// Variables
bool estadoDI0 = false;
bool estadoDIAnterior0 = false;

bool estadoDI = false;
bool estadoDIAnterior = false;

bool estadoDI2 = false;
bool estadoDIAnterior2 = false;

bool estadoDI3 = false;
bool estadoDIAnterior3 = false;

unsigned long tiempoUltimoDebounce0 = 0; 
unsigned long tiempoUltimoDebounce = 0;
unsigned long tiempoUltimoDebounce2 = 0;
unsigned long tiempoUltimoDebounce3 = 0;

unsigned long tiempoDebounce = 180; 

void reconnect() {
  while (!client.connected()) {
    if (client.connect(Id, User, CodePass)) {
      Serial.println("Conectado al broker MQTT____");
      client.subscribe("plantas/reset");

    } else {
      Serial.print("Error al conectar al broker MQTT, estado de conexión: ");
      digitalWrite(LED_BUILTIN, HIGH);
      delay(700);
      digitalWrite(LED_BUILTIN, LOW);
      delay(700);
      Serial.println(client.state());
      delay(700);
    }
  }
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);


  Serial.begin(115200);
  Serial.println("Iniciando..");

  setupOTA("plantas_deAgua.", mySSID, myPASSWORD);

  //----------------------Inicializa la conexión WiFi--------------------------------------
  WiFi.begin(mySSID, myPASSWORD);
  Serial.begin(115200);
  while (WiFi.status() != WL_CONNECTED) {

    Serial.println("Conectando....");
  }
  Serial.println("Conectado a la red WiFi");

  //----------------------Conecta al servidor MQTT-----------------------------------------------
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  //-----------------------Inicializa la entrada de planta 1--------------------------------------
  pinMode(pinDI, INPUT_PULLUP);

  //-----------------------Inicializa la entrada de planta 2--------------------------------------
  pinMode(pinDI2, INPUT_PULLUP);

  //-----------------------Inicializa la entrada de Tanque Lleno----------------------------------
  pinMode(pinDI0, INPUT_PULLUP);

//-------------------------Inicializa la entrada de planta 3--------------------------------------
  pinMode(pinDI3, INPUT_PULLUP);

}

//------------------------Acá se maneja los mensajes MQTT que reciba el ESP8266--------------------------------------
void callback(char* topic, byte* payload, unsigned int length) {

  //------------Formatea estado de Planta 1 como decimal para enviar al servidor MQTT--------
  char Planta1State[10];
  sprintf(Planta1State, "%d", estadoDIAnterior);

  //------------Formatea estado de Planta 2 como decimal para enviar al servidor MQTT--------
  char Planta2State[10];
  sprintf(Planta2State, "%d", estadoDIAnterior2);

  //------------Formatea estado de Tanque LLeno como decimal para enviar al servidor MQTT--------
  char TankFullState[10];
  sprintf(TankFullState, "%d", estadoDIAnterior0);

//------------Formatea estado de Planta 3 como decimal para enviar al servidor MQTT--------
  char Planta3State[10];
  sprintf(Planta3State, "%d", estadoDIAnterior3);

  String mensaje = "{\"data\":{\"Planta1\":";
  mensaje += Planta1State;
  mensaje += ",\"Planta2\":";
  mensaje += Planta2State;
  mensaje += ",\"TanqueLleno\":";
  mensaje += TankFullState;
  mensaje += ",\"Planta3\":";
  mensaje += Planta3State;
  mensaje += "}}";

//----Si el mensaje se recibe en el tema "reset", envia el estado actual de las entradas---------
if (strcmp(topic, "plantas/reset") == 0) { 
    client.publish("plantas/de_agua", mensaje.c_str());
    //client.publish("plantas/de_agua", mensaje2); 
    Serial.println("---------------Res-----------");
    //Serial.println(mensaje2);
 
  }
  
}

void loop() {
#ifdef defined(ESP32_RTOS) && defined(ESP32)
#else  // If you do not use FreeRTOS, you have to regulary call the handle method.
  ArduinoOTA.handle();
#endif


  if (!client.connected()) {
    digitalWrite(LED_BUILTIN, HIGH);
    reconnect();
  }

  if (client.connected()) {
    //client.subscribe("plantas/reset");
    digitalWrite(LED_BUILTIN, LOW);
    
  }

  client.loop();

  debounceDI();

}
void debounceDI() {



  //------------Formatea estado de Planta 1 como decimal para enviar al servidor MQTT--------
  char Planta1State[10];
  sprintf(Planta1State, "%d", estadoDIAnterior);

  //------------Formatea estado de Planta 2 como decimal para enviar al servidor MQTT--------
  char Planta2State[10];
  sprintf(Planta2State, "%d", estadoDIAnterior2);

  //------------Formatea estado de Tanque LLeno como decimal para enviar al servidor MQTT--------
  char TankFullState[10];
  sprintf(TankFullState, "%d", estadoDIAnterior0);

//------------Formatea estado de Planta 3 como decimal para enviar al servidor MQTT--------
  char Planta3State[10];
  sprintf(Planta3State, "%d", estadoDIAnterior3);

  String mensaje = "{\"data\":{\"Planta1\":";
  mensaje += Planta1State;
  mensaje += ",\"Planta2\":";
  mensaje += Planta2State;
  mensaje += ",\"TanqueLleno\":";
  mensaje += TankFullState;
  mensaje += ",\"Planta3\":";
  mensaje += Planta3State;
  mensaje += "}}";


//-----------------------------------------------------------------------
  int lectura = !digitalRead(pinDI); //Planta 1

  if (lectura != estadoDIAnterior) {
    tiempoUltimoDebounce = millis();
  }

  if ((millis() - tiempoUltimoDebounce) > tiempoDebounce) {
    if (lectura != estadoDI) {
      estadoDI = lectura;
      client.publish("plantas/de_agua", mensaje.c_str());
      Serial.println(mensaje);
    }
  }

  estadoDIAnterior = lectura;


//-----------------------------------------------------------------------
  int lectura2 = !digitalRead(pinDI2); //-------------------------Planta 2

  if (lectura2 != estadoDIAnterior2) {
    tiempoUltimoDebounce2 = millis();
  }

  if ((millis() - tiempoUltimoDebounce2) > tiempoDebounce) {
    if (lectura2 != estadoDI2) {
      estadoDI2 = lectura2;


      //lastPlanta1State = planta1State;  // Iguala el estado de la entrada de planta 1
      client.publish("plantas/de_agua", mensaje.c_str());
      Serial.println(mensaje);
    }
  }

  estadoDIAnterior2 = lectura2;

  //---------------------------------------------------------------------------
  int lectura0 = !digitalRead(pinDI0); //-------------------------Tanque LLeno

  if (lectura0 != estadoDIAnterior0) {
    tiempoUltimoDebounce0 = millis();
  }

  if ((millis() - tiempoUltimoDebounce0) > tiempoDebounce) {
    if (lectura0 != estadoDI0) {
      estadoDI0 = lectura0;


      //lastPlanta1State = planta1State;  // Iguala el estado de la entrada de planta 1
      client.publish("plantas/de_agua", mensaje.c_str());
      Serial.println(mensaje);
    }
  }

  estadoDIAnterior0 = lectura0;
  //-----------------------------------------------------------------------
  int lectura3 = !digitalRead(pinDI3);

  if (lectura3 != estadoDIAnterior3) {
    tiempoUltimoDebounce3 = millis();
  }

  if ((millis() - tiempoUltimoDebounce3) > tiempoDebounce) {
    if (lectura3 != estadoDI3) {
      estadoDI3 = lectura3;


      //lastPlanta1State = planta1State;  // Iguala el estado de la entrada de planta 1
      client.publish("plantas/de_agua", mensaje.c_str());
      Serial.println(mensaje);
    }
  }

  estadoDIAnterior3 = lectura3;
  
 
}



