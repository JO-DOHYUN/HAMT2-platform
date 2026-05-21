import QtQuick
import QtQuick.Controls

Rectangle {
    id: root
    property string text: ""
    property string kind: "info"
    property real uiScale: 1.0
    property int maxWidth: Math.round(180 * uiScale)

    radius: Math.round(999 * uiScale)
    implicitHeight: Math.round(24 * uiScale)
    implicitWidth: Math.min(maxWidth, badgeLabel.implicitWidth + Math.round(18 * uiScale))

    function fillColor() {
        if (kind === "ok") return "#edf8f1"
        if (kind === "warn") return "#fff5e8"
        if (kind === "bad") return "#fdeeee"
        return "#eff5ff"
    }
    function edgeColor() {
        if (kind === "ok") return "#9fd4b0"
        if (kind === "warn") return "#e7bf7b"
        if (kind === "bad") return "#ebb0aa"
        return "#c7d9ff"
    }
    function textColor() {
        if (kind === "ok") return "#166534"
        if (kind === "warn") return "#9a5800"
        if (kind === "bad") return "#b42318"
        return "#2457a6"
    }

    color: fillColor()
    border.color: edgeColor()
    border.width: 1

    Label {
        id: badgeLabel
        anchors.fill: parent
        anchors.leftMargin: Math.round(9 * root.uiScale)
        anchors.rightMargin: Math.round(9 * root.uiScale)
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter
        text: root.text
        color: root.textColor()
        font.pixelSize: Math.round(11 * root.uiScale)
        font.bold: true
        elide: Text.ElideRight
    }
}
