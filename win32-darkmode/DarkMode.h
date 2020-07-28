#pragma once
#include "IatHook.h"
#include "delayimp.h"

enum IMMERSIVE_HC_CACHE_MODE
{
	IHCM_USE_CACHED_VALUE,
	IHCM_REFRESH
};

// 1903 18362
enum PreferredAppMode
{
	Default,
	AllowDark,
	ForceDark,
	ForceLight,
	Max
};

enum WINDOWCOMPOSITIONATTRIB
{
	WCA_UNDEFINED = 0,
	WCA_NCRENDERING_ENABLED = 1,
	WCA_NCRENDERING_POLICY = 2,
	WCA_TRANSITIONS_FORCEDISABLED = 3,
	WCA_ALLOW_NCPAINT = 4,
	WCA_CAPTION_BUTTON_BOUNDS = 5,
	WCA_NONCLIENT_RTL_LAYOUT = 6,
	WCA_FORCE_ICONIC_REPRESENTATION = 7,
	WCA_EXTENDED_FRAME_BOUNDS = 8,
	WCA_HAS_ICONIC_BITMAP = 9,
	WCA_THEME_ATTRIBUTES = 10,
	WCA_NCRENDERING_EXILED = 11,
	WCA_NCADORNMENTINFO = 12,
	WCA_EXCLUDED_FROM_LIVEPREVIEW = 13,
	WCA_VIDEO_OVERLAY_ACTIVE = 14,
	WCA_FORCE_ACTIVEWINDOW_APPEARANCE = 15,
	WCA_DISALLOW_PEEK = 16,
	WCA_CLOAK = 17,
	WCA_CLOAKED = 18,
	WCA_ACCENT_POLICY = 19,
	WCA_FREEZE_REPRESENTATION = 20,
	WCA_EVER_UNCLOAKED = 21,
	WCA_VISUAL_OWNER = 22,
	WCA_HOLOGRAPHIC = 23,
	WCA_EXCLUDED_FROM_DDA = 24,
	WCA_PASSIVEUPDATEMODE = 25,
	WCA_USEDARKMODECOLORS = 26,
	WCA_LAST = 27
};

struct WINDOWCOMPOSITIONATTRIBDATA
{
	WINDOWCOMPOSITIONATTRIB Attrib;
	PVOID pvData;
	SIZE_T cbData;
};

extern "C" {
	NTSYSAPI VOID NTAPI RtlGetNtVersionNumbers(_Out_ LPDWORD major, _Out_ LPDWORD minor, _Out_ LPDWORD build);
	WINUSERAPI BOOL WINAPI SetWindowCompositionAttribute(_In_ HWND hWnd, _In_ WINDOWCOMPOSITIONATTRIBDATA*);

	// 1809 17763
	THEMEAPI_(bool) ShouldAppsUseDarkMode(); // ordinal 132
	THEMEAPI_(bool) AllowDarkModeForWindow(HWND hWnd, bool allow); // ordinal 133
	THEMEAPI_(bool) AllowDarkModeForApp(bool allow); // ordinal 135, in 1809
	THEMEAPI_(VOID) FlushMenuThemes(); // ordinal 136
	THEMEAPI_(VOID) RefreshImmersiveColorPolicyState(); // ordinal 104
	THEMEAPI_(bool) IsDarkModeAllowedForWindow(HWND hWnd); // ordinal 137
	THEMEAPI_(bool) GetIsImmersiveColorUsingHighContrast(IMMERSIVE_HC_CACHE_MODE mode); // ordinal 106
	THEMEAPI_(HTHEME) OpenNcThemeData(HWND hWnd, LPCWSTR pszClassList); // ordinal 49

	// 1903 18362
	THEMEAPI_(bool) ShouldSystemUseDarkMode(); // ordinal 138
	//THEMEAPI_(PreferredAppMode) SetPreferredAppMode(PreferredAppMode appMode); // ordinal 135, in 1903
	THEMEAPI_(bool) IsDarkModeAllowedForApp(); // ordinal 139
}

