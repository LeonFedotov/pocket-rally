#pragma once
#include <cstdint>
#include <cstring>
#include <deque>
#include <vector>
inline uint32_t fakeNow=1000;
inline uint32_t millis(){return fakeNow;}
struct FakeQueue {size_t itemSize;std::deque<std::vector<uint8_t>> items;};
using QueueHandle_t=FakeQueue*;
constexpr int pdTRUE=1;
inline QueueHandle_t xQueueCreate(int,size_t size){return new FakeQueue{size,{}};}
inline int xQueueSend(QueueHandle_t q,const void *p,int){auto b=static_cast<const uint8_t*>(p);q->items.emplace_back(b,b+q->itemSize);return pdTRUE;}
inline int xQueueReceive(QueueHandle_t q,void *p,int){if(q->items.empty())return 0;std::memcpy(p,q->items.front().data(),q->itemSize);q->items.pop_front();return pdTRUE;}
inline void xQueueReset(QueueHandle_t q){q->items.clear();}
