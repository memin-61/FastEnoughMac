#pragma once

#ifdef __APPLE__
// Show a native macOS alert with OK/Cancel buttons.
// Returns true when the user accepts, false otherwise.
bool macConfirmDialog(const char* title, const char* message);
#endif
