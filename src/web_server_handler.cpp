#include "web_server_handler.h"
#include "config.h"
#include "ntp_handler.h"
#include "serial_handler.h"
#include "wifi_manager.h"

// Web Server nesnesi
static ESP8266WebServer server(WEB_SERVER_PORT);

// Son alınan sensor verileri
static String lastSensorData = "Henuz veri yok...";

// Forward declarations
void handleRoot();
void handleCommand();
void handleStatus();
void handleNotFound();

void initWebServer(int port) {
    // Web Server rotalarını tanımla
    server.on("/", handleRoot);
    server.on("/command", handleCommand);
    server.on("/status", handleStatus);
    server.onNotFound(handleNotFound);
    
    // Web Server'ı başlat
    server.begin();
    Serial.println("\nWeb Server baslatildi!");
    Serial.print("Kontrol Paneli: http://");
    Serial.println(getIPAddress());
    Serial.println("\nKomutlar (Serial Monitor veya Web):");
    Serial.println("  havaac, havakapa, isikac, isikkapa, sulaac, sulakapa");
}

void handleWebServer() {
    server.handleClient();
}

void updateLastSensorData(const String& data) {
    lastSensorData = data;
}

String getLastSensorData() {
    return lastSensorData;
}

ESP8266WebServer& getWebServer() {
    return server;
}

// ========================================
// WEB SERVER HANDLER FONKSIYONLARI
// ========================================

void handleRoot() {
    String html = "<!DOCTYPE html><html><head>";
    html += "<meta charset='UTF-8'>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
    html += "<title>Sera Kontrol Paneli</title>";
    html += "<style>";
    html += "body{font-family:Arial;margin:0;padding:20px;background:#f0f0f0}";
    html += ".container{max-width:800px;margin:0 auto;background:white;padding:20px;border-radius:10px;box-shadow:0 2px 10px rgba(0,0,0,0.1)}";
    html += "h1{color:#333;text-align:center}";
    html += ".status{background:#e8f5e9;padding:15px;border-radius:5px;margin:20px 0}";
    html += ".controls{display:grid;grid-template-columns:1fr 1fr;gap:10px;margin:20px 0}";
    html += "button{padding:15px;font-size:16px;border:none;border-radius:5px;cursor:pointer;transition:0.3s}";
    html += ".btn-on{background:#4CAF50;color:white}.btn-on:hover{background:#45a049}";
    html += ".btn-off{background:#f44336;color:white}.btn-off:hover{background:#da190b}";
    html += ".btn-water{background:#2196F3;color:white}.btn-water:hover{background:#0b7dda}";
    html += ".info{color:#666;font-size:14px;margin-top:20px}";
    html += "</style>";
    html += "</head><body>";
    html += "<div class='container'>";
    html += "<h1>🌱 Sera Kontrol Paneli</h1>";
    html += "<div class='status' id='sensorData'>";
    html += "<strong>Son Sensor Verisi:</strong><br>";
    html += "<small id='updateTime'>" + getFormattedTime() + "</small><br>";
    html += "<pre style='overflow-x:auto'>" + lastSensorData + "</pre>";
    html += "</div>";
    html += "<div class='controls'>";
    html += "<button class='btn-on' onclick=\"cmd('havaac')\">🌬️ Hava Aç</button>";
    html += "<button class='btn-off' onclick=\"cmd('havakapa')\">🔒 Hava Kapat</button>";
    html += "<button class='btn-on' onclick=\"cmd('isikac')\">💡 Işık Aç</button>";
    html += "<button class='btn-off' onclick=\"cmd('isikkapa')\">🌙 Işık Kapat</button>";
    html += "<button class='btn-water' onclick=\"cmd('sulaac')\">💧 Sulama Aç</button>";
    html += "<button class='btn-off' onclick=\"cmd('sulakapa')\">🛑 Sulama Kapat</button>";
    html += "</div>";
    html += "<div class='info'>";
    html += "<strong>IP Adresi:</strong> " + getIPAddress() + "<br>";
    html += "<strong>RSSI:</strong> " + String(getWiFiRSSI()) + " dBm<br>";
    html += "<strong>Uptime:</strong> " + String(millis()/1000) + " saniye";
    html += "</div>";
    html += "</div>";
    html += "<script>";
    html += "function cmd(c){fetch('/command?cmd='+c).then(r=>r.text()).then(d=>{alert(d);updateStatus()})}";
    html += "function updateStatus(){fetch('/status').then(r=>r.json()).then(d=>{";
    html += "document.getElementById('sensorData').innerHTML='<strong>Son Sensor Verisi:</strong><br><small>'+new Date().toLocaleString('tr-TR')+'</small><br><pre style=\"overflow-x:auto\">'+d.lastData+'</pre>';";
    html += "})}";
    html += "setInterval(updateStatus,3000);";
    html += "updateStatus();";
    html += "</script>";
    html += "</body></html>";
    
    server.send(200, "text/html", html);
}

void handleCommand() {
    if (server.hasArg("cmd")) {
        String cmd = server.arg("cmd");
        cmd.toLowerCase();
        
        // Komut listesi kontrolü
        if (cmd == "havaac" || cmd == "havakapa" || cmd == "isikac" || 
            cmd == "isikkapa" || cmd == "sulaac" || cmd == "sulakapa") {
            
            sendCommandToArduino(cmd);
            server.send(200, "text/plain", "Komut gonderildi: " + cmd);
        } else {
            server.send(400, "text/plain", "Gecersiz komut: " + cmd);
        }
    } else {
        server.send(400, "text/plain", "Komut parametresi eksik!");
    }
}

void handleStatus() {
    String json = "{";
    json += "\"uptime\":" + String(millis()/1000) + ",";
    json += "\"rssi\":" + String(getWiFiRSSI()) + ",";
    json += "\"lastData\":\"" + lastSensorData + "\"";
    json += "}";
    
    server.send(200, "application/json", json);
}

void handleNotFound() {
    server.send(404, "text/plain", "404: Sayfa bulunamadi!");
}
