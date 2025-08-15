#ifndef DISPLAY_H
#define DISPLAY_H

//#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>





#include "Sensor.h"
#include "WiFiManage.h"



#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

void setupDisplay() {
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Endereço I2C do display
    Serial.println(F("SSD1306 allocation failed"));
    for (;;); // Não continuar se a inicialização falhar
  }

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.display();
    
    // Tela inicial
    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(10, 0);
    display.println(F("ESP8266"));
    display.setTextSize(1);
    display.setCursor(20, 30);
    display.println(F("Dashboard v2.0"));
    display.display();
    delay(2000);

}



void updateDisplay() {
 
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
 
 
  

  display.print("IP: ");
  display.println(WiFi.localIP());

  display.print("Temp: ");
  display.print(getTemperature());
  display.println(" C");




  display.display();
  
}


#endif