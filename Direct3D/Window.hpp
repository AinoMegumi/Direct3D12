#ifndef __WINDOW_HPP__
#define __WINDOW_HPP__
#include <Windows.h>
#include <string>

enum class WindowControlButton : UINT {
	Minimize = SC_MINIMIZE,
	Maximize = SC_MAXIMIZE,
	Close = SC_CLOSE
};

class ImageIcon {
private:
	std::string FilePath;
	int IconWidth, IconHeight;
public:
	ImageIcon(const std::string IconFilePath, const int Width, const int Height)
		: FilePath(IconFilePath), IconWidth(Width), IconHeight(Height) {}
	HICON GetImage(HINSTANCE InstanceHandle, const unsigned int LoadOption, const bool Cursor = false) const noexcept;
};

class MainWindow {
private:
	WNDCLASSEXA wc;
	std::string WindowTitle;
	HINSTANCE InstanceHandle;
	MainWindow(HINSTANCE hInstance, WNDPROC WndProc, LPCSTR MenuName, const std::string WindowTitle, const std::string BackgroundColorCode,
		const HICON Icon, const HICON Cursor);
public:
	MainWindow(HINSTANCE hInstance, WNDPROC WndProc, LPCSTR MenuName, const std::string WindowTitle, const std::string BackgroundColorCode);
	MainWindow(HINSTANCE hInstance, WNDPROC WndProc, LPCSTR MenuName, const std::string WindowTitle, const std::string BackgroundColorCode,
		const ImageIcon Icon, const ImageIcon Cursor);
	void CreateClientWindow(const int WindowWidth, const int WindowHeight);
	void Show(const int CmdShow);
	void Update();
	HWND hWnd;
	int WindowWidth, WindowHeight;
};
#endif
