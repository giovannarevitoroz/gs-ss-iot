#include <WiFi.h>
#include <LiquidCrystal_I2C.h>
#include <Adafruit_Sensor.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include "DHT.h"
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <time.h>  // Para obter horário atual

// Configurações Wi-Fi
const char* ssid = "Wokwi-GUEST";
const char* password = "";

// Configurações MQTT
const char* mqtt_server = "broker.hivemq.com";
const int mqtt_port = 1883;
const char* mqtt_topic = "fiap/eventos-extremos/sensor1";
const char* mqtt_alert_topic = "fiap/eventos-extremos/alertas";
const char* mqtt_client_id = "esp32-sensor-extremo-001";

// Definições de hardware
#define DHTPIN 12
#define DHTTYPE DHT22
#define LED_ALERTA_VERMELHO 13
#define LED_ALERTA_AMARELO 14
#define BUZZER_PIN 15
#define SENSOR_CHUVA_PIN 34
#define SENSOR_GAS_PIN 35
#define SENSOR_VENTO_PIN 32

// Limites para eventos extremos
#define TEMP_EXTREMA_ALTA 40.0
#define TEMP_EXTREMA_BAIXA 0.0
#define UMIDADE_EXTREMA_ALTA 80.0
#define UMIDADE_EXTREMA_BAIXA 20.0
#define CHUVA_EXTREMA 3000
#define GAS_EXTREMO 700
#define VENTO_EXTREMO 10.0
#define MOVIMENTO_EXTREMO 1.5

// Objetos globais
WiFiClient espClient;
PubSubClient client(espClient);
LiquidCrystal_I2C lcd(0x27, 16, 2);
DHT dht(DHTPIN, DHTTYPE);
Adafruit_MPU6050 mpu;

// Variáveis de estado
unsigned long lastMsgTime = 0;
bool alertaAtivo = false;
bool riscoEnchente = false;
String ultimoAlerta = "";
unsigned long ultimoAlertaSonoro = 0;

// Configuração de horário NTP para obter hora atual
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = -10800;  // Exemplo: -3 horas GMT (Brasil)
const int   daylightOffset_sec = 0;

void setup() {
  Serial.begin(115200);

  pinMode(LED_ALERTA_VERMELHO, OUTPUT);
  pinMode(LED_ALERTA_AMARELO, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  lcd.init();
  lcd.backlight();

  dht.begin();

  if (!mpu.begin()) {
    Serial.println("Falha ao iniciar MPU6050!");
    while (1);
  }

  setup_wifi();

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer); // Configurar NTP para horário

  client.setServer(mqtt_server, mqtt_port);
}

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Conectando a rede");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi conectado");
  Serial.println("Endereço IP: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Tentando conexão MQTT...");
    
    if (client.connect(mqtt_client_id)) {
      Serial.println("Conectado.");
      client.subscribe(mqtt_alert_topic);
    } else {
      Serial.print("Falha, rc=");
      Serial.print(client.state());
      Serial.println(" tentando novamente em 5 segundos...");
      delay(5000);
    }
  }
}

// Função para obter horário atual formatado
String horaAtual() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return "00:00:00";
  }
  char buffer[9];
  strftime(buffer, sizeof(buffer), "%H:%M:%S", &timeinfo);
  return String(buffer);
}

void enviarDadosMQTT(float temp, float umid, int chuva, int gas, float vento, float movimento, String alerta) {
  String horario = horaAtual();

  String payload = "{";
  payload += "\"temperatura\":" + String(temp) + ",";
  payload += "\"umidade\":" + String(umid) + ",";
  payload += "\"chuva\":" + String(chuva) + ",";
  payload += "\"gas\":" + String(gas) + ",";
  payload += "\"vento\":" + String(vento) + ",";
  payload += "\"movimento\":" + String(movimento) + ",";
  payload += "\"alerta\":\"" + alerta + "\",";
  payload += "\"localizacao\":\"AreaMonitorada1\",";
  payload += "\"horario\":\"" + horario + "\"";
  payload += "}";

  client.publish(mqtt_topic, payload.c_str());

  if (alerta != "") {
    // Inclui horário no tópico de alertas também
    String alertaHorario = alerta + " (Atualizado às " + horario + ")";
    client.publish(mqtt_alert_topic, alertaHorario.c_str());
  }

  Serial.println("Mensagem publicada via MQTT:");
  Serial.println(payload);
}

