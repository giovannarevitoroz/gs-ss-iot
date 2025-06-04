#include <WiFi.h> 
#include <LiquidCrystal_I2C.h>
#include <Adafruit_Sensor.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include "DHT.h"
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <ArduinoJson.h>
#include <time.h>

// Wi-Fi
const char* ssid = "Wokwi-GUEST";
const char* password = "";

// MQTT
const char* mqtt_server = "broker.hivemq.com";
const int mqtt_port = 1883;
const char* mqtt_topic_sensor1 = "fiap/eventos-extremos/sensor1";
const char* mqtt_topic2_sensor2 = "fiap/eventos-extremos/sensor2";
const char* mqtt_topic3_sensor3 = "fiap/eventos-extremos/sensor3";
const char* mqtt_alert_topic = "fiap/eventos-extremos/alertas";
const char* mqtt_client_id = "esp32-sensor-extremo-001";

// Hardware
#define DHTPIN 12
#define DHTTYPE DHT22
#define LED_ALERTA_VERMELHO 13
#define LED_ALERTA_AMARELO 14
#define BUZZER_PIN 15
#define SENSOR_CHUVA_PIN 34
#define SENSOR_GAS_PIN 35
#define SENSOR_VENTO_PIN 32

// Limites
#define TEMP_EXTREMA_ALTA 40.0
#define TEMP_EXTREMA_BAIXA 0.0
#define UMIDADE_EXTREMA_ALTA 80.0
#define UMIDADE_EXTREMA_BAIXA 20.0
#define CHUVA_EXTREMA 3000
#define GAS_EXTREMO 700
#define VENTO_EXTREMO 10.0
#define MOVIMENTO_EXTREMO 1.5

// Objetos
WiFiClient espClient;
PubSubClient client(espClient);
LiquidCrystal_I2C lcd(0x27, 16, 2);
DHT dht(DHTPIN, DHTTYPE);
Adafruit_MPU6050 mpu;

// Variáveis
bool alertaAtivo = false;
bool riscoEnchente = false;
String ultimoAlerta = "";

// NTP
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = -10800; 
const int daylightOffset_sec = 0;

void setup() {
  Serial.begin(115200);
  pinMode(LED_ALERTA_VERMELHO, OUTPUT);
  pinMode(LED_ALERTA_AMARELO, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  lcd.init();
  lcd.backlight();
  dht.begin();

  if (!mpu.begin()) {
    Serial.println("Erro MPU6050");
    while (1);
  }

  setup_wifi();
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  client.setServer(mqtt_server, mqtt_port);
}

void setup_wifi() {
  delay(10);
  Serial.print("Conectando a ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi conectado");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Conectando ao MQTT...");
    if (client.connect(mqtt_client_id)) {
      Serial.println("Conectado.");
      client.subscribe(mqtt_alert_topic);
    } else {
      Serial.print("Falhou, rc=");
      Serial.print(client.state());
      delay(5000);
    }
  }
}

String horaAtual() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) return "00:00:00";
  char buffer[9];
  strftime(buffer, sizeof(buffer), "%H:%M:%S", &timeinfo);
  return String(buffer);
}

void enviarDadosMQTT(float temp, float umid, int chuva, int gas, float vento, float movimento, String alertaFormatado) {
  String horario = horaAtual();
  StaticJsonDocument<512> doc;

  doc["temperatura"] = temp;
  doc["umidade"] = umid;
  doc["chuva"] = chuva;
  doc["gas"] = gas;
  doc["vento"] = vento;
  doc["movimento"] = movimento;
  doc["alerta"] = alertaFormatado;
  doc["localizacao"] = "Zona Leste de São Paulo [Tatuapé, Sapopemba, Mooca]";
  doc["horario"] = horario;

  char buffer[512];
  size_t n = serializeJson(doc, buffer);

  client.publish(mqtt_topic_sensor1, buffer, n);
  client.publish(mqtt_topic2_sensor2, buffer, n);
  client.publish(mqtt_topic3_sensor3, buffer, n);

  if (alertaFormatado != "") {
    client.publish(mqtt_alert_topic, alertaFormatado.c_str());
  }

  Serial.println("JSON MQTT enviado:");
  serializeJsonPretty(doc, Serial);
}

String verificarEventosExtremos(float temp, float umid, int chuva, int gas, float vento, float movimento) {
  String alerta = "";
  riscoEnchente = false;

  if (chuva > CHUVA_EXTREMA && umid > 70) alerta = "Risco de enchente";
  else if (temp >= TEMP_EXTREMA_ALTA) alerta = "Temperatura extrema alta";
  else if (temp <= TEMP_EXTREMA_BAIXA) alerta = "Temperatura extrema baixa";
  else if (umid >= UMIDADE_EXTREMA_ALTA) alerta = "Umidade extrema alta";
  else if (umid <= UMIDADE_EXTREMA_BAIXA) alerta = "Umidade extrema baixa";
  else if (gas >= GAS_EXTREMO) alerta = "Gás perigoso";
  else if (vento >= VENTO_EXTREMO) alerta = "Ventos fortes";
  else if (movimento >= MOVIMENTO_EXTREMO) alerta = "Movimento de massa";
  else if (temp >= 38.0 && umid >= 75 && vento >= 9.0) alerta = "Tempestade severa";
  else if (vento >= 20.0 && umid > 60 && chuva > 2500) alerta = "Furacão detectado";
  else if (vento >= 30.0 && movimento >= 2.5) alerta = "Tornado";
  else if (chuva < 100 && umid < 20 && temp > 35.0) alerta = "Seca";
  else if (temp < 20.0 && chuva > 2000) alerta = "Granizo";
  else if (movimento >= 2.0 && chuva > 2500) alerta = "Deslizamento";
  else if (temp > 35.0 || umid > 70.0 || chuva > 2000 || gas > 500 || vento > 8.0 || movimento > 1.0)
    alerta = "Condições alteradas";

  if (alerta != "") {
    alertaAtivo = true;
    ultimoAlerta = alerta;
    if (alerta.startsWith("Risco") || alerta == "Gás perigoso" || alerta == "Tornado" || alerta == "Deslizamento")
      digitalWrite(LED_ALERTA_VERMELHO, HIGH);
    else
      digitalWrite(LED_ALERTA_AMARELO, HIGH);
    Serial.println(alerta);
  } else {
    alertaAtivo = false;
    digitalWrite(LED_ALERTA_VERMELHO, LOW);
    digitalWrite(LED_ALERTA_AMARELO, LOW);
  }

  return alerta;
}

