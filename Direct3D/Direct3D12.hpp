#ifndef __DIRECT3D12_HPP__
#define __DIRECT3D12_HPP__
#include "ComPtr.hpp"
#include "Window.hpp"
#include <d3d12.h>
#include "d3dx12.h"
#include <d3d12shader.h>
#include <dxgi1_4.h>
#include <d3dcompiler.h>

namespace DirectX12 {
	constexpr unsigned int FrameCount = 2;
	enum class D3DFeatureLevel : int {
		Level12_1 = 0xc100,
		Level12_0 = 0xc000,
		Level11_1 = 0xb100,
		Level11_0 = 0xb000,
		Null = 0x000
	};
	struct CreateDeviceConfig {
		D3DFeatureLevel Level;
		bool UseWarpDevice;
	};
	class Direct3D12 {
	private:
		struct Shader {
			ComPtr<ID3DBlob> Vertex;
			ComPtr<ID3DBlob> Pixel;
		} shader;
		struct RootSignature {
			ComPtr<ID3D12RootSignature> Root;
			ComPtr<ID3DBlob> Signiture;
			ComPtr<ID3DBlob> Error;
		} sig;
		unsigned int FrameBufferIndex;
		unsigned int DescriptorSize;
		unsigned long FenceValue;
		HANDLE EventHandle;
		D3D12_VIEWPORT ViewPort;
		D3D12_RECT ScissorRect;

		ComPtr<IDXGIFactory4> Factory;
		ComPtr<ID3D12Device> Device;
		ComPtr<ID3D12CommandAllocator> CommandAllocator;
		ComPtr<ID3D12CommandQueue> CommandQueue;
		ComPtr<IDXGISwapChain3> SwapChain;
		ComPtr<ID3D12DescriptorHeap> RTVDescriptorHeap;
		ComPtr<ID3D12GraphicsCommandList> CommandList;
		ComPtr<ID3D12Resource> RenderTargetView[2];
		ComPtr<ID3D12Fence> Fence;
		ComPtr<ID3D12PipelineState> PipelineState;
	
		void CreateFactory();
		void CreateDevice(const CreateDeviceConfig Conf);
		void CreateCommandAllocator();
		void CreateCommandQueue();
		void CreateSwapChain(const MainWindow Window, const int RefreshRate);
		void CreateRenderTargetViewDescriptorHeap();
		void CreateCommandList();
		void CreateRenderTargetView();
		void CreateFence();
		void SetViewportAndScissorRect(const int WindowWidth, const int WindowHeight);
		void Present(const unsigned int SyncInterval);
		void WaitEndExecuteCommand();
		void GraphOnScreen(const unsigned int SyncInterval);
		void ResetCommandResource();
		void SetResourceBarrier(const D3D12_RESOURCE_STATES StateBefore, const D3D12_RESOURCE_STATES StateAfter);
		void CreateEmptyRootSignature();
		//void CompileShader();
		//void Clear();
	public:
		Direct3D12(const CreateDeviceConfig Conf, const MainWindow Window, const int RefreshRate = 60);
		~Direct3D12();
		void OnFrameRender();
	};
}
#endif
