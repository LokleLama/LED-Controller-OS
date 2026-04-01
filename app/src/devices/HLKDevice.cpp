#include "devices/HLKDevice.h"
#include "devices/HLKStack/HLKDistance.h"
#include "VariableStore/VariableStore.h"

#include "Mainloop.h"

#include <iostream>

HLKDevice::HLKDevice(std::shared_ptr<ICommDevice> commDevice, 
                         std::string distance, 
                         std::string presence, 
                         const std::string& name) :
    _commDevice(commDevice), _distance(distance), _presence(presence), _name(name) {
  if(!_commDevice) {
    std::cerr << "Invalid communication device for HLKDevice: " << name << std::endl;
    _status = DeviceStatus::Error;
    return;
  }

  _commDevice->registerDataReceivedCallback([this]() { return ExecuteTask(); });

  _status = DeviceStatus::Initialized;
}

const std::string HLKDevice::getDetails() const {
  return "HLKDDevice over " + _commDevice->getName() + " with distance variable '" + _distance + "' and presence variable '" + _presence + "'";
}

bool HLKDevice::ExecuteTask() {
  uint8_t buffer[32];
  int bytesRead = _commDevice->receive(buffer, sizeof(buffer));
  if(bytesRead > 0) {
    auto distancePack = _finder.fastDistanceFinder(buffer, bytesRead);
    if(distancePack) {
      auto varStore = VariableStore::getInstance();
      varStore.setVariable(_distance, distancePack->getDistance());
      varStore.setBoolVariable(_presence, distancePack->getObjectDetected());
    }
  }
  return false;
}