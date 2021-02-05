#include "win32-darkmode.h"
#include "UAHMenuBar.h"

HINSTANCE g_hInst;
HWND g_hWndListView;

INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	constexpr COLORREF darkBkColor = 0x383838;
	constexpr COLORREF darkTextColor = 0xFFFFFF;
	static HBRUSH hbrBkgnd = nullptr;
	switch (message)
	{
	case WM_INITDIALOG:
	{
		if (g_darkModeSupported)
		{
			SetWindowTheme(GetDlgItem(hDlg, IDOK), L"Explorer", nullptr);
			SendMessageW(hDlg, WM_THEMECHANGED, 0, 0);
		}
		return (INT_PTR)TRUE;
	}
	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	case WM_CTLCOLORDLG:
	case WM_CTLCOLORSTATIC:
	{
		if (g_darkModeSupported && g_darkModeEnabled)
		{
			HDC hdc = reinterpret_cast<HDC>(wParam);
			SetTextColor(hdc, darkTextColor);
			SetBkColor(hdc, darkBkColor);
			if (!hbrBkgnd)
				hbrBkgnd = CreateSolidBrush(darkBkColor);
			return reinterpret_cast<INT_PTR>(hbrBkgnd);
		}
	}
	break;
	case WM_DESTROY:
		if (hbrBkgnd)
		{
			DeleteObject(hbrBkgnd);
			hbrBkgnd = nullptr;
		}
		break;
	case WM_SETTINGCHANGE:
	{
		if (g_darkModeSupported && IsColorSchemeChangeMessage(lParam))
			SendMessageW(hDlg, WM_THEMECHANGED, 0, 0);
	}
	break;
	case WM_THEMECHANGED:
	{
		if (g_darkModeSupported)
		{
			_AllowDarkModeForWindow(hDlg, g_darkModeEnabled);
			RefreshTitleBarThemeColor(hDlg);

			HWND hButton = GetDlgItem(hDlg, IDOK);
			_AllowDarkModeForWindow(hButton, g_darkModeEnabled);
			SendMessageW(hButton, WM_THEMECHANGED, 0, 0);

			UpdateWindow(hDlg);
		}
	}
	break;
	}
	return (INT_PTR)FALSE;
}


#pragma comment(lib, "uxtheme.lib")

static HTHEME g_menuTheme = nullptr;

