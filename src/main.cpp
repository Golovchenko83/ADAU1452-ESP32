#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <I2C_eeprom.h>
#include <GyverTimer.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>
#include <ESPmDNS.h>
#include <EQ.h>
#include <Felder.h>
#define CLK GPIO_NUM_18 // энкодер
#define DT GPIO_NUM_19  // энкодер
#define KEY GPIO_NUM_17 // энкодер
#define DOUBLE_CLIK 320 // энкодер двойной клик ms
#define ADAU1452 0x38   // адрес dsp
#define MULTIPLEXOR 0x008A
#define MASTER_VOLUME 0x03BE

// I2C device found at address 0x57 Eeprom
// I2C device found at address 0x68 Часы

WiFiClient espClient;
PubSubClient client(espClient);
LiquidCrystal_I2C lcd(0x26, 20, 4);
LiquidCrystal_I2C lcd_2(0x27, 20, 4);
I2C_eeprom ee(0x57, I2C_DEVICESIZE_24LC32);
GTimer_ms t_eeprom;
GTimer_ms text_menu;
GTimer_ms dub_klik;
GTimer_ms OTA_Wifi;
byte state, state_key, vol_im = 0;
int8_t pos = 0, flag_key = 0, tab_key = 2;
byte eq_flag1 = 0, eq_flag2 = 0;
byte klik = 0, pos_state = 0, pos_state_raw = 0;
int eq_int_flag1 = 0;
const char *name_client = "Stereo-usil";          // Имя клиента и сетевого порта для ОТА
const char *ssid = "Beeline";                     // Wi-FI Имя точки доступа
const char *password = "sl908908908908sl";        // Wi-FI пароль точки доступа
const char *mqtt_server = "192.168.1.221";        // Mqtt Server
const char *mqtt_reset = "Stereo-usil-zal_reset"; // Имя топика для перезагрузки
byte Data[100] = {
    0, // master volume
    0, // surse Вход
    0, // Громкость ВЧ
    0, // Громкость CЧ
    0, // Громкость HЧ
    0, // Громкость САБ
    0,
    0,
    0,
    0,
    0, // EQ
    0, // EQ
    0, // EQ
    0, // EQ
    0, // EQ
    0, // EQ
    0, // EQ
    0, // EQ
    0, // EQ
    0, // EQ
    0, // EQ
    0, // EQ
    0, // EQ
    0, // EQ
    0, // EQ
    0, // EQ
    0,
    0,
    0,
    0,
    0, // Bluetooth mute
    0, // I2S_2 mute
    0, // PC_Reflex mute
    0, // I2S_4 mute
    0, // SPDIF mete
    0, // Компрессор
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0};
const char *eq_txt[16][2] = {{" 25 ", " Hz "},
                             {" 31 ", " Hz "},
                             {" 45 ", " Hz "},
                             {" 63 ", " Hz "},
                             {" 90 ", " Hz "},
                             {" 125", " Hz "},
                             {" 250", " Hz "},
                             {" 500", " Hz "},
                             {"1000", " Hz "},
                             {"2000", " Hz "},
                             {"4000", " Hz "},
                             {"6000", " Hz "},
                             {"8000", " Hz "},
                             {" 10 ", " KHz"},
                             {" 12 ", " KHz"},
                             {" 16 ", " KHz"}};
const byte lcd_eq[4] = {3, 2, 1, 0};
const int eq_stolb[33][5] = {{254, 254, 254, 254, -16},
                             {1, 254, 254, 254, -15},
                             {2, 254, 254, 254, -14},
                             {3, 254, 254, 254, -13},
                             {4, 254, 254, 254, -12},
                             {5, 254, 254, 254, -11},
                             {6, 254, 254, 254, -10},
                             {7, 254, 254, 254, -9},
                             {8, 254, 254, 254, -8},
                             {255, 1, 254, 254, -7},
                             {255, 2, 254, 254, -6},
                             {255, 3, 254, 254, -5},
                             {255, 4, 254, 254, -4},
                             {255, 5, 254, 254, -3},
                             {255, 6, 254, 254, -2},
                             {255, 7, 254, 254, -1},
                             {255, 8, 254, 254, 0},
                             {255, 255, 1, 254, 1},
                             {255, 255, 2, 254, 2},
                             {255, 255, 3, 254, 3},
                             {255, 255, 4, 254, 4},
                             {255, 255, 5, 254, 5},
                             {255, 255, 6, 254, 6},
                             {255, 255, 7, 254, 7},
                             {255, 255, 8, 254, 8},
                             {255, 255, 255, 1, 9},
                             {255, 255, 255, 2, 10},
                             {255, 255, 255, 3, 11},
                             {255, 255, 255, 4, 12},
                             {255, 255, 255, 5, 13},
                             {255, 255, 255, 6, 14},
                             {255, 255, 255, 7, 15},
                             {255, 255, 255, 8, 16}};
