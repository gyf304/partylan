#ifdef _WIN32

#include "ui.h"
#include <atomic>
#include <string>
#include <stdexcept>
#include <thread>
#include <iostream>

#include <Windows.h>
#include <steam_api.h>

#include "resource.h"
#include "log.h"

const wchar_t *TRAY_CLASS_NAME = L"TRAY";
const wchar_t *APP_NAME = L"PartyLAN";

enum {
	WM_READY = WM_USER + 1,
	WM_TRAY,
	WM_TRAY_ITEM,
	WM_TRAY_CLICK,
};

enum {
	IDM_EXIT = 1,
	IDM_MENU_START = 1000,
};

NOTIFYICONDATA MAIN_NID = { sizeof(MAIN_NID) };
HWND MAIN_HWND;

extern HINSTANCE HINST;

using namespace lpvpn::ui;

void convertUTF8ToUTF16(const std::string &utf8, std::wstring &utf16) {
	utf16.resize(utf8.size() + 1);
	MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, utf16.data(), utf16.size());
}

class WinMenuItem {
	public:
	WinMenuItem(HMENU hmenu, std::shared_ptr<MenuItem> menuItem, uint16_t &id): menuItem(menuItem), id(id) {
		convertUTF8ToUTF16(menuItem->text, text);
		info.cbSize = sizeof(info);
		if (menuItem->submenu != nullptr) {
			submenu = CreatePopupMenu();
			info.fMask = MIIM_ID | MIIM_SUBMENU | MIIM_STRING | MIIM_DATA;
			info.wID = id;
			info.hSubMenu = submenu;
			info.dwTypeData = (LPWSTR)text.c_str();
			info.cch = text.size();
			info.dwItemData = (ULONG_PTR)this;
			for (auto &item : *menuItem->submenu) {
				id++;
				auto itemPtr = std::make_shared<MenuItem>(item);
				submenuItems.emplace_back(submenu, itemPtr, id);
			}
		} else {
			info.cbSize = sizeof(info);
			info.fMask = MIIM_ID | MIIM_TYPE | MIIM_STATE | MIIM_DATA;
			info.wID = id;
			info.fType = MFT_STRING;
			info.fState = 0;
			info.dwTypeData = (LPWSTR)text.c_str();
			info.dwItemData = (ULONG_PTR)this;
			// if (menuItem.cb == nullptr) {
			// 	info.fState |= MFS_DISABLED;
			// }
		}
		if (hmenu != NULL) {
			InsertMenuItem(hmenu, id, TRUE, &info);
		}
	}
	~WinMenuItem() {
		if (submenu != NULL) {
			DestroyMenu(submenu);
		}
	}
	WinMenuItem *findById(uint16_t id) {
		if (this->id == id) {
			return this;
		}
		for (auto &item : submenuItems) {
			auto found = item.findById(id);
			if (found != nullptr) {
				return found;
			}
		}
		return nullptr;
	}
	uint16_t id;
	HMENU submenu = NULL;
	MENUITEMINFO info = { sizeof(info) };
	std::shared_ptr<MenuItem> menuItem;
	std::vector<WinMenuItem> submenuItems;
	std::wstring text;
};

std::mutex UI_MUTEX;
bool UI_READY = false;

std::mutex MENU_MUTEX;
std::shared_ptr<WinMenuItem> WIN_MENU = nullptr;
std::shared_ptr<WinMenuItem> ACTIVE_WIN_MENU = nullptr;

int AddIcon(HWND hwnd)
{
	// check registry for dark mode
	HKEY hKey;
	DWORD dwValue = 0;
	DWORD dwSize = sizeof(DWORD);
	LSTATUS status = RegOpenKeyEx(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", 0, KEY_READ, &hKey);
	if (status == ERROR_SUCCESS) {
		status = RegQueryValueEx(hKey, L"AppsUseLightTheme", NULL, NULL, (LPBYTE)&dwValue, &dwSize);
		RegCloseKey(hKey);
	}
	if (status != ERROR_SUCCESS) {
		dwValue = 1; // default to light mode
	}

	MAIN_NID.cbSize = sizeof(MAIN_NID);
	MAIN_NID.hWnd = hwnd;
	MAIN_NID.uID = 1;
	MAIN_NID.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	MAIN_NID.uCallbackMessage = WM_TRAY;
	MAIN_NID.hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(dwValue ? IDI_LIGHTMODEICON : IDI_DARKMODEICON));
	lstrcpy(MAIN_NID.szTip, APP_NAME);
	return Shell_NotifyIcon(NIM_ADD, &MAIN_NID);
}

