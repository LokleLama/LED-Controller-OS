#pragma once

#include "devices/IDevice.h"
#include "devices/ICommDevice.h"
#include "Utils/IRQFifo.h"
#include "Utils/Signal.h"

#include <cstdint>
#include <string>
#include <map>
#include <memory>
#include <vector>

class PassthroughMonitor : public ICreateSharedFromThis<PassthroughMonitor>, public IDevice {
public:
  class MonitorDevice : public ICreateSharedFromThis<MonitorDevice>, public ICommDevice {
  public:
    MonitorDevice(PassthroughMonitor& parent, std::shared_ptr<ICommDevice> commDevice, const std::string& name, int number, int buffersize = 128);

    const std::string getName() const override { return _name; }
    const std::string getType() const override { return "MonitorDevice"; }
    const std::string getDetails() const override;

    int send(const uint8_t* data, size_t length) override {
      return _commDevice->send(data, length);
    }
    int dataAvailable() override {
        return _fifo.count();
    }
    int receive(uint8_t* buffer, size_t length) override{
        return _fifo.readAvailable(buffer, length);
    }

    bool registerDataReceivedCallback(Mainloop::Function func, Signal signal = 0) override;

    int getBufferSize() const override {
        return _fifo.capacity();
    }

  private:
    std::string _name;
    int _number;
    std::shared_ptr<ICommDevice> _commDevice;

    IRQFifo _fifo;
    Signal _irq_signal = 0;

    void receivedData(const uint8_t* data, size_t length);
  };

  PassthroughMonitor(std::shared_ptr<ICommDevice> commDeviceA, std::shared_ptr<ICommDevice> commDeviceB, const std::string& name = "PassthroughMonitor", int buffersize = 128);

  const std::string getName() const override { return _name; }
  const std::string getType() const override { return "PassthroughMonitor"; }
  const std::string getDetails() const override;

  std::shared_ptr<MonitorDevice> getMonitorA() const { return _monitorA; }
  std::shared_ptr<MonitorDevice> getMonitorB() const { return _monitorB; }

private:
  using ReceivedFunction = std::function<void(const uint8_t* data, size_t length)>;

  std::shared_ptr<ICommDevice> _commDeviceA;
  std::shared_ptr<ICommDevice> _commDeviceB;

  std::shared_ptr<MonitorDevice> _monitorA;
  std::shared_ptr<MonitorDevice> _monitorB;

  ReceivedFunction _receivedCallbackA;
  ReceivedFunction _receivedCallbackB;

  std::string _name;
  int _junkSize;

  IRQFifo _fifoAtoB;
  IRQFifo _fifoBtoA;

  bool ExecuteTask();
  bool SignalTask();
};