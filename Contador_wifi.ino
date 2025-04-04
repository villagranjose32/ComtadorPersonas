#include <Arduino.h>
#include <WiFi.h>
#include "esp_wifi.h"

#define MAX_DEVICES 50
#define RESET_INTERVAL 30000 // Intervalo de tiempo en milisegundos para reiniciar las variables
#define SERVER_URL "http://somospromo.altervista.org/recibir_datos.php"
struct Device {
    String mac;
    unsigned long lastSeen;
};

Device detectedDevices[MAX_DEVICES];
int deviceCount = 0;

unsigned long lastResetTime = 0; // Tiempo del último reinicio

// Actualiza la lista de dispositivos detectados
void updateDeviceList(String mac) {
    unsigned long currentTime = millis();

    // Verifica si el dispositivo ya está en la lista
    for (int i = 0; i < deviceCount; i++) {
        if (detectedDevices[i].mac == mac) {
            detectedDevices[i].lastSeen = currentTime;
            return;
        }
    }

    // Si el dispositivo no está en la lista y hay espacio, lo agrega
    if (deviceCount < MAX_DEVICES) {
        detectedDevices[deviceCount].mac = mac;
        detectedDevices[deviceCount].lastSeen = currentTime;
        deviceCount++;
    }
}

// Elimina los dispositivos que han estado inactivos por más de TIMEOUT
void removeOldDevices() {
    unsigned long currentTime = millis();
    int newCount = 0;

    for (int i = 0; i < deviceCount; i++) {
        if (currentTime - detectedDevices[i].lastSeen < TIMEOUT) {
            detectedDevices[newCount++] = detectedDevices[i];
        }
    }

    deviceCount = newCount; // Actualiza el conteo de dispositivos
}

// Función callback para capturar paquetes en modo promiscuo
void snifferCallback(void* buf, wifi_promiscuous_pkt_type_t type) {
    wifi_promiscuous_pkt_t* pkt = (wifi_promiscuous_pkt_t*) buf;
    uint8_t *macAddr = pkt->payload + 10;
    char macStr[18];

    // Convierte la dirección MAC a formato legible
    sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X", 
            macAddr[0], macAddr[1], macAddr[2], macAddr[3], macAddr[4], macAddr[5]);

    // Filtra direcciones MAC aleatorias (MAC locales)
    if ((macAddr[0] & 0x02) == 0x02) return;

    // Actualiza la lista de dispositivos detectados
    updateDeviceList(macStr);
}

void setup() {
    Serial.begin(115200);

    // Configura WiFi en modo NULL para activar el modo promiscuo
    WiFi.mode(WIFI_MODE_STA);

    // Habilita el modo promiscuo y configura el callback
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_promiscuous_rx_cb(&snifferCallback);

    // Establece el canal WiFi (puedes cambiar este canal si es necesario)
    esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);

    lastResetTime = millis();  // Inicializa el tiempo del primer reinicio
}

void loop() {
    unsigned long currentTime = millis();

    // Si ha pasado el tiempo para reiniciar las variables (RESET_INTERVAL)
    if (currentTime - lastResetTime > RESET_INTERVAL) {
        // Reinicia la lista de dispositivos detectados
        deviceCount = 0;  // Resetea el contador de dispositivos
        for (int i = 0; i < MAX_DEVICES; i++) {
            detectedDevices[i].mac = "";  // Limpia cada dirección MAC en el arreglo
            detectedDevices[i].lastSeen = 0;
        }

        // Actualiza el tiempo del último reinicio
        lastResetTime = currentTime;

        Serial.println("Variables reiniciadas. Comenzando nueva detección de dispositivos.");
    }

    // Elimina dispositivos inactivos
    removeOldDevices();

    // Muestra el número de dispositivos detectados
    Serial.print("Dispositivos detectados: ");
    Serial.println(deviceCount);

    // Imprime las direcciones MAC detectadas
    for (int i = 0; i < deviceCount; i++) {
        Serial.print("MAC ");
        Serial.print(i + 1);
        Serial.print(": ");
        Serial.println(detectedDevices[i].mac);
    }

    delay(30000); // Espera 10 segundos antes de la próxima impresión
}