int DeleteIcon(HWND hwnd)
{
	MAIN_NID.cbSize = sizeof(MAIN_NID);
	MAIN_NID.hWnd = hwnd;
	MAIN_NID.uID = 1;
	return Shell_NotifyIcon(NIM_DELETE, &MAIN_NID);
}

int ShowIconBalloon(HWND hwnd, const wchar_t *title, const wchar_t *text)
{
	MAIN_NID.cbSize = sizeof(MAIN_NID);
	MAIN_NID.hWnd = hwnd;
	MAIN_NID.uID = 1;
	MAIN_NID.uFlags = NIF_INFO;
	MAIN_NID.dwInfoFlags = NIIF_INFO;
	lstrcpy(MAIN_NID.szInfoTitle, title);
	lstrcpy(MAIN_NID.szInfo, text);
	return Shell_NotifyIcon(NIM_MODIFY, &MAIN_NID);
}

int SetIconTooltip(HWND hwnd, const wchar_t *text)
{
	MAIN_NID.cbSize = sizeof(MAIN_NID);
	MAIN_NID.hWnd = hwnd;
	MAIN_NID.uID = 1;
	MAIN_NID.uFlags = NIF_TIP;
	lstrcpy(MAIN_NID.szTip, text);
	return Shell_NotifyIcon(NIM_MODIFY, &MAIN_NID);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		AddIcon(hwnd);
		PostMessage(hwnd, WM_READY, 0, 0);
		return 0;
	case WM_TRAY:
		switch (lParam) {
		case WM_RBUTTONUP:
		case WM_LBUTTONUP:
		{
			{
				std::lock_guard<std::mutex> lk(MENU_MUTEX);
				if (ACTIVE_WIN_MENU == nullptr) {
					ACTIVE_WIN_MENU = WIN_MENU;
				}
			}

			if (ACTIVE_WIN_MENU != nullptr) {
				POINT pt;
				GetCursorPos(&pt);
				SetForegroundWindow(hwnd);
				TrackPopupMenu(ACTIVE_WIN_MENU->submenu, TPM_BOTTOMALIGN, pt.x, pt.y, 0, hwnd, NULL);
				ACTIVE_WIN_MENU = nullptr;
				PostMessage(hwnd, WM_NULL, 0, 0);
			}
			return 0;
		}
		}
		return 0;
	case WM_COMMAND: {
		auto lo = LOWORD(wParam);
		if (WIN_MENU == nullptr) {
			return 0;
		}
		auto item = WIN_MENU->findById(lo);
		if (item != nullptr) {
			if (item->menuItem->cb != nullptr) {
				auto menuItem = item->menuItem;
				item->menuItem->cb(*menuItem);
			}
		}
		return 0;
	}
	case WM_TIMER:
		return 0;
	case WM_DESTROY:
		DeleteIcon(hwnd);
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

struct Message {
	std::wstring title;
	std::wstring text;
};

namespace lpvpn::ui {
	class UI::Impl {
		public:
		Impl() {
			std::lock_guard<std::mutex> lk(UI_MUTEX);
			if (UI_READY) {
				throw std::runtime_error("UI already initialized");
			}
			UI_READY = true;

			std::unique_ptr<std::runtime_error> err;
			std::atomic<bool> done;
			done.store(false);
			this->thread = std::thread([&]() {
				WNDCLASSEX wc = { sizeof(wc) };
				wc.cbSize = sizeof(wc);
				wc.lpfnWndProc = WindowProc;
				wc.hInstance = HINST;
				wc.lpszClassName = TRAY_CLASS_NAME;
				RegisterClassEx(&wc);

				MAIN_HWND = CreateWindowEx(
					0,
					TRAY_CLASS_NAME,
					APP_NAME,
					0,
					0, 0, 0, 0, // size and position
					NULL, // parent
					NULL, // menu
					HINST,
					NULL // user data
				);

				if (MAIN_HWND == NULL) {
					err = std::make_unique<std::runtime_error>("Failed to create window");
					done.store(true);
					done.notify_all();
					return;
				}

				UpdateWindow(MAIN_HWND);
				SetTimer(MAIN_HWND, 1, 1000, NULL);
				MSG msg;

				while (GetMessage(&msg, NULL, 0, 0)) {
					if (msg.message == WM_READY) {
						done.store(true);
						done.notify_all();
					}

					{
						std::lock_guard<std::mutex> lk(this->_notify_m);
						if (this->_notify != nullptr) {
							ShowIconBalloon(MAIN_HWND, this->_notify->title.c_str(), this->_notify->text.c_str());
							this->_notify.reset();
						}
					}
					
					if (msg.message == WM_CLOSE) {
						if (this->cb != nullptr) {
							Event ev(EventCode::EXIT);
							this->cb(ev);
						}
					}

					TranslateMessage(&msg);
					DispatchMessage(&msg);
				}
			});
			done.wait(false);
			if (err != nullptr) {
				throw *err;
			}
		}

		~Impl() {
			std::lock_guard<std::mutex> lk(UI_MUTEX);
			UI_READY = false;
			PostMessage(MAIN_HWND, WM_DESTROY, 0, 0);
			this->thread.join();

			LOG("UI::Impl destroyed");
		}

		void notify(const std::string &title, const std::string &text) {
			std::lock_guard<std::mutex> lk(this->_notify_m);

			std::wstring wtitle;
			std::wstring wtext;

			convertUTF8ToUTF16(title, wtitle);
			convertUTF8ToUTF16(text, wtext);

			this->_notify = std::make_unique<Message>();
			this->_notify->title = wtitle;
			this->_notify->text = wtext;
		}

		void alert(const std::string &title, const std::string &text) {
			std::wstring wtitle;
			std::wstring wtext;

			convertUTF8ToUTF16(title, wtitle);
			convertUTF8ToUTF16(text, wtext);

			MessageBox(NULL, wtext.c_str(), wtitle.c_str(), MB_OK);
		}

		void onEvent(std::function<void(Event&)> cb) {
			this->cb = cb;
		}

		void setMenu(std::unique_ptr<MenuItem> menu) {
			std::lock_guard<std::mutex> lk(MENU_MUTEX);
			if (menu == nullptr) {
				WIN_MENU = nullptr;
				return;
			}
			uint16_t id = IDM_MENU_START;
			std::shared_ptr<MenuItem> menuItem = std::move(menu);
			WIN_MENU = std::make_shared<WinMenuItem>(nullptr, menuItem, id);
		}

		void setClipboard(const std::string &text) {
			std::wstring wtext;
			convertUTF8ToUTF16(text, wtext);
			if (OpenClipboard(NULL)) {
				EmptyClipboard();
				HGLOBAL hglbCopy = GlobalAlloc(GMEM_MOVEABLE, (wtext.size() + 1) * sizeof(wchar_t));
				if (hglbCopy != NULL) {
					wchar_t *lptstrCopy = (wchar_t*)GlobalLock(hglbCopy);
					memcpy(lptstrCopy, wtext.c_str(), (wtext.size() + 1) * sizeof(wchar_t));
					GlobalUnlock(hglbCopy);
					SetClipboardData(CF_UNICODETEXT, hglbCopy);
				}
				CloseClipboard();
			}
		}

		void openURL(const std::string &url) {
			std::wstring wurl;
			convertUTF8ToUTF16(url, wurl);
			ShellExecute(NULL, L"open", wurl.c_str(), NULL, NULL, SW_SHOWNORMAL);
		}

		private:
			std::thread thread;
			std::unique_ptr<Event> _event;
			std::unique_ptr<Message> _notify;
			std::mutex _notify_m;
			std::function<void(Event&)> cb;
			std::function<void()> clickCb;
	};

	UI::UI() : impl(std::make_unique<Impl>()) {}
	UI::~UI() {};
	void UI::notify(const std::string &title, const std::string &text) { impl->notify(title, text); }
	void UI::alert(const std::string &title, const std::string &text) { impl->alert(title, text); }
	void UI::onEvent(std::function<void(Event&)> cb) { impl->onEvent(cb); }
	void UI::setMenu(std::unique_ptr<MenuItem> menu) { impl->setMenu(std::move(menu)); }
	void UI::setClipboard(const std::string &text) { impl->setClipboard(text); }
	void UI::openURL(const std::string &url) { impl->openURL(url); }
}

#endif
