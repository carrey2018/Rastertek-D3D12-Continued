////////////////////////////////////////////////////////////////////////////////
// Filename: textureshaderclass.cpp
////////////////////////////////////////////////////////////////////////////////
#include "TextureShaderClass.h"

TextureShaderClass::TextureShaderClass()
{
	m_matrixBuffer = 0;
	m_srvcbvHeap = 0;
	m_prtSignature = 0;
	m_pipelineState = 0;
	m_smplHeap = 0;
	m_comTexture = 0;
}

TextureShaderClass::TextureShaderClass(const TextureShaderClass& other)
{
}

TextureShaderClass::~TextureShaderClass()
{
}

bool TextureShaderClass::Initialize(ID3D12Device* device, HWND hwnd, ID3D12Resource* texture)
{
	bool result;

	//store the texture interface for future use
	m_comTexture = texture;

	// Initialize the vertex and pixel shaders.
	result = InitializeShader(device, hwnd, L"texture.hlsl");
	if (!result)
	{
		return false;
	}

	return true;
}

void TextureShaderClass::Shutdown()
{
	// Shutdown the vertex and pixel shaders as well as the related objects.
	ShutdownShader();

	return;
}

ID3D12RootSignature* TextureShaderClass::GetSignature()
{
	return m_prtSignature;
}

ID3D12PipelineState* TextureShaderClass::GetPSO()
{
	return m_pipelineState;
}


bool TextureShaderClass::Render(ID3D12GraphicsCommandList* cmd, int indexCount, XMMATRIX worldMatrix, XMMATRIX viewMatrix,
	XMMATRIX projectionMatrix)
{
	bool result;


	// Set the shader parameters that it will use for rendering.
	result = SetShaderParameters(cmd, worldMatrix, viewMatrix, projectionMatrix);
	if (!result)
	{
		return false;
	}

	// Now render the prepared buffers with the shader.
	RenderShader(cmd, indexCount);

	return true;
}


