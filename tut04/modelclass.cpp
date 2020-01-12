////////////////////////////////////////////////////////////////////////////////
// Filename: modelclass.cpp
////////////////////////////////////////////////////////////////////////////////
#include "modelclass.h"


ModelClass::ModelClass()
{
	m_vertexBuffer = 0;
	m_indexBuffer = 0;
	m_vertexUpload = 0;
	m_indexUpload = 0;
	m_vertexCount = 0;
	m_indexCount = 0;
}


ModelClass::ModelClass(const ModelClass& other)
{
}


ModelClass::~ModelClass()
{
}


bool ModelClass::Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* cmd)
{
	bool result;


	// Initialize the vertex and index buffers.
	result = InitializeBuffers(device, cmd);
	if(!result)
	{
		return false;
	}

	return true;
}


void ModelClass::Shutdown()
{
	// Shutdown the vertex and index buffers.
	ShutdownBuffers();

	return;
}


void ModelClass::Render(ID3D12GraphicsCommandList* cmd)
{
	// Put the vertex and index buffers on the graphics pipeline to prepare them for drawing.
	RenderBuffers(cmd);

	return;
}


int ModelClass::GetIndexCount()
{
	return m_indexCount;
}


bool ModelClass::InitializeBuffers(ID3D12Device* device, ID3D12GraphicsCommandList* cmd)
{
	VertexType* vertices;
	unsigned long* indices;
	D3D12_HEAP_DESC heapDesc;
	D3D12_RESOURCE_DESC bufferDesc;
	DXGI_SAMPLE_DESC sDesc;
	HRESULT result;
	D3D12_RESOURCE_BARRIER barrier = {};
	// Set the number of vertices in the vertex array.
	m_vertexCount = 3;

	// Set the number of indices in the index array.
	m_indexCount = 3;

	// Create the vertex array.
	vertices = new VertexType[m_vertexCount];
	if(!vertices)
	{
		return false;
	}

	// Create the index array.
	indices = new unsigned long[m_indexCount];
	if(!indices)
	{
		return false;
	}

	// Load the vertex array with data.
	vertices[0].position = XMFLOAT3(-1.0f, -1.0f, 0.0f);  // Bottom left.
	vertices[0].color = XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f);

	vertices[1].position = XMFLOAT3(0.0f, 1.0f, 0.0f);  // Top middle.
	vertices[1].color = XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f);

	vertices[2].position = XMFLOAT3(1.0f, -1.0f, 0.0f);  // Bottom right.
	vertices[2].color = XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f);

	// Load the index array with data.
	indices[0] = 0;  // Bottom left.
	indices[1] = 1;  // Top middle.
	indices[2] = 2;  // Bottom right.

	// Set up the description of the upload heap.
	heapDesc.SizeInBytes = ((sizeof(VertexType) * m_vertexCount)/65536+1)*65536 + sizeof(unsigned long)* m_indexCount;//we will put both vertex and index buffer together on the upload heap, so we need to align the data to 64k bytes
	heapDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
	heapDesc.Properties.Type = D3D12_HEAP_TYPE_CUSTOM;//We need a customized heap for the resources
	heapDesc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_COMBINE;
	heapDesc.Properties.CreationNodeMask = 0;
	heapDesc.Properties.VisibleNodeMask = 0;
	heapDesc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;//Use system memory
	heapDesc.Flags = D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;

	//Create the heap for our vertex buffer
	result = device->CreateHeap(&heapDesc, __uuidof(ID3D12Heap), (void**)&m_heapUpload);
	if (FAILED(result))
	{
		return false;
	}
	//Setup the description for the sample desc
	sDesc.Count = 1;
	sDesc.Quality = 0;
	//Setup the description for the vertex buffer
	bufferDesc.Alignment = 65536;//alignment to 64k for more than one resource on the heap
	bufferDesc.DepthOrArraySize = 1;
	bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	bufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
	bufferDesc.Height = 1;
	bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	bufferDesc.MipLevels = 1;
	bufferDesc.SampleDesc = sDesc;
	bufferDesc.Width = sizeof(VertexType) * m_vertexCount;
	
	//Now create the resources
	result = device->CreatePlacedResource(m_heapUpload, 0, &bufferDesc
		, D3D12_RESOURCE_STATE_GENERIC_READ
		, nullptr
		, IID_PPV_ARGS(&(m_vertexUpload)));
	if (FAILED(result))
	{
		return false;
	}
	bufferDesc.Alignment = 0;
	bufferDesc.Width = sizeof(unsigned long) * m_indexCount;//Set size to index buffer size
	result = device->CreatePlacedResource(m_heapUpload
		, 65536//offset for 64k alignment
		, &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr
		, IID_PPV_ARGS(&(m_indexUpload)));
	if (FAILED(result))
	{
		return false;
	}
	//Change the description for a default heap on GPU side
	heapDesc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_NOT_AVAILABLE;//CPU cannot get access to the VRAM
	heapDesc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL_L1;//L1 for VRam

	//Now create the default heap and buffers
	result = device->CreateHeap(&heapDesc, __uuidof(ID3D12Heap), (void**)&m_heapDefault);
	if (FAILED(result))
	{
		return false;
	}
	//Now create the resources
	bufferDesc.Alignment = 0;
	bufferDesc.Width = sizeof(VertexType) * m_vertexCount;//Set size to index buffer size
	result = device->CreatePlacedResource(m_heapDefault, 0, &bufferDesc
		, D3D12_RESOURCE_STATE_COPY_DEST
		, nullptr
		, IID_PPV_ARGS(&(m_vertexBuffer)));
	if (FAILED(result))
	{
		return false;
	}
	bufferDesc.Alignment = 0;//This is the last resource, then set alignment to 0
	bufferDesc.Width = sizeof(unsigned long) * m_indexCount;//Set size to index buffer size
	result = device->CreatePlacedResource(m_heapDefault
		, 65536//offset for 64k alignment
		, &bufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr
		, IID_PPV_ARGS(&(m_indexBuffer)));
	if (FAILED(result))
	{
		return false;
	}
	//Copy the vertex and index data to the buffers
	//vertices can only go into upload resource
	//[begin] copy vertices
	void* pvBegin = nullptr;
	m_vertexUpload->Map(0, nullptr, &pvBegin);
	memcpy(pvBegin, vertices, sizeof(VertexType) * m_vertexCount);
	m_vertexUpload->Unmap(0, nullptr);
	//[end] copy vertices

	//[begin] copy indices
	m_indexUpload->Map(0, nullptr, &pvBegin);
	memcpy(pvBegin, indices, sizeof(unsigned long) * m_indexCount);
	m_indexUpload->Unmap(0, nullptr);
	//[end] copy indices
	//Copy to the default heap
	//barrier.Aliasing;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = m_vertexBuffer;
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	//barrier.UAV;
	cmd->CopyBufferRegion(m_vertexBuffer, 0, m_vertexUpload, 0, sizeof(VertexType) * m_vertexCount);
	cmd->ResourceBarrier(1, &barrier);
	cmd->CopyBufferRegion(m_indexBuffer, 0, m_indexUpload, 0, sizeof(unsigned long) * m_indexCount);
	barrier.Transition.pResource = m_indexBuffer;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_INDEX_BUFFER;
	cmd->ResourceBarrier(1, &barrier);
	//Set up the views
	// Release the arrays now that the vertex and index buffers have been created and loaded.
	delete [] vertices;
	vertices = 0;

	delete [] indices;
	indices = 0;

	return true;
}


