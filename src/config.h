/**
 * @file config.h
 * @brief Konfiguracja systemu Trassar-Painter v6.0.0
 *
 * Parametry systemowe, WiFi, timing, bezpieczeństwo.
 * Piny GPIO -> patrz pins.h (jedyne źródło prawdy)
 *
 * @author Trassar251
 * @date 2026-02-02
 */

#ifndef CONFIG_H
#define CONFIG_H

// ============================================================================
// WERSJA SYSTEMU
// ============================================================================
#define FIRMWARE_VERSION    "6.0.0"
#define FIRMWARE_NAME       "Trassar-Painter"
#define FIRMWARE_SUBTITLE   "PRODUCTION MODULAR EDITION"

// ============================================================================
// KONFIGURACJA WiFi
// ============================================================================

// WiFi STA (klient - do synchronizacji NTP)
// WAŻNE: Wpisz dane swojej sieci WiFi
#define WIFI_STA_SSID       "Miastek_WiFi"
#define WIFI_STA_PASSWORD   "12345678"

// WiFi AP (Access Point - panel WWW zawsze dostępny pod 192.168.4.1)
#define WIFI_AP_SSID        "Trassar-Painter"
#define WIFI_AP_PASSWORD    "12345678"

// OTA (Over-The-Air Updates)
#define OTA_HOSTNAME        "Trassar-Painter"
#define OTA_PASSWORD        "trassar2024"

// ============================================================================
// PARAMETRY CZASOWE (ms)
// ============================================================================
#define BUTTON_DEBOUNCE_MS      200     // Debounce przycisków
#define TFT_UPDATE_INTERVAL_MS  500     // Odświeżanie wyświetlacza TFT
#define SPEED_CALC_INTERVAL_MS  1000    // Kalkulacja prędkości
#define NTP_RETRY_DELAY_MS      500     // Opóźnienie między próbami NTP
#define WIFI_CONNECT_TIMEOUT_MS 10000   // Timeout połączenia WiFi STA
#define STATUS_REFRESH_MS       500     // Auto-refresh panelu WWW

// ============================================================================
// PARAMETRY NTP
// ============================================================================
#define NTP_MAX_RETRIES         10      // Maksymalna liczba prób synchronizacji
#define NTP_SERVER_PRIMARY      "pool.ntp.org"
#define NTP_SERVER_SECONDARY    "time.nist.gov"
#define NTP_UTC_OFFSET          3600    // UTC+1 (Polska zima)
#define NTP_DST_OFFSET          3600    // +1h dla czasu letniego
#define NTP_UPDATE_INTERVAL     60000   // Interwał aktualizacji NTP (60s)

// ============================================================================
// PARAMETRY KALIBRACJI
// ============================================================================
#define CALIBRATION_DISTANCE_M  10.0f   // Dystans kalibracji (10 metrów)
#define DEFAULT_CALIBRATION     100.0f  // Domyślna kalibracja (imp/m)
#define NVS_NAMESPACE           "trassar"
#define NVS_KEY_CALIBRATION     "encCalib"

// ============================================================================
// PARAMETRY BEZPIECZEŃSTWA (SAFETY)
// ============================================================================
#define WATCHDOG_TIMEOUT_MS     5000    // 5s - timeout watchdog
#define HEARTBEAT_INTERVAL_MS   1000    // 1s - wymagany heartbeat
#define DEADMAN_TIMEOUT_MS      30000   // 30s - timeout potwierdzenia operatora
#define ENCODER_STALL_TIMEOUT_MS 5000   // 5s - timeout zatrzymania enkodera
#define ENCODER_DISCONNECT_MS   10000   // 10s - timeout odłączenia enkodera

// Limity prędkości (km/h)
#define SPEED_MIN_KMH           0.0f    // Minimalna prędkość
#define SPEED_MAX_KMH           25.0f   // Maksymalna realistyczna prędkość

// Kalibracja drift
#define CALIBRATION_DRIFT_MAX_PERCENT 20.0f  // Maksymalny drift kalibracji

// Error logging
#define ERROR_LOG_SIZE          50      // Rozmiar circular buffer błędów
#define ERROR_LOG_FILE          "/errors.log"

// LittleFS
#define MIN_FREE_SPACE_BYTES    1024    // Minimum wolnego miejsca (1KB)
#define SELFTEST_MIN_FREE_KB    10      // Minimum 10KB dla self-test

// ============================================================================
// WYŚWIETLACZ TFT (ILI9341)
// ============================================================================
#define TFT_SCREEN_WIDTH    320
#define TFT_SCREEN_HEIGHT   240

// ============================================================================
// WZORCE - LIMITY
// ============================================================================
#define PATTERN_COUNT       15  // Liczba wzorców
#define GUN_COUNT           6   // Liczba pistoletów

// ============================================================================
// JOYSTICK - PROGI
// ============================================================================
#define JOY_CENTER          2048    // Wartość środkowa ADC
#define JOY_THRESHOLD_LOW   1000    // Próg dolny (góra/lewo)
#define JOY_THRESHOLD_HIGH  3000    // Próg górny (dół/prawo)
#define JOY_MOVE_DELAY_MS   300     // Opóźnienie między ruchami

// ============================================================================
// MENU
// ============================================================================
#define MENU_ITEMS_COUNT    7

// ============================================================================
// BUZZER
// ============================================================================
#define BUZZER_CYCLE_MS     200     // Cykl beep on/off (ms)

// ============================================================================
// SAFETY CHECK INTERVALS (ms)
// ============================================================================
#define SAFETY_CHECK_INTERVAL_MS    1000    // Co 1s - heartbeat, deadman, encoder
#define CALIB_CHECK_INTERVAL_MS     60000   // Co 60s - drift kalibracji

#endif // CONFIG_H
