#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#define MQTT_VERSION MQTT_VERSION_3_1_1
#define PIR 5
#define led 4

// Wifi: SSID and password
const char* WIFI_SSID = "WIFI名称";
const char* WIFI_PASSWORD = "WIFI密码";

// MQTT: ID, server IP, port, username and password
const PROGMEM char* MQTT_CLIENT_ID = "human.light.20190714";
const PROGMEM char* MQTT_SERVER_IP = "192.168.2.106";
const PROGMEM uint16_t MQTT_SERVER_PORT = 1883;
const PROGMEM char* MQTT_USER = "";
const PROGMEM char* MQTT_PASSWORD = "";

// MQTT: topics 这里的主题要与树莓派中的配置文件中的主题保持一致
const char* MQTT_LASOR_STATE_TOPIC = "human.light.20190714";

// payloads by default (on/off)
const char* DEVICE_ON = "ON";
const char* DEVICE_OFF = "OFF";

boolean lightState = false;
int lightStep = 0;

WiFiClient wifiClient;
PubSubClient client(wifiClient);

// 推送灯的状态
void publishLasorState() {
    Serial.println("发送状态值");
    if (lightState) {
        client.publish(MQTT_LASOR_STATE_TOPIC, DEVICE_ON, true);
    } else {
        client.publish(MQTT_LASOR_STATE_TOPIC, DEVICE_OFF, true);
    }
}

// function called when a MQTT message arrived
void callback(char* p_topic, byte* p_payload, unsigned int p_length) {
    // concat the payload into a string
    String payload;
    Serial.println("INFO:callback...");
    for (uint8_t i = 0; i < p_length; i++) {
      payload.concat((char)p_payload[i]);
    }
    Serial.println(payload);
    // handle message topic
}

void reconnect() {
  Serial.println("重新连接");
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("INFO: Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASSWORD)) {
      Serial.println("INFO: connected");
      // Once connected, publish an announcement...
      publishLasorState();
      // ... and resubscribe
      // client.subscribe(MQTT_LASOR_COMMAND_TOPIC);
      // client.subscribe(MQTT_FAN_COMMAND_TOPIC);
    } else {
      Serial.print("ERROR: failed, rc=");
      Serial.print(client.state());
      Serial.println("DEBUG: try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  // init the serial
  Serial.begin(9600);

  pinMode(led, OUTPUT);
  pinMode(PIR, INPUT);

  // init the WiFi connection
  Serial.println();
  Serial.println();
  Serial.print("INFO: Connecting to ");
  WiFi.mode(WIFI_STA);
  Serial.println(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("INFO: WiFi connected");
  Serial.print("INFO: IP address: ");
  Serial.println(WiFi.localIP());

  // init the MQTT connection
  client.setServer(MQTT_SERVER_IP, MQTT_SERVER_PORT);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  int result = digitalRead(PIR);
  if(result) {
    Serial.println(result);
    digitalWrite(led, HIGH);       //发光模块点亮
    // 如果有人通过，上报开灯信息
    lightState = true;
    publishLasorState();
    delay(20000);
    // 如果灯亮，则重置
    lightStep = 0;
  }else {
    digitalWrite(led, LOW);       //发光模块熄灭
    // 如果三次都没有亮灯，则上报（终止累加）
    if(lightStep == 3){
      lightState = false;
      publishLasorState();
      lightStep = 4;
    }
    // 如果小于3，则累加
    if(lightStep < 3){
      lightStep++;
    }
  }
}
