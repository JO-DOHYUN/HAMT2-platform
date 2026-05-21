import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components" as Components

Item {
    id: root
    property real uiScale: 1.0
    property string monoFontFamily: "Monospace"

    focus: true
    Keys.onPressed: function(event) {
        if (event.key === Qt.Key_W) { appController.controlKeyboardCommand("w"); event.accepted = true }
        else if (event.key === Qt.Key_S) { appController.controlKeyboardCommand("s"); event.accepted = true }
        else if (event.key === Qt.Key_A) { appController.controlKeyboardCommand("a"); event.accepted = true }
        else if (event.key === Qt.Key_D) { appController.controlKeyboardCommand("d"); event.accepted = true }
        else if (event.key === Qt.Key_X || event.key === Qt.Key_Escape) { appController.controlSendNeutral(); event.accepted = true }
    }

    function badgeKindForReady() {
        if (!appController.connected || !appController.boardAlive)
            return "warn"
        return "ok"
    }

    function ackKind() {
        return appController.controlLastAckSummary.indexOf("REJECTED") >= 0 ? "bad" : "info"
    }

    ScrollView {
        anchors.fill: parent
        clip: true

        ColumnLayout {
            width: Math.max(root.width, Math.round(1180 * root.uiScale))
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
                            text: "차량 제어"
                            font.pixelSize: Math.round(22 * root.uiScale)
                            font.bold: true
                            color: "#102033"
                        }
                        Components.StatusBadge {
                            text: appController.controlArmed ? "ARM" : "대기"
                            kind: appController.controlArmed ? "warn" : "info"
                            uiScale: root.uiScale
                            maxWidth: Math.round(95 * root.uiScale)
                        }
                        Components.StatusBadge {
                            text: appController.boardAlive ? "보드 정상" : "보드 대기"
                            kind: root.badgeKindForReady()
                            uiScale: root.uiScale
                            maxWidth: Math.round(120 * root.uiScale)
                        }
                        Components.StatusBadge {
                            text: appController.controlTestRunning ? "테스트 중" : "수동"
                            kind: appController.controlTestRunning ? "warn" : "ok"
                            uiScale: root.uiScale
                            maxWidth: Math.round(110 * root.uiScale)
                        }
                        Item { Layout.fillWidth: true }
                        Button {
                            text: "WSAD 포커스"
                            onClicked: root.forceActiveFocus()
                        }
                    }

                    Label {
                        Layout.fillWidth: true
                        text: appController.controlStatusSummary
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

            RowLayout {
                Layout.fillWidth: true
                spacing: Math.round(10 * root.uiScale)

                Frame {
                    Layout.preferredWidth: Math.round(440 * root.uiScale)
                    Layout.fillHeight: true
                    padding: Math.round(12 * root.uiScale)
                    background: Rectangle { color: "#ffffff"; radius: 14; border.color: "#d7e0ea" }

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: Math.round(10 * root.uiScale)

                        Label {
                            text: "수동 주행 입력"
                            font.bold: true
                            font.pixelSize: Math.round(17 * root.uiScale)
                            color: "#172033"
                        }
                        Label {
                            Layout.fillWidth: true
                            text: "W/S는 전후진 목표값, A/D는 조향 목표값을 2도씩 이동합니다. 실제 CAN 출력은 20ms마다 조향 1도 이하, rpm 250 이하로 제한됩니다."
                            wrapMode: Text.WordWrap
                            color: "#64748b"
                            font.pixelSize: Math.round(11 * root.uiScale)
                        }

                        GridLayout {
                            Layout.fillWidth: true
                            columns: 2
                            columnSpacing: Math.round(8 * root.uiScale)
                            rowSpacing: Math.round(8 * root.uiScale)

                            Label { text: "목표 BUS"; color: "#475569" }
                            SpinBox {
                                from: 0
                                to: 15
                                value: appController.controlTargetBus
                                editable: true
                                Layout.fillWidth: true
                                onValueModified: appController.setControlTargetBus(value)
                            }
                            Label { text: "목표 rpm"; color: "#475569" }
                            SpinBox {
                                from: 0
                                to: 10000
                                stepSize: 100
                                value: appController.controlTargetRpm
                                editable: true
                                Layout.fillWidth: true
                                onValueModified: appController.setControlTargetRpm(value)
                            }
                            Label { text: "목표 조향"; color: "#475569" }
                            RowLayout {
                                Layout.fillWidth: true
                                Slider {
                                    from: -90
                                    to: 90
                                    stepSize: 1
                                    value: appController.controlTargetSteeringDeg
                                    Layout.fillWidth: true
                                    onMoved: appController.setControlTargetSteeringDeg(value)
                                }
                                Label {
                                    text: appController.controlTargetSteeringDeg.toFixed(1) + "°"
                                    Layout.preferredWidth: Math.round(58 * root.uiScale)
                                    horizontalAlignment: Text.AlignRight
                                    font.family: root.monoFontFamily
                                }
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            radius: 10
                            color: "#f8fafc"
                            border.color: "#dbe5f0"
                            implicitHeight: busLabel.implicitHeight + Math.round(18 * root.uiScale)
                            Label {
                                id: busLabel
                                anchors.fill: parent
                                anchors.margins: Math.round(9 * root.uiScale)
                                text: appController.controlBusSummary
                                wrapMode: Text.WordWrap
                                color: "#334155"
                                font.pixelSize: Math.round(10 * root.uiScale)
                            }
                        }

                        GridLayout {
                            columns: 3
                            Layout.alignment: Qt.AlignHCenter
                            columnSpacing: Math.round(8 * root.uiScale)
                            rowSpacing: Math.round(8 * root.uiScale)

                            Item { Layout.preferredWidth: 88; Layout.preferredHeight: 1 }
                            Button { text: "W 전진"; Layout.preferredWidth: 110; onClicked: appController.controlKeyboardCommand("w") }
                            Item { Layout.preferredWidth: 88; Layout.preferredHeight: 1 }
                            Button { text: "A 좌회전"; Layout.preferredWidth: 110; onClicked: appController.controlKeyboardCommand("a") }
                            Button { text: "X 중립"; Layout.preferredWidth: 110; onClicked: appController.controlSendNeutral() }
                            Button { text: "D 우회전"; Layout.preferredWidth: 110; onClicked: appController.controlKeyboardCommand("d") }
                            Item { Layout.preferredWidth: 88; Layout.preferredHeight: 1 }
                            Button { text: "S 후진"; Layout.preferredWidth: 110; onClicked: appController.controlKeyboardCommand("s") }
                            Item { Layout.preferredWidth: 88; Layout.preferredHeight: 1 }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            Button {
                                text: appController.controlArmed ? "제어 해제" : "제어 ARM"
                                Layout.fillWidth: true
                                highlighted: !appController.controlArmed
                                onClicked: appController.setControlArmed(!appController.controlArmed)
                            }
                            Button {
                                text: "현재 목표 적용"
                                Layout.fillWidth: true
                                onClicked: appController.controlSendManual()
                            }
                        }
                    }
                }

                Frame {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    padding: Math.round(12 * root.uiScale)
                    background: Rectangle { color: "#ffffff"; radius: 14; border.color: "#d7e0ea" }

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: Math.round(10 * root.uiScale)

                        Label {
                            text: "시험 패턴"
                            font.bold: true
                            font.pixelSize: Math.round(17 * root.uiScale)
                            color: "#172033"
                        }
                        Label {
                            Layout.fillWidth: true
                            text: "패턴은 급격한 ±90도 스텝을 쓰지 않습니다. 저속/완만한 각도 변화로 0x503, 0x510~0x513을 반복 송신하고, 실제 성공은 CAN_TX_RAW audit로만 판단합니다."
                            wrapMode: Text.WordWrap
                            color: "#64748b"
                            font.pixelSize: Math.round(11 * root.uiScale)
                        }

                        Flow {
                            Layout.fillWidth: true
                            spacing: Math.round(8 * root.uiScale)
                            Button { text: "완만 조향 -30/+30"; onClicked: appController.controlRunPattern("sweep") }
                            Button { text: "가변 rpm 조향"; onClicked: appController.controlRunPattern("variable") }
                            Button { text: "저속 피벗"; onClicked: appController.controlRunPattern("spin") }
                            Button { text: "패턴 정지"; onClicked: appController.controlStopPattern() }
                        }

                        GridLayout {
                            Layout.fillWidth: true
                            columns: 3
                            columnSpacing: Math.round(8 * root.uiScale)
                            rowSpacing: Math.round(8 * root.uiScale)

                            Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: Math.round(86 * root.uiScale)
                                radius: 10
                                color: "#eff6ff"
                                border.color: "#bfdbfe"
                                ColumnLayout {
                                    anchors.fill: parent
                                    anchors.margins: Math.round(9 * root.uiScale)
                                    spacing: 3
                                    Label { text: "Qt 요청"; font.bold: true; color: "#1d4ed8" }
                                    Label {
                                        Layout.fillWidth: true
                                        text: appController.controlLastCommandSummary
                                        wrapMode: Text.WordWrap
                                        elide: Text.ElideRight
                                        maximumLineCount: 2
                                        color: "#334155"
                                        font.family: root.monoFontFamily
                                        font.pixelSize: Math.round(10 * root.uiScale)
                                    }
                                }
                            }

                            Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: Math.round(86 * root.uiScale)
                                radius: 10
                                color: root.ackKind() === "bad" ? "#fff1f2" : "#f8fafc"
                                border.color: root.ackKind() === "bad" ? "#fecdd3" : "#dbe5f0"
                                ColumnLayout {
                                    anchors.fill: parent
                                    anchors.margins: Math.round(9 * root.uiScale)
                                    spacing: 3
                                    Label { text: "CONTROL_ACK"; font.bold: true; color: root.ackKind() === "bad" ? "#be123c" : "#475569" }
                                    Label {
                                        Layout.fillWidth: true
                                        text: appController.controlLastAckSummary
                                        wrapMode: Text.WordWrap
                                        elide: Text.ElideRight
                                        maximumLineCount: 2
                                        color: "#334155"
                                        font.family: root.monoFontFamily
                                        font.pixelSize: Math.round(10 * root.uiScale)
                                    }
                                }
                            }

                            Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: Math.round(86 * root.uiScale)
                                radius: 10
                                color: "#ecfdf3"
                                border.color: "#bbf7d0"
                                ColumnLayout {
                                    anchors.fill: parent
                                    anchors.margins: Math.round(9 * root.uiScale)
                                    spacing: 3
                                    Label { text: "실제 TX audit"; font.bold: true; color: "#166534" }
                                    Label {
                                        Layout.fillWidth: true
                                        text: appController.controlLastAuditSummary
                                        wrapMode: Text.WordWrap
                                        elide: Text.ElideRight
                                        maximumLineCount: 2
                                        color: "#334155"
                                        font.family: root.monoFontFamily
                                        font.pixelSize: Math.round(10 * root.uiScale)
                                    }
                                }
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            radius: 10
                            color: "#fff7ed"
                            border.color: "#fed7aa"
                            implicitHeight: statsLabel.implicitHeight + Math.round(18 * root.uiScale)
                            Label {
                                id: statsLabel
                                anchors.fill: parent
                                anchors.margins: Math.round(9 * root.uiScale)
                                text: appController.controlEvidenceStatsSummary
                                wrapMode: Text.WordWrap
                                color: "#9a3412"
                                font.family: root.monoFontFamily
                                font.pixelSize: Math.round(10 * root.uiScale)
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            Layout.minimumHeight: Math.round(180 * root.uiScale)
                            radius: 10
                            color: "#f8fafc"
                            border.color: "#dbe5f0"

                            ColumnLayout {
                                anchors.fill: parent
                                anchors.margins: Math.round(10 * root.uiScale)
                                spacing: Math.round(6 * root.uiScale)

                                RowLayout {
                                    Layout.fillWidth: true
                                    Label { text: "증거 타임라인"; font.bold: true; color: "#172033" }
                                    Label {
                                        text: "반복 OK 이벤트는 샘플링 표시"
                                        color: "#64748b"
                                        font.pixelSize: Math.round(11 * root.uiScale)
                                    }
                                    Item { Layout.fillWidth: true }
                                }

                                ListView {
                                    Layout.fillWidth: true
                                    Layout.fillHeight: true
                                    clip: true
                                    model: appController.controlEvidenceModel
                                    spacing: Math.round(4 * root.uiScale)
                                    delegate: Rectangle {
                                        width: ListView.view.width
                                        radius: 8
                                        color: level === "ok" ? "#ecfdf3" : (level === "error" ? "#fff1f2" : (level === "warn" ? "#fff7ed" : "#ffffff"))
                                        border.color: level === "ok" ? "#bbf7d0" : (level === "error" ? "#fecdd3" : (level === "warn" ? "#fed7aa" : "#e2e8f0"))
                                        implicitHeight: eventColumn.implicitHeight + Math.round(12 * root.uiScale)

                                        ColumnLayout {
                                            id: eventColumn
                                            anchors.left: parent.left
                                            anchors.right: parent.right
                                            anchors.verticalCenter: parent.verticalCenter
                                            anchors.margins: Math.round(7 * root.uiScale)
                                            spacing: 2

                                            RowLayout {
                                                Layout.fillWidth: true
                                                Label {
                                                    text: timeText + "  " + stage
                                                    color: "#0f172a"
                                                    font.bold: true
                                                    font.family: root.monoFontFamily
                                                    font.pixelSize: Math.round(10 * root.uiScale)
                                                }
                                                Label {
                                                    text: [commandId, bus, canId].filter(function(x) { return x && x.length > 0 }).join(" ")
                                                    color: "#475569"
                                                    font.family: root.monoFontFamily
                                                    font.pixelSize: Math.round(10 * root.uiScale)
                                                }
                                                Item { Layout.fillWidth: true }
                                            }

                                            Label {
                                                Layout.fillWidth: true
                                                text: summary
                                                color: "#334155"
                                                wrapMode: Text.WordWrap
                                                font.pixelSize: Math.round(11 * root.uiScale)
                                            }
                                        }
                                    }

                                    Label {
                                        anchors.centerIn: parent
                                        visible: appController.controlEvidenceModel.count === 0
                                        text: "아직 제어 evidence 없음"
                                        color: "#94a3b8"
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
