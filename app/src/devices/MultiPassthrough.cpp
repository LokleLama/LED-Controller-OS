#include "devices/MultiPassthrough.h"
#include "Mainloop.h"
#include <iostream>

MultiPassthrough::MultiPassthrough(std::shared_ptr<ICommDevice> commDeviceMain, std::shared_ptr<ICommDevice> commDeviceA, std::shared_ptr<ICommDevice> commDeviceB, const std::string& name, int buffersize)
    : _commDeviceMain(commDeviceMain), 
      _commDeviceA(commDeviceA), 
      _commDeviceB(commDeviceB), 
      _name(name), 
      _junkSize(buffersize / 8),
      _fifoMainToBoth(buffersize),
      _fifoAtoMain(buffersize), 
      _fifoBtoMain(buffersize){
  if(_junkSize <= 0) {
    std::cerr << "Invalid junk size calculated for MultiPassthrough device: " << name << std::endl;
    _status = DeviceStatus::Error;
    return;
  }

  _commDeviceMain->registerDataReceivedCallback([this]() { return SignalTask(); });
  _commDeviceA->registerDataReceivedCallback([this]() { return SignalTask(); });
  _commDeviceB->registerDataReceivedCallback([this]() { return SignalTask(); });

  Mainloop::getInstance().registerRegularTask(getName() + ".Worker", [this]() { return ExecuteTask(); });
    
  _status = DeviceStatus::Initialized;
}

bool MultiPassthrough::SignalTask() {
  uint8_t buffer[_junkSize];
  bool dataInQueue = false;

  if(_commDeviceA->dataAvailable() > 0){
    if(_fifoAtoMain.count() < (_fifoAtoMain.capacity() - _junkSize)){
      int readBytes = _commDeviceA->receive(buffer, _junkSize);
      if(readBytes > 0) {
        _fifoAtoMain.writeAvailable(buffer, readBytes);
      }
    }else{
      dataInQueue = true;
    }
  }

  if(_commDeviceB->dataAvailable() > 0){
    if(_fifoBtoMain.count() < (_fifoBtoMain.capacity() - _junkSize)){
      int readBytes = _commDeviceB->receive(buffer, _junkSize);
      if(readBytes > 0) {
        _fifoBtoMain.writeAvailable(buffer, readBytes);
      }
    }else{
      dataInQueue = true;
    }
  }

  if(_commDeviceMain->dataAvailable() > 0){
    if(_fifoMainToBoth.count() < (_fifoMainToBoth.capacity() - _junkSize)){
      int readBytes = _commDeviceMain->receive(buffer, _junkSize);
      if(readBytes > 0) {
        _fifoMainToBoth.writeAvailable(buffer, readBytes);
      }
    }else{
      dataInQueue = true;
    }
  }

  return dataInQueue;
}

bool MultiPassthrough::ExecuteTask() {
  uint8_t buffer[_junkSize];
  if(_fifoAtoMain.count() > 0) {
    int bytesToSend = _fifoAtoMain.peekAvailable(buffer, _junkSize);
    if(bytesToSend > 0) {
      int writtenBytes = _commDeviceMain->send(buffer, bytesToSend);
      _fifoAtoMain.remove(writtenBytes);
    }
  }
  if(_fifoBtoMain.count() > 0) {
    int bytesToSend = _fifoBtoMain.peekAvailable(buffer, _junkSize);
    if(bytesToSend > 0) {
      int writtenBytes = _commDeviceMain->send(buffer, bytesToSend);
      _fifoBtoMain.remove(writtenBytes);
    }
  }
  if(_fifoMainToBoth.count() > 0) {
    int bytesToSend = _fifoMainToBoth.peekAvailable(buffer, _junkSize);
    if(bytesToSend > 0) {
      int n = 0;
      for(; n < bytesToSend; n++) {
        if(_commDeviceA->send(&buffer[n], 1) <= 0) break;
        if(_commDeviceB->send(&buffer[n], 1) <= 0) break;
      }
      _fifoMainToBoth.remove(n);
    }
  }
  return true;
}

const std::string MultiPassthrough::getDetails() const{
  std::string details = "MultiPassthrough Device for " + _commDeviceMain->getName() + " to " + _commDeviceA->getName() + " and " + _commDeviceB->getName();
  details += "Buffer Size: " + std::to_string(_fifoMainToBoth.capacity()) + ")";
  return details;
}