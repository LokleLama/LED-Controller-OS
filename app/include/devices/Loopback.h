#pragma once

#include "devices/IDevice.h"
#include "devices/ICommDevice.h"
#include "Utils/IRQFifo.h"

#include <cstdint>
#include <string>
#include <map>
#include <memory>
#include <vector>

class Loopback : public ICreateSharedFromThis<Loopback>, public IDevice {
public:
  Loopback(std::shared_ptr<ICommDevice> commDevice, const std::string& name = "Loopback");
  Loopback(std::shared_ptr<ICommDevice> commDevice, const std::string& name, int buffersize);

  const std::string getName() const override { return _name; }
  const std::string getType() const override { return "Loopback"; }
  const std::string getDetails() const override;

private:
    std::shared_ptr<ICommDevice> _commDevice;
    std::string _name;
    IRQFifo _fifo;

    bool ExecuteTask();
};