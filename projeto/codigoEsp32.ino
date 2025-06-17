#include <WiFi.h>
#include <HTTPClient.h>
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>
#include "esp_sleep.h"
#include "BluetoothSerial.h"

// Bluetooth
BluetoothSerial SerialBT;

// Pino do sensor de fluxo de água
#define sensorPin 25
#define MIN_PULSOS 2
#define EEPROM_SIZE 8

// Pinos do encoder rotativo KY-040
#define pinCLK 14
#define pinDT 27
#define pinSW 33

// Variáveis globais
volatile int pulsos = 0;
unsigned long ultimaLeitura = 0;
unsigned long ultimoFluxoAtivo = 0;
unsigned long ultimaTransmissao = 0;
const unsigned long intervaloTransmissao = 5000;

bool fluxoAtivo = false;
float vazaoLMin = 0;
float totalLitros = 0;

float lastVazao = -1;
float lastTotal = -1;

String URL = "http://192.168.100.47/dht11_project/test_data.php";

// LCD
int lcdColumns = 16;
int lcdRows = 2;
LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows);

// WiFi
const char *ssid = "Oi_AA16";
const char *pass = "J6XbunM9";

float carregarTotalEEPROM() {
  float valor;
  EEPROM.get(0, valor);
  return isnan(valor) ? 0 : valor;
}

// Modos
int modoAtual = 1;  // 1: Medição | 2: Reset | 3: Ajuste via Bluetooth
bool aguardandoConfirmacao = false;
unsigned long ultimoDebounce = 0;
const unsigned long debounceDelay = 200;

// Interrupção do sensor
void IRAM_ATTR contarPulso() {
  pulsos++;
}

// Conexão WiFi
void connectWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;

  WiFi.begin(ssid, pass);
  Serial.print("Conectando WiFi");

  int tentativas = 0;
  while (WiFi.status() != WL_CONNECTED && tentativas < 10) {
    delay(500);
    Serial.print(".");
    tentativas++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi conectado.");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nFalha ao conectar WiFi.");
  }
}

// Envia dados HTTP e mede latência
void enviarDados(float vazao_m3, float total_m3) {
  if (WiFi.status() != WL_CONNECTED) connectWiFi();
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Sem WiFi. Dados não enviados.");
    return;
  }

  unsigned long inicio = millis();  // Início da medição de latência

  HTTPClient http;
  http.begin(URL);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");

  String postData = "vazao=" + String(vazao_m3, 6) + "&total=" + String(total_m3, 6);
  Serial.print("Enviando: ");
  Serial.println(postData);

  int httpCode = http.POST(postData);

  unsigned long fim = millis();  // Fim da medição
  Serial.print("Latência HTTP: ");
  Serial.print(fim - inicio);
  Serial.println(" ms");

  if (httpCode > 0) {
    Serial.println("Resposta: " + http.getString());
  } else {
    Serial.println("Erro HTTP: " + String(httpCode));
  }

  http.end();
}

// EEPROM
void salvarTotalEEPROM(float total) {
  EEPROM.put(0, total);
  EEPROM.commit();
}

// Deep sleep
void entrarEmSleep() {
  Serial.println("Modo: Deep Sleep - Consumo estimado: ~10 µA");
  Serial.println("Entrando em deep sleep...");

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Sem fluxo...");
  lcd.setCursor(0, 1);
  lcd.print("Dormindo...");
  delay(2000);
  lcd.noBacklight();

  WiFi.disconnect(true);
  detachInterrupt(digitalPinToInterrupt(sensorPin));
  esp_sleep_enable_ext0_wakeup((gpio_num_t)sensorPin, 0);
  esp_deep_sleep_start();
}

// Encoder: mudança de modo
void verificarMudancaModo() {
  static int ultimoCLK = HIGH;
  static int ultimoDT = HIGH;

  int atualCLK = digitalRead(pinCLK);
  int atualDT = digitalRead(pinDT);

  if (atualCLK == LOW && ultimoCLK == HIGH) {
    modoAtual++;
    if (modoAtual > 3) modoAtual = 1;

    lcd.clear();
    switch (modoAtual) {
      case 1: lcd.print("Modo 1: Medicao"); break;
      case 2: lcd.print("Modo 2: Reset"); break;
      case 3: lcd.print("Modo 3: BT Ajuste"); break;
    }

    Serial.println("Mudou para modo: " + String(modoAtual));
    aguardandoConfirmacao = false;
  }

  ultimoCLK = atualCLK;
  ultimoDT = atualDT;
}

