/**
 * @file web_panel.cpp
 * @brief Panel WWW i REST API - Trassar-Painter v7.0.0
 *
 * ZMIANY v7.0.0:
 * - Basic Auth na krytycznych endpointach (E-STOP reset, OTA)
 * - CORS headers
 * - DELETE endpointy raportów
 * - Info o karcie SD i prędkości
 * - Jeden przycisk START/PAUZA w panelu
 * - Wskaźnik minimalnej prędkości
 * - Info o cyklowaniu wzorca (linia/przerwa)
 *
 * @author Trassar251
 * @date 2026-02-02
 */

#include <Arduino.h>
#include <WebServer.h>
#include <LittleFS.h>
#include "web_panel.h"
#include "config.h"
#include "state.h"
#include "patterns.h"
#include "reports.h"
#include "rtc_ntp.h"
#include "safety.h"
#include "encoder.h"
#include "sd_card.h"

extern WebServer* pServer;
#define server (*pServer)
extern void startSystem();
extern void stopSystem();
extern void pauseSystem();
extern void startMeasuring();
extern void startService();
extern void enterMenu();
extern void changePattern(int newPattern);
extern void updateGuns();

// Helper: sprawdź autentykację
static bool checkAuth() {
    if (!server.authenticate(WEB_AUTH_USER, WEB_AUTH_PASS)) {
        server.requestAuthentication();
        return false;
    }
    return true;
}

// Helper: dodaj CORS headers
static void addCORSHeaders() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.sendHeader("Access-Control-Allow-Methods", "GET, POST, DELETE, OPTIONS");
    server.sendHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
}

// ============================================================================
// STATUS JSON
// ============================================================================

String getStatusJSON() {
    String json;
    json.reserve(1280);

    json = "{\"mode\":\"";
    json += getModeNameJSON(currentMode);
    json += "\",";

    // Wzorzec
    json += "\"pattern\":\"" + String(patterns[currentPattern].name) + "\",";
    json += "\"patternIndex\":" + String(currentPattern) + ",";
    json += "\"patternDesc\":\"" + String(patterns[currentPattern].desc) + "\",";
    json += "\"patternLine\":" + String(patterns[currentPattern].lineLength, 1) + ",";
    json += "\"patternGap\":" + String(patterns[currentPattern].gapLength, 1) + ",";
    json += "\"patternWidth\":" + String(patterns[currentPattern].width) + ",";

    // v7.0.0: Cyklowanie wzorca
    json += "\"patternDashed\":" + String(isPatternDashed(currentPattern) ? "true" : "false") + ",";
    json += "\"cycleInLine\":" + String(patternCycle.inLine ? "true" : "false") + ",";
    json += "\"cycleDistance\":" + String(patternCycle.cycleDistance, 2) + ",";

    // v7.0.0: Prędkość wystarczająca
    json += "\"speedSufficient\":" + String(speedSufficient ? "true" : "false") + ",";
    json += "\"minPaintingSpeed\":" + String(MIN_PAINTING_SPEED_KMH, 1) + ",";

    // Selektor P3
    bool p3State = selectorP3Physical || selectorP3Virtual;
    json += "\"selectorP3Physical\":" + String(selectorP3Physical ? "true" : "false") + ",";
    json += "\"selectorP3Virtual\":" + String(selectorP3Virtual ? "true" : "false") + ",";
    json += "\"selectorP3\":\"" + String(p3State ? "ODWROCONE" : "NORMALNE") + "\",";

    // Pistolety
    json += "\"guns\":[";
    for (int i = 0; i < GUN_COUNT; i++) {
        json += gunsActive[i] ? "true" : "false";
        if (i < GUN_COUNT - 1) json += ",";
    }
    json += "],";

    // Enkoder
    json += "\"encoder\":{";
    json += "\"pulses\":" + String(encoderPulses) + ",";
    json += "\"distance\":" + String(distanceTraveled, 2) + ",";
    json += "\"speed\":" + String(currentSpeed, 1) + ",";
    json += "\"calibration\":" + String(encoderCalibration, 1);
    json += "},";

    // Start od przerwy
    json += "\"startFromGap\":" + String(startFromGap ? "true" : "false") + ",";
    json += "\"gapDistance\":" + String(patterns[currentPattern].gapLength, 1) + ",";
    json += "\"gapTraveled\":" + String(gapTraveled, 2) + ",";

    // v7.0.0: SD Card
    json += "\"sdCard\":" + String(sdCardAvailable ? "true" : "false") + ",";
    if (sdCardAvailable) {
        json += "\"sdTotalMB\":" + String((unsigned long)getSDTotalMB()) + ",";
        json += "\"sdUsedMB\":" + String((unsigned long)getSDUsedMB()) + ",";
    }

    // E-STOP
    json += "\"emergencyStop\":" + String(emergencyStopActive ? "true" : "false") + ",";

    // Statystyki
    json += "\"stats\":{";
    json += "\"patternChanges\":" + String(patternChangeCount) + ",";
    json += "\"totalWorkTime\":" + String(totalWorkTime / 1000) + ",";
    json += "\"uptime\":" + String(millis() / 1000);
    json += "}}";

    return json;
}

