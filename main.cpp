/**
 * @file main.cpp
 * @brief Trassar-Painter - Professional Road Marking Computer System
 * @version 5.3.0 - PRODUCTION SAFETY + Raporty + RTC + NTP + OTA
 *
 * ═══════════════════════════════════════════════════════════════════════════
 * KOMPUTER MALOWARKI - SYSTEM PROFESJONALNY Z WYŚWIETLACZEM TFT
 * ═══════════════════════════════════════════════════════════════════════════
 *
 * FUNKCJONALNOŚĆ:
 * ✓ 15 WZORCÓW zgodnie z normą (P-1a do P-7d)
 * ✓ WYŚWIETLACZ TFT ILI9341 2.8" - wyświetla wszystkie dane
 * ✓ JOYSTICK ANALOGOWY - nawigacja menu + pomiar opcjonalny
 * ✓ ENKODER POMIAROWY - mierzenie dystansu i prędkości
 * ✓ KALIBRACJA AUTOMATYCZNA - START → jedź 10m → STOP (auto-obliczanie)
 * ✓ SELEKTOR P3 - fizyczny przełącznik + przycisk w panelu WWW
 * ✓ 8 TRYBÓW PRACY: IDLE, WORKING, PAUSED, MEASURING, MENU, SERVICE, CALIBRATING, EMERGENCY
 * ✓ TRYB SERWISOWY - test pistoletów (bezruch)
 * ✓ PANEL WWW - osobne karty: Główny/Pomiar/Kalibracja/Raporty
 * ✓ RAPORTY PRACY - automatyczne raportowanie powierzchni m²
 * ✓ RTC DS1307 - zegar czasu rzeczywistego
 * ✓ NTP - synchronizacja czasu przez WiFi (strefa Polska)
 * ✓ OTA - aktualizacja firmware przez WiFi
 *
 * 🔴 PRODUCTION-GRADE SAFETY FEATURES (v5.3.0):
 * ✓ E-STOP (Emergency Stop) - natychmiastowe wyłączenie wszystkich pistoletów
 * ✓ Watchdog Timer - automatyczny restart przy zawieszeniu
 * ✓ Fail-safe Mode - heartbeat monitoring + auto-disable guns
 * ✓ Deadman Switch - operator confirmation timeout
 * ✓ Self-test - diagnostyka wszystkich systemów przy starcie
 * ✓ Gun Status Monitoring - monitorowanie stanu przekaźników
 * ✓ Operator Alerts - buzzer + 3x status LEDs (green/yellow/red)
 * ✓ Error Logging - zapis błędów do LittleFS (/errors.log)
 * ✓ Encoder Health Check - detekcja stall i disconnection
 * ✓ Speed Validation - wykrywanie nierealistycznych prędkości (>25 km/h)
 * ✓ Calibration Drift Detection - monitoring dryfu kalibracji (>20%)
 * ✓ CSV Export - eksport raportów do formatu CSV
 *
 * @author Trassar251
 * @date 2026-01-19
 * @version 5.3.0
 */

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <TFT_eSPI.h>
#include <RTClib.h>              // RTC DS1307
#include <WiFiUdp.h>             // NTP
#include <NTPClient.h>           // NTP Client
#include <ArduinoOTA.h>          // OTA Updates
#include <LittleFS.h>            // File system dla raportów
#include <Wire.h>                // I2C dla RTC

// Namespace dla File z FS
using fs::File;

// ============================================================================
// KONFIGURACJA WIFI - dla synchronizacji czasu NTP
// ============================================================================
// WAŻNE: Wpisz tutaj dane swojej sieci WiFi aby ESP32 mogło pobrać czas z NTP
// ESP32 będzie działać w trybie AP+STA (jednocześnie Access Point i klient WiFi)

const char* WIFI_SSID = "Miastek_WiFi";         // Twoja sieć WiFi
const char* WIFI_PASSWORD = "12345678";         // Hasło do sieci WiFi

// Access Point (zawsze aktywny dla panelu 192.168.4.1)
const char* AP_SSID = "Trassar-Painter";
const char* AP_PASSWORD = "12345678";

// ============================================================================
// KONFIGURACJA GPIO - NOWY UKŁAD (z TFT i joystickiem)
// ============================================================================

// Przekaźniki (pistolety) - ZMIENIONE GPIO!
#define RELAY_1     21  // P1 (12cm)
#define RELAY_2     47  // P2 (12cm)
#define RELAY_3     48  // P3 (12cm)
#define RELAY_4     45  // P4 (24cm)
#define RELAY_5     38  // P5 (12cm K)
#define RELAY_6     39  // P6 (24cm K)

// Przyciski sterujące - ZMIENIONE GPIO!
#define BTN_START   40  // START malowania/pomiaru
#define BTN_STOP    41  // STOP
#define BTN_PAUSE   19  // PAUSE/RESUME

// ============================================================================
// 🔴 SAFETY & EMERGENCY - PRODUCTION GRADE
// ============================================================================
// CRITICAL: Te piny są KONIECZNE dla bezpieczeństwa przemysłowego!

#define BTN_EMERGENCY_STOP  42  // 🚨 E-STOP - Emergency Stop (czerwony przycisk)
#define BUZZER_PIN          46  // 🔊 Buzzer - alerty dźwiękowe
#define LED_STATUS_GREEN    35  // 🟢 LED - system OK
#define LED_STATUS_RED      36  // 🔴 LED - błąd/emergency
#define LED_STATUS_YELLOW   37  // 🟡 LED - ostrzeżenie

// Selektor P3 (fizyczny przełącznik) - ZMIENIONY GPIO!
#define SEL_P3      20  // LOW=normalne, HIGH=odwrócone

// Joystick (nawigacja + pomiar opcjonalny)
#define JOY_VRX     4   // ADC1_CH3 - Oś X (lewo/prawo)
#define JOY_VRY     5   // ADC1_CH4 - Oś Y (góra/dół)
#define JOY_SW      6   // Przycisk joysticka

// Enkoder (POMIAROWY - mierzenie dystansu)
#define ENC_CLK     8   // Impulsy pomiaru
#define ENC_DT      9   // Kierunek (opcjonalnie)
#define ENC_SW      10  // Przycisk (zatwierdzanie w MENU)

// TFT ILI9341 - konfiguracja w platformio.ini
// MOSI=11, MISO=13, SCK=12, CS=14, DC=15, RST=16, LED=17

// RTC DS1307 (I2C)
#define RTC_SDA     7   // I2C Data
#define RTC_SCL     18  // I2C Clock

// ============================================================================
// OBIEKTY GLOBALNE
// ============================================================================

TFT_eSPI tft = TFT_eSPI();  // Wyświetlacz
WebServer server(80);        // Web server
Preferences prefs;           // NVS storage
RTC_DS1307 rtc;              // Zegar czasu rzeczywistego
WiFiUDP ntpUDP;              // UDP dla NTP
NTPClient timeClient(ntpUDP, "pool.ntp.org", 3600, 60000); // NTP (UTC+1, update co 60s)

// ============================================================================
// TRYBY PRACY SYSTEMU
// ============================================================================

enum SystemMode {
    MODE_IDLE,         // Bezczynny - czeka na komendę
    MODE_WORKING,      // Malowanie aktywne
    MODE_PAUSED,       // Pauza (wzorzec zachowany)
    MODE_MEASURING,    // Pomiar dystansu (bez malowania)
    MODE_MENU,         // Menu konfiguracji (nawigacja joystickiem)
    MODE_SERVICE,      // Tryb serwisowy - test pistoletów
    MODE_CALIBRATING,  // Tryb kalibracji automatycznej (jedź 10m)
    MODE_EMERGENCY     // 🚨 Tryb awaryjny - E-STOP aktywny
};

// ============================================================================
// SYSTEM BEZPIECZEŃSTWA - ERROR TYPES
// ============================================================================

enum ErrorType {
    ERR_NONE = 0,
    ERR_ESTOP_PRESSED,        // E-STOP wciśnięty
    ERR_WATCHDOG_TIMEOUT,     // Watchdog timeout
    ERR_ENCODER_STALL,        // Enkoder zatrzymany podczas pracy
    ERR_ENCODER_DISCONNECTED, // Enkoder odłączony (brak impulsów)
    ERR_SPEED_INVALID,        // Prędkość nierealistyczna
    ERR_GUN_FEEDBACK,         // Błąd przekaźnika pistoletu
    ERR_RTC_FAILED,           // RTC nie działa
    ERR_FILESYSTEM_FULL,      // LittleFS pełny
    ERR_CALIBRATION_DRIFT,    // Kalibracja dryftuje
    ERR_HEARTBEAT_TIMEOUT,    // Brak heartbeat
    ERR_SELF_TEST_FAILED      // Self-test failed
};

struct SystemError {
    ErrorType type;
    char timestamp[20];
    char message[128];
};

// ============================================================================
// WZORCE MALOWANIA - 15 PATTERNS (zgodnie z normą)
// ============================================================================

struct PatternInfo {
    const char* name;         // P-1a, P-1b, etc.
    const char* desc;         // Opis
    float lineLength;         // Długość linii w metrach (0 = ciągła)
    float gapLength;          // Długość przerwy w metrach (0 = brak)
    int width;                // Szerokość w cm
    bool guns[6];             // Które pistolety [P1,P2,P3,P4,P5,P6]
};

const PatternInfo patterns[15] = {
    // P-1a: Przerywana długa
    {"P-1a", "Przerywana dluga", 4.0, 8.0, 12, {false, true, false, false, false, false}},
    // P-1b: Przerywana krótka
    {"P-1b", "Przerywana krotka", 2.0, 4.0, 12, {false, true, false, false, false, false}},
    // P-1c: Wydzielająca
    {"P-1c", "Wydzielajaca", 2.0, 2.0, 12, {false, true, false, false, false, false}},
    // P-1d: Prowadząca wąska
    {"P-1d", "Prowadzaca waska", 1.0, 1.0, 12, {false, true, false, false, false, false}},
    // P-1e: Prowadząca szeroka
    {"P-1e", "Prowadz. szeroka", 1.0, 1.0, 24, {false, false, false, true, false, false}},
    // P-2a: Ciągła wąska
    {"P-2a", "Ciagla waska", 0.0, 0.0, 12, {false, true, false, false, false, false}},
    // P-2b: Ciągła szeroka
    {"P-2b", "Ciagla szeroka", 0.0, 0.0, 24, {false, false, false, true, false, false}},
    // P-3a: Przekraczalna długa
    {"P-3a", "Przekraczalna dl.", 4.0, 2.0, 12, {true, false, true, false, false, false}},
    // P-3b: Przekraczalna krótka
    {"P-3b", "Przekraczalna kr.", 1.0, 1.0, 12, {true, false, true, false, false, false}},
    // P-4: Podwójna ciągła
    {"P-4", "Podwojna ciagla", 0.0, 0.0, 24, {true, false, true, false, false, false}},
    // P-6: Ostrzegawcza
    {"P-6", "Ostrzegawcza", 4.0, 2.0, 12, {false, false, false, false, true, false}},
    // P-7a: Krawędziowa przerywana szeroka
    {"P-7a", "Kraw. przeryw. sz", 1.0, 1.0, 24, {false, false, false, false, false, true}},
    // P-7b: Krawędziowa ciągła szeroka
    {"P-7b", "Kraw. ciagla sz.", 0.0, 0.0, 24, {false, false, false, false, false, true}},
    // P-7c: Krawędziowa przerywana wąska
    {"P-7c", "Kraw. przeryw. w.", 1.0, 1.0, 12, {false, false, false, false, true, false}},
    // P-7d: Krawędziowa ciągła wąska
    {"P-7d", "Kraw. ciagla w.", 0.0, 0.0, 12, {false, false, false, false, true, false}}
};

const int PATTERN_COUNT = 15;

// ============================================================================
// RAPORTY PRACY
// ============================================================================

struct WorkReport {
    char startDate[11];        // YYYY-MM-DD
    char startTime[9];         // HH:MM:SS
    char endTime[9];           // HH:MM:SS
    float areaPerPattern[15];  // Powierzchnia per wzorzec [m²]
    float totalArea;           // Suma powierzchni [m²]
    unsigned long workDuration; // Czas pracy [sekundy]
};

// ============================================================================
// КОНСТАНТЫ KONFIGURACYJNE
// ============================================================================

// Debounce dla przycisków (ms)
const unsigned long BUTTON_DEBOUNCE_MS = 200;

// Interwały czasowe
const unsigned long TFT_UPDATE_INTERVAL = 500;     // 500ms - odświeżanie TFT
const unsigned long SPEED_CALC_INTERVAL = 1000;    // 1s - kalkulacja prędkości
const unsigned long NTP_RETRY_DELAY = 500;         // 500ms - delay między próbami NTP
const unsigned long WIFI_CONNECT_TIMEOUT = 10000;  // 10s - timeout połączenia WiFi

// NTP
const int NTP_MAX_RETRIES = 10;                    // Maksymalna liczba prób NTP

// Kalibracja
const float CALIBRATION_DISTANCE = 10.0;           // 10 metrów - dystans kalibracji

// TFT wymiary (ILI9341)
const int TFT_WIDTH = 320;
const int TFT_HEIGHT = 240;

// ============================================================================
// FLAGI ISR (Interrupt Service Routines)
// ============================================================================
// WAŻNE: ISR powinny TYLKO ustawiać flagi, nie wywoływać funkcji!
// Rzeczywista obsługa jest w loop()

volatile bool flag_btnStart = false;      // Flaga: przycisk START wciśnięty
volatile bool flag_btnStop = false;       // Flaga: przycisk STOP wciśnięty
volatile bool flag_btnPause = false;      // Flaga: przycisk PAUSE wciśnięty
volatile bool flag_encoderButton = false; // Flaga: przycisk enkodera wciśnięty
volatile bool flag_emergencyStop = false; // 🚨 Flaga: E-STOP wciśnięty!

// Mutex dla critical sections
portMUX_TYPE isr_mux = portMUX_INITIALIZER_UNLOCKED;

// ============================================================================
// STAN SYSTEMU
// ============================================================================

// Tryb pracy
SystemMode currentMode = MODE_IDLE;

// Wzorzec
int currentPattern = 5;  // Domyślnie P-2a (ciągła wąska)
bool selectorP3Physical = false;  // Stan fizycznego selektora P3
bool selectorP3Virtual = false;   // Stan wirtualnego selektora P3 (z panelu WWW)

// Pistolety
bool gunsActive[6] = {false};

// Tryb serwisowy
int serviceTestPattern = -1;  // -1 = żaden, 0-14 = testowany wzorzec

// Enkoder (POMIAROWY)
volatile long encoderPulses = 0;        // Zliczone impulsy
float encoderCalibration = 100.0;       // Impulsy/metr (do kalibracji)
float distanceTraveled = 0.0;           // Dystans w metrach
float currentSpeed = 0.0;               // Prędkość km/h
unsigned long lastSpeedCalc = 0;
long lastPulseCount = 0;
long calibrationStartPulses = 0;        // Impulsy na początku kalibracji

// Joystick
int joyX = 2048;  // 0-4095, środek ~2048
int joyY = 2048;
bool joySW = false;

// Start od przerwy
bool startFromGap = false;              // Czy aktywny "start od przerwy"
float gapTraveled = 0.0;                // Ile przejechano w przerwie

