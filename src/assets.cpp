#include "assets.h"

#include <windows.h>
#include <ShObjIdl_core.h>

#include <OpenFBX/src/ofbx.h>

void AssetManager::Init()
{
	// Show the Open dialog box.
	char buffer[MAX_PATH];
	GetModuleFileNameA(NULL, buffer, MAX_PATH);
	std::filesystem::path file_path(buffer);
	while (file_path.filename().compare("bin") != 0)
	{
		file_path.remove_filename();
		file_path = std::filesystem::canonical(file_path);
	}
	file_path.remove_filename();

	Instance().base_path	= file_path;
	Instance().asset_path	= file_path.append("assets\\");
	Instance().model_path	= std::filesystem::path(Instance().asset_path).append("models\\");;
	Instance().texture_path	= std::filesystem::path(Instance().asset_path).append("textures\\");;
	Instance().shader_path	= std::filesystem::path(Instance().asset_path).append("shaders\\");;
}

void AssetManager::Destroy()
{
	for (auto[handle, texture] : Instance().m_Textures)
	{
		delete texture;
	}
}

Texture2D* AssetManager::GetTexture2D(const char* file_name)
{
	std::map<std::string, Texture2D*>& textures = Instance().m_Textures;
	auto texture = textures.find(file_name);
	if (texture == textures.end())
	{
		std::filesystem::path file_path = Instance().GetTexturePath();
		file_path.append(file_name);
		Texture2D* new_texture = new Texture2D(file_path);
		textures.insert(std::make_pair(file_name, new_texture));
		return new_texture;
	}
	return texture->second;
}

Texture2D* AssetManager::LoadTextureFromFileSystem()
{
	Texture2D* result = nullptr;

	HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED |
		COINIT_DISABLE_OLE1DDE);
	if (SUCCEEDED(hr))
	{
		IFileOpenDialog* pFileOpen;

		// Create the FileOpenDialog object.
		hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL,
			IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileOpen));

		if (SUCCEEDED(hr))
		{
			IShellItem* pFolder;
			hr = SHCreateItemFromParsingName(Instance().texture_path.wstring().c_str(), nullptr, IID_PPV_ARGS(&pFolder));
			if (SUCCEEDED(hr))
			{
				pFileOpen->SetFolder(pFolder);
				hr = pFileOpen->Show(NULL);

				// Get the file name from the dialog box.
				if (SUCCEEDED(hr))
				{
					IShellItem* pItem;
					hr = pFileOpen->GetResult(&pItem);
					if (SUCCEEDED(hr))
					{
						PWSTR pszFilePath;
						hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);

						// Display the file name to the user.
						if (SUCCEEDED(hr))
						{
							std::filesystem::path selected_file(pszFilePath);
							result = AssetManager::GetTexture2D(selected_file.filename().string().c_str());
							CoTaskMemFree(pszFilePath);
						}
						pItem->Release();
					}
				}
				pFolder->Release();
			}
			pFileOpen->Release();
		}
		CoUninitialize();
	}

	return result;
}

TextureCubeMap* AssetManager::GetTextureCubeMap(const char* filename, bool folder)
{
	std::map<std::string, TextureCubeMap*>& cubemaps = Instance().m_CubeMaps;
	auto texture = cubemaps.find(filename);
	if (texture == cubemaps.end())
	{
		std::filesystem::path file_path = Instance().GetTexturePath();
		file_path.append(filename);
		TextureCubeMap* new_texture = new TextureCubeMap(file_path, folder);
		cubemaps.insert(std::make_pair(filename, new_texture));
		return new_texture;
	}
	return texture->second;
}

RawModel* AssetManager::GetRawModel(const char* file_name)
{
	std::map<std::string, RawModel*>& models = Instance().m_Models;
	auto model = models.find(file_name);
	if (model == models.end())
	{
		std::filesystem::path file_path(GetModelPath());
		file_path.append(file_name);
		ModelData fbx_data;
		if (ParseFBX(file_path, fbx_data))
		{
			RawModel* new_model = new RawModel(fbx_data);
			models.insert(std::make_pair(file_name, new_model));
			return new_model;
		}
		// Something went horribly wrong
		exit(1);
	}
	return model->second;
}

Shader* AssetManager::GetShader(const char* file_name)
{
	auto s = Instance().m_Shaders.find(file_name);
	if (s == Instance().m_Shaders.end())
	{
		std::filesystem::path file_path(GetShaderPath());
		file_path.append(file_name);
		std::map<GLuint, std::string> shader_sources;
		if (ParseShader(file_path, shader_sources))
		{
			Shader* new_shader = new Shader(file_name, shader_sources);
			Instance().m_Shaders.insert(std::make_pair(file_name, new_shader));
			return new_shader;
		}
		exit(0);
	}
	return s->second;
}



std::string AssetManager::ReadFile(std::filesystem::path file_path)
{
	std::string result;
	std::ifstream in(file_path, std::ios::in | std::ios::binary);
	if (in)
	{
		in.seekg(0, std::ios::end);
		size_t size = in.tellg();
		if (size != -1)
		{
			result.resize(size);
			in.seekg(0, std::ios::beg);
			in.read(&result[0], size);
		}
		else
		{
			std::cout << "Error: Could not read file: " << file_path << std::endl;
		}
	}
	else
	{
		std::cout << "Error: Could not open file: " << file_path << std::endl;
	}
	return result;
}

