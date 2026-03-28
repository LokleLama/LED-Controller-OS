#include "devices/Passthrough.h"
#include "Mainloop.h"
#include <iostream>

Passthrough::Passthrough(std::shared_ptr<ICommDevice> commDeviceA, std::shared_ptr<ICommDevice> commDeviceB, const std::string& name, int buffersize)
    : _commDeviceA(commDeviceA), _commDeviceB(commDeviceB), _name(name), _buffersize(buffersize), _junkSize(buffersize / 8), _fifoAtoB(buffersize), _fifoBtoA(buffersize){
  if(_junkSize <= 0) {
    std::cerr << "Invalid junk size calculated for Passthrough device: " << name << std::endl;
    _status = DeviceStatus::Error;
    return;
  }

  _commDeviceA->registerDataReceivedCallback([this]() { return SignalTask(); });
  _commDeviceB->registerDataReceivedCallback([this]() { return SignalTask(); });

  Mainloop::getInstance().registerRegularTask(getName() + ".Worker", [this]() { return ExecuteTask(); });
    
  _status = DeviceStatus::Initialized;
}

bool Passthrough::SignalTask() {
  uint8_t buffer[_junkSize];
  bool dataInQueue = false;

  if(_commDeviceA->dataAvailable() > 0){
    if(_fifoAtoB.count() < (_buffersize - _junkSize)){
      int readBytes = _commDeviceA->receive(buffer, _junkSize);
      if(readBytes > 0) {
        _fifoAtoB.writeAvailable(buffer, readBytes);
      }
    }else{
      dataInQueue = true;
    }
  }

  if(_commDeviceB->dataAvailable() > 0){
    if(_fifoBtoA.count() < (_buffersize - _junkSize)){
      int readBytes = _commDeviceB->receive(buffer, _junkSize);
      if(readBytes > 0) {
        _fifoBtoA.writeAvailable(buffer, readBytes);
      }
    }else{
      dataInQueue = true;
    }
  }

  return dataInQueue;
}

bool Passthrough::ExecuteTask() {
  uint8_t buffer[_junkSize];
  if(_fifoAtoB.count() > 0) {
    int bytesToSend = _fifoAtoB.peekAvailable(buffer, _junkSize);
    if(bytesToSend > 0) {
      int writtenBytes = _commDeviceB->send(buffer, bytesToSend);
      _fifoAtoB.remove(writtenBytes);
    }
  }
  if(_fifoBtoA.count() > 0) {
    int bytesToSend = _fifoBtoA.peekAvailable(buffer, _junkSize);
    if(bytesToSend > 0) {
      int writtenBytes = _commDeviceA->send(buffer, bytesToSend);
      _fifoBtoA.remove(writtenBytes);
    }
  }
  return true;
}

const std::string Passthrough::getDetails() const{
  std::string details = "Passthrough Device for " + _commDeviceA->getName() + " to " + _commDeviceB->getName() + " (";
  details += "Buffer Size: " + std::to_string(_buffersize) + ")";
  return details;
}