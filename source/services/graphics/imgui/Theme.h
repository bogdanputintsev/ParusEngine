#pragma once

#pragma warning(push, 0)
#include "third-party/imgui/imgui.h"
#pragma warning(pop)

namespace parus::imgui::theme
{
    constexpr ImVec4 COLOR_BACKGROUND  = { 0.055f, 0.106f, 0.172f, 1.0f };
    constexpr ImVec4 COLOR_TITLEBAR    = { 0.027f, 0.063f, 0.094f, 1.0f };
    constexpr ImVec4 COLOR_BORDER      = { 0.118f, 0.227f, 0.345f, 1.0f };
    constexpr ImVec4 COLOR_COMMAND     = { 0.357f, 0.741f, 0.878f, 1.0f };
    constexpr ImVec4 COLOR_REPLY_TEXT  = { 0.659f, 0.784f, 0.878f, 1.0f };
    constexpr ImVec4 COLOR_REPLY_VALUE = { 0.961f, 0.769f, 0.188f, 1.0f };
    constexpr ImVec4 COLOR_INFO        = { 0.227f, 0.376f, 0.502f, 1.0f };
}