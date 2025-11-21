#include "app_interface.h"

extern "C" void app_main(AppState& state)
{
    ImGui::SetCurrentContext(state.context);

    ImGui::Text("OMG IT WORKS! asdfasdf");
    ImGui::Text("OMG IT WORKS! asdfasdf");
    ImGui::Text("OMG IT WORKS! asdfasdf");
    ImGui::Text("OMG IT WORKS! asdfasdf");
    ImGui::Text("OMG IT WORKS! asdfasdf");
    ImGui::Text("OMG IT WORKS! asdfasdf");
    ImGui::Text("OMG IT WORKS! asdfasdf");
}
