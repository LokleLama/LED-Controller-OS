#pragma once

#include "IDataStream.h"
#include <cstdint>

class IRQFifo : public IDataStream {
public:
  IRQFifo(uint32_t size) {
    _bufferSize = size;
    _buffer = new uint8_t[size];
    _head = _buffer;
    _tail = _buffer;
    _end = _buffer + size;
  }

  void push(uint8_t byte) {
    if (isFull()) {
      return; // Buffer is full, cannot push
    }
    *_head = byte;
    _head++;
    if (_head == _end) {
      _head = _buffer; // Wrap around
    }
  }

  int readAvailable(uint8_t *buffer, uint32_t size) {
    int available = count();
    if (available == 0) {
      return 0;
    }
    if (available > size) {
      available = size;
    }
    for (int i = 0; i < available; i++) {
      buffer[i] = *_tail;
      _tail++;
      if (_tail == _end) {
        _tail = _buffer; // Wrap around
      }
    }
    return available;
  }

  int writeAvailable(const uint8_t *buffer, uint32_t size) override {
    return -1;
  }

  bool isEmpty() const { return count() == 0; }
  bool isFull() const { return count() == _bufferSize; }
  uint32_t size() const { return _bufferSize; }
  uint32_t count() const {
    if (_head >= _tail) {
      return _head - _tail;
    } else {
      return (_bufferSize - (_tail - _head));
    }
  }

private:
  uint8_t *_buffer;
  uint32_t _bufferSize;

  uint8_t *_end;

  volatile uint8_t *_head;
  uint8_t *_tail;
};