// ============================================================================
// HTML PAGE (v7.0.0)
// ============================================================================

String getHTMLPage() {
    return R"HTMLCODE(<!DOCTYPE html>
<html lang="pl">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Trassar Painter v7.0.0 SD-SPEED</title>
<style>
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:'Segoe UI',Tahoma,Geneva,Verdana,sans-serif;background:linear-gradient(135deg,#0f3460 0%,#16213e 100%);color:#eaeaea;padding:10px;min-height:100vh}
.container{max-width:1000px;margin:0 auto}
.card{background:#1a1a2e;padding:20px;border-radius:15px;margin:10px 0;box-shadow:0 8px 32px rgba(0,0,0,0.5);border:1px solid #2a2a3e}
h1{color:#00d4ff;margin-bottom:5px;font-size:1.9em}
.subtitle{color:#888;font-size:0.9em;margin-bottom:15px}
.tabs{display:flex;gap:10px;margin-bottom:20px}
.tab{flex:1;padding:15px;background:#252540;border:2px solid #3a3a5a;border-radius:10px;cursor:pointer;text-align:center;font-weight:bold;transition:all 0.3s}
.tab:hover{background:#2e2e50;transform:translateY(-2px)}
.tab.active{background:#00d4ff;color:#1a1a2e;border-color:#00d4ff}
.tab-content{display:none}.tab-content.active{display:block}
.status-bar{background:linear-gradient(135deg,#252540 0%,#1e1e3a 100%);padding:18px;border-radius:12px;margin:15px 0;display:grid;grid-template-columns:repeat(3,1fr);gap:15px;border:1px solid #3a3a5a}
.status-item{display:flex;align-items:center;gap:10px}
.status-indicator{width:14px;height:14px;border-radius:50%;display:inline-block}
.status-idle{background:#666}.status-working{background:#00ff88;animation:pulse 1.5s infinite;box-shadow:0 0 15px rgba(0,255,136,0.8)}.status-paused{background:#ffaa00}.status-measuring{background:#00d4ff}.status-menu{background:#e94560}.status-service{background:#ff6b9d}.status-emergency{background:#ff0000;animation:pulse 0.5s infinite}
@keyframes pulse{0%,100%{opacity:1;transform:scale(1)}50%{opacity:0.6;transform:scale(1.15)}}
.measurements{display:grid;grid-template-columns:repeat(4,1fr);gap:15px;margin:15px 0}
.measure-box{background:linear-gradient(135deg,#2a2a3e,#1e1e2e);padding:20px;border-radius:12px;text-align:center;border:1px solid #3a3a5a}
.measure-value{font-size:2.2em;font-weight:bold;color:#00d4ff;margin-bottom:5px}
.measure-label{font-size:0.85em;color:#999}
.speed-ok .measure-value{color:#00ff88}
.speed-low .measure-value{color:#ff4444}
h3{margin:20px 0 12px 0;color:#00d4ff;font-size:1.1em;border-bottom:2px solid #00d4ff;padding-bottom:5px}
.pattern-grid{display:grid;grid-template-columns:repeat(5,1fr);gap:10px;margin:15px 0}
.pattern-btn{padding:14px 8px;border:2px solid #3a3a5a;border-radius:10px;background:#252540;cursor:pointer;font-weight:bold;font-size:0.95em;transition:all 0.2s;text-align:center;color:#eaeaea}
.pattern-btn:hover{background:#2e2e50;transform:translateY(-2px)}
.pattern-btn.active{transform:scale(1.05);background:#00d4ff;color:#1a1a2e;border-color:#00d4ff;box-shadow:0 6px 20px rgba(0,212,255,0.5)}
.pattern-desc{font-size:0.7em;opacity:0.8;margin-top:3px}
.service-pattern-btn{padding:20px 10px;border:2px solid #3a3a5a;border-radius:10px;background:#252540;cursor:pointer;font-weight:bold;font-size:1em;text-align:center;color:#eaeaea;user-select:none;touch-action:manipulation}
.service-pattern-btn:active{background:#00ff88;color:#1a1a2e;border-color:#00ff88}
.service-info{background:#2a2a3e;padding:15px;border-radius:10px;margin:15px 0;border:2px solid #ff6b9d;color:#ff6b9d;text-align:center}
.options{display:grid;grid-template-columns:repeat(2,1fr);gap:15px;margin:15px 0}
.option-box{background:#252540;padding:15px;border-radius:10px;border:1px solid #3a3a5a}
.option-box label{display:flex;align-items:center;gap:10px;cursor:pointer}
.control-buttons{display:grid;grid-template-columns:repeat(2,1fr);gap:12px;margin:20px 0}
.btn{padding:20px;border:none;border-radius:12px;font-size:1.15em;font-weight:bold;cursor:pointer;transition:all 0.3s;text-transform:uppercase;box-shadow:0 4px 15px rgba(0,0,0,0.4)}
.btn:hover{transform:translateY(-3px)}
.btn-start{background:linear-gradient(135deg,#00ff88,#00cc6e);color:#1a1a2e}
.btn-measure{background:linear-gradient(135deg,#00d4ff,#00a0cc);color:#1a1a2e}
.btn-service{background:linear-gradient(135deg,#ff6b9d,#e94560);color:#fff}
.btn-stop{background:linear-gradient(135deg,#e94560,#cc3344);color:#fff}
.btn-menu{background:linear-gradient(135deg,#9b59b6,#8e44ad);color:#fff}
.btn-estop-reset{background:linear-gradient(135deg,#ffaa00,#cc8800);color:#1a1a2e}
.guns-display{display:grid;grid-template-columns:repeat(6,1fr);gap:10px;margin:15px 0}
.gun{text-align:center;padding:18px 8px;border-radius:10px;background:#252540;border:3px solid #3a3a5a;transition:all 0.3s}
.gun.active{background:linear-gradient(135deg,#00ff88,#00cc6e);color:#1a1a2e;border-color:#00ff88;box-shadow:0 0 25px rgba(0,255,136,0.7)}
.gun-label{font-size:1em;font-weight:bold;margin-bottom:5px}
.gun-size{font-size:0.75em;opacity:0.8}
.cycle-info{background:#252540;padding:15px;border-radius:10px;margin:15px 0;border:2px solid #00d4ff;text-align:center;display:none}
.cycle-line{color:#00ff88;font-weight:bold;font-size:1.2em}
.cycle-gap{color:#ffaa00;font-weight:bold;font-size:1.2em}
.speed-warning{background:#3a1a1a;border:2px solid #ff4444;padding:15px;border-radius:10px;margin:15px 0;text-align:center;color:#ff4444;font-weight:bold;display:none}
.estop-banner{background:#ff0000;color:white;padding:20px;border-radius:10px;margin:15px 0;text-align:center;font-size:1.5em;font-weight:bold;display:none;animation:pulse 0.5s infinite}
.selector-toggle{background:#252540;padding:20px;border-radius:10px;border:2px solid #ffaa00;text-align:center;margin:15px 0}
.selector-toggle button{padding:15px 30px;background:#ffaa00;color:#1a1a2e;border:none;border-radius:10px;cursor:pointer;font-weight:bold;font-size:1.2em;transition:all 0.3s}
.selector-toggle button.active{background:#ff6b00}
.footer{text-align:center;margin-top:20px;opacity:0.6;font-size:0.85em}
.modal{display:none;position:fixed;z-index:1000;left:0;top:0;width:100%;height:100%;background:rgba(0,0,0,0.8)}
.modal-content{background:#1a1a2e;margin:10% auto;padding:30px;border:2px solid #00ff88;border-radius:15px;width:90%;max-width:500px}
.modal-header{display:flex;justify-content:space-between;align-items:center;margin-bottom:20px}
.modal-header h2{margin:0;color:#00ff88}
.close-modal{font-size:2em;cursor:pointer;color:#999}
.menu-item{background:#2a2a3e;padding:20px;margin:10px 0;border-radius:10px;border:2px solid #444;cursor:pointer;transition:all 0.3s;display:flex;align-items:center;gap:15px}
.menu-item:hover{border-color:#00ff88;transform:translateX(5px)}
.menu-icon{font-size:2em}
.menu-text h3{margin:0 0 5px 0;color:#00ff88;border:none}
.menu-text p{margin:0;color:#999;font-size:0.9em}
@media(max-width:600px){.pattern-grid{grid-template-columns:repeat(3,1fr)}.control-buttons{grid-template-columns:1fr}.guns-display{grid-template-columns:repeat(3,1fr)}.measurements{grid-template-columns:repeat(2,1fr)}.status-bar{grid-template-columns:1fr}}
</style>
</head>
<body>
<div class="container">
<div class="card">
<h1>Trassar Painter - Komputer Malowarki</h1>
<div class="subtitle">Professional Road Marking System v7.0.0 SD-SPEED EDITION</div>

<div class="estop-banner" id="estopBanner">EMERGENCY STOP AKTYWNY<br><button class="btn btn-estop-reset" onclick="resetEStop()" style="margin-top:10px;font-size:0.7em;">RESETUJ E-STOP</button></div>

<div class="tabs">
<div class="tab active" onclick="showTab(0)">Panel Glowny</div>
<div class="tab" onclick="showTab(1)">Pomiar</div>
<div class="tab" onclick="showTab(2)">Kalibracja</div>
<div class="tab" onclick="showTab(3)">Raporty</div>
</div>

<!-- TAB 0: Panel glowny -->
<div class="tab-content active" id="tab0">
<div class="status-bar">
<div class="status-item"><span class="status-indicator status-idle" id="statusDot"></span><strong>Tryb:</strong> <span id="modeText">IDLE</span></div>
<div class="status-item"><strong>Wzorzec:</strong> <span id="currentPattern">-</span></div>
<div class="status-item"><strong>SD Card:</strong> <span id="sdStatus">-</span></div>
</div>

<div class="speed-warning" id="speedWarning">PREDKOSC ZA MALA! Malowanie wymaga min. 3.0 km/h</div>

<h3>Pomiary</h3>
<div class="measurements">
<div class="measure-box"><div class="measure-value" id="distance">0.00</div><div class="measure-label">Dystans (m)</div></div>
<div class="measure-box" id="speedBox"><div class="measure-value" id="speed">0.0</div><div class="measure-label">Predkosc (km/h)</div></div>
<div class="measure-box"><div class="measure-value" id="pulses">0</div><div class="measure-label">Impulsy</div></div>
<div class="measure-box"><div class="measure-value" id="selectorP3">-</div><div class="measure-label">P3 Selektor</div></div>
</div>

<div class="cycle-info" id="cycleInfo">
<span id="cycleText">-</span>
</div>

<div id="normalMode">
<div class="selector-toggle">
<h3 style="border:none;margin:0 0 15px 0;">Selektor P3 (Wirtualny)</h3>
<button id="btnP3Selector" onclick="toggleP3()">P3: <span id="p3SelectorText">NORMALNE</span></button>
<div style="margin-top:10px;font-size:0.85em;color:#999;">Fizyczny (GPIO 20): <span id="p3Physical">-</span></div>
</div>

<h3>Opcje</h3>
<div class="options">
<div class="option-box"><label><input type="checkbox" id="chkGap"><strong>Start od przerwy</strong></label></div>
<div class="option-box" id="gapProgress" style="display:none;"><strong>Przerwa:</strong> <span id="gapTraveled">0.00</span>/<span id="gapTotal">0.0</span>m<div style="width:100%;background:#3a3a5a;height:10px;border-radius:5px;margin-top:10px;"><div id="gapBar" style="width:0%;background:#00ff88;height:100%;border-radius:5px;"></div></div></div>
</div>

<h3>Wybor wzorca (15)</h3>
<div class="pattern-grid" id="patternGrid"></div>

<h3>Sterowanie</h3>
<div class="control-buttons">
<button class="btn btn-start" id="btnStartPause" onclick="sendCmd('start')">START MALOWANIA</button>
<button class="btn btn-service" onclick="sendCmd('service')">TRYB SERWISOWY</button>
<button class="btn btn-stop" onclick="sendCmd('stop')">STOP</button>
<button class="btn btn-menu" onclick="showMenu()">MENU</button>
</div>
</div>

<div id="serviceMode" style="display:none;">
<div class="service-info"><h3 style="color:#ff6b9d;border:none;margin:0;">TRYB SERWISOWY</h3><p style="margin-top:10px;">Trzymaj przycisk aby aktywowac pistolety</p></div>
<div class="pattern-grid" id="serviceGrid"></div>
<button class="btn btn-stop" onclick="sendCmd('stop')" style="width:100%;margin-top:15px;">WYJDZ Z SERWISU</button>
</div>

<h3>Pistolety</h3>
<div class="guns-display">
<div class="gun" id="gun0"><div class="gun-label">P1</div><div class="gun-size">12cm</div></div>
<div class="gun" id="gun1"><div class="gun-label">P2</div><div class="gun-size">12cm</div></div>
<div class="gun" id="gun2"><div class="gun-label">P3</div><div class="gun-size">12cm</div></div>
<div class="gun" id="gun3"><div class="gun-label">P4</div><div class="gun-size">24cm</div></div>
<div class="gun" id="gun4"><div class="gun-label">P5</div><div class="gun-size">12cm K</div></div>
<div class="gun" id="gun5"><div class="gun-label">P6</div><div class="gun-size">24cm K</div></div>
</div>
</div>

<!-- TAB 1-3 same as before but updated -->
<div class="tab-content" id="tab1">
<h3>Tryb pomiaru dystansu</h3>
<p style="margin:15px 0;color:#999;">Pomiar odleglosci bez malowania.</p>
<div class="measurements">
<div class="measure-box"><div class="measure-value" id="distM">0.00</div><div class="measure-label">Dystans (m)</div></div>
<div class="measure-box"><div class="measure-value" id="spdM">0.0</div><div class="measure-label">Predkosc (km/h)</div></div>
</div>
<div class="control-buttons">
<button class="btn btn-measure" onclick="sendCmd('measure')">START POMIARU</button>
<button class="btn btn-stop" onclick="sendCmd('stop')">STOP</button>
</div>
</div>

<div class="tab-content" id="tab2">
<h3>Kalibracja automatyczna enkodera</h3>
<div style="background:#2a2a3e;padding:25px;border-radius:12px;margin-bottom:20px;border:2px solid #00ff88;">
<p style="color:#ccc;line-height:1.6;">1. Zaznacz 10m<br>2. START KALIBRACJI<br>3. Jedz 10m<br>4. STOP = automatyczny zapis</p>
<div class="control-buttons">
<button class="btn btn-start" onclick="startCalib()">START KALIBRACJI</button>
<button class="btn btn-stop" onclick="sendCmd('stop')">STOP (zapisz)</button>
</div>
</div>
<div style="background:#2a2a3e;padding:20px;border-radius:12px;">
<div style="display:grid;grid-template-columns:1fr 1fr;gap:15px;">
<div><strong>Impulsy:</strong><br><span style="font-size:2em;color:#00ff88;" id="pulsesC">0</span></div>
<div><strong>Kalibracja:</strong><br><span style="font-size:2em;color:#ffaa00;" id="calibC">100.0</span> imp/m</div>
</div>
</div>
</div>

<div class="tab-content" id="tab3">
<h3>Raporty pracy</h3>
<div style="background:#2a2a3e;padding:20px;border-radius:12px;margin-bottom:20px;">
<strong>Status:</strong> <span id="rptActive">-</span> | <strong>Storage:</strong> <span id="rptStorage">-</span>
</div>
<div id="reportsList"><p style="color:#999;text-align:center;">Ladowanie...</p></div>
<div style="display:flex;gap:10px;margin-top:15px;">
<button class="btn" onclick="loadReports()" style="flex:1;background:#00d4ff;color:#1a1a2e;">Odswiez</button>
<button class="btn" onclick="deleteAll()" style="flex:1;background:#e94560;">Usun wszystkie</button>
</div>
<div id="rptDetail" style="display:none;background:#2a2a3e;padding:20px;border-radius:12px;margin-top:20px;">
<div style="display:flex;justify-content:space-between;margin-bottom:10px;">
<strong id="rptTitle">Raport</strong>
<div><button class="btn" onclick="exportCSV()" style="padding:5px 15px;background:#00ff88;color:#1a1a2e;">CSV</button> <button class="btn" onclick="delReport()" style="padding:5px 15px;background:#e94560;">Usun</button> <button class="btn" onclick="closeRpt()" style="padding:5px 15px;background:#666;">X</button></div>
</div>
<pre id="rptContent" style="background:#1a1a2e;padding:15px;border-radius:8px;color:#ccc;white-space:pre-wrap;max-height:400px;overflow-y:auto;"></pre>
</div>
</div>

<div class="footer">Trassar251 v7.0.0 SD-SPEED | WiFi: Trassar-Painter | 192.168.4.1</div>
</div>

<div id="menuModal" class="modal">
<div class="modal-content">
<div class="modal-header"><h2>Menu</h2><span class="close-modal" onclick="closeMenu()">&times;</span></div>
<div class="menu-item" onclick="closeMenu();showTab(2)"><div class="menu-icon">&#9881;</div><div class="menu-text"><h3>Kalibracja</h3><p>Automatyczna kalibracja enkodera</p></div></div>
<div class="menu-item" onclick="closeMenu();showTab(3)"><div class="menu-icon">&#128202;</div><div class="menu-text"><h3>Raporty</h3><p>Przegladaj raporty pracy</p></div></div>
<div class="menu-item" onclick="if(confirm('Uruchomic OTA?')){sendCmd('menu')}"><div class="menu-icon">&#128260;</div><div class="menu-text"><h3>Aktualizacja OTA</h3><p>Firmware przez WiFi</p></div></div>
<button class="btn" onclick="closeMenu()" style="background:#666;width:100%;margin-top:15px;">Zamknij</button>
</div>
</div>

<script>
const P=[
{n:'P-1a',d:'Przerywana dluga'},{n:'P-1b',d:'Przerywana krotka'},
{n:'P-1c',d:'Wydzielajaca'},{n:'P-1d',d:'Prowadzaca waska'},
{n:'P-1e',d:'Prowadz. szeroka'},{n:'P-2a',d:'Ciagla waska'},
{n:'P-2b',d:'Ciagla szeroka'},{n:'P-3a',d:'Przekraczalna dl.'},
{n:'P-3b',d:'Przekraczalna kr.'},{n:'P-4',d:'Podwojna ciagla'},
{n:'P-6',d:'Ostrzegawcza'},{n:'P-7a',d:'Kraw. przeryw. sz.'},
{n:'P-7b',d:'Kraw. ciagla sz.'},{n:'P-7c',d:'Kraw. przeryw. w.'},
{n:'P-7d',d:'Kraw. ciagla w.'}
];
let viewId=-1,lastMode='idle';

function showTab(i){document.querySelectorAll('.tab').forEach((t,j)=>t.classList.toggle('active',j===i));document.querySelectorAll('.tab-content').forEach((t,j)=>t.classList.toggle('active',j===i));if(i===3)loadReports()}

const g=document.getElementById('patternGrid'),sg=document.getElementById('serviceGrid');
P.forEach((p,i)=>{
let b=document.createElement('button');b.className='pattern-btn';b.innerHTML='<div>'+p.n+'</div><div class="pattern-desc">'+p.d+'</div>';b.onclick=()=>chgPat(i);b.id='p'+i;g.appendChild(b);
let s=document.createElement('button');s.className='service-pattern-btn';s.innerHTML='<div>'+p.n+'</div><div class="pattern-desc">'+p.d+'</div>';s.id='sp'+i;
s.onmousedown=()=>testPat(i,1);s.onmouseup=()=>testPat(i,0);s.onmouseleave=()=>testPat(i,0);
s.ontouchstart=e=>{e.preventDefault();testPat(i,1)};s.ontouchend=e=>{e.preventDefault();testPat(i,0)};
sg.appendChild(s);
});

function sendCmd(c){fetch('/api/cmd',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'c='+c}).then(r=>r.text()).then(()=>update())}
function chgPat(i){fetch('/api/pattern',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'p='+i}).then(()=>update())}
function testPat(i,s){fetch('/api/service',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'pattern='+i+'&state='+s})}
function toggleP3(){fetch('/api/p3selector',{method:'POST'}).then(()=>update())}
function startCalib(){fetch('/api/calibration/start',{method:'POST'}).then(()=>update())}
function resetEStop(){fetch('/api/safety/reset-estop',{method:'POST'}).then(()=>update())}

function update(){
fetch('/api/status').then(r=>r.json()).then(d=>{
document.getElementById('statusDot').className='status-indicator status-'+d.mode;
document.getElementById('modeText').textContent=d.mode.toUpperCase();

// E-STOP banner
document.getElementById('estopBanner').style.display=d.emergencyStop?'block':'none';

// Start/Pause button - v7.0.0
const btn=document.getElementById('btnStartPause');
if(d.mode==='working'){btn.textContent='PAUZA';btn.className='btn';btn.style.background='linear-gradient(135deg,#ffaa00,#cc8800)';btn.style.color='#1a1a2e';btn.onclick=()=>sendCmd('pause')}
else{btn.textContent='START MALOWANIA';btn.className='btn btn-start';btn.style.background='';btn.style.color='';btn.onclick=()=>sendCmd('start')}

if(d.mode==='service'){document.getElementById('normalMode').style.display='none';document.getElementById('serviceMode').style.display='block'}
else{document.getElementById('normalMode').style.display='block';document.getElementById('serviceMode').style.display='none'}

document.getElementById('currentPattern').textContent=d.pattern+' ('+d.patternDesc+')';
document.getElementById('selectorP3').textContent=d.selectorP3;
document.getElementById('sdStatus').textContent=d.sdCard?'SD ('+d.sdTotalMB+'MB)':'LittleFS';
document.getElementById('p3Physical').textContent=d.selectorP3Physical?'ODWROCONE':'NORMALNE';
let p3b=document.getElementById('btnP3Selector'),p3t=document.getElementById('p3SelectorText');
if(d.selectorP3Virtual){p3b.classList.add('active');p3t.textContent='ODWROCONE'}else{p3b.classList.remove('active');p3t.textContent='NORMALNE'}

P.forEach((p,i)=>document.getElementById('p'+i).classList.toggle('active',i===d.patternIndex));
d.guns.forEach((a,i)=>document.getElementById('gun'+i).classList.toggle('active',a));

document.getElementById('distance').textContent=d.encoder.distance.toFixed(2);
document.getElementById('speed').textContent=d.encoder.speed.toFixed(1);
document.getElementById('pulses').textContent=d.encoder.pulses;
document.getElementById('distM').textContent=d.encoder.distance.toFixed(2);
document.getElementById('spdM').textContent=d.encoder.speed.toFixed(1);
document.getElementById('pulsesC').textContent=d.encoder.pulses;
document.getElementById('calibC').textContent=d.encoder.calibration.toFixed(1);

// Speed indicator
let sb=document.getElementById('speedBox');
sb.className='measure-box '+(d.speedSufficient?'speed-ok':'speed-low');
document.getElementById('speedWarning').style.display=(d.mode==='working'&&!d.speedSufficient)?'block':'none';

// Cycle info
let ci=document.getElementById('cycleInfo');
if(d.mode==='working'&&d.patternDashed){ci.style.display='block';let ct=document.getElementById('cycleText');if(d.cycleInLine){ct.className='cycle-line';ct.textContent='LINIA: '+d.cycleDistance.toFixed(2)+'/'+d.patternLine.toFixed(1)+'m'}else{ct.className='cycle-gap';ct.textContent='PRZERWA: '+(d.cycleDistance-d.patternLine).toFixed(2)+'/'+d.patternGap.toFixed(1)+'m'}}else{ci.style.display='none'}

if(d.startFromGap&&d.gapDistance>0){document.getElementById('gapProgress').style.display='block';document.getElementById('gapTraveled').textContent=d.gapTraveled.toFixed(2);document.getElementById('gapTotal').textContent=d.gapDistance.toFixed(1);document.getElementById('gapBar').style.width=Math.min((d.gapTraveled/d.gapDistance)*100,100)+'%'}else{document.getElementById('gapProgress').style.display='none'}
}).catch(()=>{})}

document.getElementById('chkGap').addEventListener('change',e=>{fetch('/api/option',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'startFromGap='+(e.target.checked?'1':'0')})});
setInterval(update,500);

function loadReports(){
fetch('/api/reports').then(r=>r.json()).then(d=>{
document.getElementById('rptStorage').textContent=d.storage||'LittleFS';
document.getElementById('rptActive').textContent=d.count+' raportow';
let el=document.getElementById('reportsList');
if(!d.count){el.innerHTML='<p style="color:#999;text-align:center;">Brak raportow</p>';return}
let h='';for(let i=d.count-1;i>=0;i--)h+='<button class="btn" onclick="viewRpt('+i+')" style="text-align:left;background:#1a1a2e;padding:15px;margin:5px 0;width:100%;">Raport #'+(i+1)+'</button>';
el.innerHTML=h;
})}

function viewRpt(id){viewId=id;fetch('/api/report?id='+id).then(r=>r.text()).then(c=>{document.getElementById('rptTitle').textContent='Raport #'+(id+1);document.getElementById('rptContent').textContent=c;document.getElementById('rptDetail').style.display='block'})}
function exportCSV(){if(viewId>=0)window.open('/api/report/csv?id='+viewId,'_blank')}
function closeRpt(){document.getElementById('rptDetail').style.display='none'}
function delReport(){if(viewId>=0&&confirm('Usunac raport #'+(viewId+1)+'?')){fetch('/api/report/delete?id='+viewId,{method:'DELETE'}).then(()=>{closeRpt();loadReports()})}}
function deleteAll(){if(confirm('Usunac WSZYSTKIE raporty?')){fetch('/api/reports/delete-all',{method:'DELETE'}).then(()=>loadReports())}}

function showMenu(){document.getElementById('menuModal').style.display='block'}
function closeMenu(){document.getElementById('menuModal').style.display='none'}
window.onclick=e=>{if(e.target==document.getElementById('menuModal'))closeMenu()}

update();loadReports();
</script>
</body></html>
)HTMLCODE";
}

// ============================================================================
// WEB SERVER ENDPOINTS
// ============================================================================

void setupWebServer() {
    // CORS preflight
    server.on("/api/*", HTTP_OPTIONS, []() {
        addCORSHeaders();
        server.send(204);
    });

    server.on("/", HTTP_GET, []() {
        server.send(200, "text/html", getHTMLPage());
    });

    server.on("/api/status", HTTP_GET, []() {
        addCORSHeaders();
        server.send(200, "application/json", getStatusJSON());
    });

    // v7.0.0: start = START/PAUZA (jeden przycisk)
    server.on("/api/cmd", HTTP_POST, []() {
        addCORSHeaders();
        if (server.hasArg("c")) {
            String cmd = server.arg("c");
            if (cmd == "start") {
                if (currentMode == MODE_WORKING) {
                    pauseSystem();  // Jeśli już maluje -> pauza
                } else {
                    startSystem();  // Jeśli nie maluje -> start
                }
            }
            else if (cmd == "pause") pauseSystem();
            else if (cmd == "measure") startMeasuring();
            else if (cmd == "service") startService();
            else if (cmd == "menu") enterMenu();
            else if (cmd == "stop") stopSystem();
            server.send(200, "text/plain", "OK");
        } else {
            server.send(400, "text/plain", "Missing 'c'");
        }
    });

    server.on("/api/pattern", HTTP_POST, []() {
        addCORSHeaders();
        if (server.hasArg("p")) {
            int idx = server.arg("p").toInt();
            if (idx >= 0 && idx < PATTERN_COUNT) {
                changePattern(idx);
                server.send(200, "text/plain", "OK");
            } else {
                server.send(400, "text/plain", "Invalid pattern index");
            }
        } else {
            server.send(400, "text/plain", "Missing 'p'");
        }
    });

    server.on("/api/service", HTTP_POST, []() {
        addCORSHeaders();
        if (server.hasArg("pattern") && server.hasArg("state")) {
            int pattern = server.arg("pattern").toInt();
            bool state = server.arg("state") == "1";
            if (currentMode == MODE_SERVICE && pattern >= 0 && pattern < PATTERN_COUNT) {
                serviceTestPattern = state ? pattern : -1;
                updateGuns();
            }
            server.send(200, "text/plain", "OK");
        } else {
            server.send(400, "text/plain", "Missing params");
        }
    });

    server.on("/api/p3selector", HTTP_POST, []() {
        addCORSHeaders();
        selectorP3Virtual = !selectorP3Virtual;
        if (currentMode == MODE_WORKING) updateGuns();
        server.send(200, "text/plain", "OK");
    });

    server.on("/api/calibration/start", HTTP_POST, []() {
        addCORSHeaders();
        startCalibration();
        server.send(200, "text/plain", "OK");
    });

    server.on("/api/option", HTTP_POST, []() {
        addCORSHeaders();
        if (server.hasArg("startFromGap")) {
            startFromGap = server.arg("startFromGap") == "1";
        }
        server.send(200, "text/plain", "OK");
    });

    // Raporty
    server.on("/api/reports", HTTP_GET, []() {
        addCORSHeaders();
        server.send(200, "application/json", getReportsListJSON());
    });

    server.on("/api/report", HTTP_GET, []() {
        addCORSHeaders();
        if (server.hasArg("id")) {
            int id = server.arg("id").toInt();
            if (id >= 0 && id < reportCount) {
                server.send(200, "text/plain; charset=utf-8", getReportContent(id));
            } else {
                server.send(400, "text/plain", "Invalid report id");
            }
        } else {
            server.send(400, "text/plain", "Missing 'id'");
        }
    });

    server.on("/api/report/csv", HTTP_GET, []() {
        addCORSHeaders();
        if (server.hasArg("id")) {
            int id = server.arg("id").toInt();
            if (id >= 0 && id < reportCount) {
                String csv = exportReportCSV(id);
                server.sendHeader("Content-Disposition", "attachment; filename=raport_" + String(id) + ".csv");
                server.send(200, "text/csv; charset=utf-8", csv);
            } else {
                server.send(400, "text/plain", "Invalid report id");
            }
        } else {
            server.send(400, "text/plain", "Missing 'id'");
        }
    });

    server.on("/api/report/current", HTTP_GET, []() {
        addCORSHeaders();
        String json;
        json.reserve(256);
        json = "{\"active\":";
        json += (reportActive ? "true" : "false");
        json += ",\"startDate\":\"" + String(currentReport.startDate) + "\"";
        json += ",\"startTime\":\"" + String(currentReport.startTime) + "\"";
        json += ",\"currentTime\":\"" + getCurrentDateTime() + "\"}";
        server.send(200, "application/json", json);
    });

    // v7.0.0: Usuwanie raportów
    server.on("/api/report/delete", HTTP_DELETE, []() {
        addCORSHeaders();
        if (server.hasArg("id")) {
            int id = server.arg("id").toInt();
            if (deleteReport(id)) {
                server.send(200, "text/plain", "OK");
            } else {
                server.send(400, "text/plain", "Failed");
            }
        } else {
            server.send(400, "text/plain", "Missing 'id'");
        }
    });

    server.on("/api/reports/delete-all", HTTP_DELETE, []() {
        addCORSHeaders();
        deleteAllReports();
        server.send(200, "text/plain", "OK");
    });

    // Safety API
    server.on("/api/safety/status", HTTP_GET, []() {
        addCORSHeaders();
        String json;
        json.reserve(512);
        json = "{\"emergencyStop\":";
        json += (emergencyStopActive ? "true" : "false");
        json += ",\"selfTestPassed\":";
        json += (selfTestPassed ? "true" : "false");
        json += ",\"errorCount\":";
        json += String(errorCount);
        json += ",\"sdCard\":";
        json += (sdCardAvailable ? "true" : "false");
        json += "}";
        server.send(200, "application/json", json);
    });

    server.on("/api/safety/errors", HTTP_GET, []() {
        addCORSHeaders();
        String json;
        json.reserve(2048);
        json = "[";
        int count = min(errorCount, ERROR_LOG_SIZE);
        for (int i = 0; i < count; i++) {
            int idx = (errorLogIndex - count + i + ERROR_LOG_SIZE) % ERROR_LOG_SIZE;
            if (i > 0) json += ",";
            json += "{\"type\":";
            json += String(errorLog[idx].type);
            json += ",\"timestamp\":\"";
            json += String(errorLog[idx].timestamp);
            json += "\",\"message\":\"";
            json += String(errorLog[idx].message);
            json += "\"}";
        }
        json += "]";
        server.send(200, "application/json", json);
    });

    // v7.0.0: E-STOP reset wymaga Basic Auth
    server.on("/api/safety/reset-estop", HTTP_POST, []() {
        addCORSHeaders();
        if (!checkAuth()) return;
        resetEmergencyStop();
        server.send(200, "text/plain", "OK");
    });

    server.onNotFound([]() {
        server.sendHeader("Location", "/");
        server.send(302);
    });

    server.begin();
    Serial.println("[WEB] Serwer HTTP uruchomiony na porcie 80");
}