void verificarEventosExtremos(float temp, float umid, int chuva, int gas, float vento, float movimento) {
  String alerta = "";
  riscoEnchente = false;

  if (chuva > CHUVA_EXTREMA && umid > 70) {
    alerta = "ALERTA: Risco de enchente! Chuva: " + String(chuva) + ", Umidade: " + String(umid) + "%";
    riscoEnchente = true;
  }
  else if (temp >= TEMP_EXTREMA_ALTA) {
    alerta = "ALERTA: Temperatura extrema alta (" + String(temp) + "°C)";
  }
  else if (temp <= TEMP_EXTREMA_BAIXA) {
    alerta = "ALERTA: Temperatura extrema baixa (" + String(temp) + "°C)";
  }
  else if (umid >= UMIDADE_EXTREMA_ALTA) {
    alerta = "ALERTA: Umidade extrema alta (" + String(umid) + "%)";
  }
  else if (umid <= UMIDADE_EXTREMA_BAIXA) {
    alerta = "ALERTA: Umidade extrema baixa (" + String(umid) + "%)";
  }
  else if (gas >= GAS_EXTREMO) {
    alerta = "ALERTA: Nível de gás perigoso (" + String(gas) + ")";
  }
  else if (vento >= VENTO_EXTREMO) {
    alerta = "ALERTA: Ventos fortes (" + String(vento) + " m/s)";
  }
  else if (movimento >= MOVIMENTO_EXTREMO) {
    alerta = "ALERTA: Movimento de massa detectado (" + String(movimento) + ")";
  }

  else if (temp >= 38.0 && umid >= 75 && vento >= 9.0) {
    alerta = "ALERTA: Tempestade severa se aproximando!";
  }
  else if (vento >= 20.0 && umid > 60 && chuva > 2500) {
    alerta = "ALERTA: Furacão detectado!";
  }
  else if (vento >= 30.0 && movimento >= 2.5) {
    alerta = "ALERTA: Tornado em formação!";
  }
  else if (chuva < 100 && umid < 20 && temp > 35.0) {
    alerta = "ALERTA: Seca prolongada!";
  }
  else if (temp < 20.0 && chuva > 2000) {
    alerta = "ALERTA: Risco de granizo!";
  }
  else if (movimento >= 2.0 && chuva > 2500) {
    alerta = "ALERTA: Risco de deslizamento de terra!";
  }
  else if (temp > 35.0 || umid > 70.0 || chuva > 2000 || gas > 500 || vento > 8.0 || movimento > 1.0) {
    alerta = "ATENÇÃO: Condições ambientais alteradas";
  }

  // Acionamento visual
  if (alerta.startsWith("ALERTA")) {
    digitalWrite(LED_ALERTA_VERMELHO, HIGH);
    digitalWrite(LED_ALERTA_AMARELO, LOW);
  } else if (alerta.startsWith("ATENÇÃO")) {
    digitalWrite(LED_ALERTA_VERMELHO, LOW);
    digitalWrite(LED_ALERTA_AMARELO, HIGH);
  } else {
    digitalWrite(LED_ALERTA_VERMELHO, LOW);
    digitalWrite(LED_ALERTA_AMARELO, LOW);
  }

  // Atualiza estado
  if (alerta != "") {
    alertaAtivo = true;
    ultimoAlerta = alerta;
    Serial.println(alerta);
  } else {
    alertaAtivo = false;
  }
}


void atualizarLCD(float temp, float umid, int chuva) {
  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("Temp: ");
  lcd.print(temp, 1);
  lcd.print("C");

  lcd.setCursor(10, 0);
  lcd.print("Umid: ");
  lcd.print(umid, 0);
  lcd.print("%");

  lcd.setCursor(0, 1);
  lcd.print("Chuva: ");
  lcd.print(chuva);

  if (riscoEnchente) {
    lcd.setCursor(10, 1);
    lcd.print("ENCHENTE!");
  } else if (alertaAtivo) {
    lcd.setCursor(10, 1);
    lcd.print("ALERTA!");
  }
}

void piscarLEDsAlerta() {
  if (alertaAtivo) {
    static bool ledState = false;
    ledState = !ledState;

    if (ultimoAlerta.startsWith("ALERTA")) {
      digitalWrite(LED_ALERTA_VERMELHO, ledState);
    } else {
      digitalWrite(LED_ALERTA_AMARELO, ledState);
    }
  }
}

void emitirAlertaSonoro() {
  if (alertaAtivo) {
    tone(BUZZER_PIN, 1000);  // Apita contínuo a 1kHz enquanto alerta estiver ativo
  } else {
    noTone(BUZZER_PIN); // Para o som se não houver alerta
  }
}

String gerarRotaEvacuacao() {
  // Simulação de rota, pode ser ampliada para GPS real
  return "Rua A -> Av. B -> Ponto Seguro";
}

