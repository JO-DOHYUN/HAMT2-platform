import QtQuick
import QtQuick.Controls

Flickable {
    id: root
    property Component pageComponent
    property real minPageWidth: 0
    property bool active: true
    property real uiScale: 1.0
    readonly property bool needsHorizontalScroll: contentWidth > width + 1

    clip: true
    visible: active
    boundsBehavior: Flickable.StopAtBounds
    contentWidth: pageHost.width
    contentHeight: height
    flickableDirection: Flickable.HorizontalFlick
    interactive: needsHorizontalScroll

    function clampValue(v, minV, maxV) {
        return Math.max(minV, Math.min(maxV, v))
    }

    Item {
        id: pageHost
        width: Math.max(root.width, root.minPageWidth)
        height: root.height

        Loader {
            id: pageLoader
            anchors.fill: parent
            active: root.active
            sourceComponent: root.pageComponent
        }
    }

    ScrollBar.horizontal: ScrollBar {
        id: hbar
        policy: root.needsHorizontalScroll ? ScrollBar.AsNeeded : ScrollBar.AlwaysOff
        active: root.needsHorizontalScroll && (root.movingHorizontally || root.draggingHorizontally || hovered)
    }

    WheelHandler {
        target: null
        acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
        onWheel: function(event) {
            let horizontalDelta = 0
            if (event.pixelDelta) {
                horizontalDelta = -event.pixelDelta.x
            } else if (event.angleDelta) {
                horizontalDelta = -(event.angleDelta.x / 120.0) * (72 * root.uiScale)
            }

            if (horizontalDelta === 0 && event.angleDelta)
                horizontalDelta = -(event.angleDelta.y / 120.0) * (72 * root.uiScale)

            if (horizontalDelta === 0) {
                return
            }
            const maxX = Math.max(0, root.contentWidth - root.width)
            root.contentX = root.clampValue(root.contentX + horizontalDelta, 0, maxX)
            if (event.accepted !== undefined)
                event.accepted = true
        }
    }
}