byte lastState = 0, menu_0 = 0, horiz_lev_mem = 0;
const int8_t increment[16] = {0, -1, 1, 0, 1, 0, 0, -1, -1, 0, 0, 1, 0, 1, -1, 0};
const word safeload[7] = {0x6000, 0x6001, 0x6002, 0x6003, 0x6004, 0x6005, 0x6006};
const word ADAUKEY[50] = {
    0x43E9, // ГРОМКОСТЬ ВЫХОД
    0x4341, // ГРОМКОСТЬ ВХОД
    0x43ED, // Громкость ВЧ
    0x43EE, // Громкость CЧ
    0x43EF, // Громкость HЧ
    0x43EA, // Громкость SAB
    0x0104, // EQ_25
    0x0109, // EQ_31
    0x604D, // EQ_45
    0x010E, // EQ_63
    0x6052, // EQ_90
    0x0113, // EQ_125
    0x6057, // EQ_250
    0x0118, // EQ_500
    0x605C, // EQ_1000
    0x011D, // EQ_2000
    0x012C, // EQ_4000
    0x6061, // EQ_6000
    0x0122, // EQ_8000
    0x6066, // EQ_10_kHz
    0x0127, // EQ_12_kHz
    0x606B, // EQ_16_kHz
    0x43C1, // Surse
    0x43BC, // Bluetooth mute mix
    0x43BD, // I2S_2 mute mix
    0x43BE, // PC_Reflex mute mix
    0x43BF, // I2S_4 mute mix
    0x43C0, // SPDIF mute mix
    0x43D5, // Компрессор
    0,
    0,
    0,
    0,
    0};

const byte surse_in[7][4] = {{0x00, 0x00, 0x00, 0x00},
                             {0x00, 0x00, 0x00, 0x02},
                             {0x00, 0x00, 0x00, 0x04},
                             {0x00, 0x00, 0x00, 0x06},
                             {0x00, 0x00, 0x00, 0x08},
                             {0x00, 0x00, 0x00, 0x0A},
                             {0x00, 0x00, 0x00, 0x0C}};

byte key_set1 = 0, key_set2 = 0, key_set3 = 0;
unsigned long ms_key = 0, ms_key_m;

void wi_fi_config() // Функция Настройки WiFi и OTA
{
  WiFi.setHostname(name_client); // Имя клиента в сети
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.waitForConnectResult() != WL_CONNECTED)
  {
    delay(5000);
    ESP.restart();
  }
  ArduinoOTA.setHostname(name_client); // Задаем имя сетевого порта
  ArduinoOTA.begin();                  // Инициализируем OTA
}

void wifi()
{
  ArduinoOTA.handle();     // Всегда готовы к прошивке
  client.loop();           // Проверяем сообщения и поддерживаем соедениние
  if (!client.connected()) // Проверка на подключение к MQTT
  {
    while (!client.connected())
    {
      if (client.connect(name_client)) // имя на сервере mqtt
      {
        client.subscribe(mqtt_reset); // подписались на топик
      }
      else
      {
        delay(3000);
      }
    }
  }
}

void callback(char *topic, byte *payload, unsigned int length) // Функция Приема сообщений
{
  String s = ""; // очищаем перед получением новых данных
  for (unsigned int i = 0; i < length; i++)
  {
    s = s + ((char)payload[i]); // переводим данные в String
  }
  int data = atoi(s.c_str()); // переводим данные в int
  // float data_f = atof(s.c_str()); //переводим данные в float
  if ((String(topic)) == mqtt_reset && data == 1)
  {
    ESP.restart();
  }
}

void publish_send(const char *top, float &ex_data) // Отправка Показаний с сенсоров
{
  char send_mqtt[10];
  dtostrf(ex_data, -2, 1, send_mqtt);
  client.publish(top, send_mqtt);
}

