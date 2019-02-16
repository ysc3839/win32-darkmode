#pragma once

#include <climits>

#include "targetver.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <CommCtrl.h>
#include <Uxtheme.h>
#include <WindowsX.h>
#include <Dwmapi.h>
#include <Vssym32.h>

#pragma comment(lib, "Comctl32.lib")
#pragma comment(lib, "Uxtheme.lib")
#pragma comment(lib, "Dwmapi.lib")

#include "resource.h"

#include "DarkMode.h"
#include "ListViewUtil.h"
