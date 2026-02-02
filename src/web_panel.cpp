/**
 * @file web_panel.cpp
 * @brief Panel WWW i REST API - Trassar-Painter v6.0.0
 *
 * NAPRAWIONE w v6.0.0:
 * - Walidacja zakresu danych wejściowych API (pattern index, report id)
 * - getReportsListJSON() zwraca pole "count"
 * - String.reserve() dla JSON (minimalizacja fragmentacji)
 * - Spójne wersjonowanie w interfejsie
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

// Zewnętrzne obiekty i funkcje
extern WebServer server;
extern void startSystem();
extern void stopSystem();
extern void pauseSystem();
extern void startMeasuring();
extern void startService();
extern void enterMenu();
extern void changePattern(int newPattern);
extern void updateGuns();

// ============================================================================
// STATUS JSON
// ============================================================================

String getStatusJSON() {
    String json;
    json.reserve(1024);

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

    // Statystyki
    json += "\"stats\":{";
    json += "\"patternChanges\":" + String(patternChangeCount) + ",";
    json += "\"totalWorkTime\":" + String(totalWorkTime / 1000) + ",";
    json += "\"uptime\":" + String(millis() / 1000);
    json += "}}";

    return json;
}

// ============================================================================
// HTML PAGE
// ============================================================================

String getHTMLPage() {
    return R"HTMLCODE(<!DOCTYPE html>
<html lang="pl">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Trassar Painter - Komputer Malowarki v6.0.0</title>
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
.measurements{display:grid;grid-template-columns:repeat(3,1fr);gap:15px;margin:15px 0}
.measure-box{background:linear-gradient(135deg,#2a2a3e,#1e1e2e);padding:20px;border-radius:12px;text-align:center;border:1px solid #3a3a5a}
.measure-value{font-size:2.2em;font-weight:bold;color:#00d4ff;margin-bottom:5px}
.measure-label{font-size:0.85em;color:#999}
h3{margin:20px 0 12px 0;color:#00d4ff;font-size:1.1em;border-bottom:2px solid #00d4ff;padding-bottom:5px}
.pattern-grid{display:grid;grid-template-columns:repeat(5,1fr);gap:10px;margin:15px 0}
.pattern-btn{padding:14px 8px;border:2px solid #3a3a5a;border-radius:10px;background:#252540;cursor:pointer;font-weight:bold;font-size:0.95em;transition:all 0.2s;text-align:center;color:#eaeaea}
.pattern-btn:hover{background:#2e2e50;transform:translateY(-2px);box-shadow:0 4px 12px rgba(0,212,255,0.3)}
.pattern-btn.active{transform:scale(1.05);background:#00d4ff;color:#1a1a2e;border-color:#00d4ff;box-shadow:0 6px 20px rgba(0,212,255,0.5)}
.pattern-desc{font-size:0.7em;opacity:0.8;margin-top:3px}
.service-pattern-btn{padding:20px 10px;border:2px solid #3a3a5a;border-radius:10px;background:#252540;cursor:pointer;font-weight:bold;font-size:1em;text-align:center;color:#eaeaea;user-select:none;-webkit-user-select:none;touch-action:manipulation}
.service-pattern-btn:active{background:#00ff88;color:#1a1a2e;border-color:#00ff88;box-shadow:0 0 25px rgba(0,255,136,0.7)}
.service-info{background:#2a2a3e;padding:15px;border-radius:10px;margin:15px 0;border:2px solid #ff6b9d;color:#ff6b9d;text-align:center}
.options{display:grid;grid-template-columns:repeat(2,1fr);gap:15px;margin:15px 0}
.option-box{background:#252540;padding:15px;border-radius:10px;border:1px solid #3a3a5a}
.option-box label{display:flex;align-items:center;gap:10px;cursor:pointer}
.option-box input[type="checkbox"]{width:20px;height:20px;cursor:pointer}
.control-buttons{display:grid;grid-template-columns:repeat(2,1fr);gap:12px;margin:20px 0}
.btn{padding:20px;border:none;border-radius:12px;font-size:1.15em;font-weight:bold;cursor:pointer;transition:all 0.3s;text-transform:uppercase;box-shadow:0 4px 15px rgba(0,0,0,0.4)}
.btn:hover{transform:translateY(-3px);box-shadow:0 6px 20px rgba(0,0,0,0.5)}
.btn:active{transform:translateY(-1px)}
.btn-start{background:linear-gradient(135deg,#00ff88,#00cc6e);color:#1a1a2e}
.btn-measure{background:linear-gradient(135deg,#00d4ff,#00a0cc);color:#1a1a2e}
.btn-service{background:linear-gradient(135deg,#ff6b9d,#e94560);color:#fff}
.btn-pause{background:linear-gradient(135deg,#ffaa00,#cc8800);color:#1a1a2e}
.btn-stop{background:linear-gradient(135deg,#e94560,#cc3344);color:#fff}
.btn-menu{background:linear-gradient(135deg,#9b59b6,#8e44ad);color:#fff}
.guns-display{display:grid;grid-template-columns:repeat(6,1fr);gap:10px;margin:15px 0}
.gun{text-align:center;padding:18px 8px;border-radius:10px;background:#252540;border:3px solid #3a3a5a;transition:all 0.3s}
.gun.active{background:linear-gradient(135deg,#00ff88,#00cc6e);color:#1a1a2e;border-color:#00ff88;box-shadow:0 0 25px rgba(0,255,136,0.7);transform:scale(1.05)}
.gun-label{font-size:1em;font-weight:bold;margin-bottom:5px}
.gun-size{font-size:0.75em;opacity:0.8}
.selector-toggle{background:#252540;padding:20px;border-radius:10px;border:2px solid #ffaa00;text-align:center;margin:15px 0}
.selector-toggle button{padding:15px 30px;background:#ffaa00;color:#1a1a2e;border:none;border-radius:10px;cursor:pointer;font-weight:bold;font-size:1.2em;transition:all 0.3s}
.selector-toggle button:hover{transform:scale(1.05)}
.selector-toggle button.active{background:#ff6b00;box-shadow:0 0 20px rgba(255,170,0,0.7)}
.footer{text-align:center;margin-top:20px;opacity:0.6;font-size:0.85em}
.modal{display:none;position:fixed;z-index:1000;left:0;top:0;width:100%;height:100%;background:rgba(0,0,0,0.8);animation:fadeIn 0.3s}
.modal-content{background:#1a1a2e;margin:10% auto;padding:30px;border:2px solid #00ff88;border-radius:15px;width:90%;max-width:500px;animation:slideDown 0.3s}
.modal-header{display:flex;justify-content:space-between;align-items:center;margin-bottom:20px}
.modal-header h2{margin:0;color:#00ff88}
.close-modal{font-size:2em;cursor:pointer;color:#999;transition:color 0.3s}
.close-modal:hover{color:#e94560}
.menu-item{background:#2a2a3e;padding:20px;margin:10px 0;border-radius:10px;border:2px solid #444;cursor:pointer;transition:all 0.3s;display:flex;align-items:center;gap:15px}
.menu-item:hover{border-color:#00ff88;transform:translateX(5px);box-shadow:0 0 15px rgba(0,255,136,0.3)}
.menu-icon{font-size:2em}
.menu-text h3{margin:0 0 5px 0;color:#00ff88;border:none}
.menu-text p{margin:0;color:#999;font-size:0.9em}
@keyframes fadeIn{from{opacity:0}to{opacity:1}}
@keyframes slideDown{from{transform:translateY(-50px);opacity:0}to{transform:translateY(0);opacity:1}}
@media(max-width:600px){.pattern-grid{grid-template-columns:repeat(3,1fr)}.control-buttons{grid-template-columns:1fr}.guns-display{grid-template-columns:repeat(3,1fr)}.measurements{grid-template-columns:1fr}.status-bar{grid-template-columns:1fr}.modal-content{margin:20% auto;width:95%}}
</style>
</head>
<body>
<div class="container">
<div class="card">
<h1>Trassar Painter - Komputer Malowarki</h1>
<div class="subtitle">Professional Road Marking System v6.0.0 MODULAR PRODUCTION</div>

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
<div class="status-item"><strong>Selektor P3:</strong> <span id="selectorP3">-</span></div>
</div>

<h3>Pomiary (Enkoder)</h3>
<div class="measurements">
<div class="measure-box"><div class="measure-value" id="distance">0.00</div><div class="measure-label">Dystans (m)</div></div>
<div class="measure-box"><div class="measure-value" id="speed">0.0</div><div class="measure-label">Predkosc (km/h)</div></div>
<div class="measure-box"><div class="measure-value" id="pulses">0</div><div class="measure-label">Impulsy enkodera</div></div>
</div>

<div id="normalMode">
<div class="selector-toggle">
<h3 style="border:none;margin:0 0 15px 0;">Selektor P3 (Wirtualny)</h3>
<button id="btnP3Selector" onclick="toggleP3Selector()">P3: <span id="p3SelectorText">NORMALNE</span></button>
<div style="margin-top:10px;font-size:0.85em;color:#999;">Fizyczny selektor (GPIO 20): <span id="p3Physical">-</span></div>
</div>

<h3>Opcje</h3>
<div class="options">
<div class="option-box"><label><input type="checkbox" id="chkStartFromGap"><strong>Start od przerwy</strong></label><div style="margin-top:10px;font-size:0.85em;color:#999;">Automatycznie uzywa przerwy z wzorca</div></div>
<div class="option-box" id="gapProgress" style="display:none;"><strong>Przerwa:</strong> <span id="gapTraveled">0.00</span> / <span id="gapTotal">0.0</span> m<div style="width:100%;background:#3a3a5a;height:10px;border-radius:5px;margin-top:10px;"><div id="gapBar" style="width:0%;background:#00ff88;height:100%;border-radius:5px;"></div></div></div>
</div>

<h3>Wybor wzorca (15)</h3>
<div class="pattern-grid" id="patternGrid"></div>

<h3>Sterowanie</h3>
<div class="control-buttons">
<button class="btn btn-start" onclick="sendCmd('start')">START MALOWANIA</button>
<button class="btn btn-service" onclick="sendCmd('service')">TRYB SERWISOWY</button>
<button class="btn btn-pause" onclick="sendCmd('pause')">PAUSE</button>
<button class="btn btn-menu" onclick="showMenuModal()">MENU</button>
<button class="btn btn-stop" onclick="sendCmd('stop')">STOP</button>
</div>
</div>

<div id="serviceMode" style="display:none;">
<div class="service-info"><h3 style="color:#ff6b9d;border:none;margin:0;">TRYB SERWISOWY - TEST PISTOLETOW</h3><p style="margin-top:10px;">Trzymaj przycisk wzorca aby aktywowac pistolety. Puszczenie = wylaczenie.</p></div>
<h3>Test wzorcow (przytrzymaj przycisk)</h3>
<div class="pattern-grid" id="servicePatternGrid"></div>
<div style="text-align:center;margin-top:20px;"><button class="btn btn-stop" onclick="sendCmd('stop')" style="width:50%;">WYJDZ Z TRYBU SERWISOWEGO</button></div>
</div>

<h3>Pistolety (Status Live)</h3>
<div class="guns-display">
<div class="gun" id="gun0"><div class="gun-label">P1</div><div class="gun-size">12cm</div></div>
<div class="gun" id="gun1"><div class="gun-label">P2</div><div class="gun-size">12cm</div></div>
<div class="gun" id="gun2"><div class="gun-label">P3</div><div class="gun-size">12cm</div></div>
<div class="gun" id="gun3"><div class="gun-label">P4</div><div class="gun-size">24cm</div></div>
<div class="gun" id="gun4"><div class="gun-label">P5</div><div class="gun-size">12cm K</div></div>
<div class="gun" id="gun5"><div class="gun-label">P6</div><div class="gun-size">24cm K</div></div>
</div>
</div>

<!-- TAB 1: Pomiar -->
<div class="tab-content" id="tab1">
<h3>Tryb pomiaru dystansu</h3>
<p style="margin:15px 0;color:#999;">Pomiar polega na mierzeniu odleglosci przejechanej od momentu wcisniecia startu. W trybie pomiaru pistolety NIE SA aktywne.</p>
<div class="measurements">
<div class="measure-box"><div class="measure-value" id="distanceMeasure">0.00</div><div class="measure-label">Dystans (m)</div></div>
<div class="measure-box"><div class="measure-value" id="speedMeasure">0.0</div><div class="measure-label">Predkosc (km/h)</div></div>
<div class="measure-box"><div class="measure-value" id="pulsesMeasure">0</div><div class="measure-label">Impulsy enkodera</div></div>
</div>
<div class="control-buttons">
<button class="btn btn-measure" onclick="sendCmd('measure')">START POMIARU</button>
<button class="btn btn-stop" onclick="sendCmd('stop')">STOP</button>
</div>
</div>

<!-- TAB 2: Kalibracja -->
<div class="tab-content" id="tab2">
<h3>Kalibracja automatyczna enkodera</h3>
<div style="background:#2a2a3e;padding:25px;border-radius:12px;margin-bottom:20px;border:2px solid #00ff88;">
<h4 style="color:#00ff88;margin-bottom:15px;">KALIBRACJA AUTOMATYCZNA</h4>
<div style="color:#ccc;margin-bottom:20px;line-height:1.6;">
<strong>Procedura kalibracji:</strong><br>
1. Przygotuj miare - zaznacz dokladnie <strong style="color:#00ff88;">10 metrow</strong><br>
2. Ustaw maszyne na poczatku odcinka<br>
3. Nacisnij <strong>START KALIBRACJI</strong><br>
4. Jedz powoli dokladnie 10 metrow<br>
5. Zatrzymaj sie na koncu i nacisnij <strong>STOP</strong><br>
6. System <strong>automatycznie obliczy i zapisze</strong> kalibracje!
</div>
<div class="control-buttons">
<button class="btn btn-start" onclick="startCalib()" style="font-size:1.1em;">START KALIBRACJI</button>
<button class="btn btn-stop" onclick="sendCmd('stop')" style="font-size:1.1em;">STOP (zapisz)</button>
</div>
</div>
<div style="background:#2a2a3e;padding:20px;border-radius:12px;border:2px solid #e94560;">
<h4 style="color:#e94560;margin-bottom:10px;">Aktualne dane kalibracji</h4>
<div style="display:grid;grid-template-columns:1fr 1fr;gap:15px;margin-top:15px;">
<div><strong style="color:#00d4ff;">Impulsy:</strong><br><span style="font-size:2em;color:#00ff88;" id="pulsesCalib">0</span></div>
<div><strong style="color:#00d4ff;">Kalibracja:</strong><br><span style="font-size:2em;color:#ffaa00;" id="calibCurrent">100.0</span> imp/m</div>
</div>
<div style="margin-top:15px;padding-top:15px;border-top:1px solid #444;"><strong style="color:#00d4ff;">Status:</strong> <span id="calibStatus" style="color:#00ff88;">Oczekiwanie na START</span></div>
</div>
</div>

<!-- TAB 3: Raporty -->
<div class="tab-content" id="tab3">
<h3>Raporty pracy</h3>
<div style="background:#2a2a3e;padding:20px;border-radius:12px;margin-bottom:20px;border:2px solid #00ff88;">
<h4 style="color:#00ff88;margin-bottom:15px;">Biezacy raport</h4>
<div id="currentReportStatus">
<div style="color:#ccc;"><strong>Status:</strong> <span id="reportActive" style="color:#ffaa00;">Ladowanie...</span><br><strong>Data rozpoczecia:</strong> <span id="reportStartDate">-</span><br><strong>Czas rozpoczecia:</strong> <span id="reportStartTime">-</span><br><strong>Aktualny czas:</strong> <span id="reportCurrentTime">-</span></div>
</div>
</div>
<div style="background:#2a2a3e;padding:20px;border-radius:12px;border:2px solid #00d4ff;">
<h4 style="color:#00d4ff;margin-bottom:15px;">Zapisane raporty</h4>
<div id="reportsList" style="max-height:400px;overflow-y:auto;"><p style="color:#999;text-align:center;">Ladowanie raportow...</p></div>
<div style="display:flex;gap:10px;margin-top:15px;">
<button class="btn" onclick="loadReports()" style="flex:1;background:#00d4ff;">Odswiez liste</button>
</div>
</div>
<div id="reportDetails" style="display:none;background:#2a2a3e;padding:20px;border-radius:12px;margin-top:20px;border:2px solid #e94560;">
<div style="display:flex;justify-content:space-between;align-items:center;margin-bottom:15px;">
<h4 style="color:#e94560;margin:0;">Szczegoly raportu</h4>
<div><button class="btn" onclick="exportCSV()" style="padding:5px 15px;background:#00ff88;color:#1a1a2e;margin-right:5px;">CSV</button><button class="btn" onclick="closeReportDetails()" style="padding:5px 15px;background:#666;">X Zamknij</button></div>
</div>
<pre id="reportContent" style="background:#1a1a2e;padding:15px;border-radius:8px;color:#ccc;font-size:0.9em;white-space:pre-wrap;overflow-x:auto;max-height:500px;overflow-y:auto;"></pre>
</div>
</div>
</div>

<div class="footer">Trassar251 Professional System v6.0.0 MODULAR | WiFi: Trassar-Painter | 192.168.4.1</div>
</div>

<script>
const patterns=[
{name:'P-1a',desc:'Przerywana dluga'},{name:'P-1b',desc:'Przerywana krotka'},
{name:'P-1c',desc:'Wydzielajaca'},{name:'P-1d',desc:'Prowadzaca waska'},
{name:'P-1e',desc:'Prowadz. szeroka'},{name:'P-2a',desc:'Ciagla waska'},
{name:'P-2b',desc:'Ciagla szeroka'},{name:'P-3a',desc:'Przekraczalna dl.'},
{name:'P-3b',desc:'Przekraczalna kr.'},{name:'P-4',desc:'Podwojna ciagla'},
{name:'P-6',desc:'Ostrzegawcza'},{name:'P-7a',desc:'Kraw. przeryw. sz.'},
{name:'P-7b',desc:'Kraw. ciagla sz.'},{name:'P-7c',desc:'Kraw. przeryw. w.'},
{name:'P-7d',desc:'Kraw. ciagla w.'}
];

let currentViewReportId=-1;

function showTab(idx){
document.querySelectorAll('.tab').forEach((t,i)=>{t.classList.toggle('active',i===idx)});
document.querySelectorAll('.tab-content').forEach((t,i)=>{t.classList.toggle('active',i===idx)});
if(idx===3){loadCurrentReport();loadReports();}
}

const grid=document.getElementById('patternGrid');
patterns.forEach((p,i)=>{
const btn=document.createElement('button');
btn.className='pattern-btn';btn.innerHTML='<div>'+p.name+'</div><div class="pattern-desc">'+p.desc+'</div>';
btn.onclick=()=>changePattern(i);btn.id='pattern'+i;grid.appendChild(btn);
});

const serviceGrid=document.getElementById('servicePatternGrid');
patterns.forEach((p,i)=>{
const btn=document.createElement('button');btn.className='service-pattern-btn';
btn.innerHTML='<div>'+p.name+'</div><div class="pattern-desc">'+p.desc+'</div>';btn.id='servicePattern'+i;
btn.onmousedown=()=>testPattern(i,true);btn.onmouseup=()=>testPattern(i,false);btn.onmouseleave=()=>testPattern(i,false);
btn.ontouchstart=(e)=>{e.preventDefault();testPattern(i,true)};btn.ontouchend=(e)=>{e.preventDefault();testPattern(i,false)};
serviceGrid.appendChild(btn);
});

function sendCmd(cmd){fetch('/api/cmd',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'c='+cmd}).then(r=>r.text()).then(d=>{if(d==='OK')updateStatus()})}
function changePattern(idx){fetch('/api/pattern',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'p='+idx}).then(r=>r.text()).then(d=>{if(d==='OK')updateStatus()})}
function testPattern(idx,active){fetch('/api/service',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'pattern='+idx+'&state='+(active?'1':'0')})}
function toggleP3Selector(){fetch('/api/p3selector',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:''}).then(r=>r.text()).then(d=>{if(d==='OK')updateStatus()})}
function startCalib(){fetch('/api/calibration/start',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:''}).then(r=>r.text()).then(d=>{if(d==='OK')updateStatus()})}

function updateStatus(){
fetch('/api/status').then(r=>r.json()).then(data=>{
const dot=document.getElementById('statusDot');
dot.className='status-indicator status-'+data.mode;
document.getElementById('modeText').textContent=data.mode.toUpperCase();

if(data.mode==='service'){document.getElementById('normalMode').style.display='none';document.getElementById('serviceMode').style.display='block'}
else{document.getElementById('normalMode').style.display='block';document.getElementById('serviceMode').style.display='none'}

document.getElementById('currentPattern').textContent=data.pattern+' ('+data.patternDesc+')';
document.getElementById('selectorP3').textContent=data.selectorP3;
document.getElementById('p3Physical').textContent=data.selectorP3Physical?'HIGH (odwrocone)':'LOW (normalne)';

const p3Btn=document.getElementById('btnP3Selector');const p3Text=document.getElementById('p3SelectorText');
if(data.selectorP3Virtual){p3Btn.classList.add('active');p3Text.textContent='ODWROCONE'}else{p3Btn.classList.remove('active');p3Text.textContent='NORMALNE'}

patterns.forEach((p,i)=>{document.getElementById('pattern'+i).classList.toggle('active',i===data.patternIndex)});
data.guns.forEach((active,i)=>{document.getElementById('gun'+i).classList.toggle('active',active)});

document.getElementById('distance').textContent=data.encoder.distance.toFixed(2);
document.getElementById('speed').textContent=data.encoder.speed.toFixed(1);
document.getElementById('pulses').textContent=data.encoder.pulses;
document.getElementById('distanceMeasure').textContent=data.encoder.distance.toFixed(2);
document.getElementById('speedMeasure').textContent=data.encoder.speed.toFixed(1);
document.getElementById('pulsesMeasure').textContent=data.encoder.pulses;
document.getElementById('pulsesCalib').textContent=data.encoder.pulses;
document.getElementById('calibCurrent').textContent=data.encoder.calibration.toFixed(1);

const cs=document.getElementById('calibStatus');
if(data.mode==='calibrating'){cs.textContent='KALIBRACJA W TOKU - jedz 10m i wcisnij STOP';cs.style.color='#00ff88'}
else{cs.textContent='Oczekiwanie na START';cs.style.color='#999'}

if(data.startFromGap&&data.gapDistance>0){
document.getElementById('gapProgress').style.display='block';
document.getElementById('gapTraveled').textContent=data.gapTraveled.toFixed(2);
document.getElementById('gapTotal').textContent=data.gapDistance.toFixed(1);
document.getElementById('gapBar').style.width=Math.min((data.gapTraveled/data.gapDistance)*100,100)+'%';
}else{document.getElementById('gapProgress').style.display='none'}
}).catch(e=>console.error(e));
}

document.getElementById('chkStartFromGap').addEventListener('change',(e)=>{
fetch('/api/option',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'startFromGap='+(e.target.checked?'1':'0')});
});

setInterval(updateStatus,500);

function loadCurrentReport(){
fetch('/api/report/current').then(r=>r.json()).then(data=>{
const s=document.getElementById('reportActive');
if(data.active){s.textContent='AKTYWNY';s.style.color='#00ff88';document.getElementById('reportStartDate').textContent=data.startDate;document.getElementById('reportStartTime').textContent=data.startTime}
else{s.textContent='Nieaktywny';s.style.color='#999';document.getElementById('reportStartDate').textContent='-';document.getElementById('reportStartTime').textContent='-'}
document.getElementById('reportCurrentTime').textContent=data.currentTime;
}).catch(e=>{document.getElementById('reportActive').textContent='Blad';});
}

function loadReports(){
fetch('/api/reports').then(r=>r.json()).then(data=>{
const list=document.getElementById('reportsList');
if(data.count===0){list.innerHTML='<p style="color:#999;text-align:center;padding:20px;">Brak zapisanych raportow</p>';return}
let html='<div style="display:flex;flex-direction:column;gap:10px;">';
for(let i=data.count-1;i>=0;i--){html+='<button class="btn" onclick="viewReport('+i+')" style="text-align:left;background:#1a1a2e;padding:15px;">Raport #'+(i+1)+'</button>'}
html+='</div>';list.innerHTML=html;
}).catch(e=>{document.getElementById('reportsList').innerHTML='<p style="color:#e94560;text-align:center;">Blad ladowania</p>'});
}

function viewReport(id){
currentViewReportId=id;
fetch('/api/report?id='+id).then(r=>r.text()).then(content=>{
document.getElementById('reportContent').textContent=content;
document.getElementById('reportDetails').style.display='block';
document.getElementById('reportDetails').scrollIntoView({behavior:'smooth'});
}).catch(e=>{alert('Blad ladowania raportu #'+(id+1))});
}

function exportCSV(){
if(currentViewReportId>=0){window.open('/api/report/csv?id='+currentViewReportId,'_blank')}
}

function closeReportDetails(){document.getElementById('reportDetails').style.display='none'}
setInterval(loadCurrentReport,2000);

function showMenuModal(){document.getElementById('menuModal').style.display='block'}
function closeMenuModal(){document.getElementById('menuModal').style.display='none'}
function menuCalibration(){closeMenuModal();showTab(2)}
function menuReports(){closeMenuModal();showTab(3)}
function menuOTA(){closeMenuModal();if(confirm('Uruchomic tryb OTA?\n\nKomputer bedzie czekac na aktualizacje firmware.')){sendCmd('menu');alert('Tryb OTA aktywny!\nHostname: Trassar-Painter\nHaslo: trassar2024')}}
window.onclick=function(event){if(event.target==document.getElementById('menuModal'))closeMenuModal()}

loadCurrentReport();loadReports();updateStatus();
</script>

<div id="menuModal" class="modal">
<div class="modal-content">
<div class="modal-header"><h2>Menu</h2><span class="close-modal" onclick="closeMenuModal()">&times;</span></div>
<div class="menu-item" onclick="menuCalibration()"><div class="menu-icon">&#9881;</div><div class="menu-text"><h3>Kalibracja enkodera</h3><p>Automatyczna kalibracja - jedz 10m i oblicz</p></div></div>
<div class="menu-item" onclick="menuReports()"><div class="menu-icon">&#128202;</div><div class="menu-text"><h3>Raporty pracy</h3><p>Przegladaj zapisane raporty powierzchni</p></div></div>
<div class="menu-item" onclick="menuOTA()"><div class="menu-icon">&#128260;</div><div class="menu-text"><h3>Aktualizacja OTA</h3><p>Aktualizuj firmware przez WiFi</p></div></div>
<div style="margin-top:20px;text-align:center;"><button class="btn" onclick="closeMenuModal()" style="background:#666;width:100%;">Zamknij</button></div>
</div>
</div>
</body></html>
)HTMLCODE";
}

// ============================================================================
// WEB SERVER ENDPOINTS
// ============================================================================

void setupWebServer() {
    server.on("/", HTTP_GET, []() {
        server.send(200, "text/html", getHTMLPage());
    });

    server.on("/api/status", HTTP_GET, []() {
        server.send(200, "application/json", getStatusJSON());
    });

    server.on("/api/cmd", HTTP_POST, []() {
        if (server.hasArg("c")) {
            String cmd = server.arg("c");
            if (cmd == "start") startSystem();
            else if (cmd == "measure") startMeasuring();
            else if (cmd == "service") startService();
            else if (cmd == "menu") enterMenu();
            else if (cmd == "stop") stopSystem();
            else if (cmd == "pause") pauseSystem();
            server.send(200, "text/plain", "OK");
        } else {
            server.send(400, "text/plain", "Missing 'c'");
        }
    });

    // NAPRAWIONE v6.0.0: Walidacja zakresu pattern index
    server.on("/api/pattern", HTTP_POST, []() {
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

    // NAPRAWIONE v6.0.0: Walidacja zakresu service pattern
    server.on("/api/service", HTTP_POST, []() {
        if (server.hasArg("pattern") && server.hasArg("state")) {
            int pattern = server.arg("pattern").toInt();
            bool state = server.arg("state") == "1";

            if (currentMode == MODE_SERVICE && pattern >= 0 && pattern < PATTERN_COUNT) {
                if (state) {
                    serviceTestPattern = pattern;
                    Serial.printf("[SERVICE] Test: %s\n", patterns[pattern].name);
                } else {
                    serviceTestPattern = -1;
                }
                updateGuns();
            }
            server.send(200, "text/plain", "OK");
        } else {
            server.send(400, "text/plain", "Missing params");
        }
    });

    server.on("/api/p3selector", HTTP_POST, []() {
        selectorP3Virtual = !selectorP3Virtual;
        Serial.printf("[P3] Selektor wirtualny: %s\n", selectorP3Virtual ? "ODWROCONE" : "NORMALNE");
        if (currentMode == MODE_WORKING) updateGuns();
        server.send(200, "text/plain", "OK");
    });

    server.on("/api/calibration/start", HTTP_POST, []() {
        startCalibration();
        server.send(200, "text/plain", "OK");
    });

    server.on("/api/option", HTTP_POST, []() {
        if (server.hasArg("startFromGap")) {
            startFromGap = server.arg("startFromGap") == "1";
            Serial.printf("[OPTION] Start od przerwy: %s\n", startFromGap ? "TAK" : "NIE");
        }
        server.send(200, "text/plain", "OK");
    });

    // Raporty
    server.on("/api/reports", HTTP_GET, []() {
        server.send(200, "application/json", getReportsListJSON());
    });

    // NAPRAWIONE v6.0.0: Walidacja report id
    server.on("/api/report", HTTP_GET, []() {
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

    server.on("/api/report/current", HTTP_GET, []() {
        String json;
        json.reserve(256);
        json = "{\"active\":";
        json += (reportActive ? "true" : "false");
        json += ",\"startDate\":\"" + String(currentReport.startDate) + "\"";
        json += ",\"startTime\":\"" + String(currentReport.startTime) + "\"";
        json += ",\"currentTime\":\"" + getCurrentDateTime() + "\"}";
        server.send(200, "application/json", json);
    });

    // Safety API
    server.on("/api/safety/status", HTTP_GET, []() {
        String json;
        json.reserve(512);
        json = "{\"emergencyStop\":";
        json += (emergencyStopActive ? "true" : "false");
        json += ",\"watchdogOk\":";
        json += (checkWatchdog() ? "true" : "false");
        json += ",\"heartbeatOk\":";
        json += ((millis() - lastHeartbeat < HEARTBEAT_INTERVAL_MS * 2) ? "true" : "false");
        json += ",\"selfTestPassed\":";
        json += (selfTestPassed ? "true" : "false");
        json += ",\"errorCount\":";
        json += String(errorCount);
        json += "}";
        server.send(200, "application/json", json);
    });

    server.on("/api/safety/errors", HTTP_GET, []() {
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

    // NAPRAWIONE v6.0.0: Walidacja CSV report id
    server.on("/api/report/csv", HTTP_GET, []() {
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

    server.on("/api/safety/reset-estop", HTTP_POST, []() {
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
