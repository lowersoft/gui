#include <cstdio>
#include <cstring>
#include <cstdint>

#include "gui/app.h"

class ChatApp : public gui::App {
	void OnInit() override {
		SetTheme(kThemes[1]);
		std::puts("ChatApp initialized");
	}

	void OnGui() override {
		ImGuiViewport* viewport = ImGui::GetMainViewport();

		ImGui::SetNextWindowPos(viewport->WorkPos);
		ImGui::SetNextWindowSize(viewport->WorkSize);
		ImGui::SetNextWindowViewport(viewport->ID);

		ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar |
			ImGuiWindowFlags_NoCollapse |
			ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoBringToFrontOnFocus |
			ImGuiWindowFlags_NoNavFocus |
			ImGuiWindowFlags_MenuBar;

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::Begin("Chat", nullptr, window_flags);
		ImGui::PopStyleVar();

		if (ImGui::BeginMenuBar()) {
			if (ImGui::BeginMenu("settings")) {
				if (BeginSubPopup("connection")) {
					static char ip[64] = "127.0.0.1";
					static uint16_t port = 3131;
					static char interface[16] = "wlan0";

					ImGui::InputText("peer ip", ip, IM_ARRAYSIZE(ip));
					ImGui::InputScalar("peer port", ImGuiDataType_U16, &port);
					ImGui::InputText("interface", interface, IM_ARRAYSIZE(interface));
					ImGui::EndPopup();
				}
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("general")) {
				static int mode = 0;
				if (BeginSubPopup("language")) {
					if (ImGui::MenuItem("english", nullptr, mode == 0)) mode = 0;
					if (ImGui::MenuItem("turkish", nullptr, mode == 1)) mode = 1;
					ImGui::EndPopup();
				}
				ImGui::EndMenu();
			}
			
			ImGui::EndMenuBar();
		}

		static char text_buf[128] = { 0 };
		ImGui::PushStyleColor(ImGuiCol_TextDisabled, ImVec4(0.627f, 0.627f, 0.627f, 1.0f));

		float yPos = ImGui::GetCursorPosY() + ImGui::GetContentRegionAvail().y - ImGui::GetFrameHeight();

		const float margin = 8.0f;
		const float spacing = ImGui::GetStyle().ItemSpacing.x;
		const float btnW = ImGui::CalcTextSize("send").x + ImGui::GetStyle().FramePadding.x * 2.0f;
		ImGui::SetCursorPos(ImVec2(margin, yPos - margin));
		
		ImGui::PushItemWidth(-(margin + btnW + spacing));
		const bool isEmpty = text_buf[strspn(text_buf, " \t")] == '\0';;
		if (ImGui::InputTextWithHint("##input", "write your message in here..", text_buf, IM_ARRAYSIZE(text_buf), ImGuiInputTextFlags_EnterReturnsTrue) && !isEmpty) {
			printf("%s\n", text_buf);
			memset(text_buf, 0, sizeof(text_buf));
		}
		ImGui::PopItemWidth();

		ImGui::SameLine();
		if (ImGui::Button("send", ImVec2(btnW, 0.0f)) && !isEmpty) {
			printf("%s\n", text_buf);
			memset(text_buf, 0, sizeof(text_buf));
		}
		ImGui::PopStyleColor();

		ImGui::End();
	}

private:
	static bool BeginSubPopup(const char* label) {
		if (ImGui::Selectable(label, false, ImGuiSelectableFlags_NoAutoClosePopups)) ImGui::OpenPopup(label);
		ImGui::SetNextWindowPos(ImVec2(ImGui::GetItemRectMax().x, ImGui::GetItemRectMin().y));
		return ImGui::BeginPopup(label);
	}

	static constexpr std::array<const char*, 5> kThemes = {
			"conf/theme.ini", "conf/themes/chat.ini", "conf/themes/forest.ini",
			"conf/themes/crimson.ini", "conf/themes/classic.ini" };

	bool		showDemo_ = false;
	int			counter_ = 0;
	std::size_t	themeIndex_ = 0;
	float		speed_ = 1.0f;;
	ImVec4		color_ = ImVec4(0.2f, 0.6f, 1.0f, 1.0f);
};

GUI_MAIN(ChatApp, "Chatting", 1280, 720);