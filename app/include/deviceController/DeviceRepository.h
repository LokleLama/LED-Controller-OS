#pragma once

#include "deviceController/IDeviceFactory.h"
#include "devices/IDevice.h"

#include <memory>
#include <string>
#include <vector>

class DeviceRepository {
public:   
    static DeviceRepository& getInstance();

    const std::vector<std::string> getAvailableDeviceNames(IDeviceFactory::Category category = IDeviceFactory::Category::All) const;
    const std::string getParameterInfo(const std::string& name) const;
    std::shared_ptr<IDevice> createDevice(const std::string& name, const std::vector<std::string>& params);

    const std::vector<std::shared_ptr<IDevice>>& getDevices() const {
        return _devices;
    }
    std::shared_ptr<IDevice> getDeviceByName(const std::string& name) const;

    // this template gets a device by name and casts it to the specified type, returns nullptr if the device is not found or cannot be cast to the specified type
    template<typename T>
    std::shared_ptr<T> getDevice(const std::string& expectedType, const std::string& name) const {
        auto device = getDeviceByName(name);
        if (!device || device->getType() != expectedType) {
            return nullptr;
        }
        return reinterpret_cast<T*>(device.get())->getShared();
    }

private:
    std::vector<std::shared_ptr<IDevice>> _devices;
    std::vector<std::shared_ptr<IDeviceFactory>> _factories;

    DeviceRepository();

    void addDevice(std::shared_ptr<IDevice> device);
};