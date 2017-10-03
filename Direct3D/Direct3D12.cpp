#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "Direct3D12.hpp"
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

namespace DirectX12 {
	Direct3D12::Direct3D12(const CreateDeviceConfig Conf, const MainWindow Window, const int RefreshRate) {
		this->CreateFactory();
		this->CreateDevice(Conf);
		this->CreateCommandQueue();
		this->CreateSwapChain(Window, RefreshRate);
		this->CreateRenderTargetViewDescriptorHeap();
		this->CreateRenderTargetView();

		this->CreateCommandAllocator();
		//this->CompileShader();
		
		this->CreateCommandList();
		this->CreateFence();
		this->SetViewportAndScissorRect(Window.WindowWidth, Window.WindowHeight);
		this->OnFrameRender();
	}
	
	Direct3D12::~Direct3D12() {
		CloseHandle(this->EventHandle);
	}

	void Direct3D12::CreateFactory() {
		unsigned int Flag{};
		HRESULT hr;
#ifdef _DEBUG
		{
			ComPtr<ID3D12Debug> Debug;
			hr = D3D12GetDebugInterface(IID_PPV_ARGS(Debug.GetAddressOf()));
			if (FAILED(hr)) throw std::runtime_error("Failed Get Debug Interface");
			Debug->EnableDebugLayer();
		}
		Flag |= DXGI_CREATE_FACTORY_DEBUG;
#endif
		hr = CreateDXGIFactory2(Flag, IID_PPV_ARGS(&this->Factory));
		if (FAILED(hr)) throw std::runtime_error("Failed Create Factory");
	}

