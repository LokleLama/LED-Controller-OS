#pragma once

#include "deviceController/DeviceRepository.h"
#include "devices/WS2812.h"
#include "devices/LEDStatus.h"

#include "VariableStore/VariableStore.h"
#include "Utils/ValueConverter.h"

#include "Console.h"
#include "Flash/SPFS.h"

#include "ArduinoJson.h"

#include <memory>
#include <string>
#include <vector>

#include <cstdint>
#include <iostream>

class LEDStatusFactory : public IDeviceFactory {
public:
    LEDStatusFactory(DeviceRepository& deviceRepo, const Console& console) : _deviceRepo(deviceRepo), _console(console) {}
    
    const Category getCategory() const override { return Category::UserInterface; }
    const std::vector<std::string> getDeviceNames() const override {
        static std::vector<std::string> names = {"LEDStatus"};
        return names;
    }
    const std::string& getParameterInfo() const override{
        static std::string empty = "<WS2812DeviceName> [name] [init-state] [status-colors-file]\n"
                                   "  WS2812DeviceName:   Name of the WS2812 device to use (e.g.: WS2812-0)\n"
                                   "  name:               Optional unique name for the device (default: auto-generated)\n"
                                   "  init-state:         Optional initial state for the LED status (default: \"Idle\")\n"
                                   "  status-colors-file: Optional file containing status-color mappings: a json file with format\n"
                                   "                          {\n"
                                   "                              \"OK\": \"#00FF00\",\n"
                                   "                              \"Warning\": \"#FFFF00\",\n"
                                   "                              \"Error\": \"#FF0000\",\n"
                                   "                              \"Idle\": \"#0000FF\"\n"
                                   "                          }";
        return empty;
    }
    std::shared_ptr<IDevice> createDevice(const std::string& name, const std::vector<std::string>& params) override {
        if (params.size() < 1) {
            return nullptr;
        }
        auto ws2812_device = _deviceRepo.getDevice<WS2812>("WS2812", params[0]);
        if (!ws2812_device || ws2812_device->getStatus() != IDevice::DeviceStatus::Initialized) {
            std::cout << "Invalid WS2812 device: " << params[0] << std::endl;
            return nullptr;
        }
        std::string device_name;
        if (params.size() >= 2) {
            device_name = params[1];
        } else {
            device_name = "Status-" + std::to_string(_number);
        }
        _number++;
        std::string init_state = "Idle";
        if (params.size() >= 3) {
            init_state = params[2];
        }

        auto led_status_device = std::make_shared<LEDStatus>(ws2812_device, device_name, init_state);
        if (led_status_device->getStatus() != IDevice::DeviceStatus::Initialized) {
            std::cout << "Failed to initialize LEDStatus device: " << device_name << std::endl;
            return nullptr;
        }

        if(params.size() >= 4) {
            loadStatusColorsFromFile(led_status_device, params[3]);
        }

        if(!ws2812_device->assignToUser(led_status_device)){
            std::cout << "Failed to assign WS2812 device to LEDStatus device: " << device_name << std::endl;
            return nullptr;
        }
        if (!setupVariable(led_status_device, init_state)) {
            std::cout << "Failed to setup variable for LEDStatus device: " << device_name << std::endl;
            return nullptr;
        }
        return led_status_device;
    }

private:
    DeviceRepository& _deviceRepo;
    const Console& _console;
    uint8_t _number = 0;

    bool setupVariable(std::shared_ptr<LEDStatus> device, const std::string& defaultValue) {
        auto& variableStore = VariableStore::getInstance();

        variableStore.addVariable(device->getName() + ".value", defaultValue)->setSystemVariable();
        variableStore.registerCallback(device->getName() + ".value", [device](const std::string& key, const std::string& value) {
            return device->setStatus(value);
        });

        return true;
    }

    void loadStatusColorsFromFile(std::shared_ptr<LEDStatus> device, const std::string& filename) {
        if (!_console.getFileSystem()) {
            std::cout << "No filesystem available to load status colors file: " << filename << std::endl;
            return;
        }
        auto file = _console.currentDirectory->openFile(filename);
        if (!file) {
            std::cout << "Failed to open status colors file: " << filename << std::endl;
            return;
        }

        std::string fileContent = file->readAsString();
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, fileContent);
        if (error) {
            std::cout << "Failed to parse status colors file: " << filename << " - " << error.c_str() << std::endl;
            return;
        }

        device->ClearMap();
        for (JsonPair kv : doc.as<JsonObject>()) {
            const std::string key = kv.key().c_str();
            uint32_t color = ValueConverter::toInt(kv.value().as<std::string>());

            device->AddStatusColor(key, color);
        }
    }
};
