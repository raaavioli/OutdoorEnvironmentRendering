#pragma once

#include <string>
#include <fstream>
#include <iostream>
#include <map>
#include <filesystem>

#include "texture.h"
#include "model.h"
#include "shader.h"

class AssetManager
{
public:
	AssetManager(AssetManager const&) = delete;
	void operator=(AssetManager const&) = delete;

	static void Init();
	static void Destroy();

	static AssetManager& Instance()
	{
		static AssetManager instance;
		return instance;
	}

	static Texture2D* GetTexture2D(const char* file_name);
	static Texture2D* LoadTextureFromFileSystem();

	static TextureCubeMap* GetTextureCubeMap(const char* filename, bool folder);

	static RawModel* GetRawModel(const char* file_name);

	static Shader* GetShader(const char* file_name);

	static std::string ReadFile(std::filesystem::path file_path);

	/*
	* Reloads all shaders from source avaliable in /../build/assets/shaders
	*
	* NOTE: Currently the project has to be rebuilt manually (by running cmake) for the shaders to
	* be loaded into /../build/assets/shaders from the source asset folder.
	*/
	static void ReloadShaders();

	// TODO: Implement
	// static void ReloadModels();

	// TODO: Implement
	// static void ReloadTextures();



private:
	AssetManager() {};

	static bool ParseFBX(const std::filesystem::path& file_path, ModelData& output);
	static bool ParseShader(const std::filesystem::path& file_path, std::map<GLuint, std::string>& output_sources);

	std::map<std::string, TextureCubeMap*> m_CubeMaps;
	std::map<std::string, Texture2D*> m_Textures;
	std::map<std::string, RawModel*> m_Models;
	std::map<std::string, Shader*> m_Shaders;

	static std::filesystem::path GetBasePath()		{ return Instance().base_path; };
	static std::filesystem::path GetAssetPath()		{ return Instance().asset_path; };
	static std::filesystem::path GetModelPath()		{ return Instance().model_path; };
	static std::filesystem::path GetTexturePath()	{ return Instance().texture_path; };
	static std::filesystem::path GetShaderPath()	{ return Instance().shader_path; };

	std::filesystem::path base_path;
	std::filesystem::path asset_path;
	std::filesystem::path model_path;
	std::filesystem::path texture_path;
	std::filesystem::path shader_path;
};