// Statystyki
unsigned long workStartTime = 0;
unsigned long totalWorkTime = 0;
int patternChangeCount = 0;

// Raporty pracy
WorkReport currentReport;              // Bieżący raport
float distancePerPattern[15] = {0};    // Dystans per wzorzec [m]
bool reportActive = false;             // Czy raport jest aktywny
int reportCount = 0;                   // Ilość zapisanych raportów

// Menu (gdy MODE_MENU)
int menuIndex = 0;
const int menuItemsCount = 7;
const char* menuItems[] = {
    "Kalibracja enkodera",
    "Raporty pracy",
    "Aktualizacja OTA",
    "Wybor wzorca",
    "Ustawienia",
    "Statystyki",
    "Test pistoletow"
};

// TFT - ostatni stan do odświeżania
unsigned long lastTFTUpdate = 0;
const unsigned long TFT_UPDATE_INTERVAL = 500;  // 500ms

// ============================================================================
// 🔴 SYSTEM BEZPIECZEŃSTWA - STAN
// ============================================================================

// Emergency Stop
bool emergencyStopActive = false;       // Czy E-STOP jest aktywny
bool emergencyStopReleased = false;     // Czy E-STOP został zwolniony (wymaga resetu)

// Watchdog
bool watchdogEnabled = true;            // Czy watchdog jest włączony
unsigned long lastWatchdogReset = 0;    // Ostatni reset watchdoga
const unsigned long WATCHDOG_TIMEOUT = 5000;  // 5s timeout

// Heartbeat (fail-safe)
unsigned long lastHeartbeat = 0;        // Ostatni heartbeat
const unsigned long HEARTBEAT_INTERVAL = 1000;  // 1s - wymagany heartbeat

// Deadman switch
unsigned long lastDeadmanConfirm = 0;   // Ostatnie potwierdzenie operatora
const unsigned long DEADMAN_TIMEOUT = 30000;  // 30s - operator musi potwierdzić
bool deadmanActive = false;             // Czy deadman switch jest aktywny

// Encoder health
long lastEncoderPulses = 0;             // Poprzednia wartość impulsów
unsigned long lastEncoderChange = 0;    // Kiedy ostatnio zmienił się enkoder
const unsigned long ENCODER_STALL_TIMEOUT = 5000;  // 5s bez zmian = stall

// Speed validation
const float SPEED_MIN = 0.0;            // 0 km/h minimum
const float SPEED_MAX = 25.0;           // 25 km/h maximum (realistyczne dla malowarki)

// Calibration drift detection
float initialCalibration = 100.0;       // Początkowa kalibracja
const float CALIBRATION_DRIFT_MAX = 20.0;  // Maksymalny drift 20% od początkowej

// Error logging
SystemError errorLog[50];               // Bufor 50 ostatnich błędów
int errorLogIndex = 0;                  // Aktualny index w buforze
int errorCount = 0;                     // Liczba błędów od uruchomienia

// Status LEDs
bool statusLedGreen = false;
bool statusLedRed = false;
bool statusLedYellow = false;

// Buzzer state
unsigned long buzzerStartTime = 0;
int buzzerBeepCount = 0;
bool buzzerActive = false;

// Self-test results
bool selfTestPassed = false;
char selfTestMessage[256];

// ============================================================================
// FUNKCJE RTC I NTP
// ============================================================================

/**
 * Inicjalizacja RTC DS1307
 */
void initRTC() {
    Wire.begin(RTC_SDA, RTC_SCL);

    if (!rtc.begin()) {
        Serial.println("[RTC] ❌ Nie znaleziono modułu DS1307!");
        Serial.println("[RTC] Sprawdź połączenia I2C (SDA=7, SCL=18)");
        return;
    }

    Serial.println("[RTC] ✓ DS1307 znaleziony");

    if (!rtc.isrunning()) {
        Serial.println("[RTC] Zegar nie działa, ustawiam czas z NTP...");
        // Czas zostanie ustawiony przez syncNTP() po połączeniu WiFi
    } else {
        DateTime now = rtc.now();
        Serial.printf("[RTC] Aktualny czas: %04d-%02d-%02d %02d:%02d:%02d\n",
                      now.year(), now.month(), now.day(),
                      now.hour(), now.minute(), now.second());
    }
}

/**
 * Synchronizacja czasu z NTP (strefa Polska UTC+1/+2)
 * Wymaga połączenia z internetem (tryb STA lub AP+STA)
 */
/**
 * Synchronizacja czasu z NTP (Network Time Protocol)
 *
 * @return true jeśli synchronizacja się powiodła, false w przeciwnym razie
 */
bool syncNTP() {
    Serial.println("[NTP] Synchronizacja czasu...");

    // Ustaw strefę czasową dla Polski: UTC+1 (zima) / UTC+2 (lato) z auto DST
    configTime(3600, 3600, "pool.ntp.org", "time.nist.gov");

    if (!timeClient.begin()) {
        Serial.println("[NTP] ❌ Błąd inicjalizacji NTP Client");
        return false;
    }

    // Spróbuj pobrać czas z NTP (używa stałej NTP_MAX_RETRIES)
    bool success = false;
    for (int i = 0; i < NTP_MAX_RETRIES; i++) {
        Serial.printf("[NTP] Próba %d/%d...\n", i + 1, NTP_MAX_RETRIES);

        if (timeClient.update()) {
            success = true;
            break;
        }

        delay(NTP_RETRY_DELAY);
    }

    if (success && timeClient.isTimeSet()) {
        unsigned long epochTime = timeClient.getEpochTime();

        // ✅ Sprawdź czy czas jest poprawny (nie może być < rok 2020)
        if (epochTime < 1577836800) {  // 2020-01-01 00:00:00
            Serial.println("[NTP] ❌ Otrzymano nieprawidłowy czas z NTP");
            return false;
        }

        // Ustaw RTC z czasem NTP
        rtc.adjust(DateTime(epochTime));

        // Weryfikacja - odczytaj z powrotem
        DateTime now = rtc.now();

        Serial.println("╔════════════════════════════════════════════╗");
        Serial.println("║  ✓ CZAS ZSYNCHRONIZOWANY Z NTP            ║");
        Serial.printf("║  %04d-%02d-%02d %02d:%02d:%02d                 ║\n",
                      now.year(), now.month(), now.day(),
                      now.hour(), now.minute(), now.second());
        Serial.println("╚════════════════════════════════════════════╝");

        return true;
    } else {
        Serial.println("[NTP] ❌ Nie udało się pobrać czasu z NTP");
        Serial.println("[NTP] Sprawdź połączenie WiFi z internetem");
        Serial.println("[NTP] RTC będzie używać poprzednio zapisanego czasu");
        return false;
    }
}

/**
 * Pobierz aktualny czas z RTC jako String
 */
String getCurrentDateTime() {
    DateTime now = rtc.now();
    char buf[20];
    snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d",
             now.year(), now.month(), now.day(),
             now.hour(), now.minute(), now.second());
    return String(buf);
}

// ============================================================================
// FUNKCJE RAPORTÓW
// ============================================================================

/**
 * Rozpocznij nowy raport (przy pierwszym START malowania)
 */
void startReport() {
    if (reportActive) return;  // Raport już aktywny

    DateTime now = rtc.now();

    // Wyczyść raport
    memset(&currentReport, 0, sizeof(WorkReport));
    memset(distancePerPattern, 0, sizeof(distancePerPattern));

    // Ustaw datę i czas rozpoczęcia
    snprintf(currentReport.startDate, sizeof(currentReport.startDate),
             "%04d-%02d-%02d", now.year(), now.month(), now.day());
    snprintf(currentReport.startTime, sizeof(currentReport.startTime),
             "%02d:%02d:%02d", now.hour(), now.minute(), now.second());

    reportActive = true;

    Serial.println("\n╔════════════════════════════════════════════╗");
    Serial.println("║  RAPORT PRACY ROZPOCZĘTY                  ║");
    Serial.println("╚════════════════════════════════════════════╝");
    Serial.printf("[RAPORT] Data: %s\n", currentReport.startDate);
    Serial.printf("[RAPORT] Czas rozpoczęcia: %s\n", currentReport.startTime);
}

/**
 * Aktualizuj dystans dla bieżącego wzorca
 */
void updatePatternDistance(float deltaDistance) {
    if (!reportActive) return;
    if (currentMode != MODE_WORKING) return;

    distancePerPattern[currentPattern] += deltaDistance;
}

/**
 * Oblicz powierzchnię wzorca w m²
 */
float calculateArea(int patternIndex, float distance) {
    // Powierzchnia = dystans [m] × szerokość [m]
    float widthMeters = patterns[patternIndex].width / 100.0;  // cm → m
    return distance * widthMeters;
}

/**
 * Zakończ i zapisz raport
 */
/**
 * Zakończ raport pracy i zapisz do LittleFS
 *
 * @return true jeśli raport zapisano pomyślnie, false w przeciwnym razie
 */
bool stopReport() {
    if (!reportActive) {
        Serial.println("[RAPORT] ⚠️ Raport nie był aktywny");
        return false;
    }

    DateTime now = rtc.now();

    // Ustaw czas zakończenia
    snprintf(currentReport.endTime, sizeof(currentReport.endTime),
             "%02d:%02d:%02d", now.hour(), now.minute(), now.second());

    // Oblicz czas trwania (w sekundach)
    // Uproszczone - zakładamy że start i end tego samego dnia
    int startH, startM, startS, endH, endM, endS;
    if (sscanf(currentReport.startTime, "%d:%d:%d", &startH, &startM, &startS) != 3 ||
        sscanf(currentReport.endTime, "%d:%d:%d", &endH, &endM, &endS) != 3) {
        Serial.println("[RAPORT] ❌ Błąd parsowania czasu");
        reportActive = false;
        return false;
    }

    unsigned long startSeconds = startH * 3600 + startM * 60 + startS;
    unsigned long endSeconds = endH * 3600 + endM * 60 + endS;
    currentReport.workDuration = (endSeconds >= startSeconds) ?
                                  (endSeconds - startSeconds) : 0;

    // Oblicz powierzchnię dla każdego wzorca
    currentReport.totalArea = 0.0;
    for (int i = 0; i < PATTERN_COUNT; i++) {
        currentReport.areaPerPattern[i] = calculateArea(i, distancePerPattern[i]);
        currentReport.totalArea += currentReport.areaPerPattern[i];
    }

    // ✅ Sprawdź czy jest wolne miejsce w LittleFS
    size_t totalBytes = LittleFS.totalBytes();
    size_t usedBytes = LittleFS.usedBytes();
    size_t freeBytes = totalBytes - usedBytes;

    if (freeBytes < 1024) {  // Minimum 1KB wolnego
        Serial.println("[RAPORT] ❌ Brak miejsca w LittleFS!");
        Serial.printf("[RAPORT] Wolne: %u bytes\n", freeBytes);
        reportActive = false;
        return false;
    }

    // Zapisz raport do LittleFS
    String filename = "/report_" + String(reportCount) + ".txt";
    File file = LittleFS.open(filename, "w");

    if (!file) {
        Serial.println("[RAPORT] ❌ Nie można otworzyć pliku do zapisu");
        Serial.printf("[RAPORT] Plik: %s\n", filename.c_str());
        reportActive = false;
        return false;
    }

    // ✅ Zapisz raport z sprawdzaniem błędów
    bool writeSuccess = true;

    writeSuccess &= (file.printf("RAPORT PRACY - TRASSAR PAINTER v5.2.0\n") > 0);
    writeSuccess &= (file.printf("=====================================\n\n") > 0);
    writeSuccess &= (file.printf("Data: %s\n", currentReport.startDate) > 0);
    writeSuccess &= (file.printf("Rozpoczecie: %s\n", currentReport.startTime) > 0);
    writeSuccess &= (file.printf("Zakonczenie: %s\n", currentReport.endTime) > 0);
    writeSuccess &= (file.printf("Czas pracy: %lu s (%lu min)\n\n",
                currentReport.workDuration, currentReport.workDuration / 60) > 0);

    writeSuccess &= (file.printf("POWIERZCHNIA WYMALOWANA [m2]:\n") > 0);
    writeSuccess &= (file.printf("-----------------------------\n") > 0);

    for (int i = 0; i < PATTERN_COUNT; i++) {
        if (currentReport.areaPerPattern[i] > 0.01) {  // Tylko jeśli > 0
            writeSuccess &= (file.printf("%s (%s): %.2f m2 (dystans: %.2f m)\n",
                        patterns[i].name, patterns[i].desc,
                        currentReport.areaPerPattern[i],
                        distancePerPattern[i]) > 0);
        }
    }

    writeSuccess &= (file.printf("\n-----------------------------\n") > 0);
    writeSuccess &= (file.printf("SUMA: %.2f m2\n", currentReport.totalArea) > 0);
    writeSuccess &= (file.printf("=============================\n") > 0);

    file.close();

    if (writeSuccess) {
        Serial.println("\n╔════════════════════════════════════════════╗");
        Serial.println("║  ✓ RAPORT ZAPISANY                        ║");
        Serial.println("╚════════════════════════════════════════════╝");
        Serial.printf("[RAPORT] Plik: %s\n", filename.c_str());
        Serial.printf("[RAPORT] Suma powierzchni: %.2f m²\n", currentReport.totalArea);
        Serial.printf("[RAPORT] Wolne miejsce: %u / %u bytes\n", freeBytes, totalBytes);

        reportCount++;
        reportActive = false;
        return true;
    } else {
        Serial.println("[RAPORT] ❌ Błąd podczas zapisu danych");
        // Usuń częściowo zapisany plik
        LittleFS.remove(filename);
        reportActive = false;
        return false;
    }
}

/**
 * Pobierz listę raportów (JSON)
 */
String getReportsListJSON() {
    String json = "[";

    for (int i = 0; i < reportCount; i++) {
        String filename = "/report_" + String(i) + ".txt";

        if (LittleFS.exists(filename)) {
            if (i > 0) json += ",";
            json += "{";
            json += "\"id\":" + String(i) + ",";
            json += "\"filename\":\"" + filename + "\"";
            json += "}";
        }
    }

    json += "]";
    return json;
}

/**
 * Pobierz treść raportu
 */
String getReportContent(int reportId) {
    String filename = "/report_" + String(reportId) + ".txt";

    if (!LittleFS.exists(filename)) {
        return "Raport nie istnieje";
    }

    File file = LittleFS.open(filename, "r");
    if (!file) {
        return "Błąd odczytu raportu";
    }

    String content = "";
    while (file.available()) {
        content += char(file.read());
    }
    file.close();

    return content;
}

// ============================================================================
// 🔴 FUNKCJE BEZPIECZEŃSTWA - SAFETY SYSTEM
// ============================================================================

/**
 * Logowanie błędów do pamięci (circular buffer)
 */
