#pragma once

#include "devices/IDevice.h"
#include "devices/ICommDevice.h"
#include "VariableStore/IVariable.h"
#include "devices/HLKStack/HLKPackageFinder.h"

#include <cstdint>
#include <vector>
#include <memory>
#include <string>

class HLKDevice : public ICreateSharedFromThis<HLKDevice>, public IDevice {
public:
  HLKDevice(std::shared_ptr<ICommDevice> commDevice, 
            std::string distance, 
            std::string presence, 
            const std::string& name = "HLKDevice");

  const std::string getName() const override { return _name; }
  const std::string getType() const override { return "HLKDevice"; }
  const std::string getDetails() const override;

private:
  std::shared_ptr<ICommDevice> _commDevice;

  std::string _distance;
  std::string _presence;

  std::string _name;

  HLKPackageFinder _finder;

  bool ExecuteTask();
};