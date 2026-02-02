/**
 * @file config.h
 * @brief Konfiguracja systemu - Trassar-Painter v7.0.0
 *
 * ZMIANY v7.0.0:
 * - MIN_PAINTING_SPEED_KMH = 3.0 (malowanie tylko > 3 km/h)
 * - SD_REPORTS_DIR, WEB_AUTH_USER/PASS
 *
 * @author Trassar251
 * @date 2026-02-02
 */

#ifndef CONFIG_H
#define CONFIG_H

// ============================================================================
// WERSJA FIRMWARE
// ============================================================================
#define FIRMWARE_VERSION    "7.0.0"
#define FIRMWARE_CODENAME   "SD-SPEED EDITION"

// ============================================================================
// WIFI KONFIGURACJA
// ============================================================================
#define WIFI_AP_SSID        "Trassar-Painter"
#define WIFI_AP_PASS        "Trassar2025!"
#define WIFI_STA_SSID       "Miastek_WiFi"
#define WIFI_STA_PASS       "12345678"
#define WEB_SERVER_PORT     80

// ============================================================================
// AUTENTYKACJA PANELU WWW (NOWE v7.0.0)
// ============================================================================
#define WEB_AUTH_USER       "admin"
#define WEB_AUTH_PASS       "trassar251"

// ============================================================================
// PRĘDKOŚĆ MALOWANIA (NOWE v7.0.0)
// ============================================================================
// Malowanie możliwe TYLKO powyżej tej prędkości (bezpieczeństwo)
#define MIN_PAINTING_SPEED_KMH  3.0f

// ============================================================================
// PARAMETRY ENKODERA I POMIARÓW
// ============================================================================
#define DEFAULT_CALIBRATION     100.0f
#define CALIBRATION_DISTANCE_M  10.0f
#define SPEED_CALC_INTERVAL_MS  1000
#define SPEED_MIN_KMH           0.0f
#define SPEED_MAX_KMH           25.0f

// ============================================================================
// NVS (Preferences) - klucze
// ============================================================================
#define NVS_NAMESPACE           "trassar"
#define NVS_KEY_CALIBRATION     "enc_calib"

// ============================================================================
// JOYSTICK
// ============================================================================
#define JOY_CENTER          2048
#define JOY_DEADZONE        500
#define JOY_MENU_DELAY_MS   300

// ============================================================================
// DEBOUNCE
// ============================================================================
#define BUTTON_DEBOUNCE_MS  250
#define BUZZER_CYCLE_MS     150

// ============================================================================
// SYSTEM BEZPIECZEŃSTWA
// ============================================================================
#define WATCHDOG_TIMEOUT_MS         5000
#define HEARTBEAT_INTERVAL_MS       1000
#define DEADMAN_TIMEOUT_MS          30000
#define ENCODER_STALL_TIMEOUT_MS    10000
#define ENCODER_DISCONNECT_MS       5000
#define CALIBRATION_DRIFT_MAX_PERCENT 15.0f
#define SELFTEST_MIN_FREE_KB        100
#define ERROR_LOG_SIZE              50
#define ERROR_LOG_FILE              "/error_log.txt"
#define MIN_FREE_SPACE_BYTES        4096

// ============================================================================
// KARTA SD (NOWE v7.0.0)
// ============================================================================
#define SD_REPORTS_DIR      "/raporty"
#define SD_LOGS_DIR         "/logi"

// ============================================================================
// TFT WYŚWIETLACZ
// ============================================================================
#define TFT_UPDATE_INTERVAL_MS  500
#define TFT_SCREEN_WIDTH        320
#define TFT_SCREEN_HEIGHT       240

// ============================================================================
// WZORCE MALOWANIA
// ============================================================================
#define PATTERN_COUNT       15
#define GUN_COUNT           6

// ============================================================================
// MENU
// ============================================================================
#define MENU_ITEMS_COUNT    7

// ============================================================================
// TIMERY GŁÓWNE
// ============================================================================
#define SAFETY_CHECK_INTERVAL_MS    500
#define SENSOR_READ_INTERVAL_MS     50
#define STATUS_PRINT_INTERVAL_MS    2000
#define SERIAL_BAUD                 115200

// ============================================================================
// NTP
// ============================================================================
#define NTP_SERVER          "pool.ntp.org"
#define NTP_GMT_OFFSET      3600
#define NTP_DAYLIGHT_OFFSET 3600

// ============================================================================
// OTA
// ============================================================================
#define OTA_HOSTNAME        "trassar-painter"
#define OTA_PASSWORD        "trassar251"

#endif // CONFIG_H