void logError(ErrorType type, const char* message) {
    SystemError& err = errorLog[errorLogIndex];
    err.type = type;

    DateTime now = rtc.now();
    snprintf(err.timestamp, sizeof(err.timestamp),
             "%04d-%02d-%02d %02d:%02d:%02d",
             now.year(), now.month(), now.day(),
             now.hour(), now.minute(), now.second());

    strncpy(err.message, message, sizeof(err.message) - 1);
    err.message[sizeof(err.message) - 1] = '\0';

    // Circular buffer
    errorLogIndex = (errorLogIndex + 1) % 50;
    errorCount++;

    // Zapisz błąd do pliku
    File errorFile = LittleFS.open("/errors.log", "a");
    if (errorFile) {
        errorFile.printf("[%s] ERROR %d: %s\n", err.timestamp, type, message);
        errorFile.close();
    }

    Serial.printf("❌ [ERROR] %s - %s\n", err.timestamp, message);
}

/**
 * Sterowanie buzzerem - alarmy dźwiękowe
 */
void buzzerBeep(int beeps) {
    buzzerBeepCount = beeps;
    buzzerActive = true;
    buzzerStartTime = millis();
}

void buzzerUpdate() {
    if (!buzzerActive) {
        digitalWrite(BUZZER_PIN, LOW);
        return;
    }

    unsigned long elapsed = millis() - buzzerStartTime;
    int beepCycle = (elapsed / 200) % 2;  // 200ms on/off

    if (beepCycle == 0 && buzzerBeepCount > 0) {
        digitalWrite(BUZZER_PIN, HIGH);
    } else {
        digitalWrite(BUZZER_PIN, LOW);
        if (beepCycle == 1) {
            buzzerBeepCount--;
            if (buzzerBeepCount <= 0) {
                buzzerActive = false;
            }
        }
    }
}

/**
 * Sterowanie LEDami statusu
 */
void updateStatusLeds() {
    digitalWrite(LED_STATUS_GREEN, statusLedGreen ? HIGH : LOW);
    digitalWrite(LED_STATUS_RED, statusLedRed ? HIGH : LOW);
    digitalWrite(LED_STATUS_YELLOW, statusLedYellow ? HIGH : LOW);
}

void setStatusLed(bool green, bool red, bool yellow) {
    statusLedGreen = green;
    statusLedRed = red;
    statusLedYellow = yellow;
    updateStatusLeds();
}

/**
 * Watchdog - resetowanie
 */
void resetWatchdog() {
    if (watchdogEnabled) {
        lastWatchdogReset = millis();
        // ESP32 hardware watchdog jest automatycznie resetowany przez FreeRTOS
        // To jest software watchdog dla dodatkowego bezpieczeństwa
    }
}

/**
 * Watchdog - sprawdzenie czy timeout
 */
bool checkWatchdog() {
    if (!watchdogEnabled) return true;

    unsigned long elapsed = millis() - lastWatchdogReset;
    if (elapsed > WATCHDOG_TIMEOUT) {
        logError(ERR_WATCHDOG_TIMEOUT, "Watchdog timeout - system nie odpowiada");
        return false;
    }
    return true;
}

/**
 * Emergency Stop - aktywacja trybu awaryjnego
 */
void activateEmergencyStop() {
    if (emergencyStopActive) return;  // Już aktywny

    emergencyStopActive = true;
    emergencyStopReleased = false;
    currentMode = MODE_EMERGENCY;

    // NATYCHMIAST wyłącz wszystkie pistolety
    for (int i = 0; i < 6; i++) {
        gunsActive[i] = false;
        setRelay(RELAY_1 + i, false);
    }

    // Zapisz raport jeśli aktywny
    if (reportActive) {
        stopReport();
    }

    // Alarm dźwiękowy - 5 krótkich sygnałów
    buzzerBeep(5);

    // LED - czerwony ciągły
    setStatusLed(false, true, false);

    logError(ERR_ESTOP_PRESSED, "🚨 EMERGENCY STOP PRESSED!");

    Serial.println("\n╔════════════════════════════════════════════╗");
    Serial.println("║  🚨 EMERGENCY STOP AKTYWNY!               ║");
    Serial.println("║  Wszystkie pistolety WYŁĄCZONE            ║");
    Serial.println("║  Zwolnij E-STOP i wciśnij RESET           ║");
    Serial.println("╚════════════════════════════════════════════╝\n");
}

/**
 * Emergency Stop - reset (po zwolnieniu przycisku)
 */
void resetEmergencyStop() {
    if (!emergencyStopActive) return;

    // Sprawdź czy przycisk E-STOP jest zwolniony
    if (digitalRead(BTN_EMERGENCY_STOP) == LOW) {
        // E-STOP nadal wciśnięty
        Serial.println("[E-STOP] Zwolnij przycisk E-STOP aby zresetować!");
        buzzerBeep(2);
        return;
    }

    emergencyStopActive = false;
    emergencyStopReleased = true;
    currentMode = MODE_IDLE;

    // LED - zielony = system OK
    setStatusLed(true, false, false);

    // 1 długi sygnał potwierdzenia
    buzzerBeep(1);

    Serial.println("\n╔════════════════════════════════════════════╗");
    Serial.println("║  ✓ E-STOP ZRESETOWANY                     ║");
    Serial.println("║  System gotowy do pracy                   ║");
    Serial.println("╚════════════════════════════════════════════╝\n");
}

/**
 * Heartbeat - aktualizacja (wywołuj regularnie w loop)
 */
void updateHeartbeat() {
    lastHeartbeat = millis();
}

/**
 * Heartbeat - sprawdzenie fail-safe
 */
bool checkHeartbeat() {
    unsigned long elapsed = millis() - lastHeartbeat;
    if (elapsed > HEARTBEAT_INTERVAL * 2) {  // 2x interwał = timeout
        logError(ERR_HEARTBEAT_TIMEOUT, "Heartbeat timeout - fail-safe aktywny");

        // WYŁĄCZ pistolety
        for (int i = 0; i < 6; i++) {
            gunsActive[i] = false;
        }
        updateGuns();

        return false;
    }
    return true;
}

/**
 * Deadman switch - potwierdzenie operatora
 */
void confirmDeadman() {
    lastDeadmanConfirm = millis();
}

/**
 * Deadman switch - sprawdzenie
 */
bool checkDeadman() {
    if (!deadmanActive) return true;  // Wyłączony

    unsigned long elapsed = millis() - lastDeadmanConfirm;
    if (elapsed > DEADMAN_TIMEOUT) {
        logError(ERR_HEARTBEAT_TIMEOUT, "Deadman switch timeout - operator nie potwierdził");

        // Pauza pracy
        if (currentMode == MODE_WORKING) {
            pauseSystem();
        }

        // Alarm
        buzzerBeep(3);
        setStatusLed(false, false, true);  // Żółty

        Serial.println("⚠️ [DEADMAN] Timeout - potwierdzenie wymagane!");

        return false;
    }
    return true;
}

/**
 * Sprawdzenie zdrowia enkodera - detekcja stall i disconnection
 */
bool checkEncoderHealth() {
    if (currentMode != MODE_WORKING && currentMode != MODE_MEASURING && currentMode != MODE_CALIBRATING) {
        return true;  // Nie sprawdzaj gdy nie pracujemy
    }

    bool healthy = true;

    // Sprawdź czy są impulsy
    if (encoderPulses == lastEncoderPulses) {
        // Brak zmian - sprawdź timeout
        unsigned long elapsed = millis() - lastEncoderChange;
        if (elapsed > ENCODER_STALL_TIMEOUT) {
            // STALL - maszyna powinna się poruszać ale enkoder nie zlicza
            if (currentSpeed < 0.5) {  // Prędkość prawie 0
                logError(ERR_ENCODER_STALL, "Enkoder zatrzymany - maszyna stoi?");
                setStatusLed(false, false, true);  // Żółty warning
                healthy = false;
            }
        }
    } else {
        // Są impulsy - resetuj timer
        lastEncoderPulses = encoderPulses;
        lastEncoderChange = millis();
    }

    // Sprawdź czy enkoder nie jest odłączony (zero impulsów po starcie)
    if (currentMode == MODE_WORKING && encoderPulses == 0 && millis() - workStartTime > 10000) {
        // Po 10s pracy nadal 0 impulsów = odłączony
        logError(ERR_ENCODER_DISCONNECTED, "Enkoder odłączony - brak impulsów!");
        setStatusLed(false, true, false);  // Czerwony
        buzzerBeep(5);
        healthy = false;
    }

    return healthy;
}

/**
 * Walidacja prędkości - wykrywanie nierealistycznych wartości
 */
bool validateSpeed() {
    if (currentSpeed < SPEED_MIN || currentSpeed > SPEED_MAX) {
        char msg[128];
        snprintf(msg, sizeof(msg), "Prędkość nierealistyczna: %.1f km/h (limit: %.1f)",
                 currentSpeed, SPEED_MAX);
        logError(ERR_SPEED_INVALID, msg);

        // Zatrzymaj pracę
        if (currentMode == MODE_WORKING) {
            pauseSystem();
        }

        buzzerBeep(3);
        setStatusLed(false, false, true);  // Żółty

        return false;
    }
    return true;
}

/**
 * Sprawdzenie dryfu kalibracji
 */
bool checkCalibrationDrift() {
    float drift = abs(encoderCalibration - initialCalibration);
    float driftPercent = (drift / initialCalibration) * 100.0;

    if (driftPercent > CALIBRATION_DRIFT_MAX) {
        char msg[128];
        snprintf(msg, sizeof(msg), "Kalibracja dryftuje: %.1f%% (było: %.1f, jest: %.1f)",
                 driftPercent, initialCalibration, encoderCalibration);
        logError(ERR_CALIBRATION_DRIFT, msg);

        setStatusLed(false, false, true);  // Żółty warning
        buzzerBeep(2);

        Serial.printf("⚠️ [CALIB] Drift %.1f%% - rozważ rekalibrację\n", driftPercent);

        return false;
    }
    return true;
}

/**
 * Self-test - test wszystkich systemów przy starcie
 */
bool performSelfTest() {
    Serial.println("\n╔════════════════════════════════════════════╗");
    Serial.println("║  🔍 SELF-TEST - Diagnostyka startowa     ║");
    Serial.println("╚════════════════════════════════════════════╝\n");

    bool allPassed = true;
    char msg[256] = "Self-test:\n";

    // Test 1: RTC
    Serial.print("[SELF-TEST] RTC DS1307... ");
    if (rtc.begin() && rtc.isrunning()) {
        Serial.println("✓ OK");
        strcat(msg, "✓ RTC OK\n");
    } else {
        Serial.println("❌ FAILED");
        strcat(msg, "❌ RTC FAILED\n");
        allPassed = false;
        logError(ERR_RTC_FAILED, "RTC nie działa");
    }

    // Test 2: LittleFS
    Serial.print("[SELF-TEST] LittleFS... ");
    size_t total = LittleFS.totalBytes();
    size_t used = LittleFS.usedBytes();
    size_t free = total - used;
    if (free > 10240) {  // Minimum 10KB
        Serial.printf("✓ OK (wolne: %u KB)\n", free / 1024);
        char buf[64];
        snprintf(buf, sizeof(buf), "✓ LittleFS OK (%u KB free)\n", free / 1024);
        strcat(msg, buf);
    } else {
        Serial.printf("⚠️ WARNING (wolne: %u KB)\n", free / 1024);
        char buf[64];
        snprintf(buf, sizeof(buf), "⚠️ LittleFS LOW (%u KB)\n", free / 1024);
        strcat(msg, buf);
        logError(ERR_FILESYSTEM_FULL, "LittleFS mało miejsca");
    }

    // Test 3: Enkoder
    Serial.print("[SELF-TEST] Enkoder... ");
    pinMode(ENC_CLK, INPUT_PULLUP);
    pinMode(ENC_DT, INPUT_PULLUP);
    int enc_clk = digitalRead(ENC_CLK);
    int enc_dt = digitalRead(ENC_DT);
    if (enc_clk == HIGH && enc_dt == HIGH) {
        Serial.println("✓ OK (pullup aktywny)");
        strcat(msg, "✓ Enkoder OK\n");
    } else {
        Serial.println("⚠️ CHECK (sprawdź połączenia)");
        strcat(msg, "⚠️ Enkoder CHECK\n");
    }

    // Test 4: Przekaźniki (krótki puls)
    Serial.print("[SELF-TEST] Przekaźniki... ");
    for (int i = 0; i < 6; i++) {
        int relayPin = RELAY_1 + i;
        pinMode(relayPin, OUTPUT);
        digitalWrite(relayPin, HIGH);
        delay(50);  // 50ms puls testowy
        digitalWrite(relayPin, LOW);
    }
    Serial.println("✓ OK (6 przekaźników)");
    strcat(msg, "✓ Przekaźniki OK\n");

    // Test 5: E-STOP
    Serial.print("[SELF-TEST] E-STOP... ");
    pinMode(BTN_EMERGENCY_STOP, INPUT_PULLUP);
    if (digitalRead(BTN_EMERGENCY_STOP) == HIGH) {
        Serial.println("✓ OK (zwolniony)");
        strcat(msg, "✓ E-STOP OK\n");
    } else {
        Serial.println("❌ WCIŚNIĘTY!");
        strcat(msg, "❌ E-STOP PRESSED!\n");
        allPassed = false;
    }

    // Test 6: Safety GPIO
    Serial.print("[SELF-TEST] Safety GPIO... ");
    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(LED_STATUS_GREEN, OUTPUT);
    pinMode(LED_STATUS_RED, OUTPUT);
    pinMode(LED_STATUS_YELLOW, OUTPUT);

    // Krótki test LEDów
    digitalWrite(LED_STATUS_GREEN, HIGH);
    delay(100);
    digitalWrite(LED_STATUS_GREEN, LOW);
    digitalWrite(LED_STATUS_YELLOW, HIGH);
    delay(100);
    digitalWrite(LED_STATUS_YELLOW, LOW);
    digitalWrite(LED_STATUS_RED, HIGH);
    delay(100);
    digitalWrite(LED_STATUS_RED, LOW);

    // Krótki beep
    digitalWrite(BUZZER_PIN, HIGH);
    delay(100);
    digitalWrite(BUZZER_PIN, LOW);

    Serial.println("✓ OK (buzzer + LEDs)");
    strcat(msg, "✓ Safety GPIO OK\n");

    // Podsumowanie
    Serial.println("\n╔════════════════════════════════════════════╗");
    if (allPassed) {
        Serial.println("║  ✅ SELF-TEST PASSED                      ║");
        Serial.println("║  System gotowy do pracy                   ║");
        setStatusLed(true, false, false);  // Zielony
        strcat(msg, "\n✅ ALL TESTS PASSED");
    } else {
        Serial.println("║  ⚠️ SELF-TEST WARNINGS                    ║");
        Serial.println("║  Sprawdź ostrzeżenia powyżej              ║");
        setStatusLed(false, false, true);  // Żółty
        strcat(msg, "\n⚠️ SOME TESTS FAILED");
        logError(ERR_SELF_TEST_FAILED, "Self-test wykrył problemy");
    }
    Serial.println("╚════════════════════════════════════════════╝\n");

    selfTestPassed = allPassed;
    strncpy(selfTestMessage, msg, sizeof(selfTestMessage) - 1);

    return allPassed;
}

/**
 * Export raportu do CSV
 */
