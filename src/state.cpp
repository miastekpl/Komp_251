/**
 * @file state.cpp
 * @brief Definicje zmiennych globalnych - Trassar-Painter v6.0.0
 */

#include "state.h"

// ============================================================================
// FLAGI ISR
// ============================================================================
volatile bool flag_btnStart = false;
volatile bool flag_btnStop = false;
volatile bool flag_btnPause = false;
volatile bool flag_encoderButton = false;
volatile bool flag_emergencyStop = false;

portMUX_TYPE isr_mux = portMUX_INITIALIZER_UNLOCKED;

// ============================================================================
// TRYB PRACY
// ============================================================================
SystemMode currentMode = MODE_IDLE;

// ============================================================================
// WZORZEC
// ============================================================================
int currentPattern = 5;                 // Domyślnie P-2a (ciągła wąska)
bool selectorP3Physical = false;
bool selectorP3Virtual = false;

// ============================================================================
// PISTOLETY
// ============================================================================
bool gunsActive[GUN_COUNT] = {false};

// ============================================================================
// TRYB SERWISOWY
// ============================================================================
int serviceTestPattern = -1;

// ============================================================================
// ENKODER POMIAROWY
// ============================================================================
volatile long encoderPulses = 0;
float encoderCalibration = DEFAULT_CALIBRATION;
float distanceTraveled = 0.0f;
float currentSpeed = 0.0f;
unsigned long lastSpeedCalc = 0;
long lastPulseCount = 0;
long calibrationStartPulses = 0;

// ============================================================================
// JOYSTICK
// ============================================================================
int joyX = JOY_CENTER;
int joyY = JOY_CENTER;
bool joySW = false;

// ============================================================================
// START OD PRZERWY
// ============================================================================
bool startFromGap = false;
float gapTraveled = 0.0f;

// ============================================================================
// STATYSTYKI
// ============================================================================
unsigned long workStartTime = 0;
unsigned long totalWorkTime = 0;
int patternChangeCount = 0;

// ============================================================================
// RAPORTY PRACY
// ============================================================================
WorkReport currentReport;
float distancePerPattern[PATTERN_COUNT] = {0};
bool reportActive = false;
int reportCount = 0;

// ============================================================================
// MENU
// ============================================================================
int menuIndex = 0;
const char* menuItems[] = {
    "Kalibracja enkodera",
    "Raporty pracy",
    "Aktualizacja OTA",
    "Wybor wzorca",
    "Ustawienia",
    "Statystyki",
    "Test pistoletow"
};
const int menuItemsCount = MENU_ITEMS_COUNT;

// ============================================================================
// SYSTEM BEZPIECZEŃSTWA
// ============================================================================
bool emergencyStopActive = false;
bool emergencyStopReleased = false;

bool watchdogEnabled = true;
unsigned long lastWatchdogReset = 0;

unsigned long lastHeartbeat = 0;

unsigned long lastDeadmanConfirm = 0;
bool deadmanActive = false;

long lastEncoderPulses = 0;
unsigned long lastEncoderChange = 0;

float initialCalibration = DEFAULT_CALIBRATION;

SystemError errorLog[ERROR_LOG_SIZE];
int errorLogIndex = 0;
int errorCount = 0;

bool statusLedGreen = false;
bool statusLedRed = false;
bool statusLedYellow = false;

unsigned long buzzerStartTime = 0;
int buzzerBeepCount = 0;
bool buzzerActive = false;

bool selfTestPassed = false;
char selfTestMessage[256];

// ============================================================================
// TFT
// ============================================================================
unsigned long lastTFTUpdate = 0;