void ModelClass::ShutdownBuffers()
{
	// Release the index buffer.
	if(m_indexBuffer)
	{
		m_indexBuffer->Release();
		m_indexBuffer = 0;
	}

	// Release the vertex buffer.
	if(m_vertexBuffer)
	{
		m_vertexBuffer->Release();
		m_vertexBuffer = 0;
	}
	if(m_vertexUpload)
	{
		m_vertexUpload->Release();
		m_vertexUpload = 0;
	}
	if(m_indexUpload)
	{
		m_indexUpload->Release();
		m_indexUpload = 0;
	}
	//Release the heap
	if (m_heapUpload)
	{
		m_heapUpload->Release();
		m_heapUpload = 0;
	}
	//Release the heap
	if (m_heapDefault)
	{
		m_heapDefault->Release();
		m_heapDefault = 0;
	}
	
	return;
}


void ModelClass::RenderBuffers(ID3D12GraphicsCommandList* cmd)
{
	D3D12_VERTEX_BUFFER_VIEW vbView;
	D3D12_INDEX_BUFFER_VIEW idView;
	// Set vertex buffer stride and offset.
	vbView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
	vbView.SizeInBytes = sizeof(VertexType) * m_vertexCount;
	vbView.StrideInBytes = sizeof(VertexType);
	// Set vertex buffer stride and offset.
	idView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
	idView.Format = DXGI_FORMAT_R32_UINT;
	idView.SizeInBytes = sizeof(unsigned long) * m_indexCount;
	// Set the vertex buffer to active in the input assembler so it can be rendered.
	cmd->IASetVertexBuffers(0, 1, &vbView);

    // Set the index buffer to active in the input assembler so it can be rendered.
	cmd->IASetIndexBuffer(&idView);

    // Set the type of primitive that should be rendered from this vertex buffer, in this case triangles.
	cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	return;
}