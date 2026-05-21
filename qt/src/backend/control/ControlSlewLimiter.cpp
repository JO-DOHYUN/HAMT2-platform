#include "control/ControlSlewLimiter.h"

#include <algorithm>

namespace CanMonitorControl {
namespace {

int stepTowardInt(int current, int target, int maxStep) {
    if (current < target) return std::min(current + maxStep, target);
    if (current > target) return std::max(current - maxStep, target);
    return current;
}

double stepTowardDouble(double current, double target, double maxStep) {
    if (current < target) return std::min(current + maxStep, target);
    if (current > target) return std::max(current - maxStep, target);
    return current;
}

ControlSlewTarget clampTarget(const ControlSlewTarget& target) {
    return {
        std::clamp(target.signedCommand, -10000, 10000),
        std::clamp(target.rpm, 0, 10000),
        std::clamp(target.steeringDeg, -90.0, 90.0),
    };
}

ControlSlewState clampState(const ControlSlewState& state) {
    return {
        std::clamp(state.signedCommand, -10000, 10000),
        std::clamp(state.rpm, 0, 10000),
        std::clamp(state.steeringDeg, -90.0, 90.0),
    };
}

} // namespace

void ControlSlewLimiter::reset(const ControlSlewState& state) {
    m_state = clampState(state);
    m_target = {m_state.signedCommand, m_state.rpm, m_state.steeringDeg};
}

void ControlSlewLimiter::setTarget(const ControlSlewTarget& target) {
    m_target = clampTarget(target);
}

ControlSlewState ControlSlewLimiter::step() {
    m_state.signedCommand = stepTowardInt(m_state.signedCommand,
                                          m_target.signedCommand,
                                          kMaxCommandStepPerCycle);
    m_state.rpm = stepTowardInt(m_state.rpm,
                                m_target.rpm,
                                kMaxRpmStepPerCycle);
    m_state.steeringDeg = stepTowardDouble(m_state.steeringDeg,
                                           m_target.steeringDeg,
                                           kMaxSteerStepDegPerCycle);
    return m_state;
}

} // namespace CanMonitorControl