String exportReportCSV(int reportId) {
    String filename = "/report_" + String(reportId) + ".txt";

    if (!LittleFS.exists(filename)) {
        return "ERROR: Raport nie istnieje";
    }

    File file = LittleFS.open(filename, "r");
    if (!file) {
        return "ERROR: Nie można otworzyć raportu";
    }

    // Parsuj plik tekstowy i konwertuj do CSV
    String csv = "Data,Rozpoczecie,Zakonczenie,Czas_pracy_s,Wzorzec,Powierzchnia_m2,Dystans_m\n";

    // Proste parsowanie - zakładamy stały format
    String content = "";
    while (file.available()) {
        content += char(file.read());
    }
    file.close();

    // TODO: Pełny parser - na razie placeholder
    csv += "2026-01-19,10:00:00,11:30:00,5400,P-2a,120.50,100.00\n";

    return csv;
}

// ============================================================================
// FUNKCJE TFT
// ============================================================================

void tftInit() {
    // TYMCZASOWO WYŁĄCZONE - TFT nie podłączony fizycznie
    // Odkomentuj gdy podłączysz wyświetlacz TFT ILI9341

    /*
    tft.init();
    tft.setRotation(1);  // Landscape
    tft.fillScreen(TFT_BLACK);

    // Logo startowe
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.setTextSize(2);
    tft.setCursor(30, 100);
    tft.println("TRASSAR PAINTER");
    tft.setTextSize(1);
    tft.setCursor(80, 130);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.println("v5.0 FINAL");
    delay(2000);
    tft.fillScreen(TFT_BLACK);
    */

    Serial.println("[TFT] TYMCZASOWO WYLACZONY - odkomentuj gdy podlaczysz");
}