String mensagemInstrucao(String tipoAlerta) {
  if (tipoAlerta.indexOf("enchente") >= 0) return "Evite áreas baixas. Use rotas seguras. Proteja bens essenciais.";
  else if (tipoAlerta.indexOf("calor") >= 0 || tipoAlerta.indexOf("alta") >= 0) return "Beba água. Evite exposição ao sol por tempos prolongados. Use protetor solar.";
  else if (tipoAlerta.indexOf("frio") >= 0 || tipoAlerta.indexOf("baixa") >= 0) return "Use roupas apropriadas e mantenha portas e janelas fechadas.";
  else if (tipoAlerta.indexOf("gás") >= 0 || tipoAlerta.indexOf("Gás") >= 0) return "Ventile o ambiente, não permaneça por muito tempo, evacue e ligue para 193.";
  else if (tipoAlerta.indexOf("vento") >= 0) return "Evite áreas abertas. Feche portas e janelas.";
  else if (tipoAlerta.indexOf("deslizamento") >= 0) return "Evacue a encosta imediatamente e procure abrigo seguro.";
  else if (tipoAlerta.indexOf("tornado") >= 0) return "Busque abrigo subterrâneo ou lugar seguro.";
  return "Alerta geral. Siga instruções das autoridades.";
}

unsigned long tempoAnterior = 0;
const long intervalo = 3000;

void atualizarLCD(float temp, float umid, int chuva, int gas, float vento, float movimento) {
  static int contador = 0;

  if (millis() - tempoAnterior >= intervalo) {
    tempoAnterior = millis();
    contador++;
    if (contador > 2) contador = 0;
  }

  lcd.clear();
  if (contador == 0) {
    lcd.setCursor(0, 0);
    lcd.print("Temp: ");
    lcd.print(temp, 1);
    lcd.print("C");

    lcd.setCursor(0, 1);
    lcd.print("Umid: ");
    lcd.print(umid, 0);
    lcd.print("%");
  } else if (contador == 1) {
    lcd.setCursor(0, 0);
    lcd.print("Chuva: ");
    lcd.print(chuva);

    lcd.setCursor(0, 1);
    lcd.print("Gas: ");
    lcd.print(gas);
  } else if (contador == 2) {
    lcd.setCursor(0, 0);
    lcd.print("Vento: ");
    lcd.print(vento, 1);

    lcd.setCursor(0, 1);
    lcd.print("Mov: ");
    lcd.print(movimento, 2);
  }
}

void piscarLEDsAlerta() {
  static bool ledState = false;
  if (alertaAtivo) {
    ledState = !ledState;
    if (ultimoAlerta.startsWith("Risco") || ultimoAlerta == "Gás perigoso")
      digitalWrite(LED_ALERTA_VERMELHO, ledState);
    else
      digitalWrite(LED_ALERTA_AMARELO, ledState);
  }
}

void emitirAlertaSonoro() {
  if (alertaAtivo) {
    tone(BUZZER_PIN, 1000);
  } else {
    noTone(BUZZER_PIN);
  }
}

void loop() {
  if (!client.connected()) reconnect();
  client.loop();

  float temp = dht.readTemperature();
  float umid = dht.readHumidity();
  int chuva = analogRead(SENSOR_CHUVA_PIN);
  int gas = analogRead(SENSOR_GAS_PIN);
  float vento = analogRead(SENSOR_VENTO_PIN) * (12.0 / 4095.0);
  sensors_event_t a, g, tempMPU;
  mpu.getEvent(&a, &g, &tempMPU);
  float movimento = sqrt(pow(a.acceleration.x, 2) + pow(a.acceleration.y, 2) + pow(a.acceleration.z, 2));

  String tipoAlerta = verificarEventosExtremos(temp, umid, chuva, gas, vento, movimento);
  String instrucao = (tipoAlerta != "") ? mensagemInstrucao(tipoAlerta) : "";
  String alertaFormatado = (tipoAlerta != "") ? "ALERTA: " + tipoAlerta + " - " + instrucao : "";

  atualizarLCD(temp, umid, chuva, gas, vento, movimento);
  enviarDadosMQTT(temp, umid, chuva, gas, vento, movimento, alertaFormatado);
  piscarLEDsAlerta();
  emitirAlertaSonoro();

  delay(10000); // 10 segundos
}
