#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define DHTPIN 4
#define DHTTYPE DHT11   // DHT22 kullaniyorsan DHT22 yap

const char* ssid = "WIFI_ADIN";
const char* password = "WIFI_SIFREN";

#define BOT_TOKEN "TELEGRAM_BOT_TOKEN"
#define CHAT_ID "TELEGRAM_CHAT_ID"

DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2); // Calismazsa 0x3F dene

WiFiClientSecure client;
UniversalTelegramBot bot(BOT_TOKEN, client);

unsigned long lastReadTime = 0;
unsigned long lastBotCheckTime = 0;

const unsigned long readInterval = 2000;
const unsigned long botCheckInterval = 500;

float temperature = 0;
float humidity = 0;
bool sensorOk = false;

void setup() {
  Serial.begin(115200);

  dht.begin();

  Wire.begin(21, 22); // SDA, SCL
  lcd.init();
  lcd.backlight();

  lcd.setCursor(0, 0);
  lcd.print("WiFi baglaniyor");

  WiFi.begin(ssid, password);
  client.setInsecure();

  bot.longPoll = 1;

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi baglandi");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("WiFi baglandi");
  lcd.setCursor(0, 1);
  lcd.print("/durum yaz");

  bot.sendMessage(CHAT_ID, "Sistem basladi. Sicaklik ve nem icin /durum yaz.", "");
}

void loop() {
  unsigned long currentTime = millis();

  if (currentTime - lastReadTime >= readInterval) {
    lastReadTime = currentTime;

    humidity = dht.readHumidity();
    temperature = dht.readTemperature();

    if (isnan(humidity) || isnan(temperature)) {
      sensorOk = false;

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Sensor hatasi");

      Serial.println("Sensor okunamadi!");
    } else {
      sensorOk = true;

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Sicaklik:");
      lcd.print(temperature, 1);
      lcd.print("C");

      lcd.setCursor(0, 1);
      lcd.print("Nem:");
      lcd.print(humidity, 1);
      lcd.print("%");

      Serial.print("Sicaklik: ");
      Serial.print(temperature);
      Serial.print(" C  Nem: ");
      Serial.print(humidity);
      Serial.println(" %");
    }
  }

  if (currentTime - lastBotCheckTime >= botCheckInterval) {
    lastBotCheckTime = currentTime;

    int newMessages = bot.getUpdates(bot.last_message_received + 1);

    while (newMessages) {
      for (int i = 0; i < newMessages; i++) {
        String chat_id = bot.messages[i].chat_id;
        String text = bot.messages[i].text;

        Serial.print("Gelen mesaj: ");
        Serial.println(text);
        Serial.print("Chat ID: ");
        Serial.println(chat_id);

        if (chat_id != CHAT_ID) {
          bot.sendMessage(chat_id, "Yetkisiz kullanici.", "");
          continue;
        }

        if (text == "/durum") {
          if (!sensorOk) {
            bot.sendMessage(CHAT_ID, "Sensor okunamadi. Baglantiyi kontrol et.", "");
          } else {
            String message = "Anlik ortam bilgisi:\n";
            message += "Sicaklik: " + String(temperature, 1) + " C\n";
            message += "Nem: " + String(humidity, 1) + " %";

            bot.sendMessage(CHAT_ID, message, "");
          }
        } else if (text == "/start") {
          bot.sendMessage(CHAT_ID, "Komutlar:\n/durum - Sicaklik ve nem bilgisini goster", "");
        } else {
          bot.sendMessage(CHAT_ID, "Bilinmeyen komut. /durum yaz.", "");
        }
      }

      newMessages = bot.getUpdates(bot.last_message_received + 1);
    }
  }
}