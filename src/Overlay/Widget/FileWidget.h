#pragma once
#include <string>

class FileWidget
{
public:
	std::string base_dir = "./"; // with trailing /
	//std::string extension = ""; // TODO: filter file list. append to FullPath

	char path[1200] = ""; // buffer needed for text input
	bool path_exists = false;


	bool DrawLoad(const char* label_combo, const char* label_load);

	bool DrawSave(const char* label_input, const char* label_save, const char* label_overwrite);

	std::string FullPath();
};

