#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define pinoCLK 25
#define pinoDT 5

int lcdColumns = 16;
int lcdRows = 2;
LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows);  

int contador = 0;
int ultimoEstadoCLK;

void setup() {
  Serial.begin(115200); 

  pinMode(pinoCLK, INPUT);
  pinMode(pinoDT, INPUT);

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Gire o encoder");

  ultimoEstadoCLK = digitalRead(pinoCLK);
}

void loop() {
  int estadoAtualCLK = digitalRead(pinoCLK);

  if (estadoAtualCLK != ultimoEstadoCLK) {
    if (digitalRead(pinoDT) != estadoAtualCLK) {
      contador--; // Gira para esquerda
    } else {
      contador++; // Gira para direita
    }

    lcd.clear(); // Apaga LCD
    lcd.setCursor(0, 0);
    lcd.print("Valor: ");
    lcd.print(contador);

    Serial.println(contador);
  }

  ultimoEstadoCLK = estadoAtualCLK;
}
