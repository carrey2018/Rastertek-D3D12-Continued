////////////////////////////////////////////////////////////////////////////////
// Filename: textureclass.cpp
////////////////////////////////////////////////////////////////////////////////
#include "textureclass.h"


TextureClass::TextureClass()
{
	m_targaData = 0;
	m_heapUpload = 0;
	m_heapTexture = 0;
	m_cacheTexture = 0;
	m_texture = 0;
}


TextureClass::TextureClass(const TextureClass& other)
{
}


TextureClass::~TextureClass()
{
}


bool TextureClass::Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* cmd, const char* filename)
{
	bool result;
	int height, width;
	D3D12_HEAP_DESC hpDesc;
	D3D12_RESOURCE_DESC	resourceDesc = {};
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT Srfp = {};
	D3D12_RESOURCE_BARRIER barrier = {};
	HRESULT hResult;
	unsigned int rowPitch;
	
	
	// Load the targa image data into memory.
	result = LoadTarga(filename, height, width);
	if (!result)
	{
		return false;
	}

	UINT64 pitch = (width * 4 /256+1)*256;//pitch of a slice should be aligned to 256
	UINT64 textureHeapSize = pitch * (height - 1) + width * 4;//last row is the image pitch
 
	//Create an upload heap
	hpDesc.SizeInBytes = textureHeapSize;//For an upload heap, the row pitch must be 256 byte aligned for every row, except for the very last row.
	hpDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
	hpDesc.Properties.Type = D3D12_HEAP_TYPE_CUSTOM;//crash at default
	hpDesc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_COMBINE;
	hpDesc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;
	hpDesc.Properties.CreationNodeMask = 0;
	hpDesc.Properties.VisibleNodeMask = 0;
	hpDesc.Flags = D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;

	hResult = device->CreateHeap(&hpDesc, __uuidof(ID3D12Heap), (void**)&m_heapUpload);
	if (FAILED(hResult))
	{
		return false;
	}

	// Setup the description of the texture.
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resourceDesc.Alignment = 0; // may be 0, 4KB, 64KB, or 4MB. 0 will let runtime decide between 64KB and 4MB (4MB for multi-sampled textures)
	resourceDesc.Width = textureHeapSize; // buffer width is only the length
	resourceDesc.Height = 1; // height of buffer is always 1
	resourceDesc.DepthOrArraySize = 1; // if 3d image, depth of 3d image. Otherwise an array of 1D or 2D textures (we only have one image, so we set 1)
	resourceDesc.MipLevels = 1; // Number of mipmaps. We are not generating mipmaps for this texture, so we have only one level
	resourceDesc.Format = DXGI_FORMAT_UNKNOWN; // This is the dxgi format of the image (format of the pixels)
	resourceDesc.SampleDesc.Count = 1; // This is the number of samples per pixel, we just want 1 sample
	resourceDesc.SampleDesc.Quality = 0; // The quality level of the samples. Higher is better quality, but worse performance
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR; // The arrangement of the pixels. Setting to unknown lets the driver choose the most efficient one
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	
	// Create the empty texture.
	hResult = device->CreatePlacedResource(m_heapUpload, 0, &resourceDesc
		, D3D12_RESOURCE_STATE_COPY_DEST
		, nullptr
		, IID_PPV_ARGS(&(m_cacheTexture)));
	if (FAILED(hResult))
	{
		return false;
	}

	//Modify the heap desc
	hpDesc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_NOT_AVAILABLE;//CPU not accessible
	hpDesc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL_L1;//L1 for video memory
	hpDesc.Flags = D3D12_HEAP_FLAG_DENY_RT_DS_TEXTURES | D3D12_HEAP_FLAG_DENY_BUFFERS;

	//Create the default texture heap, using either D3D12_RESOURCE_DESC or CD3DX12_RESOURCE_DESC::Tex2D
	hResult = device->CreateHeap(&hpDesc, __uuidof(ID3D12Heap), (void**)&m_heapTexture);
	if (FAILED(hResult))
	{
		return false;
	}
	
	resourceDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resourceDesc.Alignment = 0; // may be 0, 4KB, 64KB, or 4MB. 0 will let runtime decide between 64KB and 4MB (4MB for multi-sampled textures)
	resourceDesc.Width = width; // width of the texture
	resourceDesc.Height = height; // height of the texture
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

	//Create the empty texture
	hResult = device->CreatePlacedResource(m_heapTexture, 0, &resourceDesc
		, D3D12_RESOURCE_STATE_COPY_DEST
		, nullptr
		, IID_PPV_ARGS(&(m_texture)));
	if (FAILED(hResult))
	{
		return false;
	}
	
	//Get info about the region we are going to copy
	UINT   nSubRes = 1;
	UINT   ntRow = 0;
	UINT64 ntRowSize = 0;
	UINT64 txulBufSize = 0;
	device->GetCopyableFootprints(&resourceDesc, 0, nSubRes, 0, &Srfp, &ntRow, &ntRowSize, &txulBufSize);

	// Set the row pitch of the targa image data.
	rowPitch = (width * 4) * sizeof(unsigned char);
	
	//[begin] copy texture to the upload buffer
	BYTE* pData = nullptr;
	m_cacheTexture->Map(0, NULL, reinterpret_cast<void**>(&pData));//Get buffer pointer
	BYTE* pDestSlice = reinterpret_cast<BYTE*>(pData) + Srfp.Offset;//every row of pixels is taken as a slice
	const BYTE* pSrcSlice = reinterpret_cast<const BYTE*> (m_targaData);
	
	// Copy the targa image data into the texture.
	for (UINT y = 0; y < ntRow ; ++y)
	{
		memcpy(pDestSlice + static_cast<SIZE_T>(Srfp.Footprint.RowPitch)* y, pSrcSlice + static_cast<SIZE_T>(width * 4)* y, width* 4);
	}
	//[end] copy texture to the upload buffer

	m_cacheTexture->Unmap(0, NULL);

	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = m_cacheTexture;
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

	// Now we copy the upload buffer contents to the default heap
	cmd->ResourceBarrier(1, &barrier);
	
	//Specify copy dst by subresource index
	D3D12_TEXTURE_COPY_LOCATION cDst;
	cDst.pResource = m_texture;
	cDst.PlacedFootprint = {};
	cDst.SubresourceIndex = 0;
	cDst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;

	//Specify copy src by footprint
	D3D12_TEXTURE_COPY_LOCATION cSrc;
	cSrc.pResource = m_cacheTexture;
	cSrc.PlacedFootprint = Srfp;
	cSrc.SubresourceIndex = 0;
	cSrc.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;

	cmd->CopyTextureRegion(&cDst, 0, 0, 0, &cSrc, nullptr);

	barrier.Transition.pResource = m_texture;
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE|D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	cmd->ResourceBarrier(1, &barrier);
	
	// Release the targa image data now that the image data has been loaded into the texture.
	delete[] m_targaData;
	m_targaData = 0;

	return true;
}