void lcd_print_int(byte x, byte y, int data_i /*пременая выводимая*/, byte end /*Сколько знаков зачищать*/)
{
  lcd.setCursor(x, y);
  lcd.print(data_i);
  byte len_p = 0;
  while (data_i)
  {
    data_i = data_i / 10;
    len_p++;
  }
  if (len_p > 0)
  {
    len_p = end - len_p;
    while (len_p)
    {
      lcd.write(254);
      len_p--;
    }
  }
}

void rus_txt() /*Знак Для Громкости*/
{
  byte im1[8] = {0b11000, 0b11000, 0b11000, 0b11000, 0b11000, 0b11000, 0b11000, 0b11000}; // |
  byte im2[8] = {0b11011, 0b11011, 0b11011, 0b11011, 0b11011, 0b11011, 0b11011, 0b11011}; // ||
  byte im3[8] = {0b11111, 0b10000, 0b10000, 0b10000, 0b10000, 0b10000, 0b10000, 0b00000}; // Г
  byte im4[8] = {0b10000, 0b10000, 0b10000, 0b11110, 0b10001, 0b10001, 0b11110, 0b00000}; // Ь
  byte im5[8] = {0b10001, 0b10001, 0b10001, 0b01111, 0b00001, 0b00001, 0b00001, 0b00000}; // Ч
  byte im6[8] = {0b10101,
                 0b10001,
                 0b10011,
                 0b10101,
                 0b11001,
                 0b10001,
                 0b10001,
                 0b00000}; // И
  byte im7[8] = {0b01110,
                 0b01010,
                 0b01010,
                 0b01010,
                 0b01010,
                 0b11111,
                 0b10001,
                 0b00000}; // Д
  byte im8[8] = {0b00011,
                 0b00101,
                 0b01001,
                 0b01001,
                 0b10001,
                 0b10001,
                 0b10001,
                 0b00000}; // Л
  lcd.createChar(1, im1);
  lcd.createChar(2, im2);
  lcd.createChar(3, im3);
  lcd.createChar(4, im4);
  lcd.createChar(5, im5);
  lcd.createChar(6, im6);
  lcd.createChar(7, im7);
  lcd.createChar(8, im8);
}
void rus_txt2() /*Знак Для Громкости*/
{
  byte im1[8] = {0b11111,
                 0b10001,
                 0b10001,
                 0b10001,
                 0b10001,
                 0b10001,
                 0b10001,
                 0b00000};                                                                // П
  byte im2[8] = {0b11011, 0b11011, 0b11011, 0b11011, 0b11011, 0b11011, 0b11011, 0b11011}; // ||
  byte im3[8] = {0b11111, 0b10000, 0b10000, 0b10000, 0b10000, 0b10000, 0b10000, 0b00000}; // Г
  byte im4[8] = {0b10000, 0b10000, 0b10000, 0b11110, 0b10001, 0b10001, 0b11110, 0b00000}; // Ь
  byte im5[8] = {0b10001, 0b10001, 0b10001, 0b01111, 0b00001, 0b00001, 0b00001, 0b00000}; // Ч
  byte im6[8] = {0b10101,
                 0b10001,
                 0b10011,
                 0b10101,
                 0b11001,
                 0b10001,
                 0b10001,
                 0b00000}; // И
  byte im7[8] = {0b01110,
                 0b01010,
                 0b01010,
                 0b01010,
                 0b01010,
                 0b11111,
                 0b10001,
                 0b00000}; // Д
  byte im8[8] = {0b00011,
                 0b00101,
                 0b01001,
                 0b01001,
                 0b10001,
                 0b10001,
                 0b10001,
                 0b00000}; // Л
  lcd.createChar(1, im1);
  lcd.createChar(2, im2);
  lcd.createChar(3, im3);
  lcd.createChar(4, im4);
  lcd.createChar(5, im5);
  lcd.createChar(6, im6);
  lcd.createChar(7, im7);
  lcd.createChar(8, im8);
}

void eq_bar() /*Знак Для эквалайзера*/
{
  byte im8[8] = {0b11111, 0b11111, 0b11111, 0b11111, 0b11111, 0b11111, 0b11111, 0b11111};
  byte im7[8] = {0b00000, 0b11111, 0b11111, 0b11111, 0b11111, 0b11111, 0b11111, 0b11111};
  byte im6[8] = {0b00000, 0b00000, 0b11111, 0b11111, 0b11111, 0b11111, 0b11111, 0b11111};
  byte im5[8] = {0b00000, 0b00000, 0b00000, 0b11111, 0b11111, 0b11111, 0b11111, 0b11111};
  byte im4[8] = {0b00000, 0b00000, 0b00000, 0b00000, 0b11111, 0b11111, 0b11111, 0b11111};
  byte im3[8] = {0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b11111, 0b11111, 0b11111};
  byte im2[8] = {0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b11111, 0b11111};
  byte im1[8] = {0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b11111};
  lcd.createChar(1, im1);
  lcd.createChar(2, im2);
  lcd.createChar(3, im3);
  lcd.createChar(4, im4);
  lcd.createChar(5, im5);
  lcd.createChar(6, im6);
  lcd.createChar(7, im7);
  lcd.createChar(8, im8);
}

