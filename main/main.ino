#include <Servo.h>
#include <WiFi.h>
#include <PubSubClient.h>

//***********************************
//************ PINES ****************
//***********************************

// Motor Izq
#define pin_ena1 27
#define pin_n1 14
#define pin_n2 4

//Motor Der
#define pin_n3 2
#define pin_n4 32
#define pin_ena2 33

//servo
#define pin_servo 13

//HC-SR04
#define pin_trig 26
#define pin_echo 25

//IMU MPU92.65
#define pin_scl 22
#define pin_sda 21

//***********************************
//************ CONFIG ****************
//***********************************

//WiFi
const char* ssid = "missid";
const char* password = "miclave";

//MQTT
const char* mqtt_server = "ioticos.org";
const int mqtt_port = 1883;
const char* mqtt_user = "eF0qt5zQzMpiGG7";
const char* mqtt_password = "E7Hyt2VQfcSl9gk";
const String root_topic = "NvZOWtTvjtiJLYp";

//Servo
#define servo_max_ang 180
#define servo_min_ang 0 //vigilar con los servos chinos pq no llegan a 0
#define servo_speed 25
#define servo_step 3

//***********************************
//************ VARIABLES ****************
//***********************************

Servo servo;
//mqtt
WiFiClient espClient;
PubSubClient client(espClient);

//Mensajeria
String toSend = "";
char buf[20];


//***********************************
//************ FUNCIONES ************
//***********************************
int distance();
void setup_wifi();
void callback(char* topic, byte* payload, unsigned int length);
void reconect();
void movement(String side); // side: left, right,strainght,back


void setup() {
  Serial.begin(115200);

  //PINMODES
  //HC-RS04
  pinMode(pin_trig, OUTPUT);
  pinMode(pin_echo, INPUT);

  //motor izq
  pinMode(pin_ena1, OUTPUT);
  pinMode(pin_n1, OUTPUT);
  pinMode(pin_n2, OUTPUT);

  //motor der
  pinMode(pin_ena2, OUTPUT);
  pinMode(pin_n3, OUTPUT);
  pinMode(pin_n4, OUTPUT);

  digitalWrite(pin_ena1, HIGH);
  digitalWrite(pin_ena2, HIGH);

  servo.attach(pin_servo);//pin 13

  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  delay(300);// tiempo para que el servo pueda desplazarse posición inicial
  for (int posDegrees = servo_min_ang; posDegrees <= servo_max_ang; posDegrees += servo_step) {
    //servo padece a menor de 24 grados, if para no romper
    if (posDegrees > 24) {
      //mover el servo en un ángulo que será igual a i servo(i)
      servo.write(180 - posDegrees);
    }
    delay(servo_speed); // evitar dar órdenes tan rápidas al servo
    //al radar se le debe enviar la distancia y el ángulo

    //calcular distancia d = distance();
    toSend = String(distance()) + "," + String(posDegrees);
    toSend.toCharArray(buf, 20);
    //enviar información por MQTT enviar(i,d)
    client.publish("NvZOWtTvjtiJLYp/map", buf);
    client.loop();
  }
  servo.write(155);//poner servo posicion inicial
}
int distance() {
  digitalWrite(pin_trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(pin_trig, LOW);
  unsigned long timeEcho = pulseIn(pin_echo, HIGH);
  int distance = timeEcho / 58;
  return distance;
}

void movement(String action) {
  // switch no permitedatos tipo string, usar if else
  if (action == "straight") {
    //rueda1
    digitalWrite(pin_n1, LOW);
    digitalWrite(pin_n2, HIGH);
    //rueda 2
    digitalWrite(pin_n3, HIGH);
    digitalWrite(pin_n4, LOW);
  } else if (action == "back") {
    digitalWrite(pin_n1, HIGH);
    digitalWrite(pin_n2, LOW);
    digitalWrite(pin_n3, LOW);
    digitalWrite(pin_n4, HIGH);
  } else if (action == "left") {
    digitalWrite(pin_n1, HIGH);
    digitalWrite(pin_n2, LOW);
    digitalWrite(pin_n3, HIGH);
    digitalWrite(pin_n4, LOW);
  } else if (action == "right") {
    digitalWrite(pin_n1, LOW);
    digitalWrite(pin_n2, HIGH);
    digitalWrite(pin_n3, LOW);
    digitalWrite(pin_n4, HIGH);
  } else if (action == "stop") {
    digitalWrite(pin_n1, LOW);
    digitalWrite(pin_n2, LOW);
    digitalWrite(pin_n3, LOW);
    digitalWrite(pin_n4, LOW);
  }
  /*switch (action) {
    case "straight":
      //rueda1
      digitalWrite(pin_n1, LOW);
      digitalWrite(pin_n2, HIGH);
      //rueda 2
      digitalWrite(pin_n3, HIGH);
      digitalWrite(pin_n4, LOW);
      break;
    case "back":
      digitalWrite(pin_n1, HIGH);
      digitalWrite(pin_n2, LOW);
      digitalWrite(pin_n3, LOW);
      digitalWrite(pin_n4, HIGH);
      break;

    case "left":
      digitalWrite(pin_n1, HIGH);
      digitalWrite(pin_n2, LOW);
      digitalWrite(pin_n3, HIGH);
      digitalWrite(pin_n4, LOW);
      break;
    case "right":
      digitalWrite(pin_n1, LOW);
      digitalWrite(pin_n2, HIGH);
      digitalWrite(pin_n3, LOW);
      digitalWrite(pin_n4, HIGH);
      break;

    case "stop":
      digitalWrite(pin_n1, LOW);
      digitalWrite(pin_n2, LOW);
      digitalWrite(pin_n3, LOW);
      digitalWrite(pin_n4, LOW);
      break;
    }*/
}
//***********************************
//************ WiFi *****************
//***********************************

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Conectando a ");
  Serial.println(ssid);

  // WiFi.begin(ssid, password); // ERROR

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("Conectando a Red WiFi: ");
  Serial.println("Dirección IP: ");
  Serial.println(WiFi.localIP());

}

void callback(char *topic, byte *payload, unsigned int length) {
  String incoming = "";

  Serial.print("Mensaje recibido -> ");
  Serial.print(topic);
  Serial.print("");

  for (int i = 0; i < length; i++) {
    incoming += (char)payload[i];
  }
  incoming.trim();
  Serial.print("Mensaje -> " + incoming);
  String topic_str(topic);
  if (topic_str == root_topic + "/movement") {
    movement(incoming);
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Intentando conexión MQTT...");

    String clientId = "NALA_";
    clientId += String(random(0xffff, HEX));

    if (client.connect(clientId.c_str(), mqtt_user, mqtt_password)) {
      Serial.println("Conectado!");
      if (client.subscribe("NvZOWtTvjtiJLYp/movement")) {
        Serial.println("Subscripcion OK");
      } else {
        Serial.println("Fallo de Subscripcion");
      }
    } else {
      Serial.print("Fallo en la conexion MQTT -> Error: ");
      Serial.print(client.state());
      Serial.println("Intentamos de nuevo en 5 segundos...");
      delay(5000);
    }
  }

}
/******Ensamblaje*******/
/*
1- Chasís
2- Ruedas(2)Motores DC (2)
3- tercera rueda(patin de cola o rueda loca)
3- Power Bank

- ESP32
- sensor de distancia ultrasónico HC-SR04(trig usar resistencia 3K3)
- micro servo SG90
- conrolador L298N (H-Bridge)

*/
