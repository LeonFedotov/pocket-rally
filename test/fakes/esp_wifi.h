#pragma once
constexpr int ESP_OK=0,WIFI_PS_NONE=0,WIFI_SECOND_CHAN_NONE=0,WIFI_IF_STA=0;
inline int esp_wifi_set_ps(int){return ESP_OK;}
inline int esp_wifi_set_channel(int,int){return ESP_OK;}