bool TextureShaderClass::InitializeShader(ID3D12Device* device, HWND hwnd, const WCHAR* filename)
{
	HRESULT result;
	ID3DBlob* errorMessage;
	ID3DBlob* vertexShaderBuffer;
	ID3DBlob* pixelShaderBuffer;
	ID3DBlob* pSignatureBlob;
	D3D12_INPUT_ELEMENT_DESC polygonLayout[2];
	unsigned int numElements;
	D3D12_HEAP_PROPERTIES heapp = {};
	D3D12_RESOURCE_DESC bufferDesc;
	DXGI_SAMPLE_DESC sDesc;
	D3D12_DESCRIPTOR_HEAP_DESC srvcbvHeapDesc;
	D3D12_DESCRIPTOR_HEAP_DESC smplHeapDesc;//New heap desc for the sampler
	D3D12_DESCRIPTOR_RANGE1 cbvRange[3];//New ranges for 3 descs
	D3D12_ROOT_DESCRIPTOR_TABLE1 descTable;
	D3D12_ROOT_PARAMETER1  rootParameters[3];//New parameters for three descs
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};//New desc for the texture resource
	D3D12_CONSTANT_BUFFER_VIEW_DESC matrixBufferDesc = {};
	D3D12_SAMPLER_DESC smplDesc = {};

	D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};//decide version
	D3D12_VERSIONED_ROOT_SIGNATURE_DESC vrtSignatureDesc = {};
	D3D12_ROOT_SIGNATURE_DESC1 rtSignatureDesc = {};
	D3D12_GRAPHICS_PIPELINE_STATE_DESC PSODesc = {};

	// Initialize the pointers this function will use to null.
	errorMessage = 0;
	vertexShaderBuffer = 0;
	pixelShaderBuffer = 0;
	pSignatureBlob = 0;

	// Compile the vertex shader code.
	result = D3DCompileFromFile(filename, NULL, NULL, "TextureVertexShader", "vs_5_0", 0, 0, &vertexShaderBuffer, &errorMessage);
	if (FAILED(result))
	{
		// If the shader failed to compile it should have writen something to the error message.
		if (errorMessage)
		{
			OutputShaderErrorMessage(errorMessage, hwnd, filename);
		}
		// If there was  nothing in the error message then it simply could not find the shader file itself.
		else
		{
			MessageBox(hwnd, filename, L"Missing Shader File", MB_OK);
		}

		return false;
	}

	// Compile the pixel shader code.
	result = D3DCompileFromFile(filename, NULL, NULL, "TexturePixelShader", "ps_5_0", 0, 0, &pixelShaderBuffer, &errorMessage);
	if (FAILED(result))
	{
		// If the shader failed to compile it should have writen something to the error message.
		if (errorMessage)
		{
			OutputShaderErrorMessage(errorMessage, hwnd, filename);
		}
		// If there was nothing in the error message then it simply could not find the file itself.
		else
		{
			MessageBox(hwnd, filename, L"Missing Shader File", MB_OK);
		}

		return false;
	}

	// Create the vertex input layout description.
	// This setup needs to match the VertexType stucture in the ModelClass and in the shader.
	polygonLayout[0].SemanticName = "POSITION";
	polygonLayout[0].SemanticIndex = 0;
	polygonLayout[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	polygonLayout[0].InputSlot = 0;
	polygonLayout[0].AlignedByteOffset = 0;
	polygonLayout[0].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
	polygonLayout[0].InstanceDataStepRate = 0;

	polygonLayout[1].SemanticName = "TEXCOORD";
	polygonLayout[1].SemanticIndex = 0;
	polygonLayout[1].Format = DXGI_FORMAT_R32G32_FLOAT;
	polygonLayout[1].InputSlot = 0;
	polygonLayout[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	polygonLayout[1].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
	polygonLayout[1].InstanceDataStepRate = 0;


	// Get a count of the elements in the layout.
	numElements = sizeof(polygonLayout) / sizeof(polygonLayout[0]);

	//Fill up a description for the constant buffer
	heapp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapp.CreationNodeMask = 1;
	heapp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heapp.Type = D3D12_HEAP_TYPE_UPLOAD;
	heapp.VisibleNodeMask = 1;

	//Setup the description for the sample desc
	sDesc.Count = 1;
	sDesc.Quality = 0;
	//Setup the description for the vertex buffer
	bufferDesc.Alignment = 0;//for constant buffer we set it to 0
	bufferDesc.DepthOrArraySize = 1;
	bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	bufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
	bufferDesc.Height = 1;
	bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	bufferDesc.MipLevels = 1;
	bufferDesc.SampleDesc = sDesc;
	bufferDesc.Width = (sizeof(MatrixBufferType) / 256 + 1) * 256;

	// Create the constant buffer pointer so we can access the vertex shader constant buffer from within this class.
	result = device->CreateCommittedResource(&heapp, D3D12_HEAP_FLAG_NONE,
		&bufferDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_matrixBuffer));
	if (FAILED(result))
	{
		return false;
	}

	// Lock the constant buffer so it can be written to.
	m_matrixBuffer->Map(0, nullptr, &m_pDataPtr);

	// Now describe our descriptor heap
	srvcbvHeapDesc.NumDescriptors = 2;//srv+cbv, now we have 2 descriptors for constant and texture
	srvcbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvcbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	srvcbvHeapDesc.NodeMask = 0;

	//Create a new descriptor heap for both the shader resource (texture) view SRV and the constant buffer view CBV
	result = device->CreateDescriptorHeap(&srvcbvHeapDesc, IID_PPV_ARGS(&m_srvcbvHeap));
	if (FAILED(result))
	{
		return false;
	}

	//Set up the SRV desc, we only need rgba integer format for our texture
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;//rgba integer
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;

	//Now we can create the view on the desc heap
	device->CreateShaderResourceView(m_comTexture, &srvDesc, m_srvcbvHeap->GetCPUDescriptorHandleForHeapStart());//We don't need an offset for the first view on the heap
	
	// Setup the description of the dynamic matrix constant buffer that is in the vertex shader.
	matrixBufferDesc.BufferLocation = m_matrixBuffer->GetGPUVirtualAddress();
	matrixBufferDesc.SizeInBytes = (sizeof(MatrixBufferType) / 256 + 1) * 256;//Must be rounded up to 256 or the device will be removed!

	//Calculate the offset of the desc on the heap
	UINT offset = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	D3D12_CPU_DESCRIPTOR_HANDLE handle;
	handle.ptr = m_srvcbvHeap->GetCPUDescriptorHandleForHeapStart().ptr + offset;

	//Create the view on the heap
	device->CreateConstantBufferView(&matrixBufferDesc, handle);

	//Set up the sampler heap
	smplHeapDesc.NodeMask = 0;//Node mask represents the GPU no.
	smplHeapDesc.NumDescriptors = 1;//We only set one sample for simplicity in this tutorial
	smplHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
	smplHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	//Create a desc heap for the sampler
	device->CreateDescriptorHeap(&smplHeapDesc, IID_PPV_ARGS(&m_smplHeap));

	//Set up the sampler
	smplDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	smplDesc.MinLOD = 0;
	smplDesc.MaxLOD = D3D12_FLOAT32_MAX;
	smplDesc.MipLODBias = 0.0f;
	smplDesc.MaxAnisotropy = 1;
	smplDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	smplDesc.BorderColor[0] = 0.0f;
	smplDesc.BorderColor[1] = 0.0f;
	smplDesc.BorderColor[2] = 0.0f;
	smplDesc.BorderColor[3] = 0.0f;
	smplDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	smplDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	smplDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;

	//Create the sampler
	device->CreateSampler(&smplDesc, m_smplHeap->GetCPUDescriptorHandleForHeapStart());//We have only one sampler on the heap so we don't need offset
	
	//Set up the srv cbv range
	for (UINT i = 0; i < 3; ++i)
	{
		cbvRange[i].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
		cbvRange[i].NumDescriptors = 1;
		cbvRange[i].BaseShaderRegister = 0;
		cbvRange[i].RegisterSpace = 0;
		cbvRange[i].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
		cbvRange[i].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE;
	}
	cbvRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	cbvRange[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
	cbvRange[2].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;

	//Fill srv cbv desc table
	descTable.NumDescriptorRanges = 1;
	
	//Fill in root parameters
	for(UINT i = 0; i<3; ++i)
	{
		descTable.pDescriptorRanges = &cbvRange[i];
		rootParameters[i].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE; // this is a descriptor table type, not a constant
		rootParameters[i].DescriptorTable = descTable; // this is our descriptor table for this root parameter
		rootParameters[i].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL; // our VERTEX shader will be the only shader accessing this parameter for now
	}

	//Decide the highest version
	featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
	result = device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData));
	if (FAILED(result))
	{
		featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
	}

	//Fill up root signature desc
	rtSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	rtSignatureDesc.NumParameters = 3;
	rtSignatureDesc.NumStaticSamplers = 0;
	rtSignatureDesc.pParameters = rootParameters;
	rtSignatureDesc.pStaticSamplers = nullptr;

	vrtSignatureDesc.Desc_1_1 = rtSignatureDesc;
	vrtSignatureDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;

	//Compile root signature
	result = D3DX12SerializeVersionedRootSignature(&vrtSignatureDesc, featureData.HighestVersion, &pSignatureBlob, &errorMessage);
	if (FAILED(result))
	{
		return false;
	}
	result = device->CreateRootSignature(0, pSignatureBlob->GetBufferPointer(), pSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&m_prtSignature));
	if (FAILED(result))
	{
		return false;
	}

	//Build PSO
	PSODesc.InputLayout = { polygonLayout, numElements };
	PSODesc.pRootSignature = m_prtSignature;
	PSODesc.VS = CD3DX12_SHADER_BYTECODE(vertexShaderBuffer);
	PSODesc.PS = CD3DX12_SHADER_BYTECODE(pixelShaderBuffer);
	PSODesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	PSODesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	PSODesc.DepthStencilState.DepthEnable = FALSE;
	PSODesc.DepthStencilState.StencilEnable = FALSE;
	PSODesc.SampleMask = UINT_MAX;
	PSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	PSODesc.NumRenderTargets = 1;
	PSODesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	PSODesc.SampleDesc.Count = 1;
	result = device->CreateGraphicsPipelineState(&PSODesc, IID_PPV_ARGS(&m_pipelineState));
	if (FAILED(result))
	{
		return false;
	}
	if (errorMessage)
	{
		errorMessage->Release();
		errorMessage = 0;
	}
	if (vertexShaderBuffer)
	{
		vertexShaderBuffer->Release();
		vertexShaderBuffer = 0;
	}
	if (pixelShaderBuffer)
	{
		pixelShaderBuffer->Release();
		pixelShaderBuffer = 0;
	}
	if (pSignatureBlob)
	{
		pSignatureBlob->Release();
		pSignatureBlob = 0;
	}
	return true;
}


