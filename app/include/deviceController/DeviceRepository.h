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

private:
    std::vector<std::shared_ptr<IDevice>> _devices;
    std::vector<std::shared_ptr<IDeviceFactory>> _factories;

    DeviceRepository();

    void addDevice(std::shared_ptr<IDevice> device);
};