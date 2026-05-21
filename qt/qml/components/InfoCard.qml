import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components" as Components

Frame {
    id: root
    property string title: ""
    property string value: ""
    property string note: ""
    property string badgeText: ""
    property string kind: "info"
    property real uiScale: 1.0
    property bool clickable: false
    property int preferredHeight: Math.round(88 * uiScale)
    signal clicked()

    function fillColor() {
        if (kind === "ok") return "#f7fcf8"
        if (kind === "warn") return "#fffaf3"
        if (kind === "bad") return "#fff7f6"
        return "#f8fbff"
    }
    function edgeColor() {
        if (kind === "ok") return "#cfead8"
        if (kind === "warn") return "#f0d6a8"
        if (kind === "bad") return "#f1c9c4"
        return "#d5e4ff"
    }
    function valueFontSize() {
        const n = value ? value.length : 0
        if (n > 34) return Math.round(11.5 * uiScale)
        if (n > 22) return Math.round(13 * uiScale)
        if (n > 12) return Math.round(15 * uiScale)
        return Math.round(17 * uiScale)
    }

    padding: 0
    implicitHeight: Math.max(preferredHeight, contentColumn.implicitHeight + Math.round(12 * root.uiScale))
    background: Rectangle {
        radius: Math.round(10 * root.uiScale)
        gradient: Gradient {
            GradientStop { position: 0.0; color: root.fillColor() }
            GradientStop { position: 1.0; color: "#ffffff" }
        }
        border.color: root.edgeColor()
    }

    MouseArea {
        anchors.fill: parent
        enabled: root.clickable
        cursorShape: enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
        onClicked: root.clicked()
    }

    ColumnLayout {
        id: contentColumn
        anchors.fill: parent
        anchors.margins: Math.round(6 * root.uiScale)
        spacing: Math.round(2 * root.uiScale)

        RowLayout {
            Layout.fillWidth: true
            spacing: 6
            Label {
                text: root.title
                color: "#607080"
                font.pixelSize: Math.round(9.0 * root.uiScale)
                font.bold: true
                elide: Text.ElideRight
                Layout.fillWidth: true
            }
            Components.StatusBadge {
                visible: root.badgeText !== ""
                text: root.badgeText
                kind: root.kind
                uiScale: root.uiScale
                maxWidth: Math.round(112 * root.uiScale)
            }
        }

        Label {
            Layout.fillWidth: true
            text: root.value
            color: "#203246"
            font.pixelSize: Math.max(Math.round(12.0 * root.uiScale), root.valueFontSize() - Math.round(2.0 * root.uiScale))
            font.bold: true
            maximumLineCount: 3
            wrapMode: Text.Wrap
            elide: Text.ElideRight
        }

        Item { Layout.fillHeight: true; Layout.minimumHeight: Math.round(4 * root.uiScale) }

        Label {
            text: root.note
            color: "#5b6b79"
            font.pixelSize: Math.round(9.6 * root.uiScale)
            wrapMode: Text.WordWrap
            maximumLineCount: 3
            elide: Text.ElideRight
            Layout.fillWidth: true
        }
    }
}
