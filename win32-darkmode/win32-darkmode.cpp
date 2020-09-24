#include "win32-darkmode.h"

HINSTANCE g_hInst;
HWND g_hWndListView;

INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static HBRUSH hbrBkgnd = nullptr;
	switch (message)
	{
	case WM_INITDIALOG:
	{
		if (g_darkModeSupported)
		{
			HWND hButton = GetDlgItem(hDlg, IDOK);

			AllowDarkModeForWindow(hDlg, true);
			AllowDarkModeForWindow(hButton, true);

			SetWindowTheme(hButton, L"Explorer", nullptr);
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
		if (g_darkModeSupported &&
			ShouldAppsUseDarkMode() &&
			!IsHighContrast() &&
			SetDarkThemeColors(hbrBkgnd, reinterpret_cast<HDC>(wParam)))
		{
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
			RefreshTitleBarThemeColor(hDlg);

			SendMessageW(GetDlgItem(hDlg, IDOK), WM_THEMECHANGED, 0, 0);

			InvalidateRect(hDlg, nullptr, TRUE);
		}
	}
	break;
	}
	return (INT_PTR)FALSE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_CREATE:
	{
		if (g_darkModeSupported)
		{
			AllowDarkModeForWindow(hWnd, true);
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
