#pragma once

#include "hardware/uart.h"
#include "hardware/dma.h"
#include "devices/ICommDevice.h"

#include "Utils/IRQFifo.h"
#include "Utils/Signal.h"

#include <vector>
#include <memory>
#include <string>
#include <cstdint>

class UARTDevice : public ICreateSharedFromThis<UARTDevice>, public ICommDevice {
public:
    UARTDevice(int uart_number, uint8_t tx_pin, uint8_t rx_pin, uint baud_rate = 115200, int buffersize = 32);

    const std::string getName() const override { return "UART" + std::to_string(_uart_number); }
    const std::string getType() const override { return "UART"; }
    const std::string getDetails() const override;

    bool setBaudRate(uint baudRate);
    uint getBaudRate() const { return _baud_rate; }
    bool setFormat(int bits, int stopBits, int parity);

    int send(const uint8_t* data, size_t length) override;
    int dataAvailable() override {
        return _rx_fifo.count();
    }
    int receive(uint8_t* buffer, size_t length) override{
        return _rx_fifo.readAvailable(buffer, length);
    }

    bool registerDataReceivedCallback(Mainloop::Function func, Signal signal = 0) override;

    int getBufferSize() const override {
        return _rx_fifo.capacity();
    }

private:
    int _uart_number;
    uart_inst_t* _uart;
    uint8_t _tx_pin;
    uint8_t _rx_pin;
    uint _baud_rate;
    IRQFifo _rx_fifo;

    Signal _irq_signal = 0;

    static UARTDevice* _instances[2];
    static void on_uart0_rx();
    static void on_uart1_rx();

    void handleIRQ();

    bool isPinValid(uint8_t pin, int pinType) const;

    #define UART_PIN_COUNT 4
    #define UART_PIN_TX 0
    #define UART_PIN_RX 1
    #define UART_PIN_CTS 2
    #define UART_PIN_RTS 3

    bool isTXPinValid(uint8_t pin) const {return isPinValid(pin, UART_PIN_TX);}
    bool isRXPinValid(uint8_t pin) const {return isPinValid(pin, UART_PIN_RX);}
    bool isCTSPinValid(uint8_t pin) const {return isPinValid(pin, UART_PIN_CTS);}
    bool isRTSPinValid(uint8_t pin) const {return isPinValid(pin, UART_PIN_RTS);}

    static constexpr int8_t _uart0_pins[16] = {0, 1, 2, 3, 
                                               12, 13, 14, 15, 
                                               16, 17, 18, 19, 
                                               28, 29, -1, -1}; // -1 for invalid pins
    static constexpr int8_t _uart1_pins[16] = {4, 5, 6, 7, 
                                               8, 9, 10, 11, 
                                               20, 21, 22, 23, 
                                               24, 25, 26, 27};
};
