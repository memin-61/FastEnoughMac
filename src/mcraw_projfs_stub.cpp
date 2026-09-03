#ifdef __APPLE__
#include "mcraw_projfs.h"
#include "app.h"

McrawMount::~McrawMount() { unmount(); }
void McrawMount::unmount() { m_mounted=false; m_virtContext=nullptr; m_mountContext=nullptr; }
std::shared_ptr<McrawMount> McrawMount::mount(const std::string& mcrawPath, std::string& outMountPath) {
    (void)mcrawPath; (void)outMountPath;
    LOG_WARN("MCRAW", "ProjFS not supported on macOS — use local extraction instead.");
    return nullptr;
}

McrawMountManager& McrawMountManager::instance() { static McrawMountManager s; return s; }
std::string McrawMountManager::mountMcraw(const std::string& mcrawPath) { (void)mcrawPath; return ""; }
bool McrawMountManager::isInsideMount(const std::string& path) const { (void)path; return false; }
void McrawMountManager::unmountAll() {}
#endif
