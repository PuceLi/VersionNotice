#pragma once
#include <string>

struct Config {
    int         version     = 0;
    std::string language    = "zh_CN";
    bool        send_notice = true;
    bool        send_toast  = false;
    bool        send_form   = false;
};