// Botão: reset
void verificarBotaoReset() {
  static int ultimoEstadoSW = HIGH;
  int estadoSW = digitalRead(pinSW);

  if (estadoSW == LOW && ultimoEstadoSW == HIGH && millis() - ultimoDebounce > debounceDelay) {
    ultimoDebounce = millis();

    if (modoAtual == 2 && !aguardandoConfirmacao) {
      aguardandoConfirmacao = true;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Confirma reset?");
      lcd.setCursor(0, 1);
      lcd.print("Pressione SW");
    } else if (modoAtual == 2 && aguardandoConfirmacao) {
      totalLitros = 0;
      salvarTotalEEPROM(totalLitros);
      aguardandoConfirmacao = false;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Reset realizado");
      delay(1500);
      lcd.clear();
    }
  }

  ultimoEstadoSW = estadoSW;
}

// Setup
void setup() {
  Serial.begin(115200);
  SerialBT.begin("FluxoESP32");
  EEPROM.begin(EEPROM_SIZE);

  pinMode(sensorPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(sensorPin), contarPulso, RISING);

  pinMode(pinCLK, INPUT_PULLUP);
  pinMode(pinDT, INPUT_PULLUP);
  pinMode(pinSW, INPUT_PULLUP);

  lcd.init();
  lcd.backlight();

  totalLitros = carregarTotalEEPROM();
  connectWiFi();

  if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT0) {
    lcd.setCursor(0, 0);
    lcd.print("Acordou por fluxo");
    Serial.println("Acordou do deep sleep por fluxo.");
    delay(5000);

    pulsos = 0;
    attachInterrupt(digitalPinToInterrupt(sensorPin), contarPulso, RISING);
    delay(3000);
    detachInterrupt(digitalPinToInterrupt(sensorPin));

    if (pulsos < MIN_PULSOS) {
      Serial.println("Fluxo insuficiente. Voltando para sleep...");
      entrarEmSleep();
    }
  }
}

// Loop principal
void loop() {
  if (millis() - ultimaLeitura >= 1000) {
    detachInterrupt(digitalPinToInterrupt(sensorPin));

    if (pulsos >= MIN_PULSOS) {
      fluxoAtivo = true;
      vazaoLMin = pulsos / 7.5;
      totalLitros += vazaoLMin / 60.0;
      ultimoFluxoAtivo = millis();
    } else {
      fluxoAtivo = false;
      vazaoLMin = 0;
    }

    if (!fluxoAtivo && millis() - ultimoFluxoAtivo > 60000) {
      salvarTotalEEPROM(totalLitros);
      entrarEmSleep();
    }

    // Mostra modo atual e estimativa de consumo
    if (fluxoAtivo) {
      Serial.println("Modo: Ativo com fluxo - Consumo estimado: ~200 mA");
    } else {
      Serial.println("Modo: Ativo sem fluxo - Consumo estimado: ~80 mA");
    }

    if (modoAtual == 1) {
      if (abs(vazaoLMin - lastVazao) > 0.1 || abs(totalLitros - lastTotal) > 0.01) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Vazao: ");
        lcd.print(vazaoLMin, 2);
        lcd.print(" L/m");

        lcd.setCursor(0, 1);
        lcd.print("Total: ");
        lcd.print(totalLitros, 2);
        lcd.print(" L");

        lastVazao = vazaoLMin;
        lastTotal = totalLitros;
      }
    } else if (modoAtual == 2 && !aguardandoConfirmacao) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Modo Reset Manual");
      lcd.setCursor(0, 1);
      lcd.print("Pressione SW");
    } else if (modoAtual == 3) {
      lcd.setCursor(0, 0);
      lcd.print("BT: Envie litros");
    }

    Serial.printf("Vazao: %.2f L/min | Total: %.2f L\n", vazaoLMin, totalLitros);

    if (fluxoAtivo && millis() - ultimaTransmissao > intervaloTransmissao) {
      enviarDados(vazaoLMin / 1000.0, totalLitros / 1000.0);
      ultimaTransmissao = millis();
    }

    salvarTotalEEPROM(totalLitros);

    pulsos = 0;
    ultimaLeitura = millis();
    attachInterrupt(digitalPinToInterrupt(sensorPin), contarPulso, RISING);
  }

  verificarMudancaModo();
  verificarBotaoReset();

  // Modo 3: Bluetooth - ajuste de total
  if (modoAtual == 3 && SerialBT.available()) {
    String entrada = SerialBT.readStringUntil('\n');
    entrada.trim();
    float novoTotal = entrada.toFloat();

    if (novoTotal >= 0.0) {
      totalLitros = novoTotal;
      salvarTotalEEPROM(totalLitros);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Novo total:");
      lcd.setCursor(0, 1);
      lcd.print(totalLitros, 2);
      lcd.print(" L");

      Serial.println("Total ajustado via BT: " + String(totalLitros, 2));
      delay(3000);
      modoAtual = 1;
      lcd.clear();
    } else {
      Serial.println("Entrada invalida via BT.");
    }
  }
}
