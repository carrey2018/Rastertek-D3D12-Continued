////////////////////////////////////////////////////////////////////////////////
// Filename: TextureShaderClass.h
////////////////////////////////////////////////////////////////////////////////
#ifndef _TextureShaderClass_H_
#define _TextureShaderClass_H_


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
// Class name: TextureShaderClass
////////////////////////////////////////////////////////////////////////////////
class TextureShaderClass
{
private:
	struct MatrixBufferType
	{
		XMMATRIX world;
		XMMATRIX view;
		XMMATRIX projection;
	};

public:
	TextureShaderClass();
	TextureShaderClass(const TextureShaderClass&);
	~TextureShaderClass();

	bool Initialize(ID3D12Device*, HWND, ID3D12Resource*);
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
	ID3D12RootSignature* m_prtSignature;
	ID3D12PipelineState* m_pipelineState;
	ID3D12DescriptorHeap* m_srvcbvHeap;//2 views on 1 heap
	ID3D12DescriptorHeap* m_smplHeap;//1 sampler heap
	ID3D12Resource* m_comTexture;
	void* m_pDataPtr;
};

#endif
