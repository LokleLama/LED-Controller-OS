#pragma once

#include "hardware/pio.h"
#include "hardware/dma.h"
#include "devices/IDevice.h"

#include <vector>
#include <memory>
class PIODevice : public IDevice, public std::enable_shared_from_this<PIODevice> {
public:
    PIODevice(int number);

    const std::string getName() const override { return "PIO" + std::to_string(_number) + ".SM" + std::to_string(_sm); }
    const std::string getType() const override { return "PIO"; }
    const std::string getDetails() const override;

    bool addProgram(const pio_program_t *program);
    int getProgramOffset() const { return _program_offset; }
    bool setProgramOffset(int offset);

    bool useDMA32(uint transfer_count);
    bool useDMA16(uint transfer_count);
    bool useDMA8(uint transfer_count);

    // WARNING: when using DMA, make sure the data buffer remains valid until the transfer is complete
    // DMA transfer is not checked for completion in this method
    bool transfer(const std::vector<uint32_t> &data) { return transfer(data.data(), data.size()); }
    bool transfer(const uint32_t *data, size_t count);

    PIO getPIO() const { return _pio; }
    int getPIONumber() const { return _number; }
    int getSM() const { return _sm; }

    std::shared_ptr<PIODevice> getShared() {
        return shared_from_this();
    }
    
private:
    int _number;
    int _sm;
    PIO _pio{nullptr};

    int _program_offset;
    int _dma_channel;
    uint _transfer_count;

    bool useDMA(uint transfer_count, dma_channel_transfer_size_t size);
};