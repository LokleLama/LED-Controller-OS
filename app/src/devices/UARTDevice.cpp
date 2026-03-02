#include "devices/UARTDevice.h"
#include "hardware/irq.h"
#include "hardware/uart.h"
#include "hardware/gpio.h"

UARTDevice::UARTDevice(int uart_number, uint8_t tx_pin, uint8_t rx_pin, uint baud_rate) : 
    _uart_number(uart_number), _tx_pin(tx_pin), _rx_pin(rx_pin), _baud_rate(baud_rate) 
{
    if (_uart_number >= 2) {
        _status = DeviceStatus::Error;
        return;
    }
    _uart = (_uart_number == 0) ? uart0 : uart1;

    if (!isTXPinValid(_tx_pin) || !isRXPinValid(_rx_pin)) {
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
    uart_write_blocking(_uart, data, length);
    return length;
}

int UARTDevice::dataAvailable() {
    return uart_is_readable(_uart) ? 1 : 0;
}

int UARTDevice::receive(uint8_t* buffer, size_t length) {
    uart_read_blocking(_uart, buffer, length);
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

const std::string UARTDevice::getDetails() const {
    return "UART Device " + std::to_string(_uart_number) + " (TX: " + std::to_string(_tx_pin) + ", RX: " + std::to_string(_rx_pin) + ", Baud: " + std::to_string(_baud_rate) + ")";
}