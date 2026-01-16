#include <Arduino.h>
//引入灯的库
#include <Adafruit_NeoPixel.h>

// put function declarations here:
int myFunction(int, int);


//定义48号管脚，它控制ws2812的rgb-led灯
#define LED_PIN 48


void setup() {
  // put your setup code here, to run once:
  int result = myFunction(2, 3);

  //调低亮度 
  Adafruit_NeoPixel strip = Adafruit_NeoPixel(1, LED_PIN, NEO_GRB + NEO_KHZ800);
  strip.begin();
  strip.setBrightness(10);
  strip.show();
  
}

void loop() {
  // put your main code here, to run repeatedly:
  //收到串口输入1把ws2812的rgb-led灯变成红色 
  if (Serial.available() > 0) {
    int input = Serial.read();
    if (input == '1') {
      Adafruit_NeoPixel strip = Adafruit_NeoPixel(1, LED_PIN, NEO_GRB + NEO_KHZ800);
      strip.begin();
      strip.setPixelColor(0, 255, 0, 0);
      strip.show();
    }
    //收到2关灯
    if (input == '2') {
      Adafruit_NeoPixel strip = Adafruit_NeoPixel(1, LED_PIN, NEO_GRB + NEO_KHZ800);
      strip.begin();
      strip.setBrightness(0);
      strip.show();
    }
  }
}

// put function definitions here:
int myFunction(int x, int y) {
  return x + y;
}