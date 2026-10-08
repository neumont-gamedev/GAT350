#include "pch.h"
#include "Shader.h"
#include "Renderer.h"
#include "Core/File.h"

namespace nu
{
	Shader::~Shader()
	{
		if (m_gpuDevice && m_gpuShader)
		{
			SDL_ReleaseGPUShader(m_gpuDevice, m_gpuShader);
		}
	}

	bool Shader::Load(const std::string& filename, Renderer& renderer)
	{
		m_gpuDevice = renderer.m_gpuDevice;

		SDL_GPUShaderStage stage;
		if (filename.find(".vert") != std::string::npos)
		{
			stage = SDL_GPU_SHADERSTAGE_VERTEX;
		}
		else if (filename.find(".frag") != std::string::npos)
		{
			stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
		}
		else
		{
			std::cerr << "Unknown shader file type " << filename << std::endl;
			return false;
		}

		SDL_GPUShaderFormat format = SDL_GPU_SHADERFORMAT_INVALID;
		const char* entrypoint = "main";

		std::string shaderFilename = filename;
		SDL_GPUShaderFormat backendFormats = SDL_GetGPUShaderFormats(renderer.m_gpuDevice);
		if (backendFormats & SDL_GPU_SHADERFORMAT_SPIRV)
		{
			shaderFilename += ".spv";
			format = SDL_GPU_SHADERFORMAT_SPIRV;
		}
		else if (backendFormats & SDL_GPU_SHADERFORMAT_DXIL)
		{
			shaderFilename += ".dxil";
			format = SDL_GPU_SHADERFORMAT_DXIL;
		}
		else
		{
			std::cerr << "Could not find supported shader format: " << shaderFilename << std::endl;
			return false;
		}

		auto byte = ReadBinaryFile(shaderFilename);
		if (byte.empty())
		{
			std::cerr << "Could not read shader: " << shaderFilename << std::endl;
			return false;
		}

		SDL_GPUShaderCreateInfo shaderInfo = SDL_GPUShaderCreateInfo{
			.code_size = byte.size(),
			.code = byte.data(),
			.entrypoint = entrypoint,
			.format = format,
			.stage = stage,
		};

		m_gpuShader = SDL_CreateGPUShader(renderer.m_gpuDevice, &shaderInfo);
		if (m_gpuShader == nullptr)
		{
			std::cerr << "Could not create shader: " << shaderFilename << std::endl;
			return false;
		}

		return true;
	}
}
