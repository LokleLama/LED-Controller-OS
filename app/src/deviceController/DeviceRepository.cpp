#include "deviceController/DeviceRepository.h"

#include "deviceController/PIOFactory.h"

#include <algorithm>

DeviceRepository& DeviceRepository::getInstance() {
    static DeviceRepository instance;
    return instance;
}

DeviceRepository::DeviceRepository(){
    _factories.push_back(std::make_shared<PIOFactory>());
}

const std::vector<std::string> DeviceRepository::getAvailableDeviceNames(IDeviceFactory::Category category) const{
    std::vector<std::string> names;
    for (const auto& factory : _factories) {
        if (category == IDeviceFactory::Category::All || factory->getCategory() == category) {
            const auto factory_names = factory->getDeviceNames();
            names.insert(names.end(), factory_names.begin(), factory_names.end());
        }
    }
    return names;
}

const std::string DeviceRepository::getParameterInfo(const std::string& name) const {
    for (const auto& factory : _factories) {
        const auto factory_names = factory->getDeviceNames();
        if (std::find(factory_names.begin(), factory_names.end(), name) != factory_names.end()) {
            return factory->getParameterInfo();
        }
    }
    return "(unknown device)";
}

std::shared_ptr<IDevice> DeviceRepository::createDevice(const std::string& name, const std::vector<std::string>& params){
    for (const auto& factory : _factories) {
        const auto factory_names = factory->getDeviceNames();
        if (std::find(factory_names.begin(), factory_names.end(), name) != factory_names.end()) {
            std::shared_ptr<IDevice> device = factory->createDevice(name, params);
            if (!device) {
                return nullptr;
            }
            addDevice(device);
            return device;
        }
    }
    return nullptr;
}

std::shared_ptr<IDevice> DeviceRepository::getDeviceByName(const std::string& name) const {
    for (const auto& device : _devices) {
        if (device->getName() == name) {
            return device;
        }
    }
    return nullptr;
}
void DeviceRepository::addDevice(std::shared_ptr<IDevice> device) {
    _devices.push_back(device);
}