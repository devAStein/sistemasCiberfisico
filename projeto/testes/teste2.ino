#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

void setup() {
  lcd.init(); 
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Teste do LCD");
  delay(2000);
  lcd.setCursor(0, 1);
  lcd.print("ESP32 OK!");
}

void loop() {
  
}
