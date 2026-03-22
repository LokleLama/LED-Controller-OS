#pragma once

#include "hardware/uart.h"
#include "hardware/dma.h"
#include "devices/ICommDevice.h"
#include "ITask.h"

#include <vector>
#include <memory>
#include <string>
#include <cstdint>

class USBUARTDevice : public ICreateSharedFromThis<USBUARTDevice>, public ICommDevice {
public:
    USBUARTDevice(int interface_number);

    const std::string getName() const override { return "USBUART"; }
    const std::string getType() const override { return "USBUART"; }
    const std::string getDetails() const override;

    int send(const uint8_t* data, size_t length) override;
    int dataAvailable() override;
    int receive(uint8_t* buffer, size_t length) override;

    bool registerDataReceivedCallback(Mainloop::Function func, uint32_t signal = 0) override;

private:
    int _interface_number;
    uint32_t _irq_signal = 0;

    bool ExecuteTask();
};
