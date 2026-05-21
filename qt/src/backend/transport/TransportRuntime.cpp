#include "transport/TransportRuntime.h"

namespace CanMonitorTransport {

void TransportRuntime::attachWorker(SerialWorker* worker) {
    m_worker = worker;
}

bool TransportRuntime::enforceProductionLiveMode(QString* error) {
    if (!m_worker) {
        if (error) *error = QStringLiteral("transport worker is not attached");
        return false;
    }

    m_worker->setTransportMode(requiredLiveMode());
    if (error) error->clear();
    return true;
}

} // namespace CanMonitorTransport
