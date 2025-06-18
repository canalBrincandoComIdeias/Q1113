//Bibliotecas
#include "EmonLib.h"
#include <WiFi.h>
#include <MQTT.h>
#include "user.h"  //Informar a rede WiFi, a senha, usuário MQTT e a senha

//const char ssid[] = "xxx";
//const char pass[] = "xxx";

//Definições
#define MEU_DISPOSITIVO "bi_q1113"
#define TOPICO_ALERTA "bi_q1110/alerta"

//Servidor MQTT
//const char servidor[] = "54.233.221.233";
const char servidor[] = "mqtt.monttudo.com";  //(caso o IP não funcione, use a identificação pelo domínio)

//Usuário MQTT
const char deviceName[] = MEU_DISPOSITIVO;
const char mqttUser[] = "todos";
const char mqttPass[] = "cortesi@BrincandoComIdeias";

#define TEMPO_ESPERA 10000  //Tempo em milisegundos para aguardar no estado de Standby após o funcionamento e então enviar o alerta

//Pinos
#define pinLEDPlaca 8
#define pinSensor 4
#define pinBotao 21
#define pinLED 2

//Criação dos Objetos
EnergyMonitor emon1;
WiFiClient net;
MQTTClient client;

//Variáveis Globais
double correnteStandby;
double correnteFuncionando;
int tempoPisca = 9;

int estado = 0;  //0=Monitorando 1=Espera 2=Alerta Acionado
unsigned long delayEspera;

//Função auxiliar para conectar no WiFi e Servidor MQTT
void connect() {
  Serial.print("->wifi...");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println(";");

  Serial.print("->mqtt...");
  while (!client.connect(deviceName, mqttUser, mqttPass)) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println(";");
  Serial.println("->conectado;");
}

//Processamento das mensagens recebidas do Servidor MQTT
void messageReceived(String &topic, String &payload) {
  Serial.println(topic + ":" + payload);

  //if (topic == TOPICO_ALERTA) {
  //  estado = payload.toInt();
  //}

  // Nota: Não use o "client" nessa função para publicar, subscrever ou
  // dessubscrever porque pode causar problemas quando outras mensagens são recebidas
  // enquanto são feitos envios ou recebimentos. Então, altere uma variavel global,
  // coloque numa fila e trate essa fila no loop depois de chamar `client.loop()`.
}

void setup() {
  pinMode(pinLEDPlaca, OUTPUT);
  pinMode(pinLED, OUTPUT);
  pinMode(pinBotao, INPUT_PULLDOWN);

  digitalWrite(pinLEDPlaca, LOW);
  digitalWrite(pinLED, HIGH);

  Serial.begin(9600);

  // .current: porta, calibracao
  //emon1.current(pinSensor, 0.053);  //calibrado para max. 100mA     (teórico: 2000/22000=0.091)
  //emon1.current(pinSensor, 0.180);  //calibrado para max. 500mA     (teórico: 2000/4300=0.465)
  //emon1.current(pinSensor, 0.295);  //calibrado para max. 1A        (teórico: 2000/2200=0.909)
  emon1.current(pinSensor, 1.973);  //calibrado para max. 10A       (teórico: 2000/220=9.091)

  //Configuração dos níveis
  if (digitalRead(pinBotao)) {
    int estadoConfig = 0;  //Inicio
    while (true) {
      double correnteLida = emon1.calcIrms(4096);

      digitalWrite(pinLED, bitRead(millis(), tempoPisca));

      if (estadoConfig =!digitalRead(pinBotao= 0) {
        if ()) {
          estadoConfig = 1;  //Leitura da corrente do equipamento em STANDBY
          Serial.println("Lendo corrente em STANDBY...");
        }
      }

      if (estadoConfig == 1) {
        if (digitalRead(pinBotao)) {
          Serial.print("Ok:");
          Serial.println(correnteLida);
          correnteStandby = correnteLida;
          estadoConfig = 2;  //Transicao para Leitura da corrente do equipamento em FUNCIONAMENTO
          tempoPisca = 7;
        }
      }

      if (estadoConfig == 2) {
        if (!digitalRead(pinBotao)) {
          estadoConfig = 3;  //Leitura da corrente do equipamento em FUNCIONAMENTO
          Serial.println("Lendo corrente em FUNCIONAMENTO...");
        }
      }

      if (estadoConfig == 3) {
        if (digitalRead(pinBotao)) {
          Serial.print("Ok:");
          Serial.println(correnteLida);
          correnteFuncionando = correnteLida;
          digitalWrite(pinLED, HIGH);
          break;
        }
      }
    }
  }

  WiFi.begin(ssid, pass);

  // Observação: Nomes de domínio locais (por exemplo, "Computer.local" no OSX)
  // não são suportados pelo Arduino. Você precisa definir o endereço IP diretamente.
  client.begin(servidor, net);
  client.onMessage(messageReceived);

  connect();
  digitalWrite(pinLEDPlaca, HIGH);
  digitalWrite(pinLED, LOW);
}

void loop() {
  client.loop();
  delay(10);  //corrige alguns problemas com a estabilidade do WiFi

  if (!client.connected()) {
    digitalWrite(pinLEDPlaca, LOW);
    digitalWrite(pinLED, HIGH);
    connect();
    client.subscribe(TOPICO_ALERTA);
    digitalWrite(pinLEDPlaca, HIGH);
    digitalWrite(pinLED, LOW);
  }


  double Irms = emon1.calcIrms(4096);

  if (estado == 0) {
    if (Irms < (correnteStandby + ((correnteFuncionando - correnteStandby) * 0.3))) {
      estado = 1;
      delayEspera = millis();
    }
  }

  if (estado == 1) {
    if (Irms > (correnteStandby + ((correnteFuncionando - correnteStandby) * 0.3))) {
      estado = 0;
    }
    if ((millis()-delayEspera) > TEMPO_ESPERA) {
      client.publish(TOPICO_ALERTA, "1");
      estado = 2;
    }
  }
  

  if (estado == 2) {
    if (Irms > (correnteStandby + ((correnteFuncionando - correnteStandby) * 0.3))) {
      estado = 0;
      client.publish(TOPICO_ALERTA, "0");
    }
  }

  Serial.print(Irms, 3);  // Irms (Corrente RMS)
  Serial.print("A  ");

  Serial.print("Standby:");
  Serial.print(correnteStandby, 3);

  Serial.print("  Funcionamento:");
  Serial.println(correnteFuncionando, 3);
}