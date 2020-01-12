////////////////////////////////////////////////////////////////////////////////
// Filename: colorshaderclass.cpp
////////////////////////////////////////////////////////////////////////////////
#include "colorshaderclass.h"


ColorShaderClass::ColorShaderClass()
{
	m_matrixBuffer = 0;
	m_cbvHeap = 0;
	m_prtSignature = 0;
	m_pipelineState = 0;
}


ColorShaderClass::ColorShaderClass(const ColorShaderClass& other)
{
}


ColorShaderClass::~ColorShaderClass()
{
}


bool ColorShaderClass::Initialize(ID3D12Device* device, HWND hwnd)
{
	bool result;

	// Initialize the vertex and pixel shaders.
	result = InitializeShader(device, hwnd, L"color.hlsl");
	if(!result)
	{
		return false;
	}

	return true;
}


void ColorShaderClass::Shutdown()
{
	// Shutdown the vertex and pixel shaders as well as the related objects.
	ShutdownShader();

	return;
}

ID3D12RootSignature* ColorShaderClass::GetSignature()
{
	return m_prtSignature;
}

ID3D12PipelineState* ColorShaderClass::GetPSO()
{
	return m_pipelineState;
}


bool ColorShaderClass::Render(ID3D12GraphicsCommandList* cmd, int indexCount, XMMATRIX worldMatrix, XMMATRIX viewMatrix,
							  XMMATRIX projectionMatrix)
{
	bool result;


	// Set the shader parameters that it will use for rendering.
	result = SetShaderParameters(cmd, worldMatrix, viewMatrix, projectionMatrix);
	if(!result)
	{
		return false;
	}

	// Now render the prepared buffers with the shader.
	RenderShader(cmd, indexCount);

	return true;
}


bool ColorShaderClass::InitializeShader(ID3D12Device * device, HWND hwnd, const WCHAR* filename)
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
	D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc;
	D3D12_DESCRIPTOR_RANGE1 cbvRange;
	D3D12_ROOT_DESCRIPTOR_TABLE1 descTable;
	D3D12_ROOT_PARAMETER1  rootParameters;
	D3D12_CONSTANT_BUFFER_VIEW_DESC matrixBufferDesc = {};

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
	result = D3DCompileFromFile(filename, NULL, NULL, "ColorVertexShader", "vs_5_0", 0, 0, &vertexShaderBuffer, &errorMessage);
	if(FAILED(result))
	{
		// If the shader failed to compile it should have writen something to the error message.
		if(errorMessage)
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
	result = D3DCompileFromFile(filename, NULL, NULL, "ColorPixelShader", "ps_5_0", 0, 0, &pixelShaderBuffer, &errorMessage);
	if(FAILED(result))
	{
		// If the shader failed to compile it should have writen something to the error message.
		if(errorMessage)
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

	polygonLayout[1].SemanticName = "COLOR";
	polygonLayout[1].SemanticIndex = 0;
	polygonLayout[1].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
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
	bufferDesc.Width = (sizeof(MatrixBufferType)/256+1)*256;

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
	
	cbvHeapDesc.NumDescriptors = 1;//only for cbv, so there is only 1
	cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	cbvHeapDesc.NodeMask = 0;
	result = device->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&m_cbvHeap));
	if (FAILED(result))
	{
		return false;
	}

	// Setup the description of the dynamic matrix constant buffer that is in the vertex shader.
	matrixBufferDesc.BufferLocation = m_matrixBuffer->GetGPUVirtualAddress();
	matrixBufferDesc.SizeInBytes = (sizeof(MatrixBufferType) / 256 + 1) * 256;//Must be rounded up to 256 or the device will be removed!
	device->CreateConstantBufferView(&matrixBufferDesc, m_cbvHeap->GetCPUDescriptorHandleForHeapStart());

	//Set up the cbv range
	cbvRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	cbvRange.NumDescriptors = 1;
	cbvRange.BaseShaderRegister = 0;
	cbvRange.RegisterSpace = 0;
	cbvRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	cbvRange.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;

	//Fill desc table
	descTable.NumDescriptorRanges = 1;
	descTable.pDescriptorRanges = &cbvRange;

	//Fill in root parameters
	rootParameters.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE; // this is a descriptor table type, not a constant
	rootParameters.DescriptorTable = descTable; // this is our descriptor table for this root parameter
	rootParameters.ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX; // our VERTEX shader will be the only shader accessing this parameter for now

	//Decide the highest version
	featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
	result = device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData));
	if (FAILED(result))
	{
		featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
	}

	//Fill up root signature desc
	rtSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	rtSignatureDesc.NumParameters = 1;
	rtSignatureDesc.NumStaticSamplers = 0;
	rtSignatureDesc.pParameters = &rootParameters;
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


void ColorShaderClass::ShutdownShader()
{
	// Release the matrix constant buffer.
	if (m_matrixBuffer)
	{
		// Unlock the constant buffer.
		m_matrixBuffer->Unmap(0, nullptr);
		m_matrixBuffer->Release();
		m_matrixBuffer = 0;
	}

	// Release the layout.
	if(m_cbvHeap)
	{
		m_cbvHeap->Release();
		m_cbvHeap = 0;
	}

	// Release the pixel shader.
	if(m_prtSignature)
	{
		m_prtSignature->Release();
		m_prtSignature = 0;
	}

	// Release the vertex shader.
	if(m_pipelineState)
	{
		m_pipelineState->Release();
		m_pipelineState = 0;
	}

	return;
}


void ColorShaderClass::OutputShaderErrorMessage(ID3DBlob* errorMessage, HWND hwnd, const WCHAR* shaderFilename)
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
	for(i=0; i<bufferSize; i++)
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


bool ColorShaderClass::SetShaderParameters(ID3D12GraphicsCommandList* cmd, XMMATRIX worldMatrix, XMMATRIX viewMatrix,
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


void ColorShaderClass::RenderShader(ID3D12GraphicsCommandList* cmd, int indexCount)
{
	// Set the vertex input layout.
	cmd->SetDescriptorHeaps(1, &m_cbvHeap);
	cmd->SetGraphicsRootDescriptorTable(0, m_cbvHeap->GetGPUDescriptorHandleForHeapStart());
    
	// Render the triangle.
	cmd->DrawIndexedInstanced(indexCount, 1, 0, 0, 0);

	return;
}