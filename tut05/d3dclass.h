////////////////////////////////////////////////////////////////////////////////
// Filename: d3dclass.h
////////////////////////////////////////////////////////////////////////////////
#ifndef _D3DCLASS_H_
#define _D3DCLASS_H_


/////////////
// LINKING //
/////////////
#pragma once
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d12.lib")


//////////////
// INCLUDES //
//////////////
#include <dxgi1_6.h>
#include <d3dcommon.h>
#include <d3d12.h>
#include <directxmath.h>
using namespace DirectX;

////////////////////////////////////////////////////////////////////////////////
// Class name: D3DClass
////////////////////////////////////////////////////////////////////////////////
class D3DClass
{
public:
	D3DClass();
	D3DClass(const D3DClass&);
	~D3DClass();

	bool Initialize(int, int, bool, HWND, bool, float, float);
	void Shutdown();
	
	bool BeginScene(float, float, float, float);
	bool EndScene();

	void SetRootSignature(ID3D12RootSignature*);
	void SetRenderState(ID3D12PipelineState*);
	bool FinishUp();

	ID3D12Device* GetDevice();
	ID3D12GraphicsCommandList* GetCommandList();//ID3D12DeviceContext* GetDeviceContext();

	void GetProjectionMatrix(XMMATRIX&);
	void GetWorldMatrix(XMMATRIX&);
	void GetOrthoMatrix(XMMATRIX&);
	void GetVideoCardInfo(char*, int&);

private:
	bool m_vsync_enabled;
	ID3D12Device* m_device;
	ID3D12CommandQueue* m_commandQueue;
	int m_videoCardMemory;
	char m_videoCardDescription[128];
	IDXGISwapChain3* m_swapChain;
	ID3D12DescriptorHeap* m_renderTargetViewHeap;
	ID3D12Resource* m_backBufferRenderTarget[2];
	unsigned int m_bufferIndex;
	ID3D12CommandAllocator* m_commandAllocator;
	ID3D12CommandList* m_ppCommandLists[1];
	ID3D12GraphicsCommandList* m_commandList;
	ID3D12RootSignature* m_prtSignature;
	ID3D12PipelineState* m_pipelineState;
	ID3D12Fence* m_fence;
	HANDLE m_fenceEvent;
	unsigned long long m_fenceValue;
 
	D3D12_VIEWPORT m_vp;
	D3D12_RECT m_sr;

	XMMATRIX m_projectionMatrix;
	XMMATRIX m_worldMatrix;
	XMMATRIX m_orthoMatrix;
};

#endif