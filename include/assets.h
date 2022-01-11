#pragma once

#include <string>
#include <fstream>
#include <iostream>

namespace Assets
{
	const std::string assets_path = "assets/";
	const std::string models_path = assets_path + "models/";
	const std::string textures_path = assets_path + "textures/";
	const std::string shaders_path = assets_path + "shaders/";

	std::string read_file(const std::string& file_path);
}