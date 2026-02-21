#pragma once

#include <cstdint>
#include <string>
#include <memory>

class IDeviceUser {
public:
  virtual ~IDeviceUser() = default;

  virtual const std::string getName() const = 0;
};

class IDevice : public IDeviceUser {
public:
  enum class DeviceStatus {
    Unknown,
    Uninitialized,
    Initialized,
    Assigned,
    Error
  };

  static const std::string& DeviceStatusToString(IDevice::DeviceStatus status) {
    static const std::string unknown = "Unknown";
    static const std::string uninitialized = "Uninitialized";
    static const std::string initialized = "Initialized";
    static const std::string assigned = "Assigned";
    static const std::string error = "Error";
    static const std::string invalid = "Invalid";

    switch (status) {
    case IDevice::DeviceStatus::Unknown:
      return unknown;
    case IDevice::DeviceStatus::Uninitialized:
      return uninitialized;
    case IDevice::DeviceStatus::Initialized:
      return initialized;
    case IDevice::DeviceStatus::Assigned:
      return assigned;
    case IDevice::DeviceStatus::Error:
      return error;
    default:
      return invalid;
    }
  }

  virtual ~IDevice() = default;

  virtual const std::string getType() const = 0;
  virtual DeviceStatus getStatus() const { return _status; };
  virtual const std::string& getStatusString() const{
    return DeviceStatusToString(getStatus());
  }
  virtual const std::string getDetails() const = 0;

  virtual std::shared_ptr<IDeviceUser> getUser() const { return _user; }
  virtual bool assignToUser(std::shared_ptr<IDeviceUser> user) {
    if (getStatus() != DeviceStatus::Initialized || !user || _user) {
      return false;
    }
    _user = user;
    _status = DeviceStatus::Assigned;
    return true;
  }

protected:
  std::shared_ptr<IDeviceUser> _user;
  DeviceStatus _status{DeviceStatus::Unknown};
};
