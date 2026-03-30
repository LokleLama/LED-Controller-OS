#include "HLKStack/HLKPackageFinder.h"
#include "HLKStack/HLKCommand.h"
#include "HLKStack/HLKDistance.h"
#include "HLKStack/HLKStandart.h"
#include "HLKStack/IHLKPackage.h"
#include <iomanip>
#include <iostream>
#include <memory>
#include <string>

std::shared_ptr<IHLKPackage>
HLKPackageFinder::findPackage(const uint8_t *buffer, const int size) {
  if (size <= 0)
    return nullptr;

  for (int i = 0; i < size; ++i) {
    _pattern = (_pattern << 8) | buffer[i];
    switch (_state) {
    case State::WaitingForStart:
      if (buffer[i] == IHLKPackage::MinimalFrameHead) {
        _buffer.clear();
        _buffer.push_back(buffer[i]);
        _state = State::WaitingForMinimal;
      } else if (_pattern == IHLKPackage::StandartFrameHead) {
        _buffer.clear();
        _buffer.push_back(
            static_cast<uint8_t>(IHLKPackage::StandartFrameHead >> 24));
        _buffer.push_back(
            static_cast<uint8_t>(IHLKPackage::StandartFrameHead >> 16));
        _buffer.push_back(
            static_cast<uint8_t>(IHLKPackage::StandartFrameHead >> 8));
        _buffer.push_back(static_cast<uint8_t>(IHLKPackage::StandartFrameHead));
        _state = State::WaitingForStandart;
      } else if (_pattern == IHLKPackage::CommandFrameHead) {
        _buffer.clear();
        _buffer.push_back(
            static_cast<uint8_t>(IHLKPackage::CommandFrameHead >> 24));
        _buffer.push_back(
            static_cast<uint8_t>(IHLKPackage::CommandFrameHead >> 16));
        _buffer.push_back(
            static_cast<uint8_t>(IHLKPackage::CommandFrameHead >> 8));
        _buffer.push_back(static_cast<uint8_t>(IHLKPackage::CommandFrameHead));
        _state = State::WaitingForCommand;
      }
      break;

    case State::WaitingForMinimal:
      _buffer.push_back(buffer[i]);
      if (_buffer.size() >= 5 || buffer[i] == IHLKPackage::MinimalFrameTail) {
        _state = State::WaitingForStart;
        return HLKDistance::deserialize(_buffer.data(), _buffer.size());
      }
      break;

    case State::WaitingForStandart:
      _buffer.push_back(buffer[i]);
      if (_pattern == IHLKPackage::StandartFrameTail || _buffer.size() >= 250) {
        _state = State::WaitingForStart;
        return HLKStandart::deserialize(_buffer.data(), _buffer.size());
      }
      break;

    case State::WaitingForCommand:
      _buffer.push_back(buffer[i]);
      if (_pattern == IHLKPackage::CommandFrameTail || _buffer.size() >= 250) {
        _state = State::WaitingForStart;
        return HLKCommand::deserialize(_buffer.data(), _buffer.size());
      }
      break;

    default:
      std::cerr << "Unknown state: " << static_cast<int>(_state) << std::endl;
      _state = State::WaitingForStart;
      break;
    }
  }

  return nullptr;
}

std::shared_ptr<HLKDistance>
HLKPackageFinder::fastDistanceFinder(const uint8_t *buffer, const int size) {
  if (size <= 0)
    return nullptr;

  for (int i = 0; i < size; ++i) {
    _pattern = (_pattern << 8) | buffer[i];
    switch (_state) {
    case State::WaitingForStart:
      if (buffer[i] == IHLKPackage::MinimalFrameHead) {
        _buffer.clear();
        _buffer.push_back(buffer[i]);
        _state = State::WaitingForMinimal;
      }
      break;

    case State::WaitingForMinimal:
      _buffer.push_back(buffer[i]);
      if (_buffer.size() >= 5 || buffer[i] == IHLKPackage::MinimalFrameTail) {
        _state = State::WaitingForStart;
        return HLKDistance::deserialize(_buffer.data(), _buffer.size());
      }
      break;

    default:
      std::cerr << "Unknown state: " << static_cast<int>(_state) << std::endl;
      _state = State::WaitingForStart;
      break;
    }
  }

  return nullptr;
}