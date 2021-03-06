#include "Window.hpp"

struct Color {
	Color(const int Red, const int Green, const int Blue)
		: Red(Red), Green(Green), Blue(Blue) {}
	Color(const std::string ColorCode) {
		if (ColorCode.size() != 7 || '#' != ColorCode.at(0)) throw std::runtime_error("色コードが不正です。");
		auto Convert = [ColorCode](const size_t Start) { return std::stoi(ColorCode.substr(Start, 2), nullptr, 16); };
		this->Red = Convert(1);
		this->Green = Convert(3);
		this->Blue = Convert(5);
	}
	HBRUSH GetColorInfo() const {
		return CreateSolidBrush(RGB(this->Red, this->Green, this->Blue));
	}
	int Red, Green, Blue;
};

HICON ImageIcon::GetImage(HINSTANCE InstanceHandle, const unsigned int LoadOption, const bool Cursor) const noexcept {
	return (HICON)LoadImageA(InstanceHandle, this->FilePath.c_str(), Cursor ? IMAGE_CURSOR : IMAGE_ICON, this->IconWidth, this->IconHeight, LoadOption);
}

MainWindow::MainWindow(HINSTANCE hInstance, WNDPROC WndProc, LPCSTR MenuName, const std::string WindowTitle, const std::string BackgroundColorCode)
	: MainWindow(hInstance, WndProc, MenuName, WindowTitle, BackgroundColorCode, LoadIcon(NULL, IDC_ICON), LoadCursor(NULL, IDC_ARROW)) {}

MainWindow::MainWindow(HINSTANCE hInstance, WNDPROC WndProc, LPCSTR MenuName, const std::string WindowTitle, const std::string BackgroundColorCode,
	const ImageIcon Icon, const ImageIcon Cursor)
	: MainWindow(hInstance, WndProc, MenuName, WindowTitle, BackgroundColorCode, Icon.GetImage(hInstance, LR_SHARED), Cursor.GetImage(hInstance, LR_SHARED, true)) {}


MainWindow::MainWindow(HINSTANCE hInstance, WNDPROC WndProc, LPCSTR MenuName, const std::string WindowTitle, const std::string BackgroundColorCode, const HICON Icon, const HICON Cursor)
	: wc(), WindowTitle(WindowTitle), InstanceHandle(hInstance), hWnd() {
	this->wc.cbSize = sizeof(this->wc);
	this->wc.style = CS_HREDRAW | CS_VREDRAW;
	this->wc.lpfnWndProc = WndProc;
	this->wc.cbClsExtra = 0;
	this->wc.cbWndExtra = 0;
	this->wc.hInstance = hInstance;
	this->wc.hbrBackground = Color(BackgroundColorCode).GetColorInfo();
	this->wc.lpszMenuName = MenuName;
	this->wc.lpszClassName = this->WindowTitle.c_str();
	this->wc.hIcon = this->wc.hIconSm = Icon;
	this->wc.hCursor = Cursor;
}

void MainWindow::CreateClientWindow(const int WindowWidth, const int WindowHeight) {
	if (0 == RegisterClassExA(&this->wc)) throw std::runtime_error("Error in RegisterClassEx in MainWindow::CreateClientWindow");
	this->WindowWidth = WindowWidth;
	this->WindowHeight = WindowHeight;
	this->hWnd = CreateWindowExA(
		WS_EX_COMPOSITED,
		this->WindowTitle.c_str(),
		this->WindowTitle.c_str(),
		WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
		CW_USEDEFAULT, CW_USEDEFAULT,
		WindowWidth, WindowHeight, NULL, NULL, this->InstanceHandle, NULL
	);
	if (this->hWnd == NULL) throw std::runtime_error("Error in CreateWindowEx in MainWindow::CreateClientWindow");
}

void MainWindow::Show(const int CmdShow) {
	ShowWindow(this->hWnd, CmdShow);
}

void MainWindow::Update() {
	UpdateWindow(this->hWnd);
}