void volume_txt() // Отрисовка надписи ГРОМКОСТЬ
{
  lcd.write(3);
  lcd.print("POMKOCT");
  lcd.write(4);
  lcd.print(":");
}

template <size_t dlina_c, size_t dlina_d> // Определяем длину массива

void I2C_ADAU1452_write(word device, const byte (&arr)[dlina_c][dlina_d], byte set, int kol_adress) // Передача данных в ADAU
{

  // Загрузка данных в безопасный буффер
  byte adr_start = 0, adr_end = 4;
  for (int i = 0; i < kol_adress; i++)
  {
    Wire.beginTransmission(ADAU1452);
    Wire.write(highByte(safeload[i]));
    Wire.write(lowByte(safeload[i]));

    for (adr_start; adr_start < adr_end;)
    {
      Wire.write(arr[set][adr_start]);
      adr_start++;
    }

    Wire.endTransmission();
    adr_end = adr_end + 4;
  }

  // Передаем начальный адресс для загрузки данных
  Wire.beginTransmission(ADAU1452);
  Wire.write(highByte(safeload[5]));
  Wire.write(lowByte(safeload[5]));

  byte *x = (byte *)&ADAUKEY[device];
  for (byte i = 4; i > 0;)
  {
    i--;
    Wire.write(x[i]);
  }
  Wire.endTransmission();

  // Передаем количество загруженых адрессов (онже тригер на применение)
  Wire.beginTransmission(ADAU1452);
  Wire.write(highByte(safeload[6]));
  Wire.write(lowByte(safeload[6]));

  for (size_t i = 0; i < 3; i++)
  {
    Wire.write(0x00);
  }
  Wire.write(kol_adress);
  Wire.endTransmission();
  delayMicroseconds(500);
}

void horizont_level(byte lev, byte stroka) // Отрисовка горизонтальной полосы (Громкость)
{
  byte vol_lev = map(lev, 0, 120, 0, 39);

  if (vol_lev != horiz_lev_mem)
  {
    horiz_lev_mem = vol_lev;
    byte im;
    if (vol_lev % 2 == 0)
    {
      im = 1;
    }
    else
    {
      im = 2;
    }
    vol_lev = vol_lev / 2;
    lcd.setCursor(vol_lev, 3);
    lcd.write(im);
    if (vol_lev != 19)
    {
      lcd.write(254);
    }
  }
}

void surse() // Преключение входов
{
  lcd.setCursor(0, 0);
  lcd.print(Data[1]);
  lcd.write(126);
  lcd.setCursor(2, 0);

  switch (Data[1])
  {
  case 0:
    lcd.print("Bluetooth");
    I2C_ADAU1452_write(22, surse_in, Data[1], 1);
    break;
  case 1:
    lcd.print("I2S_2    ");
    I2C_ADAU1452_write(22, surse_in, Data[1], 1);
    break;
  case 2:
    lcd.print("PC_Reflex");
    I2C_ADAU1452_write(22, surse_in, Data[1], 1);
    break;
  case 3:
    lcd.print("I2S_4    ");
    I2C_ADAU1452_write(22, surse_in, Data[1], 1);
    break;
  case 4:
    lcd.print("SPDIF    ");
    I2C_ADAU1452_write(22, surse_in, Data[1], 1);
    break;
  case 5:
    lcd.print("Mixer    ");
    I2C_ADAU1452_write(22, surse_in, Data[1], 1);
    break;

  default:
    break;
  }
}

