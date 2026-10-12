#pragma once
inline uint32_t esp_random(){static uint32_t value=100;return ++value;}
