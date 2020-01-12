////////////////////////////////////////////////////////////////////////////////
// Filename: colorshaderclass.h
////////////////////////////////////////////////////////////////////////////////
#ifndef _COLORSHADERCLASS_H_
#define _COLORSHADERCLASS_H_


//////////////
// INCLUDES //
//////////////
#pragma once
#include <d3d12.h>
#include <d3dx12.h>
#include <directxmath.h>
#include <fstream>
#include <d3dcompiler.h>

#pragma comment(lib, "d3dcompiler.lib")
using namespace std;
using namespace DirectX;


////////////////////////////////////////////////////////////////////////////////
// Class name: ColorShaderClass
////////////////////////////////////////////////////////////////////////////////
class ColorShaderClass
{
private:
	struct MatrixBufferType
	{
		XMMATRIX world;
		XMMATRIX view;
		XMMATRIX projection;
	};

public:
	ColorShaderClass();
	ColorShaderClass(const ColorShaderClass&);
	~ColorShaderClass();

	bool Initialize(ID3D12Device*, HWND);
	void Shutdown();
	bool Render(ID3D12GraphicsCommandList*, int, XMMATRIX, XMMATRIX, XMMATRIX);
	ID3D12RootSignature* GetSignature();
	ID3D12PipelineState* GetPSO();

private:
	bool InitializeShader(ID3D12Device*, HWND, const WCHAR*);
	void ShutdownShader();
	void OutputShaderErrorMessage(ID3DBlob*, HWND, const WCHAR*);

	bool SetShaderParameters(ID3D12GraphicsCommandList*, XMMATRIX, XMMATRIX, XMMATRIX);
	void RenderShader(ID3D12GraphicsCommandList*, int);

private:
	ID3D12Resource* m_matrixBuffer;
	ID3D12DescriptorHeap* m_cbvHeap;
	ID3D12RootSignature* m_prtSignature;
	ID3D12PipelineState* m_pipelineState;
	void* m_pDataPtr;
};

#endif