void EQ() // Меню эквалайзера
{
  Data[key_set1 + 10] = pos;
  byte eq_g = map(pos, 0, 120, 0, 32);

  if (!menu_0) // Загружаем данные если это первый вход
  {
    eq_bar();
    lcd.clear();
    lcd.setCursor(16, 0);
    lcd.print(" EQ");
    for (size_t ip = 0; ip < 16; ip++)
    {
      eq_g = Data[ip + 10];
      eq_g = map(eq_g, 0, 120, 0, 32);
      for (size_t i = 0; i < 4; i++)
      {
        lcd.setCursor(ip, lcd_eq[i]);
        lcd.write(eq_stolb[eq_g][i]);
      }
    }
    lcd.setCursor(16, 1);
    lcd.print(eq_txt[key_set1][0]);
    lcd.setCursor(16, 2);
    lcd.print(eq_txt[key_set1][1]);
    lcd.setCursor(17, 3);

    if (eq_stolb[eq_g][4] > 0)
    {
      lcd.print("+");
    }
    else if (eq_stolb[eq_g][4] == 0)
    {
      lcd.print(" ");
    }
    lcd.print(eq_stolb[eq_g][4]);
    menu_0 = 1;
  }

  if (key_set1 != eq_flag1) // Вывод текста настраевоемой частоты
  {
    lcd.setCursor(16, 1);
    lcd.print(eq_txt[key_set1][0]);
    lcd.setCursor(16, 2);
    lcd.print(eq_txt[key_set1][1]);
    eq_flag1 = key_set1;
  }

  if ((eq_g != eq_int_flag1)) // Проверяем пора ли обновить данные
  {
    lcd.setCursor(17, 3);
    lcd.print("   ");
    lcd.setCursor(17, 3);

    if (eq_stolb[eq_g][4] > 0)
    {
      lcd.print("+");
    }
    else if (eq_stolb[eq_g][4] == 0)
    {
      lcd.print(" ");
    }
    lcd.print(eq_stolb[eq_g][4]);
    eq_int_flag1 = eq_g;

    for (size_t i = 0; i < 4; i++)
    {
      lcd.setCursor(key_set1, lcd_eq[i]);
      lcd.write(eq_stolb[eq_g][i]);
    }
    switch (key_set1)
    {
    case 0:
      I2C_ADAU1452_write(6, eq_25_Hz, eq_g, 5);
      break;
    case 1:
      I2C_ADAU1452_write(7, eq_31_Hz, eq_g, 5);
      break;
    case 2:
      I2C_ADAU1452_write(8, eq_45_Hz, eq_g, 5);
      break;
    case 3:
      I2C_ADAU1452_write(9, eq_63_Hz, eq_g, 5);
      break;
    case 4:
      I2C_ADAU1452_write(10, eq_90_Hz, eq_g, 5);
      break;
    case 5:
      I2C_ADAU1452_write(11, eq_125_Hz, eq_g, 5);
      break;
    case 6:
      I2C_ADAU1452_write(12, eq_250_Hz, eq_g, 5);
      break;
    case 7:
      I2C_ADAU1452_write(13, eq_500_Hz, eq_g, 5);
      break;
    case 8:
      I2C_ADAU1452_write(14, eq_1000_Hz, eq_g, 5);
      break;
    case 9:
      I2C_ADAU1452_write(15, eq_2000_Hz, eq_g, 5);
      break;
    case 10:
      I2C_ADAU1452_write(16, eq_4000_Hz, eq_g, 5);
      break;
    case 11:
      I2C_ADAU1452_write(17, eq_6000_Hz, eq_g, 5);
      break;
    case 12:
      I2C_ADAU1452_write(18, eq_8000_Hz, eq_g, 5);
      break;
    case 13:
      I2C_ADAU1452_write(19, eq_10_kHz, eq_g, 5);
      break;
    case 14:
      I2C_ADAU1452_write(20, eq_12_kHz, eq_g, 5);
      break;
    case 16:
      I2C_ADAU1452_write(21, eq_16_kHz, eq_g, 5);
      break;
    default:
      break;
    }
    t_eeprom.setTimeout(5000);
  }
}

