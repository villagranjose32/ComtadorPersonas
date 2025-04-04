#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include "esp_wifi.h"
#include <HTTPClient.h>

const char* ssid = "Flia Martinez";
const char* password = "azulina259";

#define SERVER_URL "http://somospromo.altervista.org/recibir_datos.php"
#define MAX_DEVICES 150      
#define RESET_INTERVAL 70000 

struct Device {
    String mac;
};

Device detectedDevices[MAX_DEVICES];
int deviceCount = 0;
unsigned long lastResetTime = 0;

WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

void enviarDatos() {
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        http.begin(SERVER_URL);
        http.addHeader("Content-Type", "application/x-www-form-urlencoded");
        
        String postData = "cantidad=" + String(deviceCount);
        int httpResponseCode = http.POST(postData);
        
        Serial.print("Respuesta del servidor: ");
        Serial.println(httpResponseCode);
        
        http.end();
    } else {
        Serial.println("Error: No hay conexión Wi-Fi.");
    }
}

void updateDeviceList(String mac) {
    for (int i = 0; i < deviceCount; i++) {
        if (detectedDevices[i].mac == mac) {
            return;
        }
    }
    if (deviceCount < MAX_DEVICES) {
        detectedDevices[deviceCount].mac = mac;
        deviceCount++;
    }
}

void snifferCallback(void* buf, wifi_promiscuous_pkt_type_t type) {
    wifi_promiscuous_pkt_t* pkt = (wifi_promiscuous_pkt_t*) buf;
    uint8_t *macAddr = pkt->payload + 10;
    char macStr[18];

    sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X", 
            macAddr[0], macAddr[1], macAddr[2], macAddr[3], macAddr[4], macAddr[5]);

    if ((macAddr[0] & 0x02) == 0x02) return;

    updateDeviceList(macStr);
}

void handleRoot() {
    server.send(200, "text/html", R"rawliteral(
        <!DOCTYPE html>
        <html>
        <head>
            <title>Contador de Personas</title>
            <style>
                body { font-family: Arial, sans-serif; text-align: center; }
                h1 { color: #333; }
                #counter { font-size: 48px; font-weight: bold; color: blue; }
            </style>
            <script>
                var socket = new WebSocket("ws://" + location.host + ":81/");
                socket.onmessage = function(event) {
                    document.getElementById("counter").innerText = event.data;
                };
            </script>
        </head>
        <body>
            <h1>Personas Detectadas:</h1>
            <p id="counter">0</p>
        </body>
        </html>
    )rawliteral");
}

void setup() {
    Serial.begin(115200);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi conectado!");
    Serial.println(WiFi.localIP());

    server.on("/", handleRoot);
    server.begin();
    webSocket.begin();

    esp_wifi_set_promiscuous(true);
    esp_wifi_set_promiscuous_rx_cb(&snifferCallback);
    esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);

    lastResetTime = millis();
}

void loop() {
    server.handleClient();
    webSocket.loop();

    unsigned long currentTime = millis();
    if (currentTime - lastResetTime > RESET_INTERVAL) {
        for (int i = 0; i < MAX_DEVICES; i++) {
            detectedDevices[i].mac = "";  
        }
        deviceCount = 0;
        lastResetTime = currentTime;
    }

    Serial.print("\rCantidad de personas: ");
    Serial.println(deviceCount);

    String msg = String(deviceCount);
    webSocket.broadcastTXT(msg);

    enviarDatos();
    delay(60000);
}
