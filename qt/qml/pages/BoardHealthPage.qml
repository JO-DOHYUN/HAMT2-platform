import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components" as Components

Item {
    id: root
    property real uiScale: 1.0
    property string monoFontFamily: "Monospace"

    function panelColor(level) {
        if (level === "ok") return "#ecfdf3"
        if (level === "warn") return "#fff7ed"
        if (level === "bad" || level === "error") return "#fff1f2"
        return "#f8fafc"
    }

    function panelBorder(level) {
        if (level === "ok") return "#bbf7d0"
        if (level === "warn") return "#fed7aa"
        if (level === "bad" || level === "error") return "#fecdd3"
        return "#dbe5f0"
    }

    function panelText(level) {
        if (level === "ok") return "#166534"
        if (level === "warn") return "#9a3412"
        if (level === "bad" || level === "error") return "#be123c"
        return "#475569"
    }

    ScrollView {
        anchors.fill: parent
        clip: true

        ColumnLayout {
            width: Math.max(root.width, Math.round(1120 * root.uiScale))
            spacing: Math.round(10 * root.uiScale)

            Frame {
                Layout.fillWidth: true
                padding: Math.round(12 * root.uiScale)
                background: Rectangle { color: "#f8fbff"; radius: 14; border.color: "#cbd9ea" }

                ColumnLayout {
                    anchors.fill: parent
                    spacing: Math.round(8 * root.uiScale)

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: Math.round(8 * root.uiScale)

                        Label {
                            text: "보드 / Evidence 상태"
                            font.pixelSize: Math.round(21 * root.uiScale)
                            font.bold: true
                            color: "#102033"
                        }
                        Components.StatusBadge {
                            text: appController.connected ? "COM OPEN" : "COM CLOSED"
                            kind: appController.connected ? "ok" : "bad"
                            uiScale: root.uiScale
                            maxWidth: Math.round(120 * root.uiScale)
                        }
                        Components.StatusBadge {
                            text: appController.boardAlive ? "BOARD ALIVE" : "ALIVE 대기"
                            kind: appController.boardAlive ? "ok" : "warn"
                            uiScale: root.uiScale
                            maxWidth: Math.round(130 * root.uiScale)
                        }
                        Components.StatusBadge {
                            text: "RX " + appController.liveRxFps + " fps"
                            kind: appController.liveRxFps > 0 ? "ok" : "info"
                            uiScale: root.uiScale
                            maxWidth: Math.round(105 * root.uiScale)
                        }
                        Components.StatusBadge {
                            text: "TX " + appController.liveTxFps + " fps"
                            kind: appController.liveTxFps > 0 ? "ok" : "info"
                            uiScale: root.uiScale
                            maxWidth: Math.round(105 * root.uiScale)
                        }
                        Components.StatusBadge {
                            text: "FAULT " + appController.typedTransportFaultCount
                            kind: appController.typedTransportFaultCount > 0 ? "bad" : "ok"
                            uiScale: root.uiScale
                            maxWidth: Math.round(110 * root.uiScale)
                        }
                        Item { Layout.fillWidth: true }
                    }

                    Label {
                        Layout.fillWidth: true
                        text: "COM open, CAPABILITY, BOARD_HEALTH, CAN_RX_RAW, CAN_TX_RAW, CONTROL_ACK는 서로 다른 evidence입니다. 실제 제어 송신 성공은 CAN_TX_RAW audit만 기준으로 봅니다."
                        wrapMode: Text.WordWrap
                        color: "#334155"
                        font.pixelSize: Math.round(12 * root.uiScale)
                    }
                    Label {
                        Layout.fillWidth: true
                        text: appController.boardConnectionSummary
                        wrapMode: Text.WordWrap
                        color: appController.boardAlive ? "#166534" : "#9a3412"
                        font.pixelSize: Math.round(11 * root.uiScale)
                    }
                }
            }

            GridView {
                id: evidenceGrid
                Layout.fillWidth: true
                Layout.preferredHeight: Math.max(Math.round(410 * root.uiScale), contentHeight + Math.round(8 * root.uiScale))
                interactive: false
                cellWidth: Math.max(Math.round(330 * root.uiScale), width / 3)
                cellHeight: Math.round(118 * root.uiScale)
                model: appController.boardEvidenceModel
                delegate: Rectangle {
                    required property string label
                    required property string value
                    required property string level
                    required property string note

                    width: evidenceGrid.cellWidth - Math.round(8 * root.uiScale)
                    height: evidenceGrid.cellHeight - Math.round(8 * root.uiScale)
                    radius: 14
                    color: root.panelColor(level)
                    border.color: root.panelBorder(level)

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: Math.round(10 * root.uiScale)
                        spacing: Math.round(5 * root.uiScale)

                        RowLayout {
                            Layout.fillWidth: true
                            Label {
                                text: label
                                font.bold: true
                                color: root.panelText(level)
                                font.pixelSize: Math.round(13 * root.uiScale)
                            }
                            Item { Layout.fillWidth: true }
                            Rectangle {
                                radius: 7
                                color: "#ffffff"
                                border.color: root.panelBorder(level)
                                implicitWidth: stateLabel.implicitWidth + Math.round(14 * root.uiScale)
                                implicitHeight: stateLabel.implicitHeight + Math.round(6 * root.uiScale)
                                Label {
                                    id: stateLabel
                                    anchors.centerIn: parent
                                    text: level.toUpperCase()
                                    color: root.panelText(level)
                                    font.bold: true
                                    font.pixelSize: Math.round(9.5 * root.uiScale)
                                }
                            }
                        }
                        Label {
                            Layout.fillWidth: true
                            text: value
                            color: "#0f172a"
                            font.family: root.monoFontFamily
                            font.pixelSize: Math.round(11 * root.uiScale)
                            wrapMode: Text.WrapAnywhere
                            maximumLineCount: 2
                            elide: Text.ElideRight
                        }
                        Label {
                            Layout.fillWidth: true
                            text: note
                            color: "#475569"
                            font.pixelSize: Math.round(10.5 * root.uiScale)
                            wrapMode: Text.WordWrap
                            maximumLineCount: 3
                            elide: Text.ElideRight
                        }
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: Math.round(10 * root.uiScale)

                Frame {
                    Layout.fillWidth: true
                    padding: Math.round(10 * root.uiScale)
                    background: Rectangle { color: "#ffffff"; radius: 14; border.color: "#d7e0ea" }
                    ColumnLayout {
                        anchors.fill: parent
                        spacing: Math.round(6 * root.uiScale)
                        Label { text: "Typed evidence 요약"; font.bold: true; color: "#172033" }
                        Label {
                            Layout.fillWidth: true
                            text: appController.typedEvidenceSummary
                            color: "#334155"
                            wrapMode: Text.WrapAnywhere
                            font.family: root.monoFontFamily
                            font.pixelSize: Math.round(10.5 * root.uiScale)
                        }
                        Label {
                            Layout.fillWidth: true
                            text: appController.typedCanSummary
                            color: "#334155"
                            wrapMode: Text.WrapAnywhere
                            font.family: root.monoFontFamily
                            font.pixelSize: Math.round(10.5 * root.uiScale)
                        }
                    }
                }

                Frame {
                    Layout.fillWidth: true
                    padding: Math.round(10 * root.uiScale)
                    background: Rectangle { color: "#ffffff"; radius: 14; border.color: "#d7e0ea" }
                    ColumnLayout {
                        anchors.fill: parent
                        spacing: Math.round(6 * root.uiScale)
                        Label { text: "제어 evidence 요약"; font.bold: true; color: "#172033" }
                        Label {
                            Layout.fillWidth: true
                            text: appController.controlEvidenceStatsSummary
                            color: "#334155"
                            wrapMode: Text.WrapAnywhere
                            font.family: root.monoFontFamily
                            font.pixelSize: Math.round(10.5 * root.uiScale)
                        }
                        Label {
                            Layout.fillWidth: true
                            text: appController.controlStatusSummary
                            color: "#334155"
                            wrapMode: Text.WordWrap
                            font.pixelSize: Math.round(10.5 * root.uiScale)
                        }
                    }
                }
            }
        }
    }
}
