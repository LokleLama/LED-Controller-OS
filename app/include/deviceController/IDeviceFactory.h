#pragma once

#include <cstdint>
#include <string>
#include <memory>
#include <vector>

#include "IDevice.h"

class IDeviceFactory {
public:
  enum class Category {
    Unknown,
    PIO,
    Communication,
    Protocol,
    Logger,
    UserInterface,

    // The 'All' category is only used for filtering and should not be returned by any factory
    All
  };

public:
    virtual ~IDeviceFactory() = default;

    virtual const Category getCategory() const { return Category::Unknown;}
    virtual const std::vector<std::string> getDeviceNames() const = 0;
    virtual const std::string& getParameterInfo() const{
        static std::string empty = "(no parameters)";
        return empty;
    }

    virtual std::shared_ptr<IDevice> createDevice(const std::string& name, const std::vector<std::string>& params) = 0;
};