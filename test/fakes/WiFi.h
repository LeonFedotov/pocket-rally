#pragma once
constexpr int WIFI_STA=1;
inline struct FakeWiFi {void mode(int){}void disconnect(){}} WiFi;
