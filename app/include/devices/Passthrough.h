#pragma once

#include "devices/IDevice.h"
#include "devices/ICommDevice.h"
#include "Utils/IRQFifo.h"

#include <cstdint>
#include <string>
#include <map>
#include <memory>
#include <vector>

class Passthrough : public ICreateSharedFromThis<Passthrough>, public IDevice {
public:
  Passthrough(std::shared_ptr<ICommDevice> commDeviceA, std::shared_ptr<ICommDevice> commDeviceB, const std::string& name = "Passthrough", int buffersize = 128);

  const std::string getName() const override { return _name; }
  const std::string getType() const override { return "Passthrough"; }
  const std::string getDetails() const override;

private:
    std::shared_ptr<ICommDevice> _commDeviceA;
    std::shared_ptr<ICommDevice> _commDeviceB;
    std::string _name;
    int _junkSize;

    IRQFifo _fifoAtoB;
    IRQFifo _fifoBtoA;

    bool ExecuteTask();
    bool SignalTask();
};