// processes messages related to UAH / custom menubar drawing.
// return true if handled, false to continue with normal processing in your wndproc
bool UAHDarkModeWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, LRESULT* lr)
{
	switch (message)
	{
	case WM_UAHDRAWMENU:
	{
		UAHMENU* pUDM = (UAHMENU*)lParam;
		RECT rc = { 0 };

		// get the menubar rect
		{
			MENUBARINFO mbi = { sizeof(mbi) };
			GetMenuBarInfo(hWnd, OBJID_MENU, 0, &mbi);

			RECT rcWindow;
			GetWindowRect(hWnd, &rcWindow);

			// the rcBar is offset by the window rect
			rc = mbi.rcBar;
			OffsetRect(&rc, -rcWindow.left, -rcWindow.top);

			rc.top -= 1;
		}

		if (!g_menuTheme) {
			g_menuTheme = OpenThemeData(hWnd, L"Menu");
		}

		DrawThemeBackground(g_menuTheme, pUDM->hdc, MENU_POPUPITEM, MPI_NORMAL, &rc, nullptr);

		return true;
	}
	case WM_UAHDRAWMENUITEM:
	{
		UAHDRAWMENUITEM* pUDMI = (UAHDRAWMENUITEM*)lParam;

		// get the menu item string
		wchar_t menuString[256] = { 0 };
		MENUITEMINFO mii = { sizeof(mii), MIIM_STRING };
		{
			mii.dwTypeData = menuString;
			mii.cch = (sizeof(menuString) / 2) - 1;

			GetMenuItemInfo(pUDMI->um.hmenu, pUDMI->umi.iPosition, TRUE, &mii);
		}

		// get the item state for drawing

		DWORD dwFlags = DT_CENTER | DT_SINGLELINE | DT_VCENTER;

		int iTextStateID = 0;
		int iBackgroundStateID = 0;
		{
			if ((pUDMI->dis.itemState & ODS_INACTIVE) | (pUDMI->dis.itemState & ODS_DEFAULT)) {
				// normal display
				iTextStateID = MPI_NORMAL;
				iBackgroundStateID = MPI_NORMAL;
			}
			if (pUDMI->dis.itemState & ODS_HOTLIGHT) {
				// hot tracking
				iTextStateID = MPI_HOT;
				iBackgroundStateID = MPI_HOT;
			}
			if (pUDMI->dis.itemState & ODS_SELECTED) {
				// clicked -- MENU_POPUPITEM has no state for this, though MENU_BARITEM does
				iTextStateID = MPI_HOT;
				iBackgroundStateID = MPI_HOT;
			}
			if ((pUDMI->dis.itemState & ODS_GRAYED) || (pUDMI->dis.itemState & ODS_DISABLED)) {
				// disabled / grey text
				iTextStateID = MPI_DISABLED;
				iBackgroundStateID = MPI_DISABLED;
			}
			if (pUDMI->dis.itemState & ODS_NOACCEL) {
				dwFlags |= DT_HIDEPREFIX;
			}
		}

		if (!g_menuTheme) {
			g_menuTheme = OpenThemeData(hWnd, L"Menu");
		}

		DrawThemeBackground(g_menuTheme, pUDMI->um.hdc, MENU_POPUPITEM, iBackgroundStateID, &pUDMI->dis.rcItem, nullptr);
		DrawThemeText(g_menuTheme, pUDMI->um.hdc, MENU_POPUPITEM, iTextStateID, menuString, mii.cch, dwFlags, 0, &pUDMI->dis.rcItem);

		return true;
	}
	case WM_THEMECHANGED:
	{
		if (g_menuTheme) {
			CloseThemeData(g_menuTheme);
			g_menuTheme = nullptr;
		}
		// continue processing in main wndproc
		return false;
	}
	default:
		return false;
	}
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	LRESULT lr = 0;
	if (UAHDarkModeWndProc(hWnd, message, wParam, lParam, &lr)) {
		return lr;
	}

	switch (message)
	{
	case WM_CREATE:
	{
		if (g_darkModeSupported)
		{
			_AllowDarkModeForWindow(hWnd, true);
			RefreshTitleBarThemeColor(hWnd);
		}

		g_hWndListView = CreateWindowW(WC_LISTVIEWW, nullptr, WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE | LVS_REPORT | LVS_SHOWSELALWAYS, 0, 0, 0, 0, hWnd, nullptr, g_hInst, nullptr);
		if (g_hWndListView)
		{
			InitListView(g_hWndListView);

			LVCOLUMNW lvc;
			lvc.mask = LVCF_WIDTH | LVCF_TEXT;
			lvc.cx = 300;
			lvc.pszText = const_cast<LPWSTR>(L"Test");
			for (int i = 0; i < 3; ++i)
				ListView_InsertColumn(g_hWndListView, i, &lvc);

			LVITEMW lvi;
			lvi.mask = LVIF_TEXT;
			lvi.iItem = INT_MAX;
			lvi.iSubItem = 0;
			lvi.pszText = const_cast<LPWSTR>(L"Test");
			for (int i = 0; i < 50; ++i)
			{
				int index = ListView_InsertItem(g_hWndListView, &lvi);
				for (int j = 1; j < 3; ++j)
					ListView_SetItemText(g_hWndListView, index, j, const_cast<LPWSTR>(L"Test"));
			}
		}
	}
	break;
	case WM_COMMAND:
	{
		int wmId = LOWORD(wParam);
		// Parse the menu selections:
		switch (wmId)
		{
		case IDM_ABOUT:
			DialogBoxW(g_hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProcW(hWnd, message, wParam, lParam);
		}
	}
	break;
	case WM_SIZE:
	{
		int clientWidth = GET_X_LPARAM(lParam), clientHeight = GET_Y_LPARAM(lParam);
		HDWP hDWP = BeginDeferWindowPos(1);
		if (hDWP != nullptr)
		{
			DeferWindowPos(hDWP, g_hWndListView, 0, 0, 0, clientWidth, clientHeight, SWP_NOZORDER);
			EndDeferWindowPos(hDWP);
		}
	}
	break;
	case WM_SETTINGCHANGE:
	{
		if (IsColorSchemeChangeMessage(lParam))
		{
			g_darkModeEnabled = _ShouldAppsUseDarkMode() && !IsHighContrast();

			RefreshTitleBarThemeColor(hWnd);
			SendMessageW(g_hWndListView, WM_THEMECHANGED, 0, 0);
		}
	}
	break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProcW(hWnd, message, wParam, lParam);
	}
	return 0;
}


int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	InitDarkMode();

	if (!g_darkModeSupported)
	{
		TaskDialog(nullptr, hInstance, L"Error", nullptr, L"Darkmode is not supported.", TDCBF_OK_BUTTON, TD_ERROR_ICON, nullptr);
	}

	WNDCLASSEXW wcex{};
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.lpfnWndProc = WndProc;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_WIN32DARKMODE));
	wcex.hCursor = LoadCursorW(nullptr, IDC_ARROW);
	wcex.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
	wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_WIN32DARKMODE);
	wcex.lpszClassName = L"win32-darkmode";
	wcex.hIconSm = LoadIconW(wcex.hInstance, MAKEINTRESOURCEW(IDI_WIN32DARKMODE));

	RegisterClassExW(&wcex);

	g_hInst = hInstance; // Store instance handle in our global variable

	HWND hWnd = CreateWindowW(L"win32-darkmode", L"win32-darkmode", WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

	if (!hWnd)
		return FALSE;

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	MSG msg;

	// Main message loop:
	while (GetMessageW(&msg, nullptr, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}

	return (int)msg.wParam;
}
