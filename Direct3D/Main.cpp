#include "Window.hpp"
#include "Direct3D12.hpp"
constexpr int WindowWidth = 1280;
constexpr int WindowHeight = 720;

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch(msg) {
		case WM_DESTROY:
			PostQuitMessage(0);
			break;
		default:
			return DefWindowProc(hWnd, msg, wParam, lParam);
	}
	return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int CmdShow) {
	try {
		MainWindow Window(hInstance, WndProc, NULL, "Direct3D12実験", "#ffc0cb");
		Window.CreateClientWindow(WindowWidth, WindowHeight);
		Window.Show(CmdShow);
		Window.Update();
		const UINT FrameCount = 2;
		DirectX12::CreateDeviceConfig Conf = {};
		Conf.Level = DirectX12::D3DFeatureLevel::Level11_0;
		Conf.UseWarpDevice = true;
		DirectX12::Direct3D12 Dx12(Conf, Window);
		MSG	msg;

		while (1) {
			if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
				if (msg.message == WM_QUIT) break;

				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
		return (int)msg.wParam;
		return 0;
	}
	catch (const std::exception& er) {
		return MessageBoxA(NULL, er.what(), "Direct3D12実験", MB_ICONERROR | MB_OK);
	}
	catch (...) {
		return -1;
	}
}