void volume_crosover(byte chanel)
{
  if (!menu_0)
  {
    rus_txt();
    delay(100);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.write(3);
    lcd.print("POMKOCT");
    lcd.write(4);
    if (chanel == 2)
    {
      lcd.print(" B");
    }
    else if (chanel == 3)
    {
      lcd.print(" C");
    }
    else if (chanel == 4)
    {
      lcd.print(" H");
    }
    else if (chanel == 5)
    {
      lcd.print(" SAB");
    }

    if (chanel != 5)
    {
      lcd.write(5);
    }
    lcd.print(":");
    lcd.setCursor(0, 1);
    lcd.print("Dbu:");
    for (size_t i = 0; i < Data[chanel]; i++)
    {
      horizont_level(i, 3);
    }
    menu_0 = 1;
  }

  Data[chanel] = pos;
  byte pos_vol = map(Data[chanel], 0, 120, 0, 69); // vch volume
  byte vol_procent = map(pos_vol, 0, 69, 0, 100);
  byte vol_db = map(pos_vol, 0, 69, 69, 0);
  if (chanel != 5)
  {
    lcd_print_int(13, 0, vol_procent, 3);
  }
  else
  {
    lcd_print_int(14, 0, vol_procent, 3); // уровень в %
  }
  lcd_print_int(5, 1, -vol_db, 3); // уровень в dbu
  I2C_ADAU1452_write(chanel, vol_70db, pos_vol, 1);
  horizont_level(Data[chanel], 3);
}

void master_volume_in() // Уравление громкостью
{
  if (menu_0)
  {
    rus_txt();
    lcd.clear();
    surse();
    lcd.setCursor(0, 2);
    volume_txt();
    for (size_t i = 0; i < Data[0]; i++)
    {
      horizont_level(i, 3);
    }
    menu_0 = 0;
  }

  text_menu.setTimeout(1000);
  Data[0] = pos;
  byte pos_vol = map(Data[0], 0, 120, 0, 120); // master volume
  byte vol_procent = map(pos_vol, 0, 120, 0, 100);
  lcd_print_int(10, 2, vol_procent, 3);
  I2C_ADAU1452_write(0, mast_vol, pos_vol, 1);
  horizont_level(Data[0], 3);
}

void button() // Обработка кнопки энкодера
{

  t_eeprom.setTimeout(5000);

  switch (key_set2) // Основное меню
  {
  case 0: // Переключение входов

    if (menu_0)
    {
      pos = Data[0];
      master_volume_in();
      surse();
    }
    else
    {
      Data[1] = key_set1;
      surse();
    }
    tab_key = 5; // глубина меню
    break;
  case 1: // Настройка Эквалайзера
    pos = Data[key_set1 + 10];
    tab_key = 15; // глубина меню
    EQ();
    break;
  case 2: // Настройка Кроссовера

    switch (key_set1)
    {
    case 0: // Настройка Кроссовера главная
      rus_txt();
      lcd.clear();
      lcd.setCursor(6, 0);
      lcd.print("HACTPO");
      lcd.write(6);
      lcd.print("KA");
      lcd.setCursor(5, 2);
      lcd.print("KPOCCOBEPA");
      break;
    case 1: // Настройка Громкости BЧ выхода
      menu_0 = 0;
      pos = Data[2];
      volume_crosover(2);
      break;
    case 2: // Настройка Громкости CЧ выхода
      menu_0 = 0;
      pos = Data[3];
      volume_crosover(3);
      break;
    case 3: // Настройка Громкости HЧ выхода
      menu_0 = 0;
      pos = Data[4];
      volume_crosover(4);
      break;
    case 4: // Настройка Громкости SAB выхода
      menu_0 = 0;
      pos = Data[5];
      volume_crosover(5);
      break;
    default:
      key_set1 = 1;
      button();
      break;
    }
    break;
  case 3: // Настройка MUTE ВХОДОВ Mixer
    switch (key_set1)
    {
    case 0:
      lcd.clear();
      lcd.setCursor(5, 0);
      lcd.print("OTK BXO");
      lcd.write(7);
      lcd.print("OB");
      lcd.setCursor(7, 1);
      lcd.print("Mixer");
      break;
    case 1:
      lcd.setCursor(3, 2);
      lcd.print("Bluetooh:");
      if (Data[30])
      {
        lcd.print("BK");
        lcd.write(8);
      }
      else
      {
        lcd.print("OTK");
      }
      I2C_ADAU1452_write(23, mute, Data[30], 1);
      break;
    case 2:
      lcd.setCursor(3, 2);
      lcd.print("I2S_2:");
      if (Data[31])
      {
        lcd.print("BK");
        lcd.write(8);
        lcd.print("   ");
      }
      else
      {
        lcd.print("OTK   ");
      }
      I2C_ADAU1452_write(24, mute, Data[31], 1);
      break;
    case 3:
      lcd.setCursor(3, 2);
      lcd.print("PC_Reflex:");
      if (Data[32])
      {
        lcd.print("BK");
        lcd.write(8);
      }
      else
      {
        lcd.print("OTK");
      }
      I2C_ADAU1452_write(25, mute, Data[32], 1);
      break;
    case 4:
      lcd.setCursor(3, 2);
      lcd.print("I2S_4:");
      if (Data[33])
      {
        lcd.print("BK");
        lcd.write(8);
        lcd.print("     ");
      }
      else
      {
        lcd.print("OTK   ");
      }
      I2C_ADAU1452_write(26, mute, Data[33], 1);
      break;
    case 5:
      lcd.setCursor(3, 2);
      lcd.print("SPDIF:");
      if (Data[34])
      {
        lcd.print("BK");
        lcd.write(8);
        lcd.print("   ");
      }
      else
      {
        lcd.print("OTK   ");
      }
      I2C_ADAU1452_write(27, mute, Data[34], 1);
      break;
    default:
      key_set1 = 1;
      button();
      break;
    }
    break;
  case 4: // Настройка Компрессор
    rus_txt2();
    lcd.clear();
    lcd.setCursor(4, 1);
    lcd.print("KOM");
    lcd.write(1);
    lcd.print("PECCOP");
    lcd.setCursor(4, 2);
    if (Data[35])
    {
      lcd.print("BK");
      lcd.write(8);
      I2C_ADAU1452_write(28, kompressor, Data[35], 1);
    }
    else
    {
      lcd.print("OTK   ");
      I2C_ADAU1452_write(28, kompressor, Data[35], 1);
    }

    break;
  case 50: // Настройка

    break;

  default:
    menu_0 = 1;
    key_set2 = 0;
    pos = Data[0];
    key_set1 = Data[1];
    master_volume_in();
    surse();
    t_eeprom.setTimeout(5000);
    break;
  }
}

