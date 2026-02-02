/**
 * @file web_panel.h
 * @brief Panel WWW i REST API - Trassar-Painter v7.0.0
 *
 * v7.0.0: Basic Auth, CORS, delete reports, SD card info
 *
 * @author Trassar251
 * @date 2026-02-02
 */

#ifndef WEB_PANEL_H
#define WEB_PANEL_H

#include <Arduino.h>

void setupWebServer();
String getStatusJSON();
String getHTMLPage();

#endif // WEB_PANEL_H
