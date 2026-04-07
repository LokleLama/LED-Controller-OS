#pragma once

#include "devices/IDevice.h"
#include "devices/ICommDevice.h"
#include "Utils/IRQFifo.h"

#include <cstdint>
#include <string>
#include <map>
#include <memory>
#include <vector>

class MultiPassthrough : public ICreateSharedFromThis<MultiPassthrough>, public IDevice {
public:
  MultiPassthrough(std::shared_ptr<ICommDevice> commDeviceMain, std::shared_ptr<ICommDevice> commDeviceA, std::shared_ptr<ICommDevice> commDeviceB, const std::string& name = "MultiPassthrough", int buffersize = 128);

  const std::string getName() const override { return _name; }
  const std::string getType() const override { return "MultiPassthrough"; }
  const std::string getDetails() const override;

private:
    std::shared_ptr<ICommDevice> _commDeviceMain;
    std::shared_ptr<ICommDevice> _commDeviceA;
    std::shared_ptr<ICommDevice> _commDeviceB;
    std::string _name;
    int _junkSize;

    IRQFifo _fifoAtoMain;
    IRQFifo _fifoBtoMain;
    IRQFifo _fifoMainToBoth;

    bool ExecuteTask();
    bool SignalTask();
};