void loop()
{
  state = digitalRead(CLK) | (digitalRead(DT) << 1);
  state_key = digitalRead(KEY);

  if (OTA_Wifi.isReady()) // Поддержание "WiFi" и "OTA"
  {
    wifi();
  }

  if (dub_klik.isReady()) // энкодер кнопка + двойной клик
  {
    if (klik == 1)
    {
      if (key_set1 < tab_key)
      {
        key_set1++;
      }
      else
        key_set1 = 0;
    }
    else if (klik > 1)
    {
      if (key_set1 > 0)
      {
        key_set1--;
      }
    }
    klik = 0;
    button();
    lcd_2.setCursor(0, 1);
    lcd_2.print("key_1:");
    lcd_2.print(key_set1);
    lcd_2.print(" key_2:");
    lcd_2.print(key_set2);
  }

  if (state != lastState) // энкодер вращение
  {
    pos += increment[state | (lastState << 2)];
    if (pos < 0)
    {
      pos = 0;
    }
    else if (pos > 120)
    {
      pos = 120;
    }
    lastState = state;

    if ((pos > pos_state_raw + 4) || (pos < pos_state_raw - 4))
    {
      pos_state_raw = pos;
      pos_state = !pos_state;
    }

    if (key_set2 == 0)
    {
      t_eeprom.setTimeout(5000);
      master_volume_in();
    }
    else if (key_set2 == 1)
    {
      EQ();
    }
    else if (key_set2 == 2)
    {
      switch (key_set1)
      {
      case 1: // Настройка Громкости BЧ выхода
        volume_crosover(2);
        break;
      case 2: // Настройка Громкости CЧ выхода
        volume_crosover(3);
        break;
      case 3: // Настройка Громкости HЧ выхода
        volume_crosover(4);
        break;
      case 4: // Настройка Громкости SAB выхода
        volume_crosover(5);
        break;

      default:
        break;
      }
    }
    else if (key_set2 == 3)
    {
      switch (key_set1)
      {
      case 1: // Bluetooth mute
        if (Data[30] != pos_state)
        {
          Data[30] = pos_state;
          button();
        }
        break;
      case 2: // I2S_2 mute
        if (Data[31] != pos_state)
        {
          Data[31] = pos_state;
          button();
        }
        break;
      case 3: // PC_Reflex mute
        if (Data[32] != pos_state)
        {
          Data[32] = pos_state;
          button();
        }
        break;
      case 4: // I2S_4 mute
        if (Data[33] != pos_state)
        {
          Data[33] = pos_state;
          button();
        }
        break;
      case 5: // SPIDF mete
        if (Data[34] != pos_state)
        {
          Data[34] = pos_state;
          button();
        }
        break;

      default:
        break;
      }
    }
    else if (key_set2 == 4) // Компрессор
    {
      if (Data[35] != pos_state)
      {
        Data[35] = pos_state;
        button();
      }
    }
    else if (key_set2 == 5)
    {
      // volume_crosover(5);
    }
    // delayMicroseconds(500);
    lcd_2.setCursor(0, 2);
    lcd_2.print("enc:");
    lcd_2.print("    ");
    lcd_2.setCursor(4, 2);
    lcd_2.print(pos);
  }

  if (state_key & !flag_key) // энкодер кнопка
  {
    ms_key_m = millis();
    flag_key = 1;
  }
  else if (flag_key & !state_key) // энкодер кнопка
  {
    ms_key = millis() - ms_key_m;
    flag_key = 0;
    lcd_2.setCursor(1, 0);
    lcd_2.print(ms_key);
    lcd_2.print("        ");

    if ((ms_key > 40) & (ms_key < 350))
    {
      klik++;
      dub_klik.setTimeout(DOUBLE_CLIK);
    }

    if (ms_key > 1000 & ms_key < 3000)
    {
      if (key_set2 < 5)
      {
        key_set2++;
        menu_0 = 0;
        key_set1 = 0;

        if (key_set2 == 0)
        {
          pos = Data[0];
          key_set1 = Data[1];
        }
      }
      else
      {
        key_set2 = 0;
        key_set1 = 0;
      }

      lcd_2.setCursor(0, 1);
      lcd_2.print("key_1:");
      lcd_2.print(key_set1);
      lcd_2.print(" key_2:");
      lcd_2.print(key_set2);
      button();
    }

    if (ms_key > 3000)
    { // Выход из Меню при длительном удержаний
      menu_0 = 1;
      key_set2 = 0;
      pos = Data[0];
      master_volume_in();
      surse();
      t_eeprom.setTimeout(5000);
    }
  }

  if (t_eeprom.isReady()) // заись в память
  {
    for (size_t i = 0; i < 100; i++)
    {
      if (ee.readByte(i) != Data[i])
      {
        ee.writeByte(i, Data[i]);
      }
    }
  }
}

void setup()
{

  Wire.begin(GPIO_NUM_21, GPIO_NUM_22);
  lcd.begin(GPIO_NUM_21, GPIO_NUM_22);
  lcd_2.begin(GPIO_NUM_21, GPIO_NUM_22);
  ee.begin();
  Wire.setClock(400000);
  wi_fi_config();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  OTA_Wifi.setInterval(20); // настроить интервал
  OTA_Wifi.setMode(AUTO);   // Авто режим
  Serial.begin(9600);
  t_eeprom.setMode(MANUAL);
  text_menu.setMode(MANUAL);
  dub_klik.setMode(MANUAL);

  for (size_t i = 0; i < 100; i++) // Читаем данные с Памяти EEPROM
  {
    Data[i] = ee.readByte(i);
  }
  // Data[2] = 54;
  // Data[3] = 54;
  // Data[4] = 69;
  rus_txt();
  lcd.backlight();
  lcd.clear();
  lcd_2.backlight();
  lcd_2.clear();
  lcd_2.setCursor(5, 0);
  lcd_2.print("Test");
  lcd.setCursor(0, 2);
  volume_txt();
  key_set1 = Data[1];
  surse();

  for (size_t i = 0; i < Data[0]; i++) // устоновка выходной громкости на прежний кровень
  {
    pos = i;
    byte pos_vol = map(pos, 0, 120, 0, 120);
    byte vol_procent = map(pos_vol, 0, 120, 0, 100);
    lcd_print_int(10, 2, vol_procent, 3);
    I2C_ADAU1452_write(0, mast_vol, pos_vol, 1);
    horizont_level(pos, 3);
    delay(5);
  }
  // устоновка громкости красовера
  for (size_t i = 2; i < 5; i++)
  {
    byte pos_vol = map(Data[i], 0, 120, 0, 69);
    I2C_ADAU1452_write(i, vol_70db, pos_vol, 1);
  }
  // Настройка Mixera
  I2C_ADAU1452_write(23, mute, Data[30], 1);
  I2C_ADAU1452_write(24, mute, Data[31], 1);
  I2C_ADAU1452_write(25, mute, Data[32], 1);
  I2C_ADAU1452_write(26, mute, Data[33], 1);
  I2C_ADAU1452_write(27, mute, Data[34], 1);
  // Компрессор
  I2C_ADAU1452_write(28, kompressor, Data[35], 1);
}