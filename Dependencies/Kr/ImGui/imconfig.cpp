#include "imgui.h"

namespace ImGui
{
bool WantCaptureInput()
{
	ImGuiIO &io = ImGui::GetIO();
	return io.WantCaptureMouse || io.WantCaptureKeyboard;
}
} // namespace ImGui
