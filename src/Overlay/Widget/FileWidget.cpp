#include "FileWidget.h"
#include <imgui.h>
#include <filesystem>

bool FileWidget::DrawLoad(const char* label_combo, const char* label_load) {
	if (ImGui::BeginCombo(label_combo, path_exists ? path : "<new file>")) {
		path_exists = false;
		for (const auto& f : std::filesystem::directory_iterator(base_dir)) {
			std::string pp = f.path().filename().string();
			if (ImGui::Selectable(pp.c_str(), strcmp(path, pp.c_str()) == 0)) {
				strcpy(path, pp.c_str());
				path_exists = true;
			}
		}
		ImGui::Selectable("<new file>", !path_exists);
		ImGui::EndCombo();
	}

	if (path_exists) {
		ImGui::SameLine();
		return ImGui::Button(label_load);
	}
	else return false;
}

bool FileWidget::DrawSave(const char* label_input, const char* label_save, const char* label_overwrite) {
	if (ImGui::InputText(label_input, path, 1200)) {
		path_exists = std::filesystem::exists(FullPath());
	}

	ImGui::SameLine();
	if (path_exists && label_overwrite != NULL ? ImGui::Button(label_overwrite) : ImGui::Button(label_save)) {
		path_exists = true; // just assumes that the save button will create a file 
		return true;
	}
	return false;
}

std::string FileWidget::FullPath() {
	return base_dir + path;
}