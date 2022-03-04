#include "rdma_device.h"

#include <infiniband/verbs.h>

#include <optional>
#include <vector>

#include "rome/logging/logging.h"
#include "rome/util/status_util.h"

namespace rome {

using ::util::AlreadyExistsErrorBuilder;
using ::util::NotFoundErrorBuilder;
using ::util::UnavailableErrorBuilder;
using ::util::UnknownErrorBuilder;

/* static */ absl::StatusOr<std::vector<std::string>>
RdmaDevice::GetAvailableDevices() {
  int num_devices;
  auto **device_list = ibv_get_device_list(&num_devices);
  ROME_CHECK_QUIET(ROME_RETURN(absl::NotFoundError("No devices found")),
                   num_devices > 0);
  std::vector<std::string> dev_names;
  dev_names.reserve(num_devices);
  for (int i = 0; i < num_devices; ++i) {
    dev_names.push_back(device_list[i]->name);
  }
  ibv_free_device_list(device_list);
  return dev_names;
}

RdmaDevice::~RdmaDevice() { protection_domains_.clear(); }

namespace {

bool IsActivePort(const ibv_port_attr &port_attr) {
  return port_attr.state == IBV_PORT_ACTIVE;
}

}  // namespace

absl::Status RdmaDevice::OpenDevice(std::string_view dev_name) {
  int num_devices;
  auto device_list =
      ibv_device_list_unique_ptr(ibv_get_device_list(&num_devices));
  ROME_CHECK_QUIET(
      ROME_RETURN(UnavailableErrorBuilder() << "No available devices"),
      num_devices > 0 && device_list != nullptr);

  // Find device called `dev_name`
  ibv_device *dev = nullptr;
  for (int i = 0; i < num_devices; ++i) {
    dev = device_list.get()[i];
    if (dev->name == dev_name) {
      continue;
    }
  }
  ROME_CHECK_QUIET(
      ROME_RETURN(NotFoundErrorBuilder() << "Device not found:" << dev_name),
      dev != nullptr);

  // Try opening the device on the provided port, or the first available.
  dev_context_ = ibv_context_unique_ptr(ibv_open_device(dev));
  ROME_CHECK_QUIET(ROME_RETURN(UnavailableErrorBuilder()
                               << "Could not open device:" << dev_name),
                   dev_context_ != nullptr);
  return absl::OkStatus();
}

absl::Status RdmaDevice::ResolvePort(std::optional<int> port) {
  if (port.has_value()) {
    // Check that the given port is active, failing if not.
    ibv_port_attr port_attr;
    ibv_query_port(dev_context_.get(), port.value(), &port_attr);
    ROME_CHECK_QUIET(
        ROME_RETURN(UnavailableErrorBuilder()
                    << "Port not active:" << std::to_string(port.value())),
        IsActivePort(port_attr));
    port_ = port.value();
    return absl::OkStatus();
  } else {
    // Find the first active port, failing if none exists.
    ibv_device_attr dev_attr;
    ibv_query_device(dev_context_.get(), &dev_attr);
    for (int i = 1; i <= dev_attr.phys_port_cnt; ++i) {
      ibv_port_attr port_attr;
      ibv_query_port(dev_context_.get(), i, &port_attr);
      if (!IsActivePort(port_attr)) {
        continue;
      } else {
        port_ = i;
        return absl::OkStatus();
      }
    }
    return absl::UnavailableError("No active ports");
  }
}

absl::Status RdmaDevice::CreateProtectionDomain(std::string_view id) {
  if (protection_domains_.find(std::string(id)) != protection_domains_.end()) {
    return AlreadyExistsErrorBuilder() << "PD already exists: {}" << id;
  }
  auto pd = ibv_pd_unique_ptr(ibv_alloc_pd(dev_context_.get()));
  if (pd == nullptr) {
    return UnknownErrorBuilder() << "Failed to allocated PD: {}" << id;
  }
  protection_domains_.emplace(id, std::move(pd));
  return absl::OkStatus();
}

absl::StatusOr<ibv_pd *> RdmaDevice::GetProtectionDomain(std::string_view id) {
  auto iter = protection_domains_.find(std::string(id));
  if (iter == protection_domains_.end()) {
    return NotFoundErrorBuilder() << "PD not found: {}" << id;
  }
  return iter->second.get();
}

}  // namespace rome