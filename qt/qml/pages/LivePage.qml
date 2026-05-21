import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components" as Components

Item {
    signal startLog()
    signal stopLog()
    signal savePendingLog()
    signal discardPendingLog()

    property bool autoFollow: true
    property real uiScale: 1.0

    function kindFromLevel(level) {
        if (level === "ERR") return "bad"
        if (level === "WARN") return "warn"
        if (level === "OK") return "ok"
        return "info"
    }

    function applyWheel(view, event) {
        let delta = 0
        if (event.pixelDelta && event.pixelDelta.y)
            delta = -event.pixelDelta.y
        else if (event.angleDelta && event.angleDelta.y)
            delta = -(event.angleDelta.y / 120.0) * (44 * uiScale)
        if (delta !== 0) {
            const maxY = Math.max(0, view.contentHeight - view.height)
            view.contentY = Math.max(0, Math.min(maxY, view.contentY + delta))
            if (event.accepted !== undefined)
                event.accepted = true
        }
    }

    Connections {
        target: appController.liveFrameView
        function onCountChanged() {
            if (autoFollow && liveList.count > 0)
                liveList.positionViewAtBeginning()
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 4

        Frame {
            Layout.fillWidth: true
            background: Rectangle { color: "#ffffff"; radius: 10; border.color: "#dbe5f0" }
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 9
                spacing: 8

                RowLayout {
                    Layout.fillWidth: true
                    Label { text: "라이브 스트림"; font.pixelSize: Math.round(19 * uiScale); font.bold: true; color: "#243447" }
                    Rectangle { width: 10; height: 10; radius: 5; color: appController.liveUiPaused ? "#d97706" : "#16a34a" }
                    Label {
                        text: appController.liveUiPaused ? "화면 정지" : "실시간 반영"
                        color: appController.liveUiPaused ? "#b45309" : "#15803d"
                    }
                    Item { Layout.fillWidth: true }
                    Label { text: "표시: " + appController.liveFrameView.count + " / 원본: " + appController.liveFrames.count; color: "#5b6673"; font.pixelSize: Math.round(12 * uiScale) }
                }

                RowLayout {
                    Layout.fillWidth: true
                    Button {
                        text: appController.logRecordingActive ? "기록 중" : (appController.logStopping ? "종료 중" : (appController.logSaving ? "저장 중" : (appController.logPendingSave ? "저장 대기" : "로그 시작")))
                        enabled: appController.connected && !appController.logRecordingActive && !appController.logPendingSave && !appController.logStopping && !appController.logSaving
                        onClicked: startLog()
                    }
                    Button { text: appController.liveUiPaused ? "화면 재개" : "화면 정지"; onClicked: appController.toggleLiveUiPaused() }
                    Button { text: "프레임 지우기"; onClicked: appController.clearFrames() }
                    Button {
                        text: appController.logRecordingActive ? "기록 중지·저장" : (appController.logStopping ? "종료 중" : (appController.logPendingSave ? "저장하기" : "저장하기"))
                        enabled: (appController.logRecordingActive || appController.logPendingSave) && !appController.logStopping && !appController.logSaving
                        onClicked: stopLog()
                    }
                    Button {
                        text: "폐기"
                        enabled: appController.logPendingSave && !appController.logRecordingActive && !appController.logStopping && !appController.logSaving
                        onClicked: discardPendingLog()
                    }
                    TextField {
                        Layout.preferredWidth: 180
                        placeholderText: "ID 필터 (예: 0x117,118)"
                        text: appController.liveFrameView.idFilter
                        selectByMouse: true
                        onTextEdited: appController.liveFrameView.idFilter = text
                    }
                    CheckBox {
                        text: "자동 맨위 따라가기"
                        checked: autoFollow
                        onToggled: autoFollow = checked
                    }
                    Item { Layout.fillWidth: true }
                }

                RowLayout {
                    Layout.fillWidth: true
                    Label {
                        text: appController.modelSourceSummary
                        Layout.fillWidth: true
                        elide: Label.ElideMiddle
                        color: appController.modelActive ? "#52606d" : "#b45309"
                    }
                    Rectangle {
                        Layout.preferredWidth: 460
                        Layout.preferredHeight: Math.round(28 * uiScale)
                        radius: 8
                        color: appController.logRecordingActive ? "#ecfdf5" : (appController.logPendingSave ? "#fff7ed" : "#f8fafc")
                        border.color: appController.logRecordingActive ? "#86efac" : (appController.logPendingSave ? "#fdba74" : "#dbe5f0")
                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 6
                            spacing: 8
                            Rectangle {
                                Layout.preferredWidth: 10
                                Layout.preferredHeight: 10
                                radius: 5
                                color: appController.logRecordingActive ? "#16a34a" : (appController.logPendingSave ? "#d97706" : "#94a3b8")
                            }
                            Label {
                                text: appController.logStatusSummary
                                Layout.fillWidth: true
                                elide: Label.ElideMiddle
                                color: appController.logRecordingActive ? "#166534" : (appController.logPendingSave ? "#9a3412" : (appController.logStopping || appController.logSaving ? "#7c2d12" : "#52606d"))
                            }
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    radius: 8
                    color: "#f8fafc"
                    border.color: "#dbe5f0"
                    implicitHeight: Math.round(36 * uiScale)
                    Label {
                        anchors.fill: parent
                        anchors.margins: 6
                        text: appController.logActionText
                        wrapMode: Text.Wrap
                        color: "#52606d"
                        verticalAlignment: Text.AlignVCenter
                    }
                }
            }
        }

        Frame {
            Layout.fillWidth: true
            background: Rectangle { color: "#ffffff"; radius: 10; border.color: "#dbe5f0" }
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 8
                spacing: 6
                RowLayout {
                    Layout.fillWidth: true
                    Components.StatusBadge { text: "SYS " + appController.systemLevel; kind: appController.systemLevel === "ERR" ? "bad" : (appController.systemLevel === "WARN" ? "warn" : "ok"); uiScale: uiScale }
                    Components.StatusBadge { text: "BUS " + appController.busHealthLevel; kind: kindFromLevel(appController.busHealthLevel); uiScale: uiScale }
                    Components.StatusBadge { text: "ACTION " + appController.operatorActionLevel; kind: kindFromLevel(appController.operatorActionLevel); uiScale: uiScale }
                    Item { Layout.fillWidth: true }
                    Label { text: appController.primaryIssueId !== "" ? (appController.primaryIssueId + " · " + appController.primaryIssueSummary) : appController.primaryIssueSummary; color: "#607080"; elide: Text.ElideRight; Layout.preferredWidth: Math.round(420 * uiScale) }
                }
                Rectangle {
                    Layout.fillWidth: true
                    radius: 8
                    color: "#f8fafc"
                    border.color: "#dbe5f0"
                    implicitHeight: Math.round(34 * uiScale)
                    Label {
                        anchors.fill: parent
                        anchors.margins: 6
                        text: appController.operatorHeadline + " · " + appController.operatorActionText
                        color: "#35506b"
                        wrapMode: Text.WordWrap
                        verticalAlignment: Text.AlignVCenter
                    }
                }
            }
        }

        Frame {
            Layout.fillWidth: true
            Layout.fillHeight: true
            background: Rectangle { color: "#ffffff"; radius: 10; border.color: "#dbe5f0" }

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 6
                spacing: 8

                RowLayout {
                    Layout.fillWidth: true
                    Label { text: "최근 라이브 프레임"; font.bold: true; color: "#243447" }
                    Item { Layout.fillWidth: true }
                    Label {
                        text: appController.liveUiPaused ? "스크롤/검토용 정지 상태" : (appController.liveFrameView.idFilter === "" ? "실시간 수신 중" : "실시간 수신 중 · ID 필터 적용")
                        color: appController.liveUiPaused ? "#b45309" : "#15803d"
                    }
                }

                ListView {
                    id: liveList
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    spacing: 4
                    reuseItems: true
                    cacheBuffer: 220
                    boundsBehavior: Flickable.StopAtBounds
                    model: appController.liveFrameView
                    onMovementStarted: autoFollow = false
                    ScrollBar.vertical: ScrollBar { policy: ScrollBar.AlwaysOn; active: true }
                    WheelHandler {
                        target: null
                        acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
                        onWheel: function(event) { autoFollow = false; applyWheel(liveList, event) }
                    }

                    delegate: Rectangle {
                        required property int index
                        required property string idText
                        required property int bus
                        required property string dataHex
                        required property string timeText
                        required property string source
                        width: ListView.view.width
                        height: Math.round(30 * uiScale)
                        radius: 8
                        color: index % 2 === 0 ? "#ffffff" : "#f7fbff"
                        border.color: "#d7e0ea"

                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 6
                            spacing: 8
                            Rectangle {
                                Layout.preferredWidth: 70
                                Layout.preferredHeight: Math.round(20 * uiScale)
                                radius: 6
                                color: "#e8f0ff"
                                Label { anchors.centerIn: parent; text: idText; color: "#1e40af"; font.bold: true; font.pixelSize: Math.round(11 * uiScale) }
                            }
                            Label { text: "BUS " + bus; color: "#52606d"; Layout.preferredWidth: 54 }
                            Label { text: dataHex; color: "#0f172a"; Layout.fillWidth: true; elide: Label.ElideRight; font.family: "Consolas"; font.pixelSize: Math.round(11 * uiScale) }
                            Label { text: timeText; color: "#52606d"; Layout.preferredWidth: 92; horizontalAlignment: Text.AlignRight }
                            Rectangle {
                                Layout.preferredWidth: 54
                                Layout.preferredHeight: Math.round(20 * uiScale)
                                radius: 6
                                color: "#e8f7ed"
                                Label { anchors.centerIn: parent; text: source; color: "#15803d" }
                            }
                        }
                    }
                }
            }
        }
    }
}
