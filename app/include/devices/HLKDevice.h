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
            std::shared_ptr<IVariable> distance, 
            std::shared_ptr<IVariable> presence, 
            const std::string& name = "HLKDevice");

  const std::string getName() const override { return _name; }
  const std::string getType() const override { return "HLKDevice"; }
  const std::string getDetails() const override;

private:
  std::shared_ptr<ICommDevice> _commDevice;

  std::shared_ptr<IVariable> _distance;
  std::shared_ptr<IVariable> _presence;

  std::string _name;

  HLKPackageFinder _finder;

  bool ExecuteTask();
};