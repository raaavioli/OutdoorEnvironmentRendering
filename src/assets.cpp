#include "assets.h"

std::string Assets::read_file(const std::string& file_path)
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
			std::cout << "Could not read file: " << file_path << std::endl;
			}
		}
	else
	{
		std::cout << "Could not open file: " << file_path << std::endl;
	}
	return result;
}