void TextureClass::Shutdown()
{
	// Release the texture cache.
	if (m_cacheTexture)
	{
		m_cacheTexture->Release();
		m_cacheTexture = 0;
	}

	// Release the texture.
	if (m_texture)
	{
		m_texture->Release();
		m_texture = 0;
	}

	// Release the upload heap.
	if (m_heapUpload)
	{
		m_heapUpload->Release();
		m_heapUpload = 0;
	}
	//release the texture heap
	if (m_heapTexture)
	{
		m_heapTexture ->Release();
		m_heapTexture = 0;
	}

	// Release the targa data.
	if (m_targaData)
	{
		delete[] m_targaData;
		m_targaData = 0;
	}

	return;
}


ID3D12Resource* TextureClass::GetTexture()
{
	return m_texture;
}


bool TextureClass::LoadTarga(const char* filename, int& height, int& width)
{
	int error, bpp, imageSize, index, i, j, k;
	FILE* filePtr;
	unsigned int count;
	TargaHeader targaFileHeader;
	unsigned char* targaImage;


	// Open the targa file for reading in binary.
	error = fopen_s(&filePtr, filename, "rb");
	if (error != 0)
	{
		return false;
	}

	// Read in the file header.
	count = (unsigned int)fread(&targaFileHeader, sizeof(TargaHeader), 1, filePtr);
	if (count != 1)
	{
		return false;
	}

	// Get the important information from the header.
	height = (int)targaFileHeader.height;
	width = (int)targaFileHeader.width;
	bpp = (int)targaFileHeader.bpp;

	// Check that it is 32 bit and not 24 bit.
	if (bpp != 32)
	{
		return false;
	}

	// Calculate the size of the 32 bit image data.
	imageSize = width * height * 4;

	// Allocate memory for the targa image data.
	targaImage = new unsigned char[imageSize];
	if (!targaImage)
	{
		return false;
	}

	// Read in the targa image data.
	count = (unsigned int)fread(targaImage, 1, imageSize, filePtr);
	if (count != imageSize)
	{
		return false;
	}

	// Close the file.
	error = fclose(filePtr);
	if (error != 0)
	{
		return false;
	}

	// Allocate memory for the targa destination data.
	m_targaData = new unsigned char[imageSize];
	if (!m_targaData)
	{
		return false;
	}

	// Initialize the index into the targa destination data array.
	index = 0;

	// Initialize the index into the targa image data.
	k = (width * height * 4) - (width * 4);

	// Now copy the targa image data into the targa destination array in the correct order since the targa format is stored upside down.
	for (j = 0; j < height; j++)
	{
		for (i = 0; i < width; i++)
		{
			m_targaData[index + 0] = targaImage[k + 2];  // Red.
			m_targaData[index + 1] = targaImage[k + 1];  // Green.
			m_targaData[index + 2] = targaImage[k + 0];  // Blue
			m_targaData[index + 3] = targaImage[k + 3];  // Alpha

			// Increment the indexes into the targa data.
			k += 4;
			index += 4;
		}

		// Set the targa image data index back to the preceding row at the beginning of the column since its reading it in upside down.
		k -= (width * 8);
	}

	// Release the targa image data now that it was copied into the destination array.
	delete[] targaImage;
	targaImage = 0;

	return true;
}