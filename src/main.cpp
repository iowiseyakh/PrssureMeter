#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Fonts/FreeSansBold24pt7b.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include <MedianTemplate.h>

#define SCREEN_WIDTH 128    // OLED display width, in pixels
#define SCREEN_HEIGHT 64    // OLED display height, in pixels
#define OLED_RESET -1       // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
MedianFilter<float, 7> filter;

enum ExpResult
{
  ExpStart,
  ExpPump,
  ExpRun,
  ExpSuccess,
  ExpFail
} expRes = ExpStart;


////////////////////////////////////////////////////////////////////////////////////
void drawValue(float value)
{
  // display.clearDisplay();
  // display.drawBitmap(0,0,bitmap,64,64,1);
  // display.display();
  // return;

  display.clearDisplay();
  display.setFont(&FreeSansBold24pt7b);
  display.setCursor(0, 40);
  display.print(value, 3);
  display.setFont(&FreeSansBold9pt7b);
  display.setCursor(0, 60);
  display.print("at");
  display.setCursor(50, 60);

  switch (expRes)
  {
  case ExpSuccess:
    display.print("Pass");
    break;
  case ExpFail:
    display.print("Fail");
    break;
  case ExpRun:
    display.print("Run");
    break;
  case ExpPump:
    display.print("Pump");
    break;
  default:
    display.print("Wait");
  }
  display.display();
}
const int beepPin = 11; // 11

////////////////////////////////////////////////////////////////////////////////////
void setup()
{
  Serial.begin(115200);

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS))
  {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ; // Don't proceed, loop forever
  }
  display.setTextSize(1); // Draw 2X-scale text
  display.setTextColor(SSD1306_WHITE);

  pinMode(beepPin, OUTPUT);
  digitalWrite(beepPin, 0);

  pinMode(12, INPUT_PULLUP);
}

////////////////////////////////////////////////////////////////////////////////////
void loop()
{
  float U = analogRead(A3) * 5.0 / 1024.0;
  //                    PSI       range    PSI to kPa   Pa to kPa
  //float P = (U - 0.5) * 100.0 / (4.5 - 0.5) * 6894.76 / 1000.0;
  float P = (U - 0.5) * 100.0 / (4.5 - 0.5) * 0.070307; // to atm
  P = filter.putGet(P);
  static float lowP;

  Serial.println("P = " + String(P, 3) + " at");

  drawValue(P);

  static unsigned long experimentEndAt = 0; // Время, когда завершится эксперимент
  if (!digitalRead(12))                     // Концевик разомкнут, кран закрыт, система герметична
  {
    if (expRes == ExpPump) // Если в предыдущий раз кран был открыт,
    {
      expRes = ExpRun;                    // значит запускаем эксперимент
      lowP = P - 0.1;                     // Запоминаем давление, при котором тест будет считаться проваленным
      experimentEndAt = millis() + 60000; // Определяем время, когда эксперимент завершится
    }
    if (expRes == ExpRun) // Эаксперимент уже запущен
    {
      if (P < lowP) // Если давление ниже границы, то сразу фейл, даже не ожидая завершения
        expRes = ExpFail;
      else if (experimentEndAt < millis()) // Время эксперимента вышло
        expRes = ExpSuccess;               // Эксперимент завершен успешно
    }
  }
  else // Концевик замкнут, кран открыт, в систему накачивается воздух
    expRes = ExpPump;

  static unsigned long nextBeepAt = 0; // Время следующего звукового сигнала
  switch (expRes)
  {
  case ExpSuccess:
    if (nextBeepAt < millis())
    {
      digitalWrite(beepPin, 1);
      delay(100);
      digitalWrite(beepPin, 0);
      delay(100);
      digitalWrite(beepPin, 1);
      delay(100);
      digitalWrite(beepPin, 0);
      nextBeepAt = millis() + 10000;
    }
    break;
  case ExpFail:
    if (nextBeepAt < millis())
    {
      digitalWrite(beepPin, 1);
      delay(300);
      digitalWrite(beepPin, 0);
      nextBeepAt = millis() + 10000;
    }
    break;
  default:
    nextBeepAt = 0;
  }
}