#pragma once

#include "SerialWorker.h"

#include <QString>

namespace CanMonitorTransport {

class TransportRuntime {
public:
    void attachWorker(SerialWorker* worker);

    SerialWorker* worker() const { return m_worker; }
    bool hasWorker() const { return m_worker != nullptr; }
    bool liveProductionTypedOnly() const { return true; }
    SerialWorker::TransportMode requiredLiveMode() const { return SerialWorker::TransportMode::TypedEvidence; }

    bool enforceProductionLiveMode(QString* error = nullptr);

private:
    SerialWorker* m_worker = nullptr;
};

} // namespace CanMonitorTransport