String mensagemInstrucao(String tipoAlerta) {
  if (tipoAlerta.indexOf("enchente") >= 0 || tipoAlerta.indexOf("Enchente") >= 0) {
    return "ALERTA DE ENCHENTE\nÁreas baixas e próximas a rios podem ser inundadas nas próximas horas. Evite sair de casa e não atravesse áreas alagadas com veículos ou a pé. Dirija-se às áreas altas e siga as rotas de evacuação indicadas pelas autoridades." + gerarRotaEvacuacao();
  } 
  else if (tipoAlerta.indexOf("temperatura extrema alta") >= 0 || tipoAlerta.indexOf("alta") >= 0) {
    return "ALERTA DE CALOR EXTREMO\nMantenha-se hidratado, evite exposição direta ao sol, principalmente entre 10h e 16h, e cuide de idosos e crianças.";
  }
  else if (tipoAlerta.indexOf("temperatura extrema baixa") >= 0 || tipoAlerta.indexOf("baixa") >= 0) {
    return "ALERTA DE FRIO EXTREMO\nUse roupas adequadas, evite exposição prolongada ao frio e cuide de pessoas vulneráveis.";
  }
  else if (tipoAlerta.indexOf("umidade extrema alta") >= 0 || tipoAlerta.indexOf("Umidade alta") >= 0) {
    return "ALERTA DE UMIDADE ALTA\nEvite áreas com mofo e mantenha ambiente ventilado.";
  }
  else if (tipoAlerta.indexOf("umidade extrema baixa") >= 0 || tipoAlerta.indexOf("Umidade baixa") >= 0) {
    return "ALERTA DE UMIDADE BAIXA\nHidrate-se e mantenha a pele protegida.";
  }
  else if (tipoAlerta.indexOf("gás perigoso") >= 0 || tipoAlerta.indexOf("gas") >= 0) {
    return "ALERTA DE GÁS\nVentile o ambiente, evite fontes de ignição e evacue se necessário.";
  }
  else if (tipoAlerta.indexOf("ventos fortes") >= 0 || tipoAlerta.indexOf("vento") >= 0) {
    return "ALERTA DE VENTO FORTE\nEvite áreas abertas e proteja-se contra objetos soltos.";
  }
  else if (tipoAlerta.indexOf("movimento de massa") >= 0 || tipoAlerta.indexOf("movimento") >= 0) {
    return "ALERTA DE MOVIMENTO DE MASSA\nEvacue a área imediatamente e procure abrigo seguro.";
  }
  else if (tipoAlerta.indexOf("Tempestade severa") >= 0) {
    return "ALERTA DE TEMPESTADE SEVERA\nVentos fortes e chuvas intensas podem causar danos. Permaneça em local seguro, evite uso de aparelhos elétricos e fique longe de árvores e estruturas frágeis.";
  }
  else if (tipoAlerta.indexOf("Furacão") >= 0) {
    return "ALERTA DE FURACÃO\nProcure abrigo imediatamente em local seguro e siga as orientações das autoridades locais. Prepare kit de emergência com água, alimentos e medicamentos.";
  }
  else if (tipoAlerta.indexOf("Tornado") >= 0) {
    return "ALERTA DE TORNADO\nBusque abrigo em ambientes internos, longe de janelas e portas. Proteja cabeça e pescoço. Evite sair até o alerta ser cancelado.";
  }
  else if (tipoAlerta.indexOf("Seca") >= 0) {
    return "ALERTA DE SECA\nEconomize água, evite desperdícios e siga as recomendações de racionamento de água das autoridades locais.";
  }
  else if (tipoAlerta.indexOf("Granizo") >= 0) {
    return "ALERTA DE GRANIZO\nPermaneça em local protegido, evite trânsito nas ruas e cuide para não ser atingido por objetos.";
  }
  else if (tipoAlerta.indexOf("deslizamento") >= 0 || tipoAlerta.indexOf("Deslizamento") >= 0) {
    return "ALERTA DE RISCO DE DESLIZAMENTO\nEvite áreas de encosta e siga as rotas de evacuação recomendadas. Não permaneça em áreas de risco até liberação das autoridades.";
  }
  else {
    return "Condições normais.";
  }
}


void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Ler sensores
  float temp = dht.readTemperature();
  float umid = dht.readHumidity();
  int chuva = analogRead(SENSOR_CHUVA_PIN); 
  int gas = analogRead(SENSOR_GAS_PIN);
  float vento = analogRead(SENSOR_VENTO_PIN) * (12.0 / 4095.0);  // Supondo conversão para m/s
  sensors_event_t a, g, tempMPU;
  mpu.getEvent(&a, &g, &tempMPU);
  float movimento = sqrt(a.acceleration.x*a.acceleration.x + a.acceleration.y*a.acceleration.y + a.acceleration.z*a.acceleration.z);

  // Verificar alertas
  verificarEventosExtremos(temp, umid, chuva, gas, vento, movimento);

  // Atualizar LCD
  atualizarLCD(temp, umid, chuva);

  // Enviar dados MQTT e alertas
  String instrucao = alertaAtivo ? mensagemInstrucao(ultimoAlerta) : "";
  enviarDadosMQTT(temp, umid, chuva, gas, vento, movimento, instrucao);

  // Piscar LEDs e emitir som
  piscarLEDsAlerta();
  emitirAlertaSonoro();

  delay(10000);  // Atualiza a cada 10 segundos
}