	void Direct3D12::CreateDevice(const CreateDeviceConfig Conf) {
		const D3DFeatureLevel Arr[4] = {
			D3DFeatureLevel::Level12_1,
			D3DFeatureLevel::Level12_0,
			D3DFeatureLevel::Level11_1,
			D3DFeatureLevel::Level11_0
		};
		auto CreateDeviceAssist = [](const ComPtr<IDXGIAdapter> Adapter, const D3DFeatureLevel FeatureLevel, ComPtr<ID3D12Device> &Dev) {
			return D3D12CreateDevice(Adapter.Get(), static_cast<D3D_FEATURE_LEVEL>(FeatureLevel), IID_PPV_ARGS(&Dev));
		};
		if (Conf.UseWarpDevice) {
			ComPtr<IDXGIAdapter> WarpAdapter;
			if (SUCCEEDED(this->Factory->EnumWarpAdapter(IID_PPV_ARGS(&WarpAdapter)))) {
				if (Conf.Level == D3DFeatureLevel::Null) {
					bool Succeed = false;
					for (int i = 0; i < 4; i++) {
						if (FAILED(CreateDeviceAssist(WarpAdapter, Arr[i], this->Device))) continue;
						Succeed = true;
						break;
					}
					if (!Succeed) throw std::runtime_error("Direct3D 12に対応したWarpデバイスが見つかりませんでした。");
				}
				else if (FAILED(CreateDeviceAssist(WarpAdapter, Conf.Level, this->Device)))
					throw std::runtime_error("指定された機能レベルに対応したWarpデバイスが見つかりませんでした。");
			}
			else throw std::runtime_error("Warpデバイスが見つかりませんでした。");
		} // UseWarpDevice
		else {
			ComPtr<IDXGIAdapter1> HardwareAdapter;
			if (Conf.Level == D3DFeatureLevel::Null) {
				bool Succeed = false;
				for (int i = 0; i < 4; i++) {
					for (unsigned int AdapterIndex = 0; DXGI_ERROR_NOT_FOUND != this->Factory->EnumAdapters1(AdapterIndex, &HardwareAdapter); AdapterIndex++) {
						DXGI_ADAPTER_DESC1 desc;
						HardwareAdapter->GetDesc1(&desc);
						if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) continue;
						if (SUCCEEDED(D3D12CreateDevice(HardwareAdapter.Get(), static_cast<D3D_FEATURE_LEVEL>(Arr[i]), _uuidof(ID3D12Device), nullptr))) break;
					} // グラフィックアクセラレータ検索のループ
					const HRESULT hr = [](const ComPtr<IDXGIAdapter> Adapter, const D3DFeatureLevel FeatureLevel, ComPtr<ID3D12Device> &Dev) {
						return D3D12CreateDevice(Adapter.Get(), static_cast<D3D_FEATURE_LEVEL>(FeatureLevel), IID_PPV_ARGS(&Dev));
					}(HardwareAdapter, Arr[i], this->Device);
					Succeed = (hr == S_OK);
					if (SUCCEEDED(hr)) break;
				} // FeatureLevelのループ
				if (!Succeed) throw std::runtime_error("Direct3D 12に対応したグラフィックアクセラレータが見つかりませんでした。");
			} // Level == Null
			else {
				for (unsigned int AdapterIndex = 0; DXGI_ERROR_NOT_FOUND != this->Factory->EnumAdapters1(AdapterIndex, &HardwareAdapter); AdapterIndex++) {
					DXGI_ADAPTER_DESC1 desc;
					HardwareAdapter->GetDesc1(&desc);
					if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) continue;
					if (SUCCEEDED(D3D12CreateDevice(HardwareAdapter.Get(), static_cast<D3D_FEATURE_LEVEL>(Conf.Level), _uuidof(ID3D12Device), nullptr))) break;
				} // グラフィックアクセラレータ検索のループ
				const HRESULT hr = CreateDeviceAssist(HardwareAdapter, Conf.Level, this->Device);
				if (FAILED(hr)) throw std::runtime_error("指定された機能レベルに対応したグラフィックアクセラレータが見つかりませんでした。");
			} // Level != Null
		} 
	}
	
	void Direct3D12::CreateCommandAllocator() {
		const HRESULT hr = this->Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&this->CommandAllocator));
		if (FAILED(hr)) throw std::runtime_error("Failed Create Command Allocator");
	}
	
	void Direct3D12::CreateCommandQueue() {
		D3D12_COMMAND_QUEUE_DESC desc{};
		desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		desc.Priority = 0;
		desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		desc.NodeMask = 0;
		HRESULT hr = this->Device->CreateCommandQueue(&desc, IID_PPV_ARGS(this->CommandQueue.GetAddressOf()));
		if (FAILED(hr)) throw std::runtime_error("Failed Create CommandQueue");
		HANDLE FenceEvent = CreateEventEx(nullptr, NULL, NULL, EVENT_ALL_ACCESS);
		if (FenceEvent == NULL) throw std::runtime_error("FenceEventの作成に失敗しました。");
	}
	
	void Direct3D12::CreateSwapChain(const MainWindow Window, const int RefreshRate) {
		/*
		ComPtr<IDXGISwapChain> chain;
		DXGI_SWAP_CHAIN_DESC desc;
		ZeroMemory(&desc, sizeof(&desc));
		desc.BufferDesc.Width = Window.WindowWidth;
		desc.BufferDesc.Height = Window.WindowHeight;
		desc.OutputWindow = Window.hWnd;
		desc.Windowed = TRUE;
		desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_SHADER_INPUT;
		desc.BufferCount = FrameCount;
		desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		desc.BufferDesc.RefreshRate.Numerator = RefreshRate;
		desc.BufferDesc.RefreshRate.Denominator = 1;
		desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
		desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		HRESULT hr = this->Factory->CreateSwapChain(this->CommandQueue.Get(), &desc, chain.GetAddressOf());
		if (FAILED(hr)) throw std::runtime_error("Failed Create Swap Chain");
		hr = chain->QueryInterface(this->SwapChain.GetAddressOf());
		if (FAILED(hr)) throw std::runtime_error("Failed Query Interface");
		*/
		DXGI_SWAP_CHAIN_DESC1 desc = {};
		desc.BufferCount = FrameCount;
		desc.Width = Window.WindowWidth;
		desc.Height = Window.WindowHeight;
		desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_SHADER_INPUT;
		desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Flags = 0;
		ComPtr<IDXGISwapChain1> SwapChain1;
		HRESULT hr = this->Factory->CreateSwapChainForHwnd(this->CommandQueue.Get(), Window.hWnd, &desc, nullptr, nullptr, &SwapChain1);
		if (FAILED(hr)) throw std::runtime_error("Failed Create Swap Chain");
		hr = this->Factory->MakeWindowAssociation(Window.hWnd, DXGI_MWA_NO_ALT_ENTER);
		if (FAILED(hr)) throw std::runtime_error("Failed Make Window Association");
		hr = SwapChain1.As(&this->SwapChain);
		if (FAILED(hr)) throw std::runtime_error("Failed Convert SwapChain3");
		this->FrameBufferIndex = this->SwapChain->GetCurrentBackBufferIndex();
	}
	
	void Direct3D12::CreateRenderTargetViewDescriptorHeap() {
		D3D12_DESCRIPTOR_HEAP_DESC Desc;
		ZeroMemory(&Desc, sizeof(Desc));
		Desc.NumDescriptors = FrameCount;
		Desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		Desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		const HRESULT hr = this->Device->CreateDescriptorHeap(&Desc, IID_PPV_ARGS(this->RTVDescriptorHeap.GetAddressOf()));
		if (FAILED(hr)) throw std::runtime_error("Failed Create Descriptor Heap");
		this->DescriptorSize = this->Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	}

	void Direct3D12::CreateCommandList() {
		const HRESULT hr = this->Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, this->CommandAllocator.Get(), this->PipelineState.Get(), IID_PPV_ARGS(this->CommandList.GetAddressOf()));
		if (FAILED(hr)) throw std::runtime_error("Failed Create Command List");
		this->CommandList->Close();
	}
	
	void Direct3D12::CreateRenderTargetView() {
		D3D12_RENDER_TARGET_VIEW_DESC Desc = []() {
				D3D12_RENDER_TARGET_VIEW_DESC Desc;
				Desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
				Desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
				Desc.Texture2D.MipSlice = 0;
				Desc.Texture2D.PlaneSlice = 0;
				return Desc;
		}();
		CD3DX12_CPU_DESCRIPTOR_HANDLE DescHandle(this->RTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
		for (unsigned int i = 0; i < FrameCount; i++) {
			const HRESULT hr = this->SwapChain->GetBuffer(i, IID_PPV_ARGS(&this->RenderTargetView[i]));
			if (FAILED(hr)) throw std::runtime_error("Failed Get Buffer");
			this->Device->CreateRenderTargetView(this->RenderTargetView[i].Get(), &Desc, DescHandle);
			DescHandle.Offset(1, this->DescriptorSize);
		}
	}
	
	void Direct3D12::CreateFence() {
		this->EventHandle = CreateEvent(0, FALSE, FALSE, 0);
		const HRESULT hr = this->Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&this->Fence));
		if (FAILED(hr)) throw std::runtime_error("Error in CreateFence in DirectX12::Direct3D12::CreateFence\nCause : " + std::to_string(hr));
		this->FenceValue = 1;
	}

	void Direct3D12::SetViewportAndScissorRect(const int WindowWidth, const int WindowHeight) {
		this->ViewPort.TopLeftX = 0;
		this->ViewPort.TopLeftY = 0;
		this->ViewPort.Width = static_cast<float>(WindowWidth);
		this->ViewPort.Height = static_cast<float>(WindowHeight);
		this->ViewPort.MinDepth = D3D12_MIN_DEPTH;
		this->ViewPort.MaxDepth = D3D12_MAX_DEPTH;
		this->ScissorRect.left = 0;
		this->ScissorRect.top = 0;
		this->ScissorRect.right = WindowWidth;
		this->ScissorRect.bottom = WindowHeight;
	}

	void Direct3D12::SetResourceBarrier(const D3D12_RESOURCE_STATES StateBefore, const D3D12_RESOURCE_STATES StateAfter) {
		D3D12_RESOURCE_BARRIER desc = {};
		desc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		desc.Transition.pResource = this->RenderTargetView[0].Get();
		desc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		desc.Transition.StateBefore = StateBefore;
		desc.Transition.StateAfter = StateAfter;
		this->CommandList->ResourceBarrier(1, &desc);
	}

	void Direct3D12::Present(const unsigned int SyncInterval) {
		this->CommandList->Close();
		ID3D12CommandList	*ppCommandLists[] = { this->CommandList.Get() };
		this->CommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
		this->WaitEndExecuteCommand();
		this->GraphOnScreen(SyncInterval);
		this->ResetCommandResource();
	}

	void Direct3D12::WaitEndExecuteCommand() {
		this->Fence->Signal(0);
		this->Fence->SetEventOnCompletion(1, this->EventHandle);
		this->CommandQueue->Signal(this->Fence.Get(), 1);
		WaitForSingleObject(this->EventHandle, INFINITE);
	}

	void Direct3D12::GraphOnScreen(const unsigned int SyncInterval) {
		this->SwapChain->Present(SyncInterval, 0);
	}

	void Direct3D12::ResetCommandResource() {
		this->CommandAllocator->Reset();
		this->CommandList->Reset(this->CommandAllocator.Get(), nullptr);
	}

	void Direct3D12::OnFrameRender() {
		this->ResetCommandResource();
		this->CommandList->SetGraphicsRootSignature(this->sig.Root.Get());
		this->CommandList->RSSetViewports(0, &this->ViewPort); 
		this->CommandList->RSSetScissorRects(1, &this->ScissorRect);
		this->SetResourceBarrier(D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
		CD3DX12_CPU_DESCRIPTOR_HANDLE DescHandle(this->RTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), 0, this->DescriptorSize);
		this->CommandList->OMSetRenderTargets(1, &DescHandle, FALSE, nullptr);
		float ClearColor[4] = { static_cast<float>(0xff) / 255.0f, static_cast<float>(0xc0) / 255.0f, static_cast<float>(0xcb) / 255.0f, 1.0f };
		this->CommandList->ClearRenderTargetView(DescHandle, ClearColor, 0, nullptr); // ここで落ちてる
		this->SetResourceBarrier(D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
		this->Present(0);
	}

	/*void Direct3D12::Clear() {
		this->CommandList->OMSetRenderTargets(1, &this->CPUDescriptorHandle, FALSE, 深度ステンシルバッファ);

	}*/

	void Direct3D12::CreateEmptyRootSignature() {
		D3D12_ROOT_SIGNATURE_DESC desc = {};
		desc.NumParameters = 0;
		desc.pParameters = nullptr;
		desc.NumStaticSamplers = 0;
		desc.pStaticSamplers = nullptr;
		desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
		HRESULT hr = D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &this->sig.Signiture, &this->sig.Error);
		if (FAILED(hr)) throw std::runtime_error("Error in SerializeRootSigniture function.");
		hr = this->Device->CreateRootSignature(0, this->sig.Signiture->GetBufferPointer(), this->sig.Signiture->GetBufferSize(), IID_PPV_ARGS(&this->sig.Root));
		if (FAILED(hr)) throw std::runtime_error("Error in CreateRootSignature function.");
	}

	/*void Direct3D12::CompileShader() {
#ifdef _DEBUG
		unsigned int CompileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
		unsigned int CompileFlags = 0;
#endif
		auto D3DCompileFromFileFunc = [](LPCSTR EntryPoint, LPCSTR Target, const unsigned int CompileFlags, ID3D10Blob** ppCode) {
			return D3DCompileFromFile(L"shader.hlsl", nullptr, nullptr, EntryPoint, Target, CompileFlags, 0, ppCode, nullptr);
		};
		HRESULT hr = D3DCompileFromFileFunc("VSMain", "vs_5_0", CompileFlags, &this->shader.Vertex);
		if (FAILED(hr)) throw std::runtime_error("Error in D3DCompileFromFile(VSMain)\nErrorCode : " + std::to_string(hr));
		hr = D3DCompileFromFileFunc("PSMain", "ps_5_0", CompileFlags, &this->shader.Pixel);
		if (FAILED(hr)) throw std::runtime_error("Error in D3DCompileFromFile(PSMain)\nErrorCode : " + std::to_string(hr));
		D3D12_GRAPHICS_PIPELINE_STATE_DESC PipelineStateDesc = {};
		D3D12_INPUT_ELEMENT_DESC InputElementDesc[] = {
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA }
		};
		PipelineStateDesc.InputLayout = { InputElementDesc, _countof(InputElementDesc) };
		PipelineStateDesc.pRootSignature = this->sig.Root.Get();
		auto ShaderByteCode = [](ComPtr<ID3DBlob> Shader) {
			D3D12_SHADER_BYTECODE Code = {};
			Code.pShaderBytecode = Shader->GetBufferPointer();
			Code.BytecodeLength = Shader->GetBufferSize();
			return Code;
		};
		PipelineStateDesc.VS = ShaderByteCode(this->shader.Vertex);
		PipelineStateDesc.PS = ShaderByteCode(this->shader.Pixel);
		PipelineStateDesc.RasterizerState = []() {
			D3D12_RASTERIZER_DESC desc;
			desc.FillMode = D3D12_FILL_MODE_SOLID;
			desc.CullMode = D3D12_CULL_MODE_BACK;
			desc.FrontCounterClockwise = FALSE;
			desc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
			desc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
			desc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
			desc.DepthClipEnable = TRUE;
			desc.MultisampleEnable = FALSE;
			desc.AntialiasedLineEnable = FALSE;
			desc.ForcedSampleCount = 0;
			desc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
			return desc;
		}();
		PipelineStateDesc.BlendState = []() {
			D3D12_BLEND_DESC desc = {};
			desc.AlphaToCoverageEnable = FALSE;
			desc.IndependentBlendEnable = FALSE;
			for (unsigned int i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; i++)
				desc.RenderTarget[i] = []() {
				D3D12_RENDER_TARGET_BLEND_DESC desc;
				desc.BlendEnable = FALSE;
				desc.LogicOpEnable = FALSE;
				desc.SrcBlend = D3D12_BLEND_ONE;
				desc.DestBlend = D3D12_BLEND_ZERO;
				desc.BlendOp = D3D12_BLEND_OP_ADD;
				desc.SrcBlendAlpha = D3D12_BLEND_ONE;
				desc.DestBlendAlpha = D3D12_BLEND_ZERO;
				desc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
				desc.LogicOp = D3D12_LOGIC_OP_NOOP;
				desc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
				return desc;
			}();
			return desc;
		}();
		PipelineStateDesc.DepthStencilState.DepthEnable = FALSE;
		PipelineStateDesc.DepthStencilState.StencilEnable = FALSE;
		PipelineStateDesc.SampleMask = std::numeric_limits<unsigned int>::max();
		PipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		PipelineStateDesc.NumRenderTargets = 1;
		PipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		PipelineStateDesc.SampleDesc.Count = 1;
		hr = this->Device->CreateGraphicsPipelineState(&PipelineStateDesc, IID_PPV_ARGS(&this->PipelineState));
		if (FAILED(hr)) throw std::runtime_error("Error in CreateGraphicsPipelineState function.");
	}*/

}
