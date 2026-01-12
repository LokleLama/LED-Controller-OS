#pragma once

#include <cstdint>
#include <string>

class IDeviceUser;

class IDevice {
public:
  enum class DeviceStatus {
    Unknown,
    Uninitialized,
    Initialized,
    Assigned,
    Error
  };

  virtual ~IDevice() = default;

  virtual const std::string& getName() const = 0;
  virtual const std::string& getType() const = 0;
  virtual DeviceStatus getStatus() const { return _status; };
  virtual const std::string& getStatusString() const{
    return toStriong(getStatus());
  }
  virtual const std::string& getDetails() const = 0;

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

class IDeviceUser {
public:
  virtual ~IDeviceUser() = default;

  virtual const std::string& getName() const = 0;
};


static std::string toStriong(IDevice::DeviceStatus status) {
  switch (status) {
  case IDevice::DeviceStatus::Unknown:
    return "Unknown";
  case IDevice::DeviceStatus::Uninitialized:
    return "Uninitialized";
  case IDevice::DeviceStatus::Initialized:
    return "Initialized";
  case IDevice::DeviceStatus::Assigned:
    return "Assigned";
  case IDevice::DeviceStatus::Error:
    return "Error";
  default:
    return "Invalid";
  }
}
