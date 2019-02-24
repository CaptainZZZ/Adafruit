#include <DHT.h>
#include <Adafruit_MQTT.h>
#include <Adafruit_MQTT_Client.h>
#include <ESP8266WiFi.h>
#define WLAN_SSID       ""
#define WLAN_PASS       ""
#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883
#define AIO_USERNAME    ""
#define AIO_KEY         ""
#define DHTPIN 14
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);
WiFiClient client;
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_USERNAME, AIO_KEY);
Adafruit_MQTT_Publish temperature = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/temperature");
Adafruit_MQTT_Publish humidity = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/humidity");
Adafruit_MQTT_Publish Net = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/Net");
Adafruit_MQTT_Subscribe ledonoff = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/LED");
void setup() {
  dht.begin();
  Serial.begin(9600);
  delay(10);
  // Connect to WiFi access point.
  Serial.println(); Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WLAN_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WLAN_SSID, WLAN_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("WiFi connected");
  Serial.println("IP address: "); Serial.println(WiFi.localIP());
  mqtt.subscribe(&ledonoff);
}
uint32_t x = 0;
int accu_time = 0;
int pub_time = 10000;
void loop() {
  
  //String net_info_str= String("Network SSID "+WiFi.SSID(0)+", RSSI "+WiFi.RSSI(0)+" dBm");
  float t, rh;
  //net_info_str.toCharArray(net_info,net_info_str.length());
  //Serial.println(net_info_str);
  //Serial.println(net_info);
  MQTT_connect();
  if (accu_time >= pub_time) {
    WiFi.scanNetworks();
    Serial.print(WiFi.SSID(0));
    Serial.print(",");
    Serial.print(WiFi.RSSI(0));
    Serial.print("dbm,channel ");
    Serial.print(WiFi.channel(0));
    Serial.print("\n");
    t = (float)dht.readTemperature();
    rh = (float)dht.readHumidity();
    temperature.publish(t);
    humidity.publish(rh);
    Net.publish(WiFi.RSSI(0));
    analogWrite(2, 300);
    delay(100);
    analogWrite(2, 1024);
    accu_time = 0;
  }
  accu_time = accu_time + 300;
  delay(250);
  Adafruit_MQTT_Subscribe *subscription;
  while ((subscription = mqtt.readSubscription(100))) {
    // Check if its the onoff button feed
    if (subscription == &ledonoff) {
      Serial.print(F("On-Off button: "));
      Serial.println((char *)ledonoff.lastread);
      if (strcmp((char *)ledonoff.lastread, "ON") == 0) {
        analogWrite(13, 0);
        analogWrite(12, 0);
      }
      if (strcmp((char *)ledonoff.lastread, "OFF") == 0) {
        analogWrite(13, 1023);
        analogWrite(12, 1023);
      }
    }
  }
}
void MQTT_connect() {
  int8_t ret;
  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }
  Serial.print("Connecting to MQTT... ");
  int retry_interval = 10000;
  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
    Serial.println(mqtt.connectErrorString(ret));
    Serial.println("Retrying MQTT connection in 10 seconds...");
    mqtt.disconnect();
    delay(retry_interval);
    fade(1, 2);
    fade(1, 13);
    retry_interval = retry_interval * 2;
    if (retry_interval/864000000>=1){
      ESP.restart();
    }
  }
  Serial.println("MQTT Connected!");
  fade(2, 2);
}
int fade(int x, int y) {
  //x is number of runs and y is pin#
  for (int i = x; i >= 1; i -= 1) {
    for (int fadeValue = 1024 ; fadeValue >= 0; fadeValue -= 16) {
      analogWrite(y, fadeValue);
      delay(20);
    }
    for (int fadeValue = 0 ; fadeValue <= 1024; fadeValue += 16) {
      analogWrite(y, fadeValue);
      delay(20);
    }
  }
}