using fnOpenNcThemeData = HTHEME(WINAPI*)(HWND hWnd, LPCWSTR pszClassList); // ordinal 49
using fnSetPreferredAppMode = PreferredAppMode(WINAPI*)(PreferredAppMode appMode); // ordinal 135, in 1903

bool g_darkModeSupported = false;
bool g_darkModeEnabled = false;
DWORD g_buildNumber = 0;

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
	if (IsDarkModeAllowedForWindow(hWnd) &&
		ShouldAppsUseDarkMode() &&
		!IsHighContrast())
	{
		dark = TRUE;
	}
	if (g_buildNumber < 18362)
		SetPropW(hWnd, L"UseImmersiveDarkModeColors", reinterpret_cast<HANDLE>(static_cast<INT_PTR>(dark)));
	else
	{
		WINDOWCOMPOSITIONATTRIBDATA data = { WCA_USEDARKMODECOLORS, &dark, sizeof(dark) };
		SetWindowCompositionAttribute(hWnd, &data);
	}
}

bool IsColorSchemeChangeMessage(LPARAM lParam)
{
	bool is = false;
	if (lParam && CompareStringOrdinal(reinterpret_cast<LPCWCH>(lParam), -1, L"ImmersiveColorSet", -1, TRUE) == CSTR_EQUAL)
	{
		RefreshImmersiveColorPolicyState();
		is = true;
	}
	GetIsImmersiveColorUsingHighContrast(IHCM_REFRESH);
	return is;
}

bool IsColorSchemeChangeMessage(UINT message, LPARAM lParam)
{
	if (message == WM_SETTINGCHANGE)
		return IsColorSchemeChangeMessage(lParam);
	return false;
}

void _AllowDarkModeForApp(bool allow)
{
	if (g_buildNumber < 18362)
		AllowDarkModeForApp(allow);
	else
		reinterpret_cast<fnSetPreferredAppMode>(AllowDarkModeForApp)(allow ? AllowDark : Default);
}

void FixDarkScrollBar()
{
	HMODULE hComctl = LoadLibraryExW(L"comctl32.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
	if (hComctl)
	{
		auto addr = FindDelayLoadThunkInModule(hComctl, "uxtheme.dll", 49); // OpenNcThemeData
		if (addr)
		{
			DWORD oldProtect;
			if (VirtualProtect(addr, sizeof(IMAGE_THUNK_DATA), PAGE_READWRITE, &oldProtect))
			{
				auto MyOpenThemeData = [](HWND hWnd, LPCWSTR classList) -> HTHEME {
					if (wcscmp(classList, L"ScrollBar") == 0)
					{
						hWnd = nullptr;
						classList = L"Explorer::ScrollBar";
					}
					return OpenNcThemeData(hWnd, classList);
				};

				addr->u1.Function = reinterpret_cast<ULONG_PTR>(static_cast<fnOpenNcThemeData>(MyOpenThemeData));
				VirtualProtect(addr, sizeof(IMAGE_THUNK_DATA), oldProtect, &oldProtect);
			}
		}
	}
}

constexpr bool CheckBuildNumber(DWORD buildNumber)
{
	return (buildNumber == 17763 || // 1809
		buildNumber == 18362 || // 1903
		buildNumber == 18363 || // 1909
		buildNumber == 19041); // 2004
}

void InitDarkMode()
{
		DWORD major, minor;
		RtlGetNtVersionNumbers(&major, &minor, &g_buildNumber);
		g_buildNumber &= ~0xF0000000;
		if (major == 10 && minor == 0 && CheckBuildNumber(g_buildNumber))
		{
			__try
			{
				__HrLoadAllImportsForDll("UxTheme.dll"); // Case sensitive
			}
			__except (GetExceptionCode() == VcppException(ERROR_SEVERITY_ERROR, ERROR_PROC_NOT_FOUND) ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
			{
				return;
			}

			g_darkModeSupported = true;

			_AllowDarkModeForApp(true);
			RefreshImmersiveColorPolicyState();

			g_darkModeEnabled = ShouldAppsUseDarkMode() && !IsHighContrast();

			FixDarkScrollBar();
		}
}