void TextureShaderClass::ShutdownShader()
{
	// Release the matrix constant buffer.
	if (m_matrixBuffer)
	{
		// Unlock the constant buffer.
		m_matrixBuffer->Unmap(0, nullptr);
		m_matrixBuffer->Release();
		m_matrixBuffer = 0;
	}

	// Release the desc heap.
	if (m_srvcbvHeap)
	{
		m_srvcbvHeap->Release();
		m_srvcbvHeap = 0;
	}

	// Release the sampler desc heap
	if (m_smplHeap)
	{
		m_smplHeap->Release();
		m_smplHeap = 0;
	}
	// Release the signature.
	if (m_prtSignature)
	{
		m_prtSignature->Release();
		m_prtSignature = 0;
	}

	// Release the PSO.
	if (m_pipelineState)
	{
		m_pipelineState->Release();
		m_pipelineState = 0;
	}

	return;
}


void TextureShaderClass::OutputShaderErrorMessage(ID3DBlob* errorMessage, HWND hwnd, const WCHAR* shaderFilename)
{
	char* compileErrors;
	unsigned long bufferSize, i;
	ofstream fout;


	// Get a pointer to the error message text buffer.
	compileErrors = (char*)(errorMessage->GetBufferPointer());

	// Get the length of the message.
	bufferSize = errorMessage->GetBufferSize();

	// Open a file to write the error message to.
	fout.open("shader-error.txt");

	// Write out the error message.
	for (i = 0; i < bufferSize; i++)
	{
		fout << compileErrors[i];
	}

	// Close the file.
	fout.close();

	// Release the error message.
	errorMessage->Release();
	errorMessage = 0;

	// Pop a message up on the screen to notify the user to check the text file for compile errors.
	MessageBox(hwnd, L"Error compiling shader.  Check shader-error.txt for message.", shaderFilename, MB_OK);

	return;
}


