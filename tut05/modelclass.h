////////////////////////////////////////////////////////////////////////////////
// Filename: modelclass.h
////////////////////////////////////////////////////////////////////////////////
#ifndef _MODELCLASS_H_
#define _MODELCLASS_H_


//////////////
// INCLUDES //
//////////////
#pragma once
#include <d3d12.h>
#include <directxmath.h>
using namespace DirectX;

///////////////////////
// MY CLASS INCLUDES //
///////////////////////
#include "textureclass.h"


////////////////////////////////////////////////////////////////////////////////
// Class name: ModelClass
////////////////////////////////////////////////////////////////////////////////
class ModelClass
{
private:
	struct VertexType
	{
		XMFLOAT3 position;
		XMFLOAT2 texture;
	};

public:
	ModelClass();
	ModelClass(const ModelClass&);
	~ModelClass();

	bool Initialize(ID3D12Device*, ID3D12GraphicsCommandList*, const char*);
	void Shutdown();
	void Render(ID3D12GraphicsCommandList*);

	int GetIndexCount();
	ID3D12Resource* GetTexture();

private:
	bool InitializeBuffers(ID3D12Device*, ID3D12GraphicsCommandList*);
	void ShutdownBuffers();
	void RenderBuffers(ID3D12GraphicsCommandList* );
	bool LoadTexture(ID3D12Device*, ID3D12GraphicsCommandList*, const char*);
	void ReleaseTexture();

private:
	ID3D12Heap* m_heapUpload, * m_heapDefault;
	ID3D12Resource *m_vertexBuffer, *m_indexBuffer, *m_vertexUpload, *m_indexUpload;
	int m_vertexCount, m_indexCount;
	TextureClass* m_Texture;
};

#endif