#include "devices/Loopback.h"
#include <iostream>

Loopback::Loopback(std::shared_ptr<ICommDevice> commDevice, const std::string& name, int buffersize)
    : _commDevice(commDevice), _name(name), _buffersize(buffersize) {
  _commDevice->registerDataReceivedCallback([this]() { return ExecuteTask(); });
    
  _status = DeviceStatus::Initialized;
}

bool Loopback::ExecuteTask() {
  std::vector<uint8_t> buffer(_buffersize);
  while(_commDevice->dataAvailable() > 0){
    int readBytes = _commDevice->receive(buffer.data(), _buffersize);
    if(readBytes > 0) {
      _commDevice->send(buffer.data(), readBytes);
    }
  };
  return true;
}

const std::string Loopback::getDetails() const{
  std::string details = "Loopback Device for " + _commDevice->getName() + " (";
  details += "Buffer Size: " + std::to_string(_buffersize) + ")";
  return details;
}