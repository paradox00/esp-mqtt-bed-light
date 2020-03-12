#pragma once
#include <WString.h>

void wifi_setup(const String& ssid, const String& pwd, const String& name);
void wifi_loop(void);