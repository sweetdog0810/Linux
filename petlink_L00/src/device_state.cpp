#include "device_state.hpp"

#include <sstream>

std::string face_mode_to_string(FaceMode mode)
{
    switch (mode) {
    case FaceMode::Normal: return "NORMAL";
    case FaceMode::Happy: return "HAPPY";
    case FaceMode::Angry: return "ANGRY";
    case FaceMode::Sleep: return "SLEEP";
    case FaceMode::Sad  : return "SAD"  ;
    }
    return "UNKNOWN";
}

bool apply_command(DeviceState& state, const Command& command, std::string& result)
{
    switch (command.type) {
    case CommandType::SetFace:
        if (command.text_value == "normal") state.face = FaceMode::Normal;
        else if (command.text_value == "happy") state.face = FaceMode::Happy;
        else if (command.text_value == "angry") state.face = FaceMode::Angry;
        else if (command.text_value == "sleep") state.face = FaceMode::Sleep;
        else if (command.text_value == "sad")   state.face = FaceMode::Sad;
        else {
            result = "内部错误：无效表情";
            return false;
        }
        result = "表情已切换为 " + face_mode_to_string(state.face);
        return true;

    case CommandType::SetServo:
        if (command.number_value < 0 || command.number_value > 180) {
            result = "舵机角度必须在 0～180 度之间";
            return false;
        }
        state.servo_angle = command.number_value;
        result = "舵机角度已设置为 " + std::to_string(state.servo_angle) + " 度";
        return true;

    case CommandType::SetLight:
        if (command.number_value < 0 || command.number_value > 4095) {
            result = "模拟光照值必须在 0～4095 之间";
            return false;
        }
        state.light_value = command.number_value;
        result = "模拟光照值已设置为 " + std::to_string(state.light_value);
        return true;

    default:
        result = "该命令不修改设备状态";
        return false;
    }
}

std::string state_to_string(const DeviceState& state)
{
    std::ostringstream output;
    output << "Face: " << face_mode_to_string(state.face) << '\n'
           << "Servo angle: " << state.servo_angle << " deg\n"
           << "Light value: " << state.light_value;
    return output.str();
}
