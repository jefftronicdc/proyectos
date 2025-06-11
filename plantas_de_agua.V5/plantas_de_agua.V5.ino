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
WiFiClient espClient;

PubSubClient client(espClient);

//-------------------------Configuro las entradas ------------------------------------------

enum SensorIndex { PLANTA1, PLANTA2, TANQUE_LLENO, PLANTA3, SENSOR_COUNT };

const uint8_t sensorPins[SENSOR_COUNT] = { D6, D7, D5, D1 };
bool sensorState[SENSOR_COUNT] = {false};
bool sensorPrevState[SENSOR_COUNT] = {false};
unsigned long lastDebounceTime[SENSOR_COUNT] = {0};

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
  while (WiFi.status() != WL_CONNECTED) {

    Serial.println("Conectando....");
  }
  Serial.println("Conectado a la red WiFi");

  //----------------------Conecta al servidor MQTT-----------------------------------------------
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  //-----------------------Inicializa las entradas-------------------------------------
  for (int i = 0; i < SENSOR_COUNT; i++) {
    pinMode(sensorPins[i], INPUT_PULLUP);
  }

}

//------------------------Acá se maneja los mensajes MQTT que reciba el ESP8266--------------------------------------
void callback(char* topic, byte* payload, unsigned int length) {
  char mensaje[80];
  snprintf(mensaje, sizeof(mensaje),
           "{\"data\":{\"Planta1\":%d,\"Planta2\":%d,\"TanqueLleno\":%d,\"Planta3\":%d}}",
           sensorState[PLANTA1], sensorState[PLANTA2], sensorState[TANQUE_LLENO],
           sensorState[PLANTA3]);

  if (strcmp(topic, "plantas/reset") == 0) {
    client.publish("plantas/de_agua", mensaje);
    Serial.println("---------------Res-----------");
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
  bool cambio = false;

  for (int i = 0; i < SENSOR_COUNT; i++) {
    int lectura = !digitalRead(sensorPins[i]);

    if (lectura != sensorPrevState[i]) {
      lastDebounceTime[i] = millis();
    }

    if ((millis() - lastDebounceTime[i]) > tiempoDebounce) {
      if (lectura != sensorState[i]) {
        sensorState[i] = lectura;
        cambio = true;
      }
    }

    sensorPrevState[i] = lectura;
  }

  if (cambio) {
    char mensaje[80];
    snprintf(mensaje, sizeof(mensaje),
             "{\"data\":{\"Planta1\":%d,\"Planta2\":%d,\"TanqueLleno\":%d,\"Planta3\":%d}}",
             sensorState[PLANTA1], sensorState[PLANTA2],
             sensorState[TANQUE_LLENO], sensorState[PLANTA3]);
    client.publish("plantas/de_agua", mensaje);
    Serial.println(mensaje);
  }
}



