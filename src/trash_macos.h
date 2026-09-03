#pragma once
#include <string>
#include <vector>

#ifdef __APPLE__
bool trashFilesMac(const std::vector<std::string>& paths, std::string& error);
#endif
