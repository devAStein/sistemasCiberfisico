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

void setup() {
  // put your setup code here, to run once:

}

void loop() {
  // put your main code here, to run repeatedly:

}
