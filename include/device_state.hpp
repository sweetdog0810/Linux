#pragma once

#include "command_parser.hpp"

#include <string>

enum class FaceMode {
    Normal,
    Happy,
    Angry,
    Sleep,
    Sad
};

struct DeviceState {
    FaceMode face{FaceMode::Normal};
    int servo_angle{90};
    int light_value{1800};
};

bool apply_command(DeviceState& state, const Command& command, std::string& result);
std::string face_mode_to_string(FaceMode mode);
std::string state_to_string(const DeviceState& state);
