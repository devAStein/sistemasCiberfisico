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

void setup() {
  // put your setup code here, to run once:

}

void loop() {
  // put your main code here, to run repeatedly:

}