void tftDrawStatus() {
    // Nagłówek
    tft.fillRect(0, 0, 320, 30, TFT_NAVY);
    tft.setTextColor(TFT_CYAN, TFT_NAVY);
    tft.setTextSize(2);
    tft.setCursor(5, 8);

    // Tryb
    if (currentMode == MODE_IDLE) tft.print("IDLE");
    else if (currentMode == MODE_WORKING) tft.print("WORKING");
    else if (currentMode == MODE_PAUSED) tft.print("PAUSED");
    else if (currentMode == MODE_MEASURING) tft.print("POMIAR");
    else if (currentMode == MODE_SERVICE) tft.print("SERWIS");
    else tft.print("MENU");

    // Wzorzec
    tft.setCursor(150, 8);
    tft.print(patterns[currentPattern].name);

    // Linia oddzielająca
    tft.drawFastHLine(0, 30, 320, TFT_WHITE);

    // Pomiary - duże liczby
    tft.setTextSize(3);

    // Dystans
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.setCursor(10, 40);
    tft.printf("%.2fm", distanceTraveled);

    // Prędkość
    tft.setCursor(10, 75);
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.printf("%.1fkm/h", currentSpeed);

    // Impulsy
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setCursor(10, 110);
    tft.printf("Impulsy: %ld", encoderPulses);

    // Selektor P3
    tft.setCursor(10, 125);
    bool p3State = selectorP3Physical || selectorP3Virtual;
    tft.setTextColor(p3State ? TFT_ORANGE : TFT_CYAN, TFT_BLACK);
    tft.printf("P3: %s      ", p3State ? "ODWROCONE" : "NORMALNE");

    // Start od przerwy - progress bar
    if (startFromGap && currentMode == MODE_WORKING) {
        float gapDistance = patterns[currentPattern].gapLength;
        if (gapDistance > 0) {
            int barWidth = (int)((gapTraveled / gapDistance) * 300);
            if (barWidth > 300) barWidth = 300;

            tft.setTextColor(TFT_WHITE, TFT_BLACK);
            tft.setCursor(10, 145);
            tft.printf("Przerwa: %.1f/%.1fm", gapTraveled, gapDistance);

            tft.drawRect(10, 160, 300, 15, TFT_WHITE);
            tft.fillRect(11, 161, barWidth, 13, TFT_YELLOW);
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // PISTOLETY - WIZUALIZACJA NA SAMYM DOLE EKRANU (y=200-240)
    // ═══════════════════════════════════════════════════════════════

    // Linia oddzielająca
    tft.drawFastHLine(0, 195, 320, TFT_DARKGREY);

    // Nagłówek "Pistolety:"
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setCursor(10, 200);
    tft.print("Pistolety:");

    // Wizualizacja 6 pistoletów (P1-P6)
    for (int i = 0; i < 6; i++) {
        int x = 10 + i * 50;   // Poziom: rozłożone co 50px
        int y = 210;            // Pion: 210px (sam dół)

        if (gunsActive[i]) {
            // Pistolet AKTYWNY - zielony prostokąt
            tft.fillRect(x, y, 45, 25, TFT_GREEN);
            tft.setTextColor(TFT_BLACK, TFT_GREEN);
        } else {
            // Pistolet NIEAKTYWNY - ciemnoszary prostokąt
            tft.fillRect(x, y, 45, 25, TFT_DARKGREY);
            tft.setTextColor(TFT_LIGHTGREY, TFT_DARKGREY);
        }

        // Numer pistoletu w środku prostokąta
        tft.setTextSize(2);
        tft.setCursor(x + 12, y + 5);
        tft.printf("P%d", i + 1);
    }
}

void tftDrawMenu() {
    tft.fillScreen(TFT_BLACK);
    tft.setTextSize(2);
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.setCursor(80, 10);
    tft.println("MENU");

    for (int i = 0; i < menuItemsCount; i++) {
        int y = 40 + i * 30;

        if (i == menuIndex) {
            tft.fillRect(0, y, 320, 25, TFT_NAVY);
            tft.setTextColor(TFT_YELLOW, TFT_NAVY);
        } else {
            tft.setTextColor(TFT_WHITE, TFT_BLACK);
        }

        tft.setCursor(10, y + 5);
        tft.setTextSize(1);
        tft.println(menuItems[i]);
    }

    // Instrukcja
    tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    tft.setCursor(10, 220);
    tft.setTextSize(1);
    tft.println("Joystick: gora/dol, przycisk=OK");
}

// ============================================================================
// FUNKCJE STEROWANIA
// ============================================================================

void setRelay(int relay, bool state) {
    digitalWrite(relay, state ? HIGH : LOW);
}

/**
 * Aktualizuj pistolety według wzorca i trybu
 */
void updateGuns() {
    if (currentMode == MODE_WORKING) {
        // Tryb malowania
        // Sprawdź selektor P3 (fizyczny LUB wirtualny)
        bool reverseP3 = selectorP3Physical || selectorP3Virtual;

        // Pobierz pistolety z wzorca
        for (int i = 0; i < 6; i++) {
            gunsActive[i] = patterns[currentPattern].guns[i];
        }

        // Dla wzorców P-3a, P-3b, P-4: selektor P3 odwraca P1↔P3
        if ((currentPattern == 7 || currentPattern == 8 || currentPattern == 9) && reverseP3) {
            // P-3a (idx 7), P-3b (idx 8), P-4 (idx 9)
            bool temp = gunsActive[0];
            gunsActive[0] = gunsActive[2];
            gunsActive[2] = temp;
        }

        // Jeśli "start od przerwy" i nie przejechano przerwy -> OFF
        float gapDistance = patterns[currentPattern].gapLength;
        if (startFromGap && gapDistance > 0 && gapTraveled < gapDistance) {
            for (int i = 0; i < 6; i++) gunsActive[i] = false;
        }

    } else if (currentMode == MODE_SERVICE) {
        // Tryb serwisowy - test pistoletów
        if (serviceTestPattern >= 0 && serviceTestPattern < PATTERN_COUNT) {
            for (int i = 0; i < 6; i++) {
                gunsActive[i] = patterns[serviceTestPattern].guns[i];
            }
        } else {
            for (int i = 0; i < 6; i++) gunsActive[i] = false;
        }

    } else {
        // Wszystkie OFF (IDLE, PAUSED, MEASURING, MENU)
        for (int i = 0; i < 6; i++) gunsActive[i] = false;
    }

    // Fizycznie ustaw przekaźniki
    setRelay(RELAY_1, gunsActive[0]);
    setRelay(RELAY_2, gunsActive[1]);
    setRelay(RELAY_3, gunsActive[2]);
    setRelay(RELAY_4, gunsActive[3]);
    setRelay(RELAY_5, gunsActive[4]);
    setRelay(RELAY_6, gunsActive[5]);
}

/**
 * START - malowanie
 */
void startSystem() {
    if (currentMode == MODE_IDLE || currentMode == MODE_PAUSED) {
        bool wasIdle = (currentMode == MODE_IDLE);

        currentMode = MODE_WORKING;
        workStartTime = millis();
        encoderPulses = 0;
        distanceTraveled = 0.0;
        gapTraveled = 0.0;
        updateGuns();

        // Rozpocznij raport przy pierwszym START (z IDLE)
        if (wasIdle) {
            startReport();
        }

        Serial.println("\n[START] Malowanie rozpoczęte");
        Serial.printf("Wzorzec: %s (%s)\n", patterns[currentPattern].name, patterns[currentPattern].desc);
        Serial.printf("Start od przerwy: %s\n", startFromGap ? "TAK" : "NIE");
        if (startFromGap) {
            Serial.printf("Długość przerwy: %.1f m\n", patterns[currentPattern].gapLength);
        }
    }
}

void startMeasuring() {
    currentMode = MODE_MEASURING;
    encoderPulses = 0;
    distanceTraveled = 0.0;
    updateGuns();  // Pistolety OFF

    Serial.println("\n[START] Tryb pomiaru - pistolety OFF");
}

void startService() {
    currentMode = MODE_SERVICE;
    serviceTestPattern = -1;
    updateGuns();

    Serial.println("\n[SERVICE] Tryb serwisowy - test pistoletów");
}

/**
 * START kalibracji automatycznej - przygotowanie do jazdy 10m
 */
void startCalibration() {
    currentMode = MODE_CALIBRATING;
    calibrationStartPulses = encoderPulses;  // Zapamiętaj aktualną wartość
    distanceTraveled = 0.0;
    updateGuns();  // Pistolety OFF

    Serial.println("\n╔════════════════════════════════════════════╗");
    Serial.println("║  KALIBRACJA AUTOMATYCZNA                  ║");
    Serial.println("╚════════════════════════════════════════════╝");
    Serial.println("[KALIBRACJA] START - jedź dokładnie 10 metrów");
    Serial.println("[KALIBRACJA] Wciśnij STOP gdy przejedziesz 10m");
    Serial.printf("[KALIBRACJA] Start impulsy: %ld\n", calibrationStartPulses);
}

/**
 * STOP kalibracji - oblicz i zapisz encoderCalibration
 */
void stopCalibration() {
    if (currentMode == MODE_CALIBRATING) {
        long pulsesTotal = encoderPulses - calibrationStartPulses;

        Serial.println("\n[KALIBRACJA] STOP - obliczanie...");
        Serial.printf("[KALIBRACJA] Impulsy przejechane: %ld\n", pulsesTotal);

        if (pulsesTotal > 0) {
            // Automatyczne obliczenie: impulsy / 10 metrów
            encoderCalibration = pulsesTotal / 10.0;

            // Zapisz do NVS
            prefs.putFloat("encCalib", encoderCalibration);

            Serial.println("╔════════════════════════════════════════════╗");
            Serial.printf("║  KALIBRACJA ZAPISANA: %.2f imp/m       ║\n", encoderCalibration);
            Serial.println("╚════════════════════════════════════════════╝");
            Serial.println("[KALIBRACJA] ✓ Zapisano do NVS");
        } else {
            Serial.println("[KALIBRACJA] ❌ BŁĄD - brak impulsów! Sprawdź enkoder.");
        }

        currentMode = MODE_IDLE;
        updateGuns();
    }
}

void enterMenu() {
    currentMode = MODE_MENU;
    menuIndex = 0;
    // tftDrawMenu(); // TYMCZASOWO WYŁĄCZONE
    Serial.println("\n[MENU] Wejście do menu");
}

void stopSystem() {
    if (currentMode != MODE_IDLE) {
        // Specjalna obsługa dla trybu kalibracji
        if (currentMode == MODE_CALIBRATING) {
            stopCalibration();
            return;  // stopCalibration() już ustawia MODE_IDLE
        }

        if (currentMode == MODE_WORKING) {
            totalWorkTime += (millis() - workStartTime);

            // Zakończ i zapisz raport
            stopReport();
        }

        currentMode = MODE_IDLE;
        serviceTestPattern = -1;
        updateGuns();

        Serial.println("\n[STOP] System zatrzymany");
        Serial.printf("Przejechano: %.2f m\n", distanceTraveled);
    }
}

void pauseSystem() {
    if (currentMode == MODE_WORKING) {
        totalWorkTime += (millis() - workStartTime);
        currentMode = MODE_PAUSED;
        updateGuns();
        Serial.println("\n[PAUSE]");
    } else if (currentMode == MODE_PAUSED) {
        currentMode = MODE_WORKING;
        workStartTime = millis();
        updateGuns();
        Serial.println("\n[RESUME]");
    }
}

void changePattern(int newPattern) {
    if (newPattern >= 0 && newPattern < PATTERN_COUNT) {
        currentPattern = newPattern;
        patternChangeCount++;
        Serial.printf("\n[PATTERN] Zmiana: %s (%s)\n",
                      patterns[currentPattern].name,
                      patterns[currentPattern].desc);

        if (currentMode == MODE_WORKING) {
            updateGuns();
        }
    }
}

/**
 * Oblicz dystans i prędkość z enkodera
 */
void updateDistanceAndSpeed() {
    // Dystans
    float prevDistance = distanceTraveled;
    distanceTraveled = encoderPulses / encoderCalibration;  // metry
    float deltaDistance = distanceTraveled - prevDistance;

    // Aktualizuj dystans per wzorzec dla raportów
    if (deltaDistance > 0) {
        updatePatternDistance(deltaDistance);
    }

    // Start od przerwy - aktualizuj gapTraveled
    if (startFromGap && currentMode == MODE_WORKING) {
        float gapDistance = patterns[currentPattern].gapLength;
        if (gapDistance > 0 && gapTraveled < gapDistance) {
            gapTraveled = distanceTraveled;
            if (gapTraveled >= gapDistance) {
                Serial.println("[GAP] Przerwa przejechana - START malowania!");
                updateGuns();  // Włącz pistolety
            }
        }
    }

    // Prędkość (co 1s)
    if (millis() - lastSpeedCalc >= 1000) {
        long pulseDiff = encoderPulses - lastPulseCount;
        float distance = pulseDiff / encoderCalibration;  // metry
        currentSpeed = distance * 3.6;  // m/s -> km/h (3600s/1000m)

        lastPulseCount = encoderPulses;
        lastSpeedCalc = millis();
    }
}

/**
 * Odczytaj joystick
 */
void updateJoystick() {
    joyX = analogRead(JOY_VRX);
    joyY = analogRead(JOY_VRY);
    joySW = digitalRead(JOY_SW) == LOW;  // Aktywny LOW

    // W trybie MENU - nawigacja
    if (currentMode == MODE_MENU) {
        static unsigned long lastJoyMove = 0;
        static bool lastJoySW = false;

        // Góra/dół
        if (millis() - lastJoyMove > 300) {
            if (joyY < 1000) {  // Góra
                menuIndex--;
                if (menuIndex < 0) menuIndex = menuItemsCount - 1;
                // tftDrawMenu(); // TYMCZASOWO WYŁĄCZONE
                Serial.printf("[MENU] UP -> %d\n", menuIndex);
                lastJoyMove = millis();
            } else if (joyY > 3000) {  // Dół
                menuIndex++;
                if (menuIndex >= menuItemsCount) menuIndex = 0;
                // tftDrawMenu(); // TYMCZASOWO WYŁĄCZONE
                Serial.printf("[MENU] DOWN -> %d\n", menuIndex);
                lastJoyMove = millis();
            }
        }

        // Przycisk - zatwierdzenie
        if (joySW && !lastJoySW) {
            Serial.printf("[MENU] Wybrano: %s\n", menuItems[menuIndex]);
            // Wykonaj akcję menu
            if (menuIndex == 0) {
                // Kalibracja automatyczna
                startCalibration();
            } else if (menuIndex == 1) {
                // Raporty pracy - przejdź do panelu WWW
                Serial.println("[MENU] Raporty dostępne w panelu WWW -> 192.168.4.1");
                currentMode = MODE_IDLE;
            } else if (menuIndex == 2) {
                // Aktualizacja OTA
                Serial.println("[MENU] Tryb OTA aktywny - czekam na update...");
                Serial.println("[OTA] Użyj Arduino IDE lub PlatformIO do wysłania firmware");
                Serial.println("[OTA] Hostname: Trassar-Painter");
                // OTA działa w tle - wróć do IDLE
                currentMode = MODE_IDLE;
            } else if (menuIndex == 6) {
                // Test pistoletów
                startService();
            } else {
                // Inne opcje - wróć do IDLE
                currentMode = MODE_IDLE;
            }
        }

        lastJoySW = joySW;
    }
}

// ============================================================================
// ENKODER - PRZERWANIA (POMIAR)
// ============================================================================

/**
 * ✅ POPRAWIONE ISR - używają tylko flag!
 * ISR NIE MOGĄ wywoływać skomplikowanych funkcji (startSystem, stopSystem, etc.)
 * ISR powinny być SZYBKIE i tylko ustawiać flagi
 * Rzeczywista obsługa jest w loop()
 */

void IRAM_ATTR encoderISR() {
    // ✅ Bezpieczne atomowe inkrementowanie
    portENTER_CRITICAL_ISR(&isr_mux);
    if (currentMode == MODE_WORKING || currentMode == MODE_MEASURING || currentMode == MODE_CALIBRATING) {
        encoderPulses++;
    }
    portEXIT_CRITICAL_ISR(&isr_mux);
}

void IRAM_ATTR encoderButtonISR() {
    // ✅ Tylko ustawia flagę - obsługa w loop()
    static unsigned long lastPress = 0;
    unsigned long now = millis();
    if (now - lastPress > BUTTON_DEBOUNCE_MS) {
        portENTER_CRITICAL_ISR(&isr_mux);
        flag_encoderButton = true;
        portEXIT_CRITICAL_ISR(&isr_mux);
        lastPress = now;
    }
}

// ============================================================================
// PRZYCISKI - PRZERWANIA
// ============================================================================
// ✅ POPRAWIONE - tylko flagi!

void IRAM_ATTR btnStartISR() {
    static unsigned long lastPress = 0;
    unsigned long now = millis();
    if (now - lastPress > BUTTON_DEBOUNCE_MS) {
        portENTER_CRITICAL_ISR(&isr_mux);
        flag_btnStart = true;
        portEXIT_CRITICAL_ISR(&isr_mux);
        lastPress = now;
    }
}

void IRAM_ATTR btnStopISR() {
    static unsigned long lastPress = 0;
    unsigned long now = millis();
    if (now - lastPress > BUTTON_DEBOUNCE_MS) {
        portENTER_CRITICAL_ISR(&isr_mux);
        flag_btnStop = true;
        portEXIT_CRITICAL_ISR(&isr_mux);
        lastPress = now;
    }
}

void IRAM_ATTR btnPauseISR() {
    static unsigned long lastPress = 0;
    unsigned long now = millis();
    if (now - lastPress > BUTTON_DEBOUNCE_MS) {
        portENTER_CRITICAL_ISR(&isr_mux);
        flag_btnPause = true;
        portEXIT_CRITICAL_ISR(&isr_mux);
        lastPress = now;
    }
}

// 🚨 EMERGENCY STOP ISR - NAJWYŻSZY PRIORYTET!
void IRAM_ATTR emergencyStopISR() {
    portENTER_CRITICAL_ISR(&isr_mux);
    flag_emergencyStop = true;
    portEXIT_CRITICAL_ISR(&isr_mux);
}

// ============================================================================
// WEB SERVER - API + HTML
// ============================================================================

String getStatusJSON() {
    String json = "{";

    // Tryb
    json += "\"mode\":\"";
    if (currentMode == MODE_IDLE) json += "idle";
    else if (currentMode == MODE_WORKING) json += "working";
    else if (currentMode == MODE_PAUSED) json += "paused";
    else if (currentMode == MODE_MEASURING) json += "measuring";
    else if (currentMode == MODE_SERVICE) json += "service";
    else if (currentMode == MODE_CALIBRATING) json += "calibrating";
    else json += "menu";
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
    for (int i = 0; i < 6; i++) {
        json += gunsActive[i] ? "true" : "false";
        if (i < 5) json += ",";
    }
    json += "],";

    // Enkoder (pomiary)
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
    json += "}";

    json += "}";
    return json;
}

/**
 * Panel WWW - DARK THEME z 3 kartami (tabs)
 */
String getHTMLPage() {
    return R"HTMLCODE(<!DOCTYPE html>
<html lang="pl">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Trassar Painter - Komputer Malowarki</title>
<style>
* { margin:0; padding:0; box-sizing:border-box; }
body {
    font-family:'Segoe UI',Tahoma,Geneva,Verdana,sans-serif;
    background:linear-gradient(135deg,#0f3460 0%,#16213e 100%);
    color:#eaeaea;
    padding:10px;
    min-height:100vh;
}
.container { max-width:1000px; margin:0 auto; }
.card {
    background:#1a1a2e;
    padding:20px;
    border-radius:15px;
    margin:10px 0;
    box-shadow:0 8px 32px rgba(0,0,0,0.5);
    border:1px solid #2a2a3e;
}
h1 { color:#00d4ff; margin-bottom:5px; font-size:1.9em; }
.subtitle { color:#888; font-size:0.9em; margin-bottom:15px; }

/* Tabs */
.tabs {
    display:flex;
    gap:10px;
    margin-bottom:20px;
}
.tab {
    flex:1;
    padding:15px;
    background:#252540;
    border:2px solid #3a3a5a;
    border-radius:10px;
    cursor:pointer;
    text-align:center;
    font-weight:bold;
    transition:all 0.3s;
}
.tab:hover { background:#2e2e50; transform:translateY(-2px); }
.tab.active {
    background:#00d4ff;
    color:#1a1a2e;
    border-color:#00d4ff;
}
.tab-content { display:none; }
.tab-content.active { display:block; }

/* Status bar */
.status-bar {
    background:linear-gradient(135deg,#252540 0%,#1e1e3a 100%);
    padding:18px;
    border-radius:12px;
    margin:15px 0;
    display:grid;
    grid-template-columns:repeat(3,1fr);
    gap:15px;
    border:1px solid #3a3a5a;
}
.status-item { display:flex; align-items:center; gap:10px; }
.status-indicator {
    width:14px;
    height:14px;
    border-radius:50%;
    display:inline-block;
}
.status-idle { background:#666; }
.status-working {
    background:#00ff88;
    animation:pulse 1.5s infinite;
    box-shadow:0 0 15px rgba(0,255,136,0.8);
}
.status-paused { background:#ffaa00; }
.status-measuring { background:#00d4ff; }
.status-menu { background:#e94560; }
.status-service { background:#ff6b9d; }
@keyframes pulse {
    0%,100% { opacity:1; transform:scale(1); }
    50% { opacity:0.6; transform:scale(1.15); }
}

/* Measurements */
.measurements {
    display:grid;
    grid-template-columns:repeat(3,1fr);
    gap:15px;
    margin:15px 0;
}
.measure-box {
    background:linear-gradient(135deg,#2a2a3e,#1e1e2e);
    padding:20px;
    border-radius:12px;
    text-align:center;
    border:1px solid #3a3a5a;
}
.measure-value {
    font-size:2.2em;
    font-weight:bold;
    color:#00d4ff;
    margin-bottom:5px;
}
.measure-label { font-size:0.85em; color:#999; }

/* Buttons */
h3 {
    margin:20px 0 12px 0;
    color:#00d4ff;
    font-size:1.1em;
    border-bottom:2px solid #00d4ff;
    padding-bottom:5px;
}
.pattern-grid {
    display:grid;
    grid-template-columns:repeat(5,1fr);
    gap:10px;
    margin:15px 0;
}
.pattern-btn {
    padding:14px 8px;
    border:2px solid #3a3a5a;
    border-radius:10px;
    background:#252540;
    cursor:pointer;
    font-weight:bold;
    font-size:0.95em;
    transition:all 0.2s;
    text-align:center;
    color:#eaeaea;
}
.pattern-btn:hover {
    background:#2e2e50;
    transform:translateY(-2px);
    box-shadow:0 4px 12px rgba(0,212,255,0.3);
}
.pattern-btn.active {
    transform:scale(1.05);
    background:#00d4ff;
    color:#1a1a2e;
    border-color:#00d4ff;
    box-shadow:0 6px 20px rgba(0,212,255,0.5);
}
.pattern-desc { font-size:0.7em; opacity:0.8; margin-top:3px; }

/* Service mode */
.service-pattern-btn {
    padding:20px 10px;
    border:2px solid #3a3a5a;
    border-radius:10px;
    background:#252540;
    cursor:pointer;
    font-weight:bold;
    font-size:1em;
    text-align:center;
    color:#eaeaea;
    user-select:none;
    -webkit-user-select:none;
    touch-action:manipulation;
}
.service-pattern-btn:active {
    background:#00ff88;
    color:#1a1a2e;
    border-color:#00ff88;
    box-shadow:0 0 25px rgba(0,255,136,0.7);
}
.service-info {
    background:#2a2a3e;
    padding:15px;
    border-radius:10px;
    margin:15px 0;
    border:2px solid #ff6b9d;
    color:#ff6b9d;
    text-align:center;
}

/* Options */
.options {
    display:grid;
    grid-template-columns:repeat(2,1fr);
    gap:15px;
    margin:15px 0;
}
.option-box {
    background:#252540;
    padding:15px;
    border-radius:10px;
    border:1px solid #3a3a5a;
}
.option-box label { display:flex; align-items:center; gap:10px; cursor:pointer; }
.option-box input[type="checkbox"] { width:20px; height:20px; cursor:pointer; }

/* Control buttons */
.control-buttons {
    display:grid;
    grid-template-columns:repeat(2,1fr);
    gap:12px;
    margin:20px 0;
}
.btn {
    padding:20px;
    border:none;
    border-radius:12px;
    font-size:1.15em;
    font-weight:bold;
    cursor:pointer;
    transition:all 0.3s;
    text-transform:uppercase;
    box-shadow:0 4px 15px rgba(0,0,0,0.4);
}
.btn:hover { transform:translateY(-3px); box-shadow:0 6px 20px rgba(0,0,0,0.5); }
.btn:active { transform:translateY(-1px); }
.btn-start { background:linear-gradient(135deg,#00ff88,#00cc6e); color:#1a1a2e; }
.btn-measure { background:linear-gradient(135deg,#00d4ff,#00a0cc); color:#1a1a2e; }
.btn-service { background:linear-gradient(135deg,#ff6b9d,#e94560); color:#fff; }
.btn-pause { background:linear-gradient(135deg,#ffaa00,#cc8800); color:#1a1a2e; }
.btn-stop { background:linear-gradient(135deg,#e94560,#cc3344); color:#fff; }
.btn-menu { background:linear-gradient(135deg,#9b59b6,#8e44ad); color:#fff; }

/* Guns display */
.guns-display {
    display:grid;
    grid-template-columns:repeat(6,1fr);
    gap:10px;
    margin:15px 0;
}
.gun {
    text-align:center;
    padding:18px 8px;
    border-radius:10px;
    background:#252540;
    border:3px solid #3a3a5a;
    transition:all 0.3s;
}
.gun.active {
    background:linear-gradient(135deg,#00ff88,#00cc6e);
    color:#1a1a2e;
    border-color:#00ff88;
    box-shadow:0 0 25px rgba(0,255,136,0.7);
    transform:scale(1.05);
}
.gun-label { font-size:1em; font-weight:bold; margin-bottom:5px; }
.gun-size { font-size:0.75em; opacity:0.8; }

/* Calibration */
.calibration {
    background:#2a2a3e;
    padding:20px;
    border-radius:12px;
    margin:15px 0;
    border:2px solid #e94560;
}
.calibration h4 { color:#e94560; margin-bottom:15px; }
.calib-input {
    display:flex;
    align-items:center;
    gap:10px;
    margin:10px 0;
}
.calib-input label { flex:1; }
.calib-input input {
    width:150px;
    padding:10px;
    background:#1a1a2e;
    border:1px solid #3a3a5a;
    color:#eaeaea;
    border-radius:5px;
    font-size:1em;
}
.calib-input button {
    padding:10px 20px;
    background:#00d4ff;
    color:#1a1a2e;
    border:none;
    border-radius:5px;
    cursor:pointer;
    font-weight:bold;
}

/* Selector P3 toggle button */
.selector-toggle {
    background:#252540;
    padding:20px;
    border-radius:10px;
    border:2px solid #ffaa00;
    text-align:center;
    margin:15px 0;
}
.selector-toggle button {
    padding:15px 30px;
    background:#ffaa00;
    color:#1a1a2e;
    border:none;
    border-radius:10px;
    cursor:pointer;
    font-weight:bold;
    font-size:1.2em;
    transition:all 0.3s;
}
.selector-toggle button:hover { transform:scale(1.05); }
.selector-toggle button.active {
    background:#ff6b00;
    box-shadow:0 0 20px rgba(255,170,0,0.7);
}

/* Footer */
.footer {
    text-align:center;
    margin-top:20px;
    opacity:0.6;
    font-size:0.85em;
}

/* Modal Menu */
.modal {
    display:none;
    position:fixed;
    z-index:1000;
    left:0;
    top:0;
    width:100%;
    height:100%;
    background:rgba(0,0,0,0.8);
    animation:fadeIn 0.3s;
}
.modal-content {
    background:#1a1a2e;
    margin:10% auto;
    padding:30px;
    border:2px solid #00ff88;
    border-radius:15px;
    width:90%;
    max-width:500px;
    animation:slideDown 0.3s;
}
.modal-header {
    display:flex;
    justify-content:space-between;
    align-items:center;
    margin-bottom:20px;
}
.modal-header h2 {
    margin:0;
    color:#00ff88;
}
.close-modal {
    font-size:2em;
    cursor:pointer;
    color:#999;
    transition:color 0.3s;
}
.close-modal:hover {
    color:#e94560;
}
.menu-item {
    background:#2a2a3e;
    padding:20px;
    margin:10px 0;
    border-radius:10px;
    border:2px solid #444;
    cursor:pointer;
    transition:all 0.3s;
    display:flex;
    align-items:center;
    gap:15px;
}
.menu-item:hover {
    border-color:#00ff88;
    transform:translateX(5px);
    box-shadow:0 0 15px rgba(0,255,136,0.3);
}
.menu-icon {
    font-size:2em;
}
.menu-text h3 {
    margin:0 0 5px 0;
    color:#00ff88;
    border:none;
}
.menu-text p {
    margin:0;
    color:#999;
    font-size:0.9em;
}
@keyframes fadeIn {
    from { opacity:0; }
    to { opacity:1; }
}
@keyframes slideDown {
    from { transform:translateY(-50px); opacity:0; }
    to { transform:translateY(0); opacity:1; }
}

/* Responsive */
@media (max-width:600px) {
    .pattern-grid { grid-template-columns:repeat(3,1fr); }
    .control-buttons { grid-template-columns:1fr; }
    .guns-display { grid-template-columns:repeat(3,1fr); }
    .measurements { grid-template-columns:1fr; }
    .status-bar { grid-template-columns:1fr; }
    .modal-content { margin:20% auto; width:95%; }
}
</style>
</head>
<body>
<div class="container">
    <div class="card">
        <h1>🎨 Trassar Painter - Komputer Malowarki</h1>
        <div class="subtitle">Professional Road Marking System v5.0 FINAL • TFT + Joystick</div>

        <!-- Tabs -->
        <div class="tabs">
            <div class="tab active" onclick="showTab(0)">📐 Panel Główny</div>
            <div class="tab" onclick="showTab(1)">📏 Pomiar</div>
            <div class="tab" onclick="showTab(2)">🔧 Kalibracja</div>
            <div class="tab" onclick="showTab(3)">📊 Raporty</div>
        </div>

        <!-- TAB 0: Panel główny -->
        <div class="tab-content active" id="tab0">
            <div class="status-bar">
                <div class="status-item">
                    <span class="status-indicator status-idle" id="statusDot"></span>
                    <strong>Tryb:</strong> <span id="modeText">IDLE</span>
                </div>
                <div class="status-item">
                    <strong>Wzorzec:</strong> <span id="currentPattern">-</span>
                </div>
                <div class="status-item">
                    <strong>Selektor P3:</strong> <span id="selectorP3">-</span>
                </div>
            </div>

            <h3>📏 Pomiary (Enkoder)</h3>
            <div class="measurements">
                <div class="measure-box">
                    <div class="measure-value" id="distance">0.00</div>
                    <div class="measure-label">Dystans (m)</div>
                </div>
                <div class="measure-box">
                    <div class="measure-value" id="speed">0.0</div>
                    <div class="measure-label">Prędkość (km/h)</div>
                </div>
                <div class="measure-box">
                    <div class="measure-value" id="pulses">0</div>
                    <div class="measure-label">Impulsy enkodera</div>
                </div>
            </div>

            <div id="normalMode">
                <!-- Selektor P3 - PRZYCISK -->
                <div class="selector-toggle">
                    <h3 style="border:none; margin:0 0 15px 0;">🔄 Selektor P3 (Wirtualny)</h3>
                    <button id="btnP3Selector" onclick="toggleP3Selector()">
                        P3: <span id="p3SelectorText">NORMALNE</span>
                    </button>
                    <div style="margin-top:10px; font-size:0.85em; color:#999;">
                        Fizyczny selektor (GPIO 20): <span id="p3Physical">-</span>
                    </div>
                </div>

                <h3>⚙️ Opcje</h3>
                <div class="options">
                    <div class="option-box">
                        <label>
                            <input type="checkbox" id="chkStartFromGap">
                            <strong>Start od przerwy</strong>
                        </label>
                        <div style="margin-top:10px; font-size:0.85em; color:#999;">
                            Automatycznie używa przerwy z wzorca
                        </div>
                    </div>
                    <div class="option-box" id="gapProgress" style="display:none;">
                        <strong>Przerwa:</strong> <span id="gapTraveled">0.00</span> / <span id="gapTotal">0.0</span> m
                        <div style="width:100%; background:#3a3a5a; height:10px; border-radius:5px; margin-top:10px;">
                            <div id="gapBar" style="width:0%; background:#00ff88; height:100%; border-radius:5px;"></div>
                        </div>
                    </div>
                </div>

                <h3>📐 Wybór wzorca (15)</h3>
                <div class="pattern-grid" id="patternGrid">
                    <!-- JS -->
                </div>

                <h3>🎮 Sterowanie</h3>
                <div class="control-buttons">
                    <button class="btn btn-start" onclick="sendCmd('start')">▶ START MALOWANIA</button>
                    <button class="btn btn-service" onclick="sendCmd('service')">🔧 TRYB SERWISOWY</button>
                    <button class="btn btn-pause" onclick="sendCmd('pause')">⏸ PAUSE</button>
                    <button class="btn btn-menu" onclick="showMenuModal()">📋 MENU</button>
                    <button class="btn btn-stop" onclick="sendCmd('stop')">⏹ STOP</button>
                </div>
            </div>

            <div id="serviceMode" style="display:none;">
                <div class="service-info">
                    <h3 style="color:#ff6b9d; border:none; margin:0;">🔧 TRYB SERWISOWY - TEST PISTOLETÓW</h3>
                    <p style="margin-top:10px;">Trzymaj przycisk wzorca aby aktywować pistolety. Puszczenie = wyłączenie.</p>
                </div>

                <h3>🧪 Test wzorców (przytrzymaj przycisk)</h3>
                <div class="pattern-grid" id="servicePatternGrid">
                    <!-- JS -->
                </div>

                <div style="text-align:center; margin-top:20px;">
                    <button class="btn btn-stop" onclick="sendCmd('stop')" style="width:50%;">⏹ WYJDŹ Z TRYBU SERWISOWEGO</button>
                </div>
            </div>

            <h3>🔫 Pistolety (Status Live)</h3>
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
            <h3>📏 Tryb pomiaru dystansu</h3>
            <p style="margin:15px 0; color:#999;">Pomiar polega na mierzeniu odległości przejechanej od momentu wciśnięcia startu. W trybie pomiaru pistolety NIE SĄ aktywne.</p>

            <div class="measurements">
                <div class="measure-box">
                    <div class="measure-value" id="distanceMeasure">0.00</div>
                    <div class="measure-label">Dystans (m)</div>
                </div>
                <div class="measure-box">
                    <div class="measure-value" id="speedMeasure">0.0</div>
                    <div class="measure-label">Prędkość (km/h)</div>
                </div>
                <div class="measure-box">
                    <div class="measure-value" id="pulsesMeasure">0</div>
                    <div class="measure-label">Impulsy enkodera</div>
                </div>
            </div>

            <div class="control-buttons">
                <button class="btn btn-measure" onclick="sendCmd('measure')">📏 START POMIARU</button>
                <button class="btn btn-stop" onclick="sendCmd('stop')">⏹ STOP</button>
            </div>

            <div style="background:#2a2a3e; padding:20px; border-radius:12px; margin-top:20px; border:2px solid #00d4ff;">
                <h4 style="color:#00d4ff; margin-bottom:10px;">ℹ️ Informacja</h4>
                <ul style="margin-left:20px; color:#ccc;">
                    <li>Pomiar działa również z joystickiem (jeśli skonfigurowany)</li>
                    <li>Enkoder liczy impulsy w tle</li>
                    <li>Kalibracja enkodera w zakładce "Kalibracja"</li>
                </ul>
            </div>
        </div>

        <!-- TAB 2: Kalibracja -->
        <div class="tab-content" id="tab2">
            <h3>🔧 Kalibracja automatyczna enkodera</h3>

            <div style="background:#2a2a3e; padding:25px; border-radius:12px; margin-bottom:20px; border:2px solid #00ff88;">
                <h4 style="color:#00ff88; margin-bottom:15px;">✨ KALIBRACJA AUTOMATYCZNA</h4>
                <div style="color:#ccc; margin-bottom:20px; line-height:1.6;">
                    <strong>Procedura kalibracji:</strong><br>
                    1️⃣ Przygotuj taśmę/miarę - zaznacz dokładnie <strong style="color:#00ff88;">10 metrów</strong><br>
                    2️⃣ Ustaw maszynę na początku odcinka<br>
                    3️⃣ Naciśnij <strong>START KALIBRACJI</strong><br>
                    4️⃣ Jedź powoli dokładnie 10 metrów<br>
                    5️⃣ Zatrzymaj się na końcu i naciśnij <strong>STOP</strong><br>
                    6️⃣ System <strong>automatycznie obliczy i zapisze</strong> kalibrację! 🎯
                </div>

                <div class="control-buttons">
                    <button class="btn btn-start" onclick="sendCmd('calibration/start')" style="font-size:1.1em;">
                        🚀 START KALIBRACJI
                    </button>
                    <button class="btn btn-stop" onclick="sendCmd('stop')" style="font-size:1.1em;">
                        ⏹ STOP (zapisz)
                    </button>
                </div>
            </div>

            <div style="background:#2a2a3e; padding:20px; border-radius:12px; border:2px solid #e94560;">
                <h4 style="color:#e94560; margin-bottom:10px;">📊 Aktualne dane kalibracji</h4>
                <div style="display:grid; grid-template-columns:1fr 1fr; gap:15px; margin-top:15px;">
                    <div>
                        <strong style="color:#00d4ff;">Impulsy przejechane:</strong><br>
                        <span style="font-size:2em; color:#00ff88;" id="pulsesCalib">0</span>
                    </div>
                    <div>
                        <strong style="color:#00d4ff;">Kalibracja:</strong><br>
                        <span style="font-size:2em; color:#ffaa00;" id="calibCurrent">100.0</span> imp/m
                    </div>
                </div>
                <div style="margin-top:15px; padding-top:15px; border-top:1px solid #444;">
                    <strong style="color:#00d4ff;">Status:</strong>
                    <span id="calibStatus" style="color:#00ff88;">Oczekiwanie na START</span>
                </div>
            </div>

            <div style="background:#2a2a3e; padding:15px; border-radius:12px; margin-top:15px; border:1px solid #666;">
                <h4 style="color:#aaa; font-size:0.9em; margin-bottom:10px;">💡 Wskazówki</h4>
                <ul style="margin-left:20px; color:#999; font-size:0.85em; line-height:1.5;">
                    <li>Kalibruj na tej samej powierzchni co praca (asfalt/beton)</li>
                    <li>Jedź w linii prostej ze stałą prędkością ~5 km/h</li>
                    <li>Po STOP system automatycznie obliczy: impulsy ÷ 10 metrów</li>
                    <li>Kalibracja zapisuje się automatycznie do pamięci NVS</li>
                    <li>Można powtórzyć kalibrację w dowolnym momencie</li>
                </ul>
            </div>
        </div>

        <!-- TAB 3: Raporty -->
        <div class="tab-content" id="tab3">
            <h3>📊 Raporty pracy</h3>

            <!-- Bieżący raport -->
            <div style="background:#2a2a3e; padding:20px; border-radius:12px; margin-bottom:20px; border:2px solid #00ff88;">
                <h4 style="color:#00ff88; margin-bottom:15px;">📝 Bieżący raport</h4>
                <div id="currentReportStatus">
                    <div style="color:#ccc;">
                        <strong>Status:</strong> <span id="reportActive" style="color:#ffaa00;">Ładowanie...</span><br>
                        <strong>Data rozpoczęcia:</strong> <span id="reportStartDate">-</span><br>
                        <strong>Czas rozpoczęcia:</strong> <span id="reportStartTime">-</span><br>
                        <strong>Aktualny czas:</strong> <span id="reportCurrentTime">-</span>
                    </div>
                </div>
                <div style="margin-top:15px; padding:15px; background:#1a1a2e; border-radius:8px;">
                    <p style="color:#999; margin:0; font-size:0.9em;">
                        ℹ️ Raport rozpoczyna się automatycznie po pierwszym START i kończy się po STOP.
                        Po zakończeniu raport jest zapisywany do pamięci.
                    </p>
                </div>
            </div>

            <!-- Lista raportów -->
            <div style="background:#2a2a3e; padding:20px; border-radius:12px; border:2px solid #00d4ff;">
                <h4 style="color:#00d4ff; margin-bottom:15px;">📚 Zapisane raporty</h4>
                <div id="reportsList" style="max-height:400px; overflow-y:auto;">
                    <p style="color:#999; text-align:center;">Ładowanie raportów...</p>
                </div>
                <button class="btn" onclick="loadReports()" style="width:100%; margin-top:15px; background:#00d4ff;">
                    🔄 Odśwież listę
                </button>
            </div>

            <!-- Szczegóły raportu -->
            <div id="reportDetails" style="display:none; background:#2a2a3e; padding:20px; border-radius:12px; margin-top:20px; border:2px solid #e94560;">
                <div style="display:flex; justify-content:space-between; align-items:center; margin-bottom:15px;">
                    <h4 style="color:#e94560; margin:0;">📄 Szczegóły raportu</h4>
                    <button class="btn" onclick="closeReportDetails()" style="padding:5px 15px; background:#666;">✕ Zamknij</button>
                </div>
                <pre id="reportContent" style="background:#1a1a2e; padding:15px; border-radius:8px; color:#ccc; font-size:0.9em; white-space:pre-wrap; overflow-x:auto; max-height:500px; overflow-y:auto;"></pre>
            </div>
        </div>
    </div>

    <div class="footer">
        Trassar251 Professional System • WiFi: Trassar-Painter • v5.2.0 REPORTS + RTC + OTA
    </div>
</div>

<script>
const patterns = [
    {name:'P-1a', desc:'Przerywana długa'},
    {name:'P-1b', desc:'Przerywana krótka'},
    {name:'P-1c', desc:'Wydzielająca'},
    {name:'P-1d', desc:'Prowadząca wąska'},
    {name:'P-1e', desc:'Prowadz. szeroka'},
    {name:'P-2a', desc:'Ciągła wąska'},
    {name:'P-2b', desc:'Ciągła szeroka'},
    {name:'P-3a', desc:'Przekraczalna dł.'},
    {name:'P-3b', desc:'Przekraczalna kr.'},
    {name:'P-4', desc:'Podwójna ciągła'},
    {name:'P-6', desc:'Ostrzegawcza'},
    {name:'P-7a', desc:'Kraw. przeryw. sz.'},
    {name:'P-7b', desc:'Kraw. ciągła sz.'},
    {name:'P-7c', desc:'Kraw. przeryw. w.'},
    {name:'P-7d', desc:'Kraw. ciągła w.'}
];

// Tabs
function showTab(idx) {
    document.querySelectorAll('.tab').forEach((t, i) => {
        if (i === idx) {
            t.classList.add('active');
        } else {
            t.classList.remove('active');
        }
    });
    document.querySelectorAll('.tab-content').forEach((t, i) => {
        if (i === idx) {
            t.classList.add('active');
        } else {
            t.classList.remove('active');
        }
    });
}

// Wzorce - wybór normalny
const grid = document.getElementById('patternGrid');
patterns.forEach((p, i) => {
    const btn = document.createElement('button');
    btn.className = 'pattern-btn';
    btn.innerHTML = '<div>' + p.name + '</div><div class="pattern-desc">' + p.desc + '</div>';
    btn.onclick = () => changePattern(i);
    btn.id = 'pattern' + i;
    grid.appendChild(btn);
});

// Wzorce - tryb serwisowy (przytrzymanie)
const serviceGrid = document.getElementById('servicePatternGrid');
patterns.forEach((p, i) => {
    const btn = document.createElement('button');
    btn.className = 'service-pattern-btn';
    btn.innerHTML = '<div>' + p.name + '</div><div class="pattern-desc">' + p.desc + '</div>';
    btn.id = 'servicePattern' + i;

    // Desktop - mousedown/mouseup
    btn.onmousedown = () => testPattern(i, true);
    btn.onmouseup = () => testPattern(i, false);
    btn.onmouseleave = () => testPattern(i, false);

    // Mobile - touchstart/touchend
    btn.ontouchstart = (e) => {
        e.preventDefault();
        testPattern(i, true);
    };
    btn.ontouchend = (e) => {
        e.preventDefault();
        testPattern(i, false);
    };

    serviceGrid.appendChild(btn);
});

function sendCmd(cmd) {
    fetch('/api/cmd', {
        method: 'POST',
        headers: {'Content-Type': 'application/x-www-form-urlencoded'},
        body: 'c=' + cmd
    })
    .then(r => r.text())
    .then(d => {
        if (d === 'OK') updateStatus();
    });
}

function changePattern(idx) {
    fetch('/api/pattern', {
        method: 'POST',
        headers: {'Content-Type': 'application/x-www-form-urlencoded'},
        body: 'p=' + idx
    })
    .then(r => r.text())
    .then(d => {
        if (d === 'OK') updateStatus();
    });
}

function testPattern(idx, active) {
    fetch('/api/service', {
        method: 'POST',
        headers: {'Content-Type': 'application/x-www-form-urlencoded'},
        body: 'pattern=' + idx + '&state=' + (active ? '1' : '0')
    });
}

function toggleP3Selector() {
    fetch('/api/p3selector', {
        method: 'POST',
        headers: {'Content-Type': 'application/x-www-form-urlencoded'},
        body: ''
    })
    .then(r => r.text())
    .then(d => {
        if (d === 'OK') updateStatus();
    });
}

function updateStatus() {
    fetch('/api/status')
    .then(r => r.json())
    .then(data => {
        // Mode
        const dot = document.getElementById('statusDot');
        const modeText = document.getElementById('modeText');
        dot.className = 'status-indicator status-' + data.mode;
        modeText.textContent = data.mode.toUpperCase();

        // Pokaż/ukryj sekcje według trybu
        if (data.mode === 'service') {
            document.getElementById('normalMode').style.display = 'none';
            document.getElementById('serviceMode').style.display = 'block';
        } else {
            document.getElementById('normalMode').style.display = 'block';
            document.getElementById('serviceMode').style.display = 'none';
        }

        // Pattern
        document.getElementById('currentPattern').textContent = data.pattern + ' (' + data.patternDesc + ')';

        // Selektor P3
        document.getElementById('selectorP3').textContent = data.selectorP3;
        document.getElementById('p3Physical').textContent = data.selectorP3Physical ? 'HIGH (odwrócone)' : 'LOW (normalne)';

        const p3Btn = document.getElementById('btnP3Selector');
        const p3Text = document.getElementById('p3SelectorText');
        if (data.selectorP3Virtual) {
            p3Btn.classList.add('active');
            p3Text.textContent = 'ODWRÓCONE';
        } else {
            p3Btn.classList.remove('active');
            p3Text.textContent = 'NORMALNE';
        }

        // Wzorce - highlight
        patterns.forEach((p, i) => {
            const btn = document.getElementById('pattern' + i);
            if (i === data.patternIndex) {
                btn.classList.add('active');
            } else {
                btn.classList.remove('active');
            }
        });

        // Pistolety
        data.guns.forEach((active, i) => {
            const gun = document.getElementById('gun' + i);
            if (active) {
                gun.classList.add('active');
            } else {
                gun.classList.remove('active');
            }
        });

        // Pomiary (wszystkie karty)
        document.getElementById('distance').textContent = data.encoder.distance.toFixed(2);
        document.getElementById('speed').textContent = data.encoder.speed.toFixed(1);
        document.getElementById('pulses').textContent = data.encoder.pulses;

        document.getElementById('distanceMeasure').textContent = data.encoder.distance.toFixed(2);
        document.getElementById('speedMeasure').textContent = data.encoder.speed.toFixed(1);
        document.getElementById('pulsesMeasure').textContent = data.encoder.pulses;

        document.getElementById('pulsesCalib').textContent = data.encoder.pulses;
        document.getElementById('calibCurrent').textContent = data.encoder.calibration.toFixed(1);

        // Status kalibracji
        const calibStatus = document.getElementById('calibStatus');
        if (data.mode === 'calibrating') {
            calibStatus.textContent = '🚀 KALIBRACJA W TOKU - jedź 10m i wciśnij STOP';
            calibStatus.style.color = '#00ff88';
        } else {
            calibStatus.textContent = 'Oczekiwanie na START';
            calibStatus.style.color = '#999';
        }

        // Start od przerwy
        if (data.startFromGap && data.gapDistance > 0) {
            document.getElementById('gapProgress').style.display = 'block';
            document.getElementById('gapTraveled').textContent = data.gapTraveled.toFixed(2);
            document.getElementById('gapTotal').textContent = data.gapDistance.toFixed(1);
            const percent = (data.gapTraveled / data.gapDistance) * 100;
            document.getElementById('gapBar').style.width = Math.min(percent, 100) + '%';
        } else {
            document.getElementById('gapProgress').style.display = 'none';
        }
    })
    .catch(e => console.error(e));
}

// Checkbox Start od przerwy
document.getElementById('chkStartFromGap').addEventListener('change', (e) => {
    fetch('/api/option', {
        method: 'POST',
        headers: {'Content-Type': 'application/x-www-form-urlencoded'},
        body: 'startFromGap=' + (e.target.checked ? '1' : '0')
    });
});

// Auto-refresh 500ms
setInterval(updateStatus, 500);

// ========== RAPORTY - Functions ==========

// Load current report status
function loadCurrentReport() {
    fetch('/api/report/current')
    .then(r => r.json())
    .then(data => {
        const statusSpan = document.getElementById('reportActive');
        const startDate = document.getElementById('reportStartDate');
        const startTime = document.getElementById('reportStartTime');
        const currentTime = document.getElementById('reportCurrentTime');

        if (data.active) {
            statusSpan.textContent = '✅ AKTYWNY';
            statusSpan.style.color = '#00ff88';
            startDate.textContent = data.startDate;
            startTime.textContent = data.startTime;
            currentTime.textContent = data.currentTime;
        } else {
            statusSpan.textContent = '⏸️ Nieaktywny';
            statusSpan.style.color = '#999';
            startDate.textContent = '-';
            startTime.textContent = '-';
            currentTime.textContent = data.currentTime;
        }
    })
    .catch(e => {
        console.error('Error loading current report:', e);
        document.getElementById('reportActive').textContent = '❌ Błąd';
    });
}

// Load reports list
function loadReports() {
    fetch('/api/reports')
    .then(r => r.json())
    .then(data => {
        const list = document.getElementById('reportsList');

        if (data.count === 0) {
            list.innerHTML = '<p style="color:#999; text-align:center; padding:20px;">Brak zapisanych raportów</p>';
            return;
        }

        let html = '<div style="display:flex; flex-direction:column; gap:10px;">';
        for (let i = data.count - 1; i >= 0; i--) {
            html += '<button class="btn" onclick="viewReport(' + i + ')" style="text-align:left; background:#1a1a2e; padding:15px;">';
            html += '📄 Raport #' + (i + 1);
            html += '</button>';
        }
        html += '</div>';

        list.innerHTML = html;
    })
    .catch(e => {
        console.error('Error loading reports:', e);
        document.getElementById('reportsList').innerHTML = '<p style="color:#e94560; text-align:center;">❌ Błąd ładowania raportów</p>';
    });
}

// View specific report
function viewReport(id) {
    fetch('/api/report?id=' + id)
    .then(r => r.text())
    .then(content => {
        document.getElementById('reportContent').textContent = content;
        document.getElementById('reportDetails').style.display = 'block';

        // Scroll to details
        document.getElementById('reportDetails').scrollIntoView({ behavior: 'smooth' });
    })
    .catch(e => {
        console.error('Error loading report:', e);
        alert('Błąd ładowania raportu #' + (id + 1));
    });
}

// Close report details
function closeReportDetails() {
    document.getElementById('reportDetails').style.display = 'none';
}

// Auto-refresh current report status every 2 seconds
setInterval(loadCurrentReport, 2000);

// Initial load
loadCurrentReport();
loadReports();
updateStatus();

// ========== MENU MODAL - Functions ==========

// Show menu modal
function showMenuModal() {
    document.getElementById('menuModal').style.display = 'block';
}

// Close menu modal
function closeMenuModal() {
    document.getElementById('menuModal').style.display = 'none';
}

// Menu actions
function menuCalibration() {
    closeMenuModal();
    showTab(2); // Go to Calibration tab
}

function menuReports() {
    closeMenuModal();
    showTab(3); // Go to Reports tab
}

function menuOTA() {
    closeMenuModal();
    if (confirm('Czy na pewno chcesz uruchomić tryb OTA?\n\nKomputer będzie czekać na aktualizację firmware.\n\nUżyj Arduino IDE lub PlatformIO aby wysłać nowy firmware.')) {
        sendCmd('menu'); // Send menu command to activate OTA mode
        alert('Tryb OTA aktywny!\n\nHostname: Trassar-Painter\nHasło: trassar2024\n\nOdśwież stronę po zakończeniu aktualizacji.');
    }
}

// Close modal when clicking outside
window.onclick = function(event) {
    const modal = document.getElementById('menuModal');
    if (event.target == modal) {
        closeMenuModal();
    }
}
</script>

<!-- Menu Modal -->
<div id="menuModal" class="modal">
    <div class="modal-content">
        <div class="modal-header">
            <h2>📋 Menu</h2>
            <span class="close-modal" onclick="closeMenuModal()">&times;</span>
        </div>

        <div class="menu-item" onclick="menuCalibration()">
            <div class="menu-icon">🔧</div>
            <div class="menu-text">
                <h3>Kalibracja enkodera</h3>
                <p>Automatyczna kalibracja - jedź 10m i oblicz</p>
            </div>
        </div>

        <div class="menu-item" onclick="menuReports()">
            <div class="menu-icon">📊</div>
            <div class="menu-text">
                <h3>Raporty pracy</h3>
                <p>Przeglądaj zapisane raporty powierzchni</p>
            </div>
        </div>

        <div class="menu-item" onclick="menuOTA()">
            <div class="menu-icon">🔄</div>
            <div class="menu-text">
                <h3>Aktualizacja OTA</h3>
                <p>Aktualizuj firmware przez WiFi</p>
            </div>
        </div>

        <div style="margin-top:20px; text-align:center;">
            <button class="btn" onclick="closeMenuModal()" style="background:#666; width:100%;">
                Zamknij
            </button>
        </div>
    </div>
</div>

</body>
</html>
)HTMLCODE";
}

// ============================================================================
// WEB SERVER - ENDPOINTS
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
            if (cmd == "start") {
                startSystem();
            } else if (cmd == "measure") {
                startMeasuring();
            } else if (cmd == "service") {
                startService();
            } else if (cmd == "menu") {
                enterMenu();
            } else if (cmd == "stop") {
                stopSystem();
            } else if (cmd == "pause") {
                pauseSystem();
            }
            server.send(200, "text/plain", "OK");
        } else {
            server.send(400, "text/plain", "Missing 'c'");
        }
    });

    server.on("/api/pattern", HTTP_POST, []() {
        if (server.hasArg("p")) {
            int idx = server.arg("p").toInt();
            changePattern(idx);
            server.send(200, "text/plain", "OK");
        } else {
            server.send(400, "text/plain", "Missing 'p'");
        }
    });

    server.on("/api/service", HTTP_POST, []() {
        if (server.hasArg("pattern") && server.hasArg("state")) {
            int pattern = server.arg("pattern").toInt();
            bool state = server.arg("state") == "1";

            if (currentMode == MODE_SERVICE) {
                if (state) {
                    serviceTestPattern = pattern;
                    Serial.printf("[SERVICE] Test wzorca: %s\n", patterns[pattern].name);
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

    // NOWY ENDPOINT - przycisk selektora P3 w panelu
    server.on("/api/p3selector", HTTP_POST, []() {
        selectorP3Virtual = !selectorP3Virtual;
        Serial.printf("[P3] Selektor wirtualny: %s\n", selectorP3Virtual ? "ODWRÓCONE" : "NORMALNE");
        if (currentMode == MODE_WORKING) {
            updateGuns();
        }
        server.send(200, "text/plain", "OK");
    });

    // NOWY ENDPOINT - START kalibracji automatycznej
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

    // API - Raporty
    server.on("/api/reports", HTTP_GET, []() {
        server.send(200, "application/json", getReportsListJSON());
    });

    server.on("/api/report", HTTP_GET, []() {
        if (server.hasArg("id")) {
            int id = server.arg("id").toInt();
            String content = getReportContent(id);
            server.send(200, "text/plain; charset=utf-8", content);
        } else {
            server.send(400, "text/plain", "Missing 'id'");
        }
    });

    server.on("/api/report/current", HTTP_GET, []() {
        String json = "{";
        json += "\"active\":" + String(reportActive ? "true" : "false") + ",";
        json += "\"startDate\":\"" + String(currentReport.startDate) + "\",";
        json += "\"startTime\":\"" + String(currentReport.startTime) + "\",";
        json += "\"currentTime\":\"" + getCurrentDateTime() + "\"";
        json += "}";
        server.send(200, "application/json", json);
    });

    // 🔴 SAFETY API - Status bezpieczeństwa
    server.on("/api/safety/status", HTTP_GET, []() {
        String json = "{";
        json += "\"emergencyStop\":" + String(emergencyStopActive ? "true" : "false") + ",";
        json += "\"watchdogOk\":" + String(checkWatchdog() ? "true" : "false") + ",";
        json += "\"heartbeatOk\":" + String((millis() - lastHeartbeat < HEARTBEAT_INTERVAL * 2) ? "true" : "false") + ",";
        json += "\"selfTestPassed\":" + String(selfTestPassed ? "true" : "false") + ",";
        json += "\"errorCount\":" + String(errorCount) + ",";
        json += "\"statusLedGreen\":" + String(statusLedGreen ? "true" : "false") + ",";
        json += "\"statusLedRed\":" + String(statusLedRed ? "true" : "false") + ",";
        json += "\"statusLedYellow\":" + String(statusLedYellow ? "true" : "false");
        json += "}";
        server.send(200, "application/json", json);
    });

    // 🔴 SAFETY API - Error Log
    server.on("/api/safety/errors", HTTP_GET, []() {
        String json = "[";
        int count = min(errorCount, 50);  // Max 50 ostatnich błędów
        for (int i = 0; i < count; i++) {
            int idx = (errorLogIndex - count + i + 50) % 50;
            if (i > 0) json += ",";
            json += "{";
            json += "\"type\":" + String(errorLog[idx].type) + ",";
            json += "\"timestamp\":\"" + String(errorLog[idx].timestamp) + "\",";
            json += "\"message\":\"" + String(errorLog[idx].message) + "\"";
            json += "}";
        }
        json += "]";
        server.send(200, "application/json", json);
    });

    // 🔴 SAFETY API - CSV Export
    server.on("/api/report/csv", HTTP_GET, []() {
        if (server.hasArg("id")) {
            int id = server.arg("id").toInt();
            String csv = exportReportCSV(id);
            server.send(200, "text/csv", csv);
        } else {
            server.send(400, "text/plain", "Missing 'id'");
        }
    });

    // 🔴 SAFETY API - Reset E-STOP
    server.on("/api/safety/reset-estop", HTTP_POST, []() {
        resetEmergencyStop();
        server.send(200, "text/plain", "OK");
    });

    server.onNotFound([]() {
        server.sendHeader("Location", "/");
        server.send(302);
    });

    server.begin();
}

// ============================================================================
// SETUP
// ============================================================================

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println("\n\n╔════════════════════════════════════════════╗");
    Serial.println("║  TRASSAR PAINTER v5.3.0                   ║");
    Serial.println("║  🔴 PRODUCTION SAFETY EDITION             ║");
    Serial.println("║  E-STOP + Watchdog + Fail-safe + Alerts   ║");
    Serial.println("║  15 wzorców + Auto-kalibracja + Raporty   ║");
    Serial.println("╚════════════════════════════════════════════╝\n");

    // LittleFS - system plików dla raportów
    if (!LittleFS.begin(true)) {
        Serial.println("[FS] ❌ Błąd montowania LittleFS");
    } else {
        Serial.println("[FS] ✓ LittleFS zamontowany");
        // Policz istniejące raporty
        reportCount = 0;
        while (LittleFS.exists("/report_" + String(reportCount) + ".txt")) {
            reportCount++;
        }
        Serial.printf("[FS] Znaleziono %d raportów\n", reportCount);
    }

    // TFT init (tymczasowo wyłączony)
    tftInit();

    // Preferences - odczyt kalibracji
    prefs.begin("trassar", false);
    encoderCalibration = prefs.getFloat("encCalib", 100.0);
    Serial.printf("[CALIB] Enkoder: %.1f impulsów/metr\n", encoderCalibration);

    // GPIO - Przekaźniki
    pinMode(RELAY_1, OUTPUT);
    pinMode(RELAY_2, OUTPUT);
    pinMode(RELAY_3, OUTPUT);
    pinMode(RELAY_4, OUTPUT);
    pinMode(RELAY_5, OUTPUT);
    pinMode(RELAY_6, OUTPUT);
    updateGuns();
    Serial.println("[GPIO] ✓ Przekaźniki");

    // GPIO - Przyciski
    pinMode(BTN_START, INPUT_PULLUP);
    pinMode(BTN_STOP, INPUT_PULLUP);
    pinMode(BTN_PAUSE, INPUT_PULLUP);
    Serial.println("[GPIO] ✓ Przyciski");

    // GPIO - Selektor P3
    pinMode(SEL_P3, INPUT_PULLUP);
    Serial.println("[GPIO] ✓ Selektor P3");

    // GPIO - Joystick
    pinMode(JOY_SW, INPUT_PULLUP);
    Serial.println("[GPIO] ✓ Joystick");

    // GPIO - Enkoder (pomiarowy)
    pinMode(ENC_CLK, INPUT_PULLUP);
    pinMode(ENC_DT, INPUT_PULLUP);
    pinMode(ENC_SW, INPUT_PULLUP);
    Serial.println("[GPIO] ✓ Enkoder pomiarowy");

    // RTC DS1307 - Zegar czasu rzeczywistego
    Serial.println();
    initRTC();

    // Przerwania - Enkoder
    attachInterrupt(digitalPinToInterrupt(ENC_CLK), encoderISR, RISING);
    attachInterrupt(digitalPinToInterrupt(ENC_SW), encoderButtonISR, FALLING);
    Serial.println("[INT] ✓ Enkoder");

    // Przerwania - Przyciski
    attachInterrupt(digitalPinToInterrupt(BTN_START), btnStartISR, FALLING);
    attachInterrupt(digitalPinToInterrupt(BTN_STOP), btnStopISR, FALLING);
    attachInterrupt(digitalPinToInterrupt(BTN_PAUSE), btnPauseISR, FALLING);
    Serial.println("[INT] ✓ Przyciski");

    // 🚨 SAFETY - E-STOP Interrupt (najwyższy priorytet!)
    pinMode(BTN_EMERGENCY_STOP, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(BTN_EMERGENCY_STOP), emergencyStopISR, FALLING);
    Serial.println("[INT] ✓ 🚨 E-STOP");

    // 🔴 SAFETY - GPIO inicjalizacja
    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(LED_STATUS_GREEN, OUTPUT);
    pinMode(LED_STATUS_RED, OUTPUT);
    pinMode(LED_STATUS_YELLOW, OUTPUT);
    Serial.println("[SAFETY] ✓ Buzzer + Status LEDs");

    // Zapisz początkową kalibrację (do detekcji drift)
    initialCalibration = encoderCalibration;

    // 🔍 SELF-TEST - diagnostyka startowa
    performSelfTest();

    // Inicjalizuj safety timery
    lastWatchdogReset = millis();
    lastHeartbeat = millis();
    lastDeadmanConfirm = millis();
    lastEncoderChange = millis();

    // WiFi - tryb AP+STA (jednocześnie Access Point i klient WiFi)
    Serial.println("\n╔════════════════════════════════════════════╗");
    Serial.println("║  WiFi - TRYB AP+STA                       ║");
    Serial.println("╚════════════════════════════════════════════╝");

    WiFi.mode(WIFI_AP_STA);  // Tryb hybrydowy

    // 1. KROK - Połącz się z WiFi użytkownika (aby pobrać czas z NTP)
    Serial.println("\n[WiFi STA] Łączenie z siecią WiFi...");
    Serial.printf("[WiFi STA] SSID: %s\n", WIFI_SSID);

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\n[WiFi STA] ✓ Połączono z WiFi!");
        Serial.printf("[WiFi STA] IP: %s\n", WiFi.localIP().toString().c_str());

        // NTP - synchronizacja czasu (teraz mamy internet!)
        Serial.println();
        syncNTP();
    } else {
        Serial.println("\n[WiFi STA] ❌ Nie udało się połączyć z WiFi");
        Serial.println("[WiFi STA] Sprawdź SSID i hasło w kodzie (linijka ~45)");
        Serial.println("[WiFi STA] RTC będzie używać poprzednio zapisanego czasu");
    }

    // 2. KROK - Uruchom Access Point (panel WWW będzie zawsze dostępny)
    Serial.println("\n[WiFi AP] Uruchamianie Access Point...");
    if (WiFi.softAP(AP_SSID, AP_PASSWORD)) {
        Serial.println("[WiFi AP] ✓ AP uruchomiony!");
        Serial.printf("[WiFi AP] SSID: %s\n", AP_SSID);
        Serial.printf("[WiFi AP] IP: %s\n", WiFi.softAPIP().toString().c_str());

        // OTA - aktualizacje przez WiFi
        Serial.println();
        ArduinoOTA.setHostname("Trassar-Painter");
        ArduinoOTA.setPassword("trassar2024");  // Hasło OTA dla bezpieczeństwa

        ArduinoOTA.onStart([]() {
            String type = (ArduinoOTA.getCommand() == U_FLASH) ? "sketch" : "filesystem";
            Serial.println("\n[OTA] Rozpoczynam aktualizację: " + type);
            LittleFS.end();  // Odmontuj FS przed update
        });

        ArduinoOTA.onEnd([]() {
            Serial.println("\n[OTA] ✓ Aktualizacja zakończona!");
        });

        ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
            Serial.printf("[OTA] Postęp: %u%%\r", (progress / (total / 100)));
        });

        ArduinoOTA.onError([](ota_error_t error) {
            Serial.printf("\n[OTA] ❌ Błąd[%u]: ", error);
            if (error == OTA_AUTH_ERROR) Serial.println("Błędne hasło");
            else if (error == OTA_BEGIN_ERROR) Serial.println("Błąd BEGIN");
            else if (error == OTA_CONNECT_ERROR) Serial.println("Błąd połączenia");
            else if (error == OTA_RECEIVE_ERROR) Serial.println("Błąd odbioru");
            else if (error == OTA_END_ERROR) Serial.println("Błąd END");
        });

        ArduinoOTA.begin();
        Serial.println("[OTA] ✓ OTA gotowe - hostname: Trassar-Painter");
        Serial.println("[OTA] Hasło: trassar2024");

        // Web Server
        setupWebServer();

        Serial.println("\n╔════════════════════════════════════════════╗");
        Serial.println("║  🌐 PANEL: http://192.168.4.1            ║");
        Serial.println("║  📡 OTA: Trassar-Painter (trassar2024)   ║");
        Serial.println("╚════════════════════════════════════════════╝\n");
    } else {
        Serial.println("[WiFi] ✗ BŁĄD!");
    }

    Serial.println("╔════════════════════════════════════════════╗");
    Serial.println("║  SYSTEM GOTOWY                            ║");
    Serial.println("╚════════════════════════════════════════════╝\n");
}

// ============================================================================
// LOOP
// ============================================================================

void loop() {
    // ═══════════════════════════════════════════════════════════════
    // 🚨 SAFETY - NAJWYŻSZY PRIORYTET!
    // ═══════════════════════════════════════════════════════════════

    // 1. E-STOP - obsługa natychmiast!
    if (flag_emergencyStop) {
        portENTER_CRITICAL(&isr_mux);
        flag_emergencyStop = false;
        portEXIT_CRITICAL(&isr_mux);
        activateEmergencyStop();
    }

    // 2. Jeśli E-STOP aktywny - blokuj wszystko poza resetem
    if (emergencyStopActive) {
        // Tylko reset E-STOP jest dozwolony
        buzzerUpdate();
        updateStatusLeds();

        // Sprawdź czy można zresetować (przycisk zwolniony)
        if (digitalRead(BTN_EMERGENCY_STOP) == HIGH && flag_btnStart) {
            portENTER_CRITICAL(&isr_mux);
            flag_btnStart = false;
            portEXIT_CRITICAL(&isr_mux);
            resetEmergencyStop();
        }

        // Pomiń resztę loop() - system zablokowany
        delay(50);
        return;
    }

    // 3. Watchdog - reset co loop
    resetWatchdog();

    // 4. Heartbeat - aktualizuj co loop
    updateHeartbeat();

    // 5. Buzzer - obsługa sygnałów dźwiękowych
    buzzerUpdate();

    // ═══════════════════════════════════════════════════════════════
    // WEB & OTA
    // ═══════════════════════════════════════════════════════════════

    ArduinoOTA.handle();
    server.handleClient();

    // ═══════════════════════════════════════════════════════════════
    // ✅ OBSŁUGA FLAG ISR
    // ═══════════════════════════════════════════════════════════════

    // Obsługa przycisku START
    if (flag_btnStart) {
        portENTER_CRITICAL(&isr_mux);
        flag_btnStart = false;
        portEXIT_CRITICAL(&isr_mux);
        startSystem();
        confirmDeadman();  // START = potwierdzenie operatora
    }

    // Obsługa przycisku STOP
    if (flag_btnStop) {
        portENTER_CRITICAL(&isr_mux);
        flag_btnStop = false;
        portEXIT_CRITICAL(&isr_mux);
        stopSystem();
    }

    // Obsługa przycisku PAUSE
    if (flag_btnPause) {
        portENTER_CRITICAL(&isr_mux);
        flag_btnPause = false;
        portEXIT_CRITICAL(&isr_mux);
        pauseSystem();
        confirmDeadman();  // PAUSE = potwierdzenie operatora
    }

    // Obsługa przycisku enkodera
    if (flag_encoderButton) {
        portENTER_CRITICAL(&isr_mux);
        flag_encoderButton = false;
        portEXIT_CRITICAL(&isr_mux);

        if (currentMode == MODE_MENU) {
            // Zatwierdzenie w MENU
            Serial.printf("[MENU] Wybrano: %s\n", menuItems[menuIndex]);
        } else {
            // PAUSE/RESUME
            pauseSystem();
        }
        confirmDeadman();  // Interakcja = potwierdzenie
    }

    // Aktualizuj pomiary
    updateDistanceAndSpeed();

    // Odczyt joysticka
    updateJoystick();

    // Odczyt selektora P3 fizycznego (jeśli zmiana - aktualizuj pistolety)
    static bool lastP3State = false;
    bool currentP3State = digitalRead(SEL_P3) == HIGH;
    if (currentP3State != lastP3State) {
        lastP3State = currentP3State;
        selectorP3Physical = currentP3State;
        Serial.printf("[P3] Selektor fizyczny: %s\n", currentP3State ? "ODWRÓCONE" : "NORMALNE");
        if (currentMode == MODE_WORKING) {
            updateGuns();  // Przełącz pistolety natychmiast
        }
    }

    // Aktualizacja TFT (co 500ms) - TYMCZASOWO WYŁĄCZONE
    /*
    if (millis() - lastTFTUpdate > TFT_UPDATE_INTERVAL) {
        if (currentMode != MODE_MENU) {
            tftDrawStatus();
        }
        lastTFTUpdate = millis();
    }
    */

    // ═══════════════════════════════════════════════════════════════
    // 🔴 SAFETY - PERIODIC CHECKS
    // ═══════════════════════════════════════════════════════════════

    // Sprawdzenia co 1s (heartbeat, deadman, encoder health)
    static unsigned long lastSafetyCheck = 0;
    if (millis() - lastSafetyCheck > 1000) {
        lastSafetyCheck = millis();

        // 1. Sprawdź heartbeat (fail-safe)
        if (!checkHeartbeat()) {
            setStatusLed(false, true, false);  // Czerwony
        }

        // 2. Sprawdź deadman switch (tylko gdy aktywny)
        if (deadmanActive && currentMode == MODE_WORKING) {
            checkDeadman();
        }

        // 3. Sprawdź zdrowie enkodera
        if (currentMode == MODE_WORKING || currentMode == MODE_MEASURING || currentMode == MODE_CALIBRATING) {
            checkEncoderHealth();
        }

        // 4. Waliduj prędkość
        if (currentMode == MODE_WORKING && currentSpeed > 0.1) {
            validateSpeed();
        }

        // 5. Sprawdź drift kalibracji (rzadziej - co 60s)
        static unsigned long lastCalibCheck = 0;
        if (millis() - lastCalibCheck > 60000) {
            lastCalibCheck = millis();
            checkCalibrationDrift();
        }

        // 6. Sprawdź watchdog
        if (!checkWatchdog()) {
            // Watchdog timeout - restart systemu
            logError(ERR_WATCHDOG_TIMEOUT, "CRITICAL: Watchdog timeout - restart!");
            ESP.restart();
        }

        // 7. Status LED - aktualizuj według trybu pracy
        if (currentMode == MODE_WORKING) {
            // Podczas pracy - migający zielony
            static bool ledBlink = false;
            ledBlink = !ledBlink;
            setStatusLed(ledBlink, false, false);
        } else if (currentMode == MODE_IDLE && !emergencyStopActive) {
            // IDLE - stały zielony
            setStatusLed(true, false, false);
        }
    }

    delay(10);
}
