/*Мониторинг движения через Telegram: пошаговое руководство для ESP8266 и PIR-датчика
 https://dzen.ru/a/Z52hiE1fTRXRXTIJ*/

#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>

// Замените на свои сетевые данные
#define WIFI_SSID "your_SSID" // Замените на имя вашей Wi-Fi сети
#define WIFI_PASSWORD "your_PASSWORD" // Замените пароль вашей Wi-Fi сети

// Инициализация Telegram бота
#define BOT_TOKEN "your_BOT_TOKEN" // Ваш Токен
#define CHAT_ID "your_CHAT_ID" // Замените на ID вашего пользователя, не группы

const unsigned long BOT_MTBS = 1000; // среднее время между сообщениями сканирования

X509List cert(TELEGRAM_CERTIFICATE_ROOT);
WiFiClientSecure secured_client;
UniversalTelegramBot bot(BOT_TOKEN, secured_client);
unsigned long bot_lasttime; // последнее сканирование сообщений было сделано

const int PIR_PIN = D5;     // PIR-датчик подключен к D5
int previousPirState = LOW; // Переменная для хранения предыдущего состояния датчика

void handleNewMessages(int numNewMessages)
{
  Serial.print("Обработка новых сообщений: ");
  Serial.println(numNewMessages);

  for (int i = 0; i < numNewMessages; i++)
  {
    String chat_id = bot.messages[i].chat_id;
    String text = bot.messages[i].text;

    text.replace("@" + String(BOT_TOKEN).substring(0, String(BOT_TOKEN).indexOf(':')), "");
    String from_name = bot.messages[i].from_name;
    if (from_name == "")
      from_name = "Guest";

    if (text == "/status")
    {
      int pirState = digitalRead(PIR_PIN);
      if (pirState == HIGH)
      {
        if (bot.sendMessage(chat_id, "Движение обнаружено", ""))
        {
          Serial.println("Отправлен статус движения: Движение обнаружено");
        }
        else
        {
          Serial.println("Не удалось отправить сообщение о движении в Telegram");
        }
      }
      else
      {
        if (bot.sendMessage(chat_id, "Движение отсутствует", ""))
        {
          Serial.println("Отправлен статус движения: Движение отсутствует");
        }
        else
        {
          Serial.println("Не удалось отправить сообщение об отсутствии движения в Telegram");
        }
      }
    }
    if (text == "/start")
    {
      String welcome = "Добро пожаловать, " + from_name + ".\n";
      welcome += "Этот бот отслеживает состояние PIR-датчика.\n\n";
      welcome += "------------------------------\n";
      welcome += "/status : показать текущий статус движения\n";
      welcome += "------------------------------\n";
      bot.sendMessage(chat_id, welcome, "");
    }
  }
}

void setup()
{
  Serial.begin(115200);
  Serial.println();
    pinMode(PIR_PIN, INPUT);

  // Подключение к Wi-Fi
  configTime(0, 0, "pool.ntp.org");
  secured_client.setTrustAnchors(&cert);
  Serial.print("Соединяемся с Wi-Fi ");
  Serial.print(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 10)
  {
    Serial.print(".");
    delay(500);
    attempts++;
  }

  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("\nНе удалось подключиться к Wi-Fi!");
    while (true)
      ; // Зависаем здесь
  }
  Serial.print("\nСоединились. IP адрес: ");
  Serial.println(WiFi.localIP());

  Serial.print("Получение времени: ");
  time_t now = time(nullptr);
  int time_attempts = 0;
  while (now < 24 * 3600 && time_attempts < 10)
  {
    Serial.print(".");
    delay(100);
    now = time(nullptr);
    time_attempts++;
  }
  if (now < 24 * 3600) {
     Serial.println("\nНе удалось получить время с сервера NTP!");
     while (true); // Зависаем здесь
  }

  Serial.println(now);

  // Ожидание стабилизации PIR-датчика
  Serial.println("Инициализация PIR-датчика...");
  delay(60000); // Ожидание 1 минуту
  Serial.println("PIR-датчик готов к работе.");
}

void loop()
{
  // Проверка новых сообщений от Telegram
  if (millis() - bot_lasttime > BOT_MTBS)
  {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    while (numNewMessages)
    {
      Serial.println("Ответ получен");
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
    bot_lasttime = millis();
  }

  // Проверка подключения к Wi-Fi
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("Потеряно соединение с Wi-Fi. Переподключение...");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    delay(5000);
  }
  // Чтение состояния PIR-датчика
  int pirState = digitalRead(PIR_PIN);

  if (pirState != previousPirState)
  {
    delay(500);
    if (pirState == HIGH)
    {
      Serial.println("Отправка сообщения: Движение обнаружено!");
        if (bot.sendMessage(CHAT_ID, "Движение обнаружено!", ""))
          {
          Serial.println("Сообщение отправлено в Telegram.");
         }
         else
          {
           Serial.println("Не удалось отправить сообщение в Telegram");
          }
    }
    previousPirState = pirState;
  }
}