bool AssetManager::ParseFBX(const std::filesystem::path& file_path, ModelData& output)
{
	std::string path_str = file_path.string();
	std::string ext_str = path_str.substr(path_str.find_last_of(".") + 1);
	if (ext_str.compare("fbx") == 0 || ext_str.compare("FBX") == 0)
	{
		std::string fbx_data = ReadFile(file_path);
		if (fbx_data.empty()) {
			std::cout << "ERROR: FBX file is empty (" << path_str << ")" << std::endl;
			return false;
		}

		ofbx::IScene* scene = ofbx::load((ofbx::u8*)fbx_data.data(), fbx_data.size(), (ofbx::u64)ofbx::LoadFlags::TRIANGULATE);
		if (!scene) {
			std::cout << ofbx::getError() << std::endl;
			std::cout << "ERROR: FBX was not successfully loaded (" << path_str << ")" << std::endl;
			return false;
		}

		output.vertices.clear();
		output.indices.clear();

		int total_vertices = 0;
		int total_indices = 0;
		int mesh_count = scene->getMeshCount();
		for (int m = 0; m < mesh_count; m++) {
			const ofbx::Mesh* mesh = scene->getMesh(m);
			const ofbx::Geometry* geom = mesh->getGeometry();
			const ofbx::Vec3* vertices = geom->getVertices();
			const ofbx::Vec3* normals = geom->getNormals();
			const ofbx::Vec4* colors = geom->getColors();
			const ofbx::Vec2* uvs = geom->getUVs();
			const int vertex_count = geom->getVertexCount();

			/* TODO: Optimize vertex re-use, or use meshoptimizer */
			total_vertices += vertex_count;
			output.vertices.reserve(total_vertices);

			double* matrix = mesh->getLocalTransform().m;
			glm::vec4 v1(matrix[0], matrix[1], matrix[2], matrix[3]);
			glm::vec4 v2(matrix[4], matrix[5], matrix[6], matrix[7]);
			glm::vec4 v3(matrix[8], matrix[9], matrix[10], matrix[11]);
			/* TODO: Don't multiply matrix[15] with 100
			 * TODO: Fix so that matrix is stored uploaded as uniform instead of multiplied directly into the local position
			 */
			glm::vec4 v4(matrix[12], matrix[13], matrix[14], matrix[15] * 100);
			glm::mat4 model_matrix = glm::mat4(v1, v2, v3, v4);
			for (int v = 0; v < vertex_count; v++)
			{
				Vertex vertex;
				glm::vec4 pos = model_matrix * glm::vec4(vertices[v].x, vertices[v].y, vertices[v].z, 1.0);
				vertex.position = pos / pos.w;
				vertex.normal = model_matrix * glm::vec4(normals[v].x, normals[v].y, normals[v].z, 0.0);
				vertex.uv = uvs ? glm::vec2(uvs[v].x, 1 - uvs[v].y) : glm::vec2(0.0f, 0.0f);
				output.vertices.push_back(vertex);
			}

			const int index_count = geom->getIndexCount();
			output.indices.reserve(index_count);
			const int* face_indices = geom->getFaceIndices();
			for (int i = 0; i < index_count; i++)
			{
				int idx = face_indices[i] < 0 ? -(face_indices[i] + 1) : face_indices[i];
				output.indices.push_back(total_indices + idx);
			}
			total_indices += index_count;
		}
		return true;
	}
	else
	{
		std::cout << "Error: unsupported file extension '" << ext_str << "' for file '" << path_str << "'" << std::endl;
		return false;
	}
	return true;
}

void AssetManager::ReloadShaders()
{
	for (auto& [name, shader] : Instance().m_Shaders)
	{
		std::filesystem::path file_path(GetShaderPath());
		file_path.append(name);
		std::map<GLuint, std::string> shader_sources;
		if (ParseShader(file_path, shader_sources))
		{
			shader->Reload(shader_sources);
		}
		else
		{
			std::cout << "Error: Could not reload shader " << name << std::endl;
		}
	}
}

bool AssetManager::ParseShader(const std::filesystem::path& file_path, std::map<GLuint, std::string>& output_sources)
{
	output_sources.clear();

	std::string shader_source = AssetManager::ReadFile(file_path);
	if (shader_source.size() == 0)
		return false;

	size_t pos = 0;
	const std::string TYPE_START = "__";
	// TODO: Currently windows only using \r\n. Make OS agnostic
	const std::string CARRIAGE_RETURN = "\r\n";
	const std::string TYPE_END = TYPE_START + CARRIAGE_RETURN;
	while (pos != std::string::npos)
	{
		pos = shader_source.find(TYPE_START, pos);
		if (pos == std::string::npos)
		{
			std::cout << "Error: Could not find shader type in '" << file_path << "'" << std::endl;
			return false;
		}
		pos += TYPE_START.size();

		size_t type_end = shader_source.find(TYPE_END, pos);
		std::string shader_type = shader_source.substr(pos, type_end - pos);
		GLuint gl_shader_type = Shader::GetGLShaderTypeFromString(shader_type);
		if (gl_shader_type == GL_INVALID_VALUE)
		{
			std::cout << "Error: Shader type '" << shader_type << "' in '" << file_path << "' is incorrect" << std::endl;
			return false;
		}
		pos = type_end + TYPE_END.size();

		// Find start of next shader or end of file
		size_t next_pos = shader_source.find(TYPE_START, pos);
		std::string shader_code;
		if (next_pos == std::string::npos)
			shader_code = shader_source.substr(pos);
		else
			shader_code = shader_source.substr(pos, next_pos - pos);

		output_sources.emplace(std::make_pair(gl_shader_type, shader_code));

		pos = next_pos;
	}
	return true;
}