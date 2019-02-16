#pragma once

enum IMMERSIVE_HC_CACHE_MODE
{
	IHCM_USE_CACHED_VALUE,
	IHCM_REFRESH
};

using fnRtlGetNtVersionNumbers = void (WINAPI *)(LPDWORD major, LPDWORD minor, LPDWORD build);
using fnShouldAppsUseDarkMode = bool (WINAPI *)();
using fnAllowDarkModeForWindow = bool (WINAPI *)(HWND hWnd, bool allow);
using fnAllowDarkModeForApp = bool (WINAPI *)(bool allow);
using fnFlushMenuThemes = void (WINAPI *)();
using fnRefreshImmersiveColorPolicyState = void (WINAPI *)();
using fnIsDarkModeAllowedForWindow = bool (WINAPI *)(HWND hWnd);
using fnGetIsImmersiveColorUsingHighContrast = bool (WINAPI *)(IMMERSIVE_HC_CACHE_MODE mode);

fnShouldAppsUseDarkMode _ShouldAppsUseDarkMode = nullptr;
fnAllowDarkModeForWindow _AllowDarkModeForWindow = nullptr;
fnAllowDarkModeForApp _AllowDarkModeForApp = nullptr;
fnFlushMenuThemes _FlushMenuThemes = nullptr;
fnRefreshImmersiveColorPolicyState _RefreshImmersiveColorPolicyState = nullptr;
fnIsDarkModeAllowedForWindow _IsDarkModeAllowedForWindow = nullptr;
fnGetIsImmersiveColorUsingHighContrast _GetIsImmersiveColorUsingHighContrast = nullptr;

bool g_darkModeSupported = false;
bool g_darkModeEnabled = false;

bool AllowDarkModeForWindow(HWND hWnd, bool allow)
{
	if (g_darkModeSupported)
		return _AllowDarkModeForWindow(hWnd, allow);
	return false;
}

bool IsHighContrast()
{
	HIGHCONTRASTW highContrast = { sizeof(highContrast) };
	if (SystemParametersInfoW(SPI_GETHIGHCONTRAST, sizeof(highContrast), &highContrast, FALSE))
		return highContrast.dwFlags & HCF_HIGHCONTRASTON;
	return false;
}

void RefreshTitleBarThemeColor(HWND hWnd)
{
	BOOL dark = FALSE;
	if (_IsDarkModeAllowedForWindow(hWnd) &&
		_ShouldAppsUseDarkMode() &&
		!IsHighContrast())
	{
		dark = TRUE;
	}
	DwmSetWindowAttribute(hWnd, 19, &dark, sizeof(dark));
}

bool IsColorSchemeChangeMessage(LPARAM lParam)
{
	bool is = false;
	if (lParam && CompareStringOrdinal(reinterpret_cast<LPCWCH>(lParam), -1, L"ImmersiveColorSet", -1, TRUE) == CSTR_EQUAL)
	{
		_RefreshImmersiveColorPolicyState();
		is = true;
	}
	_GetIsImmersiveColorUsingHighContrast(IHCM_REFRESH);
	return is;
}

bool IsColorSchemeChangeMessage(UINT message, LPARAM lParam)
{
	if (message == WM_SETTINGCHANGE)
		return IsColorSchemeChangeMessage(lParam);
	return false;
}

void InitDarkMode()
{
	fnRtlGetNtVersionNumbers RtlGetNtVersionNumbers = reinterpret_cast<fnRtlGetNtVersionNumbers>(GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "RtlGetNtVersionNumbers"));
	if (RtlGetNtVersionNumbers)
	{
		DWORD major, minor, build;
		RtlGetNtVersionNumbers(&major, &minor, &build);
		build &= ~0xF0000000;
		if (major == 10 && minor == 0 && 17763 <= build && build <= 18290) // Windows 10 1809 10.0.17763 - Insider 10.0.18290
		{
			HMODULE hUxtheme = LoadLibraryExW(L"uxtheme.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
			if (hUxtheme)
			{
				_RefreshImmersiveColorPolicyState = reinterpret_cast<fnRefreshImmersiveColorPolicyState>(GetProcAddress(hUxtheme, MAKEINTRESOURCEA(104)));
				_GetIsImmersiveColorUsingHighContrast = reinterpret_cast<fnGetIsImmersiveColorUsingHighContrast>(GetProcAddress(hUxtheme, MAKEINTRESOURCEA(106)));
				_ShouldAppsUseDarkMode = reinterpret_cast<fnShouldAppsUseDarkMode>(GetProcAddress(hUxtheme, MAKEINTRESOURCEA(132)));
				_AllowDarkModeForWindow = reinterpret_cast<fnAllowDarkModeForWindow>(GetProcAddress(hUxtheme, MAKEINTRESOURCEA(133)));
				_AllowDarkModeForApp = reinterpret_cast<fnAllowDarkModeForApp>(GetProcAddress(hUxtheme, MAKEINTRESOURCEA(135)));
				_FlushMenuThemes = reinterpret_cast<fnFlushMenuThemes>(GetProcAddress(hUxtheme, MAKEINTRESOURCEA(136)));
				_IsDarkModeAllowedForWindow = reinterpret_cast<fnIsDarkModeAllowedForWindow>(GetProcAddress(hUxtheme, MAKEINTRESOURCEA(137)));

				if (_RefreshImmersiveColorPolicyState &&
					_ShouldAppsUseDarkMode &&
					_AllowDarkModeForWindow &&
					_AllowDarkModeForApp &&
					_FlushMenuThemes &&
					_IsDarkModeAllowedForWindow)
				{
					g_darkModeSupported = true;

					_AllowDarkModeForApp(true);
					_RefreshImmersiveColorPolicyState();

					g_darkModeEnabled = _ShouldAppsUseDarkMode() && !IsHighContrast();
				}
			}
		}
	}
}
