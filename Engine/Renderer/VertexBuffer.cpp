#include "pch.h"
#include "VertexBuffer.h"

namespace nu
{
	VertexBuffer::~VertexBuffer()
	{
		// release the buffer
		if (m_gpuBuffer && m_gpuDevice)
		{
			SDL_ReleaseGPUBuffer(m_gpuDevice, m_gpuBuffer);
		}
	}

	bool VertexBuffer::Create(uint32_t vertexCount, uint32_t vertexSize, const uint8_t* data, SDL_GPUDevice* gpuDevice)
	{
		m_vertexCount = vertexCount;
		m_gpuDevice = gpuDevice;

		uint32_t verticesSize = vertexCount * vertexSize;

		SDL_GPUBufferCreateInfo vertexBufferCreateInfo = SDL_GPUBufferCreateInfo{
			.usage = SDL_GPU_BUFFERUSAGE_VERTEX,
			.size = verticesSize,
		};

		m_gpuBuffer = SDL_CreateGPUBuffer(gpuDevice, &vertexBufferCreateInfo);
		if (m_gpuBuffer == nullptr)
		{
			std::cerr << "Could not create vertex buffer: " << SDL_GetError() << std::endl;

			return false;
		}

		SDL_GPUTransferBufferCreateInfo transferBufferCreateInfo = SDL_GPUTransferBufferCreateInfo{
			.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
			.size = verticesSize,
		};
		SDL_GPUTransferBuffer* transferBuffer = SDL_CreateGPUTransferBuffer(gpuDevice, &transferBufferCreateInfo);
		if (transferBuffer == nullptr)
		{
			std::cerr << "Could not create transfer buffer: " << SDL_GetError() << std::endl;
			
			SDL_ReleaseGPUBuffer(gpuDevice, m_gpuBuffer);
			m_gpuBuffer = nullptr;

			return false;
		}

		uint8_t* transferData = static_cast<uint8_t*>(SDL_MapGPUTransferBuffer(gpuDevice, transferBuffer, false));
		if (transferData == nullptr)
		{
			std::cerr << "Could not map transfer buffer: " << SDL_GetError() << std::endl;

			SDL_ReleaseGPUTransferBuffer(gpuDevice, transferBuffer);
			SDL_ReleaseGPUBuffer(gpuDevice, m_gpuBuffer);
			m_gpuBuffer = nullptr;

			return false;
		}

		memcpy(transferData, data, verticesSize);
		SDL_UnmapGPUTransferBuffer(gpuDevice, transferBuffer);

		// upload the transfer data to the vertex buffer
		SDL_GPUCommandBuffer* commandBuffer = SDL_AcquireGPUCommandBuffer(gpuDevice);
		if (commandBuffer == nullptr)
		{
			std::cerr << "Could not acquire GPU command buffer: " << SDL_GetError() << std::endl;

			SDL_ReleaseGPUTransferBuffer(gpuDevice, transferBuffer);
			SDL_ReleaseGPUBuffer(gpuDevice, m_gpuBuffer);
			m_gpuBuffer = nullptr;

			return false;
		}

		SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(commandBuffer);

		SDL_GPUTransferBufferLocation bufferLocation = SDL_GPUTransferBufferLocation{
			.transfer_buffer = transferBuffer,
			.offset = 0,
		};

		SDL_GPUBufferRegion bufferRegion = SDL_GPUBufferRegion{
			.buffer = m_gpuBuffer,
			.offset = 0,
			.size = verticesSize,
		};

		SDL_UploadToGPUBuffer(copyPass, &bufferLocation, &bufferRegion, false);

		SDL_EndGPUCopyPass(copyPass);
		if (!SDL_SubmitGPUCommandBuffer(commandBuffer))
		{
			std::cerr << "Could not submit GPU command buffer: " << SDL_GetError() << std::endl;

			SDL_ReleaseGPUTransferBuffer(gpuDevice, transferBuffer);
			SDL_ReleaseGPUBuffer(gpuDevice, m_gpuBuffer);
			m_gpuBuffer = nullptr;

			return false;
		}

		SDL_ReleaseGPUTransferBuffer(gpuDevice, transferBuffer);

		return true;
	}

}
