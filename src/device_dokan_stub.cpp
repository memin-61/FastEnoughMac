#ifdef __APPLE__
#include "device_dokan.h"
#include "app.h"

DeviceMountManager& DeviceMountManager::instance(int slot) {
    static DeviceMountManager s[2];
    return s[slot & 1];
}

bool DeviceMountManager::mount(DeviceClient* device, const std::string& storageRoot, const std::string& mountPoint) {
    (void)device; (void)storageRoot; (void)mountPoint;
    LOG_WARN("Dokan", "Drive mount not supported on macOS — Dokan is Windows-only. Use File Browser instead.");
    return false;
}

void DeviceMountManager::unmount() {
    m_mounted = false;
}
#endif
