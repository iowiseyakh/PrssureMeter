#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Fonts/FreeSansBold24pt7b.h>
#include <Fonts/FreeSansBold9pt7b.h>

#define SCREEN_WIDTH 128    // OLED display width, in pixels
#define SCREEN_HEIGHT 64    // OLED display height, in pixels
#define OLED_RESET -1       // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

enum ExpResult
{
  ExpStart,
  ExpPump,
  ExpRun,
  ExpSuccess,
  ExpFail
} expRes = ExpStart;

struct SensorData
{
  uint8_t status;
  uint8_t bridge[3];
  uint8_t temp[2];
  long getBridge() { return ((long)bridge[0]) << 16 | ((long)bridge[1]) << 8 | bridge[2]; }
  long getTemp() { return ((long)temp[0]) << 8 | temp[1]; }
} sensorData, calibData;

////////////////////////////////////////////////////////////////////////////////////
void readData(SensorData *data)
{
  int err = 0;

  Wire.beginTransmission(0x78);
  Wire.write(0xAC) == 1 || Serial.println("Wire.write(0xAC) error");
  (err = Wire.endTransmission()) == 0 || Serial.println("Wire.endTransmission() error" + String(err));

  delay(200);

  Wire.requestFrom(0x78, sizeof(SensorData), true) == sizeof(SensorData) || Serial.println("Wire.requestFrom(...) error");
  int i = 0;
  while (Wire.available())
  {
    int r = Wire.read();
    ((uint8_t *)data)[i++] = r;
    // Serial.print(r, HEX);
    // Serial.print(' ');
  }
  // Serial.println(String(data->getTemp()) + "\t" + String(data->getBridge()));
}

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
  display.print("kPa");
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
  Wire.begin();

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS))
  {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ; // Don't proceed, loop forever
  }
  display.setFont(&FreeSansBold24pt7b);
  display.setTextSize(1); // Draw 2X-scale text
  display.setTextColor(SSD1306_WHITE);

  pinMode(beepPin, OUTPUT);
  digitalWrite(beepPin, 0);

  pinMode(12, INPUT_PULLUP);

  // readData(&calibData);
  // Serial.println("Calib: " + String(calibData.getTemp()) + "    " + String(calibData.getBridge()));
}

////////////////////////////////////////////////////////////////////////////////////
void loop()
{
  readData(&sensorData);

  // 8406124  Данные с датчика при нормальном атмосферном давлении
  float Pmin = 0.0, Dmin = 8406124.0; // Давление калибровки и значение АЦП при этом давлении
  // float Pmax = 40.0, Dmax = 10000000.0; // Давление калибровки и значение АЦП при этом давлении
  // float Pmin = -40.0, Dmin = 2516582.4; // Давление калибровки и значение АЦП при этом давлении
  float Pmax = 40.0, Dmax = 14260633.6; // Давление калибровки и значение АЦП при этом давлении

  float P = (Pmax - Pmin) / (Dmax - Dmin) * ((float)sensorData.getBridge() - Dmin) + Pmin;

  Serial.println("P = " + String(P, 3) + " kPa,   ADC = " + String(sensorData.getBridge()) + ",   T = " + String(sensorData.getTemp()));

  drawValue(P);

  static unsigned long experimentEndAt = 0; // Время, когда завершится эксперимент
  if (!digitalRead(12))                      // Концевик разомкнут, кран закрыт, система герметична
  {
    if (expRes == ExpPump) // Если в предыдущий раз кран был открыт,
    {
      expRes = ExpRun;                    // значит запускаем эксперимент
      experimentEndAt = millis() + 60000; // Определяем время, когда эксперимент завершится
    }
    if (expRes == ExpRun) // Эаксперимент уже запущен
    {
      if (P < 40) // Если давление ниже границы, то сразу фейл, даже не ожидая завершения
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