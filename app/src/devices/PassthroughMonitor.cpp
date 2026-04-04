#include "devices/PassthroughMonitor.h"
#include "Mainloop.h"
#include <iostream>

PassthroughMonitor::PassthroughMonitor(std::shared_ptr<ICommDevice> commDeviceA, std::shared_ptr<ICommDevice> commDeviceB, const std::string& name, int buffersize)
    : _commDeviceA(commDeviceA), _commDeviceB(commDeviceB), _name(name), _junkSize(buffersize / 8), _fifoAtoB(buffersize), _fifoBtoA(buffersize){
  if(_junkSize <= 0) {
    std::cerr << "Invalid junk size calculated for PassthroughMonitor device: " << name << std::endl;
    _status = DeviceStatus::Error;
    return;
  }

  _commDeviceA->registerDataReceivedCallback([this]() { return SignalTask(); });
  _commDeviceB->registerDataReceivedCallback([this]() { return SignalTask(); });

  _monitorA = std::make_shared<MonitorDevice>(*this, commDeviceA, name + "." + commDeviceA->getName(), 0, buffersize);
  _monitorB = std::make_shared<MonitorDevice>(*this, commDeviceB, name + "." + commDeviceB->getName(), 1, buffersize);

  Mainloop::getInstance().registerRegularTask(getName() + ".Worker", [this]() { return ExecuteTask(); });

  _status = DeviceStatus::Initialized;
}

PassthroughMonitor::MonitorDevice::MonitorDevice(PassthroughMonitor& parent, std::shared_ptr<ICommDevice> commDevice, const std::string& name, int number, int buffersize)
    : _name(name), _number(number), _commDevice(commDevice), _fifo(buffersize) {
  if(number == 0) {
    parent._receivedCallbackA = [this](const uint8_t* data, size_t length) { receivedData(data, length); };
  } else {
    parent._receivedCallbackB = [this](const uint8_t* data, size_t length) { receivedData(data, length); };
  }
  _status = DeviceStatus::Initialized;
}

bool PassthroughMonitor::SignalTask() {
  uint8_t buffer[_junkSize];
  bool dataInQueue = false;

  if(_commDeviceA->dataAvailable() > 0){
    if(_fifoAtoB.count() < (_fifoAtoB.capacity() - _junkSize)){
      int readBytes = _commDeviceA->receive(buffer, _junkSize);
      if(readBytes > 0) {
        _fifoAtoB.writeAvailable(buffer, readBytes);
        _receivedCallbackA(buffer, readBytes);
      }
    }else{
      dataInQueue = true;
    }
  }

  if(_commDeviceB->dataAvailable() > 0){
    if(_fifoBtoA.count() < (_fifoBtoA.capacity() - _junkSize)){
      int readBytes = _commDeviceB->receive(buffer, _junkSize);
      if(readBytes > 0) {
        _fifoBtoA.writeAvailable(buffer, readBytes);
        _receivedCallbackB(buffer, readBytes);
      }
    }else{
      dataInQueue = true;
    }
  }

  return dataInQueue;
}

void PassthroughMonitor::MonitorDevice::receivedData(const uint8_t* data, size_t length) {
  if(length > 0) {
    _fifo.writeAvailable(data, length);
    if(_irq_signal != 0) {
      Mainloop::getInstance().triggerSignal(_irq_signal);
    }
  }
}

bool PassthroughMonitor::MonitorDevice::registerDataReceivedCallback(Mainloop::Function func, uint32_t signal)  {
  if(_irq_signal != 0) {
    return false;
  }
  if(signal == 0) {
    signal = 0x6D6F6E30 + _number;
  }
  _irq_signal = signal;
  Mainloop::getInstance().registerSignalTask(getName() + ".DataReceived", func, _irq_signal);
  return true;
}

bool PassthroughMonitor::ExecuteTask() {
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

const std::string PassthroughMonitor::getDetails() const{
  std::string details = "PassthroughMonitor Device for " + _commDeviceA->getName() + " to " + _commDeviceB->getName() + " (";
  details += "Buffer Size: " + std::to_string(_fifoAtoB.capacity()) + ")";
  return details;
}

const std::string PassthroughMonitor::MonitorDevice::getDetails() const {
  return "Monitor Device (" + _name + ") (Buffer Size: " + std::to_string(_fifo.capacity()) + ")";
}