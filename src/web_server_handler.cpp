#include "web_server_handler.h"
#include "config.h"
#include "ntp_handler.h"
#include "serial_handler.h"
#include "wifi_manager.h"
#include "decision_tree.h"

// Web Server nesnesi
static ESP8266WebServer server(WEB_SERVER_PORT);

// Son alınan sensor verileri
static String lastSensorData = "Henuz veri yok...";

// Forward declarations
void handleRoot();
void handleCommand();
void handleStatus();
void handleDecision();
void handleNotFound();

void initWebServer(int port) {
    // Web Server rotalarını tanımla
    server.on("/", handleRoot);
    server.on("/command", handleCommand);
    server.on("/status", handleStatus);
    server.on("/decision", handleDecision);
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
    String html = F("<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width,initial-scale=1'><title>Sera Kontrol</title><style>");
    html += F("*{box-sizing:border-box}body{font-family:Arial;margin:0;padding:8px;background:#1a1a2e;color:#eee}");
    html += F(".c{max-width:800px;margin:0 auto}.hdr{background:#4ecca3;color:#000;padding:10px;border-radius:8px;text-align:center;margin-bottom:10px}");
    html += F(".box{background:#16213e;padding:12px;border-radius:8px;margin-bottom:10px}.box h3{margin:0 0 10px;color:#4ecca3;font-size:14px}");
    html += F(".grid{display:grid;grid-template-columns:repeat(3,1fr);gap:8px}.item{background:#0f3460;padding:8px;border-radius:5px;text-align:center}");
    html += F(".val{font-size:18px;font-weight:bold;color:#4ecca3}.lbl{font-size:10px;color:#888}");
    html += F(".st{display:inline-block;padding:2px 8px;border-radius:10px;font-size:10px;font-weight:bold}");
    html += F(".ok{background:#4ecca3;color:#000}.warn{background:#f9a825;color:#000}.bad{background:#e74c3c;color:#fff}");
    html += F(".btn{padding:10px;border:none;border-radius:5px;cursor:pointer;font-weight:bold;font-size:12px;width:100%}");
    html += F(".on{background:#4ecca3;color:#000}.off{background:#e74c3c;color:#fff}");
    html += F(".btns{display:grid;grid-template-columns:1fr 1fr;gap:5px}.row{display:flex;justify-content:space-between;padding:4px 0;border-bottom:1px solid #333;font-size:12px}");
    html += F(".raw{background:#0a0a15;padding:8px;border-radius:5px;font-family:monospace;font-size:9px;word-break:break-all;max-height:80px;overflow:auto;color:#0f9}");
    html += F("</style></head><body><div class='c'>");
    
    html += F("<div class='hdr' id='mode'>🌱 Sera Kontrol - Yükleniyor...</div>");
    
    // Sensör Verileri
    html += F("<div class='box'><h3>📊 Sensörler</h3><div class='grid'>");
    html += F("<div class='item'><div class='val' id='t'>--</div><div class='lbl'>Sıcaklık °C</div></div>");
    html += F("<div class='item'><div class='val' id='h'>--</div><div class='lbl'>Nem %</div></div>");
    html += F("<div class='item'><div class='val' id='c'>--</div><div class='lbl'>CO2 ppm</div></div>");
    html += F("<div class='item'><div class='val' id='s'>--</div><div class='lbl'>Toprak %</div></div>");
    html += F("<div class='item'><div class='val' id='l'>--</div><div class='lbl'>Işık lux</div></div>");
    html += F("<div class='item'><div class='val' id='p'>--</div><div class='lbl'>Basınç hPa</div></div>");
    html += F("</div></div>");
    
    // Karar Durumu
    html += F("<div class='box'><h3>🧠 Karar: <span id='code'>--</span></h3>");
    html += F("<div id='desc' style='color:#aaa;font-size:12px'>Bekleniyor</div>");
    html += F("<div style='margin-top:8px;display:flex;flex-wrap:wrap;gap:5px'>");
    html += F("<span>Sıc:<span id='st' class='st'>--</span></span>");
    html += F("<span>Nem:<span id='sh' class='st'>--</span></span>");
    html += F("<span>CO2:<span id='sc' class='st'>--</span></span>");
    html += F("<span>Top:<span id='ss' class='st'>--</span></span>");
    html += F("<span>Işık:<span id='sl' class='st'>--</span></span>");
    html += F("</div></div>");
    
    // Kontroller
    html += F("<div class='box'><h3>🎮 Kontrol</h3><div class='btns'>");
    html += F("<button class='btn on' onclick=\"X('havaac')\">Fan Aç</button><button class='btn off' onclick=\"X('havakapa')\">Fan Kapat</button>");
    html += F("<button class='btn on' onclick=\"X('isikac')\">Işık Aç</button><button class='btn off' onclick=\"X('isikkapa')\">Işık Kapat</button>");
    html += F("<button class='btn on' onclick=\"X('sulaac')\">Sulama Aç</button><button class='btn off' onclick=\"X('sulakapa')\">Sulama Kapat</button>");
    html += F("</div><div style='margin-top:8px'>");
    html += F("<div class='row'><span>Fan:</span><span id='f'>--</span></div>");
    html += F("<div class='row'><span>Işık:</span><span id='i'>--</span></div>");
    html += F("<div class='row'><span>Pompa:</span><span id='pm'>--</span></div>");
    html += F("</div></div>");
    
    // Sistem
    html += F("<div class='box'><h3>ℹ️ Sistem</h3>");
    html += F("<div class='row'><span>IP:</span><span>");
    html += getIPAddress();
    html += F("</span></div>");
    html += F("<div class='row'><span>Uptime:</span><span id='up'>--</span></div>");
    html += F("<div class='row'><span>Güncelleme:</span><span id='tm'>--</span></div>");
    html += F("</div>");
    
    // Ham Veri
    html += F("<div class='box'><h3>📋 Ham JSON</h3><div class='raw' id='raw'>--</div></div>");
    
    html += F("</div>");
    
    // JavaScript - minimal
    html += F("<script>");
    html += F("function X(c){fetch('/command?cmd='+c);}");
    html += F("function G(i){return document.getElementById(i);}");
    html += F("function cls(v){if(!v)return'';if(v=='OPTIMAL'||v=='NORMAL'||v=='YETERLI')return'ok';if(v.indexOf('ASIRI')>=0||v.indexOf('KUF')>=0||v.indexOf('KRITIK')>=0||v.indexOf('ACIL')>=0)return'bad';return'warn';}");
    html += F("function U(){");
    html += F("fetch('/status').then(r=>r.json()).then(d=>{");
    html += F("var s=d.data;G('raw').textContent=s?JSON.stringify(s):'Veri yok';");
    html += F("if(s){G('t').textContent=s.temp!=null?s.temp.toFixed(1):'--';");
    html += F("G('h').textContent=s.hum!=null?s.hum.toFixed(1):'--';");
    html += F("G('c').textContent=s.co2||'--';");
    html += F("G('s').textContent=s.soil!=null?s.soil.toFixed(1):'--';");
    html += F("G('l').textContent=s.lux!=null?Math.round(s.lux):'--';");
    html += F("G('p').textContent=s.pres!=null?s.pres.toFixed(1):'--';");
    html += F("G('f').textContent=s.fan?'AÇIK':'KAPALI';");
    html += F("G('i').textContent=s.light?'AÇIK':'KAPALI';");
    html += F("G('pm').textContent=s.pump?'AÇIK':'KAPALI';}");
    html += F("var u=d.up||0;G('up').textContent=Math.floor(u/3600)+'s '+Math.floor((u%3600)/60)+'dk';");
    html += F("G('tm').textContent=new Date().toLocaleTimeString('tr-TR');");
    html += F("}).catch(e=>{});");
    html += F("fetch('/decision').then(r=>r.json()).then(d=>{");
    html += F("G('mode').innerHTML=(d.mode=='GECE'?'🌙 GECE':'☀️ GÜNDÜZ')+' - Saat: '+(d.hour||'--')+':00';");
    html += F("G('code').textContent=d.lastCode||'--';G('desc').textContent=d.lastDesc||'';");
    html += F("var m=[['st','tempStatus'],['sh','humStatus'],['sc','co2Status'],['ss','soilStatus'],['sl','luxStatus']];");
    html += F("m.forEach(function(x){var e=G(x[0]);var v=d[x[1]]||'--';e.textContent=v;e.className='st '+cls(v);});");
    html += F("}).catch(e=>{});}");
    html += F("U();setInterval(U,3000);");
    html += F("</script></body></html>");
    
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
    String json = "{\"up\":";
    json += String(millis()/1000);
    json += ",\"rssi\":";
    json += String(getWiFiRSSI());
    json += ",\"data\":";
    json += lastSensorData.startsWith("{") ? lastSensorData : "null";
    json += "}";
    
    server.send(200, "application/json", json);
}

void handleDecision() {
    String json = getDecisionStatusJSON();
    server.send(200, "application/json", json);
}

void handleNotFound() {
    server.send(404, "text/plain", "404: Sayfa bulunamadi!");
}
