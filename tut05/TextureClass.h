////////////////////////////////////////////////////////////////////////////////
// Filename: textureclass.h
////////////////////////////////////////////////////////////////////////////////
#ifndef _TEXTURECLASS_H_
#define _TEXTURECLASS_H_


//////////////
// INCLUDES //
//////////////
#pragma once
#include <d3d12.h>
#include <stdio.h>


////////////////////////////////////////////////////////////////////////////////
// Class name: TextureClass
////////////////////////////////////////////////////////////////////////////////
class TextureClass
{
private:
	struct TargaHeader
	{
		unsigned char data1[12];
		unsigned short width;
		unsigned short height;
		unsigned char bpp;
		unsigned char data2;
	};

public:
	TextureClass();
	TextureClass(const TextureClass&);
	~TextureClass();

	bool Initialize(ID3D12Device*, ID3D12GraphicsCommandList*, const char*);
	void Shutdown();

	ID3D12Resource* GetTexture();

private:
	bool LoadTarga(const char*, int&, int&);

private:
	unsigned char* m_targaData;
	ID3D12Heap* m_heapUpload;
	ID3D12Heap* m_heapTexture;
	ID3D12Resource* m_cacheTexture;
	ID3D12Resource* m_texture;
	
};

#endif

