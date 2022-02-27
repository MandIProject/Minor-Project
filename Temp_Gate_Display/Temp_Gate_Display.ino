#include <Servo.h>
#include <Adafruit_MLX90614.h>
#include <Arduino.h>
#include <ESP8266WiFi.h>

#include <U8g2lib.h>
#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif

#define RED_LED D6
#define GREEN_LED D3
#define YELLOW_LED D5
#define BUTTON D7
#define SERVO_PIN D4

U8G2_SSD1306_128X64_NONAME_1_SW_I2C u8g2(U8G2_R0, /* clock=*/ SCL, /* data=*/ SDA, /* reset=*/ -1);
Servo myservo;
Adafruit_MLX90614 mlx = Adafruit_MLX90614();

const char *auth = "a0SHUV6doj1fws8b01D8dG6ujFHqaFa7";

const char *ssid = "Wifi_2ghz";
const char *pass = "Voidsetup()1";

const char *host = "blynk-cloud.com";
const unsigned int port = 8080;

bool button_status = false;
float temp = 0.0;
String response;

WiFiClient client;

ICACHE_RAM_ATTR void button_pressed()
{
  button_status = true;
}

void connectNetwork()
{
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi connected");
}

bool httpRequest(const String& method,
                 const String& request,
                 String&       response)
{
  Serial.print(F("Connecting to "));
  Serial.print(host);
  Serial.print(":");
  Serial.print(port);
  Serial.print("... ");
  if (client.connect(host, port)) {
    Serial.println("OK");
  } else {
    Serial.println("failed");
    return false;
  }

  client.print(method); client.println(F(" HTTP/1.1"));
  client.print(F("Host: ")); client.println(host);
  client.println(F("Connection: close"));
  if (request.length()) {
    client.println(F("Content-Type: application/json"));
    client.print(F("Content-Length: ")); client.println(request.length());
    client.println();
    client.print(request);
  } else {
    client.println();
  }

  //Serial.println("Waiting response");
  int timeout = millis() + 5000;
  while (client.available() == 0) {
    if (timeout - millis() < 0) {
      Serial.println(">>> Client Timeout !");
      client.stop();
      return false;
    }
  }

  //Serial.println("Reading response");
  int contentLength = -1;
  while (client.available()) {
    String line = client.readStringUntil('\n');
    line.trim();
    line.toLowerCase();
    if (line.startsWith("content-length:")) {
      contentLength = line.substring(line.lastIndexOf(':') + 1).toInt();
    } else if (line.length() == 0) {
      break;
    }
  }

  //Serial.println("Reading response body");
  response = "";
  response.reserve(contentLength + 1);
  while (response.length() < contentLength && client.connected()) {
    while (client.available()) {
      char c = client.read();
      response += c;
    }
  }
  client.stop();
  return true;
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  delay(10);

  pinMode(RED_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(YELLOW_LED, OUTPUT);
  pinMode(BUTTON, INPUT);
  attachInterrupt(digitalPinToInterrupt(BUTTON), button_pressed, FALLING);
  
  u8g2.begin();
  u8g2.enableUTF8Print();
  myservo.attach(SERVO_PIN);

  if (!mlx.begin()) {
    Serial.println("Error connecting to MLX sensor. Check wiring.");
    while (1);
  };

  connectNetwork();
}

void write_Temp(float temp){
  Serial.print("Sending value: ");
  Serial.println(temp);

  String putData = String("[\"") + temp + "\"]";
  if (httpRequest(String("PUT /") + auth + "/update/V12", putData, response)) {
    if (response.length() != 0) {
      Serial.print("WARNING: ");
      Serial.println(response);
    }
  }
}

void loop() {
  // put your main code here, to run repeatedly:
  if(button_status == true){
      Serial.println("Button Pressed");
      temp = mlx.readObjectTempC();
      if(isnan(temp)){
        Serial.println("Error reading");
        return;
      }
      Serial.print(temp);
      Serial.println("°C");
      u8g2.setFont(u8g2_font_logisoso16_tf);
      u8g2.firstPage();
      do {
        u8g2.setCursor(0, 20);
        u8g2.print(temp);
        u8g2.println("°C");
      } while ( u8g2.nextPage() );
      delay(2000);
      write_Temp(temp);
      if(temp >= 30 && temp <= 37){
        digitalWrite(GREEN_LED, HIGH);
        digitalWrite(RED_LED, LOW);
        digitalWrite(YELLOW_LED, LOW);
        u8g2.setFont(u8g2_font_logisoso16_tf);
        u8g2.firstPage();
        do {
          u8g2.setCursor(0, 20);
          u8g2.println(F("Entry"));
          u8g2.setCursor(0, 50);
          u8g2.println(F("Allowed"));
        } while ( u8g2.nextPage() );
        myservo.write(180);
        delay(2000);
        myservo.write(0);
      }else{
        digitalWrite(GREEN_LED, LOW);
        digitalWrite(RED_LED, HIGH);
        digitalWrite(YELLOW_LED, LOW);
        u8g2.setFont(u8g2_font_logisoso16_tf);
        u8g2.firstPage();
        do {
          u8g2.setCursor(0, 20);
          u8g2.println(F("Not"));
          u8g2.setCursor(0, 50);
          u8g2.println(F("Allowed"));
        } while ( u8g2.nextPage() );
        myservo.write(0);
        delay(2000);
      }
    button_status = false;
  }else if(button_status == false){
    digitalWrite(GREEN_LED, LOW);
    digitalWrite(RED_LED, LOW);
    digitalWrite(YELLOW_LED, HIGH);
    u8g2.setFont(u8g2_font_logisoso16_tf);
    u8g2.firstPage();
    do {
      u8g2.setCursor(0, 20);
      u8g2.println(F("Wait!"));
    } while ( u8g2.nextPage() );
  }
}
