#include "devices/UARTDevice.h"
#include "hardware/irq.h"
#include "hardware/uart.h"
#include "hardware/gpio.h"
#include <iostream>

UARTDevice* UARTDevice::_instances[2] = {nullptr, nullptr};

UARTDevice::UARTDevice(int uart_number, uint8_t tx_pin, uint8_t rx_pin, uint baud_rate, int buffersize) :
    _uart_number(uart_number), _tx_pin(tx_pin), _rx_pin(rx_pin), _baud_rate(baud_rate), _rx_fifo(buffersize)
{
    if (_uart_number >= 2 || _instances[_uart_number] != nullptr) {
        _status = DeviceStatus::Error;
        return;
    }
    _uart = (_uart_number == 0) ? uart0 : uart1;

    if (!isTXPinValid(_tx_pin)) {
        std::cerr << "Invalid TX pin " << static_cast<int>(_tx_pin) << " for UART" << _uart_number << ", possible pins are: ";
        for (int i = UART_PIN_TX; i < 16; i += UART_PIN_COUNT) {
            std::cerr << static_cast<int>((_uart_number == 0 ? _uart0_pins : _uart1_pins)[i]) << " ";
        }
        std::cerr << std::endl;
        _status = DeviceStatus::Error;
        return;
    }
    if (!isRXPinValid(_rx_pin)) {
        std::cerr << "Invalid RX pin " << static_cast<int>(_rx_pin) << " for UART" << _uart_number << ", possible pins are: ";
        for (int i = UART_PIN_RX; i < 16; i += UART_PIN_COUNT) {
            std::cerr << static_cast<int>((_uart_number == 0 ? _uart0_pins : _uart1_pins)[i]) << " ";
        }
        std::cerr << std::endl;
        _status = DeviceStatus::Error;
        return;
    }

    gpio_init(_tx_pin);
    gpio_set_dir(_tx_pin, GPIO_OUT);
    gpio_put(_tx_pin, 1);

    gpio_init(_rx_pin);
    gpio_set_dir(_rx_pin, GPIO_IN);
    gpio_pull_up(_rx_pin);

    gpio_set_function(_tx_pin, GPIO_FUNC_UART);
    gpio_set_function(_rx_pin, GPIO_FUNC_UART);

    _baud_rate = uart_init(_uart, _baud_rate);

    // Enable UART interrupts
    if(_uart_number == 0){
        _instances[0] = this;
        irq_set_exclusive_handler(UART0_IRQ, on_uart0_rx);
        irq_set_enabled(UART0_IRQ, true);
    } else {
        _instances[1] = this;
        irq_set_exclusive_handler(UART1_IRQ, on_uart1_rx);
        irq_set_enabled(UART1_IRQ, true);
    }
    uart_set_irq_enables(_uart, true, false);
    uart_set_fifo_enabled(_uart, true);

    _status = DeviceStatus::Initialized;
}

bool UARTDevice::setBaudRate(uint baudRate) {
    _baud_rate = uart_set_baudrate(_uart, baudRate);
    return true;
}

bool UARTDevice::setFormat(int bits, int stopBits, int parity) {
    uart_set_format(_uart, bits, stopBits, (uart_parity_t)parity);
    return true;
}

int UARTDevice::send(const uint8_t* data, size_t length) {
    for (size_t i = 0; i < length; i++) {
        if(!uart_is_writable(_uart)) {
            return i; // Return number of bytes sent so far
        }
        uart_get_hw(_uart)->dr = data[i];
    }
    return length;
}

bool UARTDevice::isPinValid(uint8_t pin, int pinType) const {
    auto* validPins = (_uart_number == 0) ? _uart0_pins : _uart1_pins;
    for (int i = pinType; i < 16; i += UART_PIN_COUNT) {
        if (validPins[i] == pin) {
            return true;
        }
    }
    return false;
}

bool UARTDevice::registerDataReceivedCallback(Mainloop::Function func, uint32_t signal) {
    if(_irq_signal != 0) {
        return false;
    }
    if(signal == 0) {
        signal = 0x41525430 + _uart_number;
    }
    _irq_signal = signal;
    Mainloop::getInstance().registerSignalTask(getName() + ".DataReceived", func, _irq_signal);
    return true;
}

const std::string UARTDevice::getDetails() const {
    return "UART Device " + std::to_string(_uart_number) + " (TX: " + std::to_string(_tx_pin) + ", RX: " + std::to_string(_rx_pin) + ", Baud: " + std::to_string(_baud_rate) + ")";
}

void UARTDevice::handleIRQ() {
    if(uart_is_readable(_uart) != 0) {
        while(uart_is_readable(_uart) != 0 && !_rx_fifo.isFull()) {
            _rx_fifo.push(uart_getc(_uart));
        }
        if(_irq_signal != 0) {
            Mainloop::getInstance().triggerSignal(_irq_signal);
        }
    }
}

void UARTDevice::on_uart0_rx() {
    if(_instances[0]) {
        _instances[0]->handleIRQ();
    }
}

void UARTDevice::on_uart1_rx() {
    if(_instances[1]) {
        _instances[1]->handleIRQ();
    }
}

