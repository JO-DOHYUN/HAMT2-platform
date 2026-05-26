import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components" as Components

Item {
    signal openReplay()
    signal openTypedReplay()
    signal playReplay(real speed)
    signal pauseReplay()
    signal stopReplay()
    signal setReplayLoop(bool enabled)
    signal seekReplay(real progress)
    signal stepReplay(int delta)

    property bool autoFollow: true
    property real uiScale: 1.0
    property bool internalSeekUpdate: false
    property real pendingSeekValue: 0.0
    property bool pendingSeekInitialized: false
    property bool seekCommitPending: false
    property bool showTimingMarkers: true
    property bool showValueMarkers: true
    property bool showAlarmMarkers: true

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

    function markerVisible(kind) {
        if (kind === "timing") return showTimingMarkers
        if (kind === "value") return showValueMarkers
        if (kind === "alarm") return showAlarmMarkers
        return true
    }

    function markerColor(kind, severity) {
        if (severity === "ERR") return "#c0392b"
        if (kind === "timing") return "#2563eb"
        if (kind === "value") return "#d97706"
        if (kind === "alarm") return "#b45309"
        return "#52606d"
    }

    function markerTip(modelData) {
        return (modelData.kind === "timing" ? "주기" : (modelData.kind === "value" ? "값" : "경보"))
               + " · " + modelData.idText + " · " + modelData.severity
               + " · " + modelData.timeText
               + (modelData.note !== "" ? ("\n" + modelData.note) : "")
    }

    Connections {
        target: appController.replayFrameView
        function onCountChanged() {
            if (autoFollow && replayList.count > 0)
                replayList.positionViewAtBeginning()
        }
    }

    Connections {
        target: appController
        function onReplayStateChanged() {
            if (replaySlider.pressed)
                return
            if (appController.replayRebuilding)
                pendingSeekValue = appController.replayTargetProgress
            else
                pendingSeekValue = appController.replayProgress
            pendingSeekInitialized = true
            seekCommitPending = false
        }
    }

    Component.onCompleted: {
        pendingSeekValue = appController.replayProgress
        pendingSeekInitialized = true
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 2

        Frame {
            Layout.fillWidth: true
            background: Rectangle { color: "#ffffff"; radius: 10; border.color: "#dbe5f0" }
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 5
                spacing: 2

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 5
                    Button { text: "재생 열기"; font.pixelSize: Math.round(10.8 * uiScale); onClicked: openReplay() }
                    Button { text: "Typed 폴더"; font.pixelSize: Math.round(10.8 * uiScale); onClicked: openTypedReplay() }
                    Label { text: "x" + appController.replaySpeed.toFixed(2); color: "#35506b"; font.pixelSize: Math.round(10.6 * uiScale) }
                    Button { text: "◀주"; font.pixelSize: Math.round(10.6 * uiScale); enabled: appController.replayLoaded && appController.replayTimingMarkerCount > 0; onClicked: appController.seekReplayIssue("timing", -1) }
                    Button { text: "주▶"; font.pixelSize: Math.round(10.6 * uiScale); enabled: appController.replayLoaded && appController.replayTimingMarkerCount > 0; onClicked: appController.seekReplayIssue("timing", 1) }
                    Button { text: "◀값"; font.pixelSize: Math.round(10.6 * uiScale); enabled: appController.replayLoaded && appController.replayValueMarkerCount > 0; onClicked: appController.seekReplayIssue("value", -1) }
                    Button { text: "값▶"; font.pixelSize: Math.round(10.6 * uiScale); enabled: appController.replayLoaded && appController.replayValueMarkerCount > 0; onClicked: appController.seekReplayIssue("value", 1) }
                    Button { text: "◀경"; font.pixelSize: Math.round(10.6 * uiScale); enabled: appController.replayLoaded && appController.replayAlarmMarkerCount > 0; onClicked: appController.seekReplayIssue("alarm", -1) }
                    Button { text: "경▶"; font.pixelSize: Math.round(10.6 * uiScale); enabled: appController.replayLoaded && appController.replayAlarmMarkerCount > 0; onClicked: appController.seekReplayIssue("alarm", 1) }
                    TextField {
                        id: replaySeekIdField
                        Layout.preferredWidth: 96
                        placeholderText: "ID"
                        selectByMouse: true
                        onAccepted: if (appController.replayLoaded && text !== "") appController.seekReplayId(text, 1)
                    }
                    Button { text: "◀ID"; font.pixelSize: Math.round(10.6 * uiScale); enabled: appController.replayLoaded && replaySeekIdField.text !== ""; onClicked: appController.seekReplayId(replaySeekIdField.text, -1) }
                    Button { text: "ID▶"; font.pixelSize: Math.round(10.6 * uiScale); enabled: appController.replayLoaded && replaySeekIdField.text !== ""; onClicked: appController.seekReplayId(replaySeekIdField.text, 1) }
                    CheckBox { text: "주"; checked: showTimingMarkers; onToggled: showTimingMarkers = checked }
                    CheckBox { text: "값"; checked: showValueMarkers; onToggled: showValueMarkers = checked }
                    CheckBox { text: "경"; checked: showAlarmMarkers; onToggled: showAlarmMarkers = checked }
                    CheckBox { text: "자동"; checked: autoFollow; onToggled: autoFollow = checked }
                    TextField {
                        Layout.preferredWidth: 110
                        placeholderText: "필터 ID"
                        text: appController.replayFrameView.idFilter
                        selectByMouse: true
                        onTextEdited: appController.replayFrameView.idFilter = text
                    }
                    Item { Layout.fillWidth: true }
                    Label {
                        text: appController.replayPath === "" ? "재생 파일 미선택" : appController.replayPath
                        color: "#607080"
                        Layout.preferredWidth: Math.round(280 * uiScale)
                        elide: Label.ElideMiddle
                        horizontalAlignment: Text.AlignRight
                        font.pixelSize: Math.round(10.6 * uiScale)
                    }
                }

                Item {
                    Layout.fillWidth: true
                    Layout.preferredHeight: Math.round(20 * uiScale)
                    visible: appController.replayLoaded

                    Rectangle {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        height: Math.max(5, Math.round(5 * uiScale))
                        radius: height / 2
                        color: "#edf2f7"
                        border.color: "#dbe5f0"
                    }

                    Repeater {
                        model: appController.replayIssueMarkers
                        delegate: Rectangle {
                            required property var modelData
                            visible: markerVisible(modelData.kind)
                            width: modelData.severity === "ERR" ? Math.max(5, Math.round(5 * uiScale)) : Math.max(3, Math.round(3 * uiScale))
                            height: Math.round(16 * uiScale)
                            radius: width / 2
                            color: markerColor(modelData.kind, modelData.severity)
                            border.color: "#ffffff"
                            x: Math.max(0, Math.min(parent.width - width, modelData.progress * Math.max(1, parent.width - width)))
                            y: Math.round(2 * uiScale)
                            opacity: modelData.severity === "ERR" ? 0.98 : 0.78
                            TapHandler {
                                onTapped: appController.jumpReplayToFrameIndex(modelData.index,
                                    (modelData.kind === "timing" ? "주기" : (modelData.kind === "value" ? "값" : "경보"))
                                    + " 마커 이동 · " + modelData.idText + " · " + modelData.timeText)
                            }
                            HoverHandler { id: markerHover }
                            ToolTip.visible: markerHover.hovered
                            ToolTip.text: markerTip(modelData)
                        }
                    }
                }

                Label {
                    Layout.fillWidth: true
                    text: appController.replayCursorSummary
                    color: appController.replayPlaying ? "#15803d" : (appController.replayRebuilding ? "#0f4c81" : "#52606d")
                    elide: Label.ElideRight
                    font.pixelSize: Math.round(10.8 * uiScale)
                }
                Label {
                    Layout.fillWidth: true
                    text: "legacy .bin은 파일로 열고, 새 typed 녹화는 capture.stream 파일 또는 *.typed 폴더로 엽니다."
                    color: "#64748b"
                    elide: Label.ElideRight
                    font.pixelSize: Math.round(10.4 * uiScale)
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
                    Components.StatusBadge { text: "REPLAY " + (appController.replayPlaying ? "RUN" : "HOLD"); kind: appController.replayPlaying ? "warn" : "info"; uiScale: uiScale }
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
                anchors.margins: 5
                spacing: 6

                RowLayout {
                    Layout.fillWidth: true
                    Label { text: "재생 프레임"; font.bold: true; color: "#243447"; font.pixelSize: Math.round(11.0 * uiScale) }
                    Item { Layout.fillWidth: true }
                    Label { text: (appController.replayPlaying ? ("x" + appController.replaySpeed.toFixed(2)) : "정지") + " · " + appController.replayIssueSummary; color: appController.replayPlaying ? "#b45309" : "#15803d" }
                }

                ListView {
                    id: replayList
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    spacing: 2
                    reuseItems: true
                    cacheBuffer: 220
                    boundsBehavior: Flickable.StopAtBounds
                    model: appController.replayFrameView
                    onMovementStarted: autoFollow = false
                    ScrollBar.vertical: ScrollBar { policy: ScrollBar.AlwaysOn; active: true }
                    WheelHandler {
                        target: null
                        acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
                        onWheel: function(event) { autoFollow = false; applyWheel(replayList, event) }
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
                        color: index % 2 === 0 ? "#fffdf8" : "#fff8ef"
                        border.color: "#ead9b5"

                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 5
                            spacing: 8
                            Rectangle {
                                Layout.preferredWidth: 70
                                Layout.preferredHeight: Math.round(20 * uiScale)
                                radius: 6
                                color: "#fff0cc"
                                Label { anchors.centerIn: parent; text: idText; color: "#9a5b00"; font.bold: true; font.pixelSize: Math.round(11 * uiScale) }
                            }
                            Label { text: "BUS " + bus; color: "#6b7280"; Layout.preferredWidth: 54 }
                            Label { text: dataHex; color: "#3b2f1d"; Layout.fillWidth: true; elide: Label.ElideRight; font.family: "Consolas"; font.pixelSize: Math.round(11 * uiScale) }
                            Label { text: timeText; color: "#6b7280"; Layout.preferredWidth: 92; horizontalAlignment: Text.AlignRight }
                            Rectangle {
                                Layout.preferredWidth: 54
                                Layout.preferredHeight: Math.round(20 * uiScale)
                                radius: 6
                                color: "#fff0cc"
                                Label { anchors.centerIn: parent; text: source; color: "#9a5b00" }
                            }
                        }
                    }
                }
            }
        }
    }
}