bool TextureShaderClass::SetShaderParameters(ID3D12GraphicsCommandList* cmd, XMMATRIX worldMatrix, XMMATRIX viewMatrix,
	XMMATRIX projectionMatrix)
{
	MatrixBufferType cbData;

	// Transpose the matrices to prepare them for the shader.
	cbData.world = XMMatrixTranspose(worldMatrix);
	cbData.view = XMMatrixTranspose(viewMatrix);
	cbData.projection = XMMatrixTranspose(projectionMatrix);

	// Copy the matrices into the constant buffer.
	memcpy(m_pDataPtr, &cbData, sizeof(MatrixBufferType));

	return true;
}


void TextureShaderClass::RenderShader(ID3D12GraphicsCommandList* cmd, int indexCount)
{
	// Set the texture.
	cmd->SetDescriptorHeaps(1, &m_srvcbvHeap);
	cmd->SetGraphicsRootDescriptorTable(0, m_srvcbvHeap->GetGPUDescriptorHandleForHeapStart());
	
	//Set the constant
	D3D12_GPU_DESCRIPTOR_HANDLE handle;
	handle.ptr = m_srvcbvHeap->GetGPUDescriptorHandleForHeapStart().ptr + 32;
	cmd->SetDescriptorHeaps(1, &m_srvcbvHeap);
	cmd->SetGraphicsRootDescriptorTable(1, handle);//offset 1 for the 2nd element in the table

	//Set sampler
	cmd->SetDescriptorHeaps(1, &m_smplHeap);
	cmd->SetGraphicsRootDescriptorTable(2, m_smplHeap->GetGPUDescriptorHandleForHeapStart());//offset 2 for the 3rd element in the table

	// Render the triangle.
	cmd->DrawIndexedInstanced(indexCount, 1, 0, 0, 0);

	return;
}
