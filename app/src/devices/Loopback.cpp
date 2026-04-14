#include "devices/Loopback.h"
#include <iostream>

Loopback::Loopback(std::shared_ptr<ICommDevice> commDevice, const std::string& name)
    : Loopback(commDevice, name, 4 * commDevice->getBufferSize()) { }

Loopback::Loopback(std::shared_ptr<ICommDevice> commDevice, const std::string& name, int buffersize)
    : _commDevice(commDevice), _name(name), _fifo(buffersize) {
  _commDevice->registerDataReceivedCallback([this](TaskPID) { return ExecuteTask(); });
    
  _status = DeviceStatus::Initialized;
}

bool Loopback::ExecuteTask() {
  uint8_t buffer[_commDevice->getBufferSize()];

  if(_commDevice->dataAvailable() > 0 && _fifo.count() < (_fifo.capacity() - sizeof(buffer))) {
    int readBytes = _commDevice->receive(buffer, sizeof(buffer));
    _fifo.writeAvailable(buffer, readBytes);
  }

  if(_fifo.count() > 0) {
    int bytesToSend = _fifo.peekAvailable(buffer, sizeof(buffer));
    int writtenBytes = _commDevice->send(buffer, bytesToSend);
    _fifo.remove(writtenBytes);
  }
  
  return _fifo.count() > 0 || _commDevice->dataAvailable() > 0;
}

const std::string Loopback::getDetails() const{
  std::string details = "Loopback Device for " + _commDevice->getName() + " (";
  details += "Buffer Size: " + std::to_string(_fifo.capacity()) + ")";
  return details;
}