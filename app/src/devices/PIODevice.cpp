#include "devices/PIODevice.h"
#include "hardware/dma.h"
#include "hardware/irq.h"

PIODevice::PIODevice(int number) : _number(number) {
    _program_offset = -1;

    if (number == 0) {
        _pio = pio0;
    } else if (number == 1) {
        _pio = pio1;
    } else {
        _status = DeviceStatus::Error;
        _sm = 0xFF;
        return;
    }
    // Claim a state machine
    _sm = pio_claim_unused_sm(_pio, true);
    if(_sm < 0) {
        _status = DeviceStatus::Error;
        return;
    }

    _status = DeviceStatus::Initialized;
}

bool PIODevice::addProgram(const pio_program_t *program) {
    if(_status != DeviceStatus::Assigned && _program_offset < 0) {
        return false;
    }
    _program_offset = pio_add_program(_pio, program);
    return _program_offset >= 0;
}

bool PIODevice::setProgramOffset(int offset) {
    if(_status != DeviceStatus::Assigned && _program_offset < 0) {
        return false;
    }
    _program_offset = offset;
    return true;
}

bool PIODevice::useDMA(uint transfer_count) {
    if(_status != DeviceStatus::Assigned) {
        return false;
    }
    // Claim a DMA channel
    _dma_channel = dma_claim_unused_channel(false);
    if(_dma_channel < 0) {
        return false;
    }
    dma_channel_config dc = dma_channel_get_default_config(_dma_channel);
    channel_config_set_transfer_data_size(&dc, DMA_SIZE_32);
    channel_config_set_read_increment(&dc, true);
    channel_config_set_write_increment(&dc, false);
    channel_config_set_dreq(&dc, pio_get_dreq(_pio, _sm, true));
    dma_channel_configure(_dma_channel, &dc, &_pio->txf[_sm], nullptr, transfer_count, false);

    _transfer_count = transfer_count;
    return true;
}

bool PIODevice::transfer(const uint32_t *data, size_t count) {
    if(_status != DeviceStatus::Assigned) {
        return false;
    }

    if((_dma_channel < 0) || (count != _transfer_count)) {
        for (size_t i = 0; i < count; ++i) {
            pio_sm_put_blocking(_pio, _sm, data[i]);
        }
    }else{
        if(dma_channel_is_busy(_dma_channel)) {
            return false; // DMA is busy
        }
        // Start the DMA transfer
        dma_channel_transfer_from_buffer_now(_dma_channel, data, _transfer_count);
    }
    return true;
}

const std::string PIODevice::getDetails() const {
    static std::string details;
    details = "PIO" + std::to_string(_number) + ".SM" + std::to_string(_sm) + "\n";
    details += "Program Offset: " + std::to_string(_program_offset) + "\n";
    if(_dma_channel < 0) {
        details += "DMA not used\n";
    } else {
        details += "DMA (CH " + std::to_string(_dma_channel);
        details += ") transfer size: " + std::to_string(_transfer_count) + "\n";
    }
    return details;
}