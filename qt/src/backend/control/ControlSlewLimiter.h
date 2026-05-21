#pragma once

namespace CanMonitorControl {

struct ControlSlewTarget {
    int signedCommand = 0;
    int rpm = 0;
    double steeringDeg = 0.0;
};

struct ControlSlewState {
    int signedCommand = 0;
    int rpm = 0;
    double steeringDeg = 0.0;
};

class ControlSlewLimiter {
public:
    static constexpr int kMaxCommandStepPerCycle = 250;
    static constexpr int kMaxRpmStepPerCycle = 250;
    static constexpr double kMaxSteerStepDegPerCycle = 1.0;

    void reset(const ControlSlewState& state = {});
    void setTarget(const ControlSlewTarget& target);
    ControlSlewState step();

    const ControlSlewTarget& target() const { return m_target; }
    const ControlSlewState& state() const { return m_state; }

private:
    ControlSlewTarget m_target;
    ControlSlewState m_state;
};

} // namespace CanMonitorControl
