import QtQuick
import QtCore
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import "pages" as Pages
import "components" as Components

ApplicationWindow {
    id: root
    width: 1680
    height: 980
    visible: true
    title: "CAN Monitor Reboot"
    color: "#f1f3f6"
    font.pixelSize: Math.round(13 * uiScale)
    font.family: uiFontFamily
    minimumWidth: 1120
    minimumHeight: 720

    property bool workspaceMode: false
    property int uiScaleIndex: 1
    readonly property var uiScaleSteps: [0.9, 1.0, 1.1]
    readonly property real uiScale: uiScaleSteps[Math.max(0, Math.min(uiScaleIndex, uiScaleSteps.length - 1))]
    readonly property string uiFontFamily: Qt.platform.os === "windows" ? "Malgun Gothic" : ""
    readonly property string monoFontFamily: Qt.platform.os === "windows" ? "Cascadia Mono" : "Monospace"

    Component.onCompleted: Qt.callLater(syncAnalysisVisibility)

    function visiblePanelCount() {
        let count = 0
        for (let i = 0; i < workspacePanels.count; ++i) {
            if (workspacePanels.get(i).opened)
                count++
        }
        return count
    }

    function togglePanel(index) {
        const current = workspacePanels.get(index).opened
        workspacePanels.setProperty(index, "opened", !current)
        syncAnalysisVisibility()
    }

    function enablePanel(index) {
        workspacePanels.setProperty(index, "opened", true)
        syncAnalysisVisibility()
    }

    function setWorkspacePreset(primary, secondary, tertiary) {
        for (let i = 0; i < workspacePanels.count; ++i) {
            const on = i === primary || i === secondary || i === tertiary
            workspacePanels.setProperty(i, "opened", on)
        }
        syncAnalysisVisibility()
    }

    function openPanelByKey(key) {
        const map = { overview: 0, live: 1, replay: 2, timing: 3, value: 4, graph: 5, graph_overview: 6, alarm: 7, settings: 8, control: 9, board: 10 }
        const index = map[key] !== undefined ? map[key] : 0
        tabs.currentIndex = index
        if (root.workspaceMode)
            root.enablePanel(index)
        syncAnalysisVisibility()
    }

    function panelVisible(index) {
        return root.workspaceMode ? workspacePanels.get(index).opened : (tabs.currentIndex === index)
    }

    function syncAnalysisVisibility() {
        appController.setPanelActive("overview", panelVisible(0))
        appController.setPanelActive("live", panelVisible(1))
        appController.setPanelActive("timing", panelVisible(3))
        appController.setPanelActive("value", panelVisible(4))
        appController.setPanelActive("graph", panelVisible(5))
        appController.setPanelActive("alarm", panelVisible(7))
    }

    function focusIssue(key, idText) {
        root.openPanelByKey(key)
        if (key === "timing")
            appController.focusTimingId(idText)
        else if (key === "value")
            appController.focusValueId(idText)
        else if (key === "alarm")
            appController.focusAlarmId(idText)
    }

    function focusPrimaryIssue() {
        const key = appController.primaryIssueTargetTab
        const idText = appController.primaryIssueId
        if (key === "timing" || key === "value" || key === "alarm")
            root.focusIssue(key, idText)
        else
            root.openPanelByKey(key)
    }

    function seekPrimaryIssue(direction) {
        if (!appController.primaryIssueSeekAvailable)
            return
        root.openPanelByKey("replay")
        appController.seekReplayPrimaryIssue(direction)
    }

    function componentForKey(key) {
        if (key === "overview") return overviewPageComponent
        if (key === "live") return livePageComponent
        if (key === "replay") return replayPageComponent
        if (key === "timing") return timingPageComponent
        if (key === "value") return valuePageComponent
        if (key === "graph") return graphPageComponent
        if (key === "graph_overview") return graphOverviewPageComponent
        if (key === "alarm") return alarmPageComponent
        if (key === "control") return controlPageComponent
        if (key === "board") return boardHealthPageComponent
        return settingsPageComponent
    }

    function minimumPageWidthForKey(key) {
        if (key === "overview") return 1080
        if (key === "live") return 980
        if (key === "replay") return 1040
        if (key === "timing") return 1140
        if (key === "value") return 1020
        if (key === "graph") return 1140
        if (key === "graph_overview") return 1180
        if (key === "alarm") return 1100
        if (key === "control") return 1120
        if (key === "board") return 1120
        return 1020
    }

    footer: ToolBar {
        padding: 0
        implicitHeight: Math.round(30 * root.uiScale)
        background: Rectangle {
            color: "#ffffff"
            border.color: "#d7e0ea"
        }

        Flickable {
            anchors.fill: parent
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            flickableDirection: Flickable.HorizontalFlick
            contentWidth: footerRow.implicitWidth + Math.round(20 * root.uiScale)
            contentHeight: height
            interactive: contentWidth > width + 1

            RowLayout {
                id: footerRow
                anchors.verticalCenter: parent.verticalCenter
                anchors.left: parent.left
                anchors.leftMargin: Math.round(10 * root.uiScale)
                spacing: Math.round(8 * root.uiScale)

                Components.StatusBadge { text: appController.analysisSourceText; kind: appController.replayAnalysisActive ? "warn" : "ok"; uiScale: root.uiScale }
                Components.StatusBadge { text: "SYS " + appController.systemLevel; kind: appController.systemLevel === "ERR" ? "bad" : (appController.systemLevel === "WARN" ? "warn" : "ok"); uiScale: root.uiScale }
                Components.StatusBadge {
                    text: "ACTION " + appController.operatorActionLevel
                    kind: appController.operatorActionLevel === "ERR" ? "bad" : (appController.operatorActionLevel === "WARN" ? "warn" : "info")
                    uiScale: root.uiScale
                }
                Rectangle {
                    Layout.preferredWidth: Math.round(620 * root.uiScale)
                    Layout.preferredHeight: Math.round(26 * root.uiScale)
                    radius: 8
                    color: "#f7faff"
                    border.color: "#dbe5f0"
                    Label {
                        anchors.fill: parent
                        anchors.margins: 6
                        text: appController.rootCauseSummary
                        color: "#35506b"
                        elide: Text.ElideRight
                        verticalAlignment: Text.AlignVCenter
                        font.pixelSize: Math.round(11 * root.uiScale)
                    }
                }
                Rectangle {
                    Layout.preferredWidth: Math.round(470 * root.uiScale)
                    Layout.preferredHeight: Math.round(26 * root.uiScale)
                    radius: 8
                    color: appController.operatorActionLevel === "ERR" ? "#fff1f2" : (appController.operatorActionLevel === "WARN" ? "#fffaf0" : "#f8fafc")
                    border.color: appController.operatorActionLevel === "ERR" ? "#fecdd3" : (appController.operatorActionLevel === "WARN" ? "#f3d7a1" : "#dbe5f0")
                    Label {
                        anchors.fill: parent
                        anchors.margins: 6
                        text: appController.operatorActionText
                        color: appController.operatorActionLevel === "ERR" ? "#9f1239" : (appController.operatorActionLevel === "WARN" ? "#9a5b00" : "#52606d")
                        elide: Text.ElideRight
                        verticalAlignment: Text.AlignVCenter
                        font.pixelSize: Math.round(11 * root.uiScale)
                    }
                }
                Button {
                    text: "최상위 보기"
                    font.pixelSize: Math.round(11 * root.uiScale)
                    onClicked: root.focusPrimaryIssue()
                }
                Button {
                    text: "◀ 이슈"
                    font.pixelSize: Math.round(11 * root.uiScale)
                    enabled: appController.primaryIssueSeekAvailable
                    onClicked: root.seekPrimaryIssue(-1)
                }
                Button {
                    text: "이슈 ▶"
                    font.pixelSize: Math.round(11 * root.uiScale)
                    enabled: appController.primaryIssueSeekAvailable
                    onClicked: root.seekPrimaryIssue(1)
                }
                Rectangle {
                    Layout.preferredWidth: Math.round(280 * root.uiScale)
                    Layout.preferredHeight: Math.round(26 * root.uiScale)
                    radius: 8
                    color: "#fffaf0"
                    border.color: "#f3d7a1"
                    Label {
                        anchors.fill: parent
                        anchors.margins: 6
                        text: appController.replayIssueSummary
                        color: "#9a5b00"
                        elide: Text.ElideRight
                        verticalAlignment: Text.AlignVCenter
                        font.pixelSize: Math.round(11 * root.uiScale)
                    }
                }
                Rectangle {
                    Layout.preferredWidth: Math.round(280 * root.uiScale)
                    Layout.preferredHeight: Math.round(26 * root.uiScale)
                    radius: 8
                    color: "#f8fafc"
                    border.color: "#dbe5f0"
                    Label {
                        anchors.fill: parent
                        anchors.margins: 6
                        text: "업타임 " + appController.sessionUptimeText
                        color: "#52606d"
                        elide: Text.ElideRight
                        verticalAlignment: Text.AlignVCenter
                        font.pixelSize: Math.round(11 * root.uiScale)
                    }
                }
                Rectangle {
                    Layout.preferredWidth: Math.round(360 * root.uiScale)
                    Layout.preferredHeight: Math.round(26 * root.uiScale)
                    radius: 8
                    color: "#f7faff"
                    border.color: "#dbe5f0"
                    Label {
                        anchors.fill: parent
                        anchors.margins: 6
                        text: appController.operatorRecentSummary
                        color: "#35506b"
                        elide: Text.ElideRight
                        verticalAlignment: Text.AlignVCenter
                        font.pixelSize: Math.round(11 * root.uiScale)
                    }
                }
                Button { text: "분석 초기화"; font.pixelSize: Math.round(11 * root.uiScale); onClicked: appController.resetAnalysisContext() }
                Button { text: "필터 초기화"; font.pixelSize: Math.round(11 * root.uiScale); onClicked: appController.resetAllAnalysisFilters() }
                Button {
                    text: "라이브 복귀"
                    font.pixelSize: Math.round(12 * root.uiScale)
                    enabled: appController.connected && appController.replayAnalysisHeld
                    onClicked: appController.useLiveAnalysis()
                }
            }

            ScrollBar.horizontal: ScrollBar { }
        }
    }

    header: ToolBar {
        padding: 0
        implicitHeight: Math.round(44 * root.uiScale)
        background: Rectangle {
            gradient: Gradient {
                GradientStop { position: 0.0; color: "#ffffff" }
                GradientStop { position: 1.0; color: "#f7faff" }
            }
            border.color: "#d7e0ea"
        }

        Flickable {
            id: toolbarFlick
            anchors.fill: parent
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            flickableDirection: Flickable.HorizontalFlick
            contentWidth: toolbarContent.width
            contentHeight: height
            interactive: contentWidth > width + 1

            Item {
                id: toolbarContent
                width: toolbarRow.implicitWidth + Math.round(16 * root.uiScale)
                height: toolbarFlick.height

                RowLayout {
                    id: toolbarRow
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                    anchors.leftMargin: Math.round(8 * root.uiScale)
                    spacing: Math.round(8 * root.uiScale)

                    ComboBox {
                        id: portCombo
                        Layout.preferredWidth: 170
                        model: appController.availablePorts
                    }
                    Button { text: "포트 새로고침"; font.pixelSize: Math.round(12 * root.uiScale); onClicked: appController.refreshPorts() }
                    Button { text: "연결"; font.pixelSize: Math.round(12 * root.uiScale); onClicked: appController.connectPort(portCombo.currentText) }
                    Button { text: "해제"; font.pixelSize: Math.round(12 * root.uiScale); onClicked: appController.disconnectPort() }
                    Components.StatusBadge {
                        text: appController.transportModeText
                        kind: appController.transportMode === "typed" ? "ok" : "info"
                        uiScale: root.uiScale
                        maxWidth: Math.round(132 * root.uiScale)
                    }
                    Components.StatusBadge {
                        visible: appController.transportMode === "typed"
                        text: appController.typedCanSummary
                        kind: appController.typedTransportFaultCount > 0 ? "warn" : (appController.typedRecordCount > 0 ? "ok" : "info")
                        uiScale: root.uiScale
                        maxWidth: Math.round(520 * root.uiScale)
                    }
                    Components.StatusBadge {
                        visible: appController.transportMode === "typed"
                        text: appController.typedEvidenceSummary
                        kind: appController.typedTransportFaultCount > 0 ? "warn" : "info"
                        uiScale: root.uiScale
                        maxWidth: Math.round(520 * root.uiScale)
                    }
                    Components.StatusBadge {
                        text: appController.logRecordingActive ? "LOG REC" : (appController.logPendingSave ? "SAVE PENDING" : "LOG IDLE")
                        kind: appController.logRecordingActive ? "ok" : (appController.logPendingSave ? "warn" : "info")
                        uiScale: root.uiScale
                    }

                    Rectangle { Layout.preferredWidth: 1; Layout.fillHeight: true; color: "#d7e0ea" }

                    Button { text: "모델 선택"; font.pixelSize: Math.round(12 * root.uiScale); onClicked: modelDialog.open() }
                    Button { text: "기본 모델"; font.pixelSize: Math.round(12 * root.uiScale); onClicked: appController.useBundledModel() }
                    Button { text: "모델 해제"; font.pixelSize: Math.round(12 * root.uiScale); onClicked: appController.clearModel() }
                    Rectangle {
                        Layout.preferredWidth: 310
                        Layout.fillHeight: true
                        radius: 10
                        color: appController.modelActive ? "#eef6ff" : "#fff7ed"
                        border.color: appController.modelActive ? "#bfd6ff" : "#f2c485"
                        Label {
                            anchors.fill: parent
                            anchors.margins: 7
                            text: appController.modelSourceSummary
                            verticalAlignment: Text.AlignVCenter
                            elide: Text.ElideMiddle
                            color: appController.modelActive ? "#2457a6" : "#a05a00"
                            font.pixelSize: Math.round(12 * root.uiScale)
                        }
                        ToolTip.visible: modelHover.hovered
                        ToolTip.text: appController.modelSourceSummary
                        HoverHandler { id: modelHover }
                    }

                    Item { Layout.fillWidth: true }

                    Components.StatusBadge {
                        text: appController.connected ? "연결됨" : "미연결"
                        kind: appController.connected ? "ok" : "bad"
                        uiScale: root.uiScale
                    }
                    ComboBox {
                        id: scaleCombo
                        Layout.preferredWidth: 78
                        model: ["90%", "100%", "110%"]
                        currentIndex: root.uiScaleIndex
                        onActivated: root.uiScaleIndex = currentIndex
                    }
                }
            }

            ScrollBar.horizontal: ScrollBar {
                policy: toolbarFlick.interactive ? ScrollBar.AsNeeded : ScrollBar.AlwaysOff
            }
        }

    }

    FileDialog {
        id: modelDialog
        title: "모델 JSON 선택 (1파일 팩)"
        nameFilters: ["JSON files (*.json)"]
        onAccepted: appController.modelPath = selectedFile.toString()
    }

    FileDialog {
        id: replayDialog
        title: "재생 파일 선택 (.bin 또는 capture.stream)"
        nameFilters: ["Replay files (*.bin capture.stream)", "Legacy BIN (*.bin)", "Typed stream (capture.stream)", "All files (*)"]
        onAccepted: appController.loadReplay(selectedFile.toString())
    }

    FolderDialog {
        id: typedReplayFolderDialog
        title: "Typed capture 폴더 선택"
        onAccepted: appController.loadReplay(selectedFolder.toString())
    }

    FileDialog {
        id: logSaveDialog
        title: "로그 저장"
        fileMode: FileDialog.SaveFile
        currentFile: appController.suggestedLogSavePath
        nameFilters: ["BIN files (*.bin)"]
        onAccepted: appController.finalizePendingLogSave(selectedFile.toString())
    }

    Connections {
        target: appController
        function onLogStateChanged() {
            if (appController.logPendingSave && !appController.logRecordingActive && !appController.logStopping && !appController.logSaving && !logSaveDialog.visible)
                logSaveDialog.open()
        }
    }

    FileDialog {
        id: snapshotDialog
        title: "분석 스냅샷 저장"
        fileMode: FileDialog.SaveFile
        currentFolder: StandardPaths.writableLocation(StandardPaths.DocumentsLocation)
        nameFilters: ["JSON files (*.json)"]
        onAccepted: appController.exportAnalysisSnapshot(selectedFile.toString())
    }

    Shortcut {
        sequences: [StandardKey.Open]
        enabled: !modelDialog.visible && !snapshotDialog.visible && !logSaveDialog.visible
        onActivated: {
            if ((!root.workspaceMode && tabs.currentIndex === 2) || (root.workspaceMode && panelVisible(2))) replayDialog.open()
            else modelDialog.open()
        }
    }
    Shortcut {
        sequence: "Ctrl+R"
        enabled: !replayDialog.visible && !modelDialog.visible && !snapshotDialog.visible && !logSaveDialog.visible
        onActivated: replayDialog.open()
    }
    Shortcut {
        sequence: "Ctrl+E"
        enabled: !snapshotDialog.visible && !modelDialog.visible && !replayDialog.visible && !logSaveDialog.visible
        onActivated: snapshotDialog.open()
    }
    Shortcut {
        sequence: "Ctrl+L"
        enabled: !logSaveDialog.visible && !modelDialog.visible && !replayDialog.visible && !snapshotDialog.visible
        onActivated: {
            if (appController.logRecordingActive || appController.logPendingSave) appController.stopLog()
            else if (appController.connected && !appController.logStopping && !appController.logSaving) appController.startLog()
        }
    }
    Shortcut {
        sequence: "Space"
        enabled: appController.replayLoaded && !replayDialog.visible && !modelDialog.visible && !snapshotDialog.visible && !logSaveDialog.visible
        onActivated: {
            if (appController.replayPlaying) appController.pauseReplay()
            else appController.playReplay(appController.replaySpeed > 0 ? appController.replaySpeed : 1.0)
        }
    }
    Shortcut {
        sequence: "Left"
        enabled: appController.replayLoaded && !replayDialog.visible && !modelDialog.visible && !snapshotDialog.visible && !logSaveDialog.visible
        onActivated: appController.stepReplay(-1)
    }
    Shortcut {
        sequence: "Right"
        enabled: appController.replayLoaded && !replayDialog.visible && !modelDialog.visible && !snapshotDialog.visible && !logSaveDialog.visible
        onActivated: appController.stepReplay(1)
    }
    Shortcut {
        sequence: "F6"
        enabled: !replayDialog.visible && !modelDialog.visible && !snapshotDialog.visible && !logSaveDialog.visible
        onActivated: root.focusPrimaryIssue()
    }
    Shortcut {
        sequence: "F7"
        enabled: appController.primaryIssueSeekAvailable && !replayDialog.visible && !modelDialog.visible && !snapshotDialog.visible && !logSaveDialog.visible
        onActivated: root.seekPrimaryIssue(-1)
    }
    Shortcut {
        sequence: "F8"
        enabled: appController.primaryIssueSeekAvailable && !replayDialog.visible && !modelDialog.visible && !snapshotDialog.visible && !logSaveDialog.visible
        onActivated: root.seekPrimaryIssue(1)
    }

    ListModel {
        id: workspacePanels
        ListElement { key: "overview"; title: "개요"; opened: true }
        ListElement { key: "live"; title: "라이브"; opened: true }
        ListElement { key: "replay"; title: "재생"; opened: false }
        ListElement { key: "timing"; title: "주기"; opened: false }
        ListElement { key: "value"; title: "값"; opened: false }
        ListElement { key: "graph"; title: "그래프"; opened: false }
        ListElement { key: "graph_overview"; title: "전체그래프"; opened: false }
        ListElement { key: "alarm"; title: "경보"; opened: false }
        ListElement { key: "settings"; title: "설정"; opened: false }
        ListElement { key: "control"; title: "제어"; opened: false }
        ListElement { key: "board"; title: "보드"; opened: false }
    }

    Component {
        id: overviewPageComponent
        Pages.OverviewPage {
            uiScale: root.uiScale
            onOpenPanel: function(key) { root.openPanelByKey(key) }
            onFocusIssue: function(key, idText) { root.focusIssue(key, idText) }
            onOpenReplay: replayDialog.open()
            onExportSnapshot: snapshotDialog.open()
        }
    }

    Component {
        id: livePageComponent
        Pages.LivePage {
            uiScale: root.uiScale
            onStartLog: appController.startLog()
            onStopLog: appController.stopLog()
            onSavePendingLog: logSaveDialog.open()
            onDiscardPendingLog: appController.discardPendingLog()
        }
    }

    Component {
        id: replayPageComponent
        Pages.ReplayPage {
            uiScale: root.uiScale
            onOpenReplay: replayDialog.open()
            onOpenTypedReplay: typedReplayFolderDialog.open()
            onPlayReplay: function(speed) { appController.playReplay(speed) }
            onPauseReplay: appController.pauseReplay()
            onStopReplay: appController.stopReplay()
            onSetReplayLoop: function(enabled) { appController.setReplayLoop(enabled) }
            onSeekReplay: function(progress) { appController.seekReplay(progress) }
            onStepReplay: function(delta) { appController.stepReplay(delta) }
        }
    }

    Component { id: timingPageComponent; Pages.TimingPage { uiScale: root.uiScale } }
    Component { id: valuePageComponent; Pages.ValuePage { uiScale: root.uiScale; uiFontFamily: root.uiFontFamily; monoFontFamily: root.monoFontFamily } }
    Component { id: graphPageComponent; Pages.GraphPage { uiScale: root.uiScale } }
    Component { id: graphOverviewPageComponent; Pages.GraphOverviewPage { uiScale: root.uiScale } }
    Component { id: alarmPageComponent; Pages.AlarmPage { uiScale: root.uiScale } }
    Component { id: controlPageComponent; Pages.ControlPage { uiScale: root.uiScale; monoFontFamily: root.monoFontFamily } }
    Component { id: boardHealthPageComponent; Pages.BoardHealthPage { uiScale: root.uiScale; monoFontFamily: root.monoFontFamily } }
    Component {
        id: settingsPageComponent
        Pages.SettingsPage {
            uiScale: root.uiScale
            onExportSnapshot: snapshotDialog.open()
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Math.round(4 * root.uiScale)
        spacing: Math.round(4 * root.uiScale)

        Frame {
            Layout.fillWidth: true
            padding: 0
            background: Rectangle { color: "#ffffff"; radius: 10; border.color: "#d7e0ea" }
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: Math.round(4 * root.uiScale)
                spacing: Math.round(3 * root.uiScale)

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Math.round(6 * root.uiScale)
                    TabBar {
                        id: tabs
                        Layout.fillWidth: true
                        currentIndex: 0
                        implicitHeight: Math.round(28 * root.uiScale)
                        onCurrentIndexChanged: root.syncAnalysisVisibility()
                        TabButton { text: "개요"; font.pixelSize: Math.round(11.2 * root.uiScale); onClicked: if (root.workspaceMode) root.enablePanel(0) }
                        TabButton { text: "라이브"; font.pixelSize: Math.round(11.2 * root.uiScale); onClicked: if (root.workspaceMode) root.enablePanel(1) }
                        TabButton { text: "재생"; font.pixelSize: Math.round(11.2 * root.uiScale); onClicked: if (root.workspaceMode) root.enablePanel(2) }
                        TabButton { text: "주기"; font.pixelSize: Math.round(11.2 * root.uiScale); onClicked: if (root.workspaceMode) root.enablePanel(3) }
                        TabButton { text: "값"; font.pixelSize: Math.round(11.2 * root.uiScale); onClicked: if (root.workspaceMode) root.enablePanel(4) }
                        TabButton { text: "그래프"; font.pixelSize: Math.round(11.2 * root.uiScale); onClicked: if (root.workspaceMode) root.enablePanel(5) }
                        TabButton { text: "전체그래프"; font.pixelSize: Math.round(11.2 * root.uiScale); onClicked: if (root.workspaceMode) root.enablePanel(6) }
                        TabButton { text: "경보"; font.pixelSize: Math.round(11.2 * root.uiScale); onClicked: if (root.workspaceMode) root.enablePanel(7) }
                        TabButton { text: "설정"; font.pixelSize: Math.round(11.2 * root.uiScale); onClicked: if (root.workspaceMode) root.enablePanel(8) }
                        TabButton { text: "제어"; font.pixelSize: Math.round(11.2 * root.uiScale); onClicked: if (root.workspaceMode) root.enablePanel(9) }
                        TabButton { text: "보드"; font.pixelSize: Math.round(11.2 * root.uiScale); onClicked: if (root.workspaceMode) root.enablePanel(10) }
                    }
                    Switch {
                        id: workspaceSwitch
                        checked: root.workspaceMode
                        text: checked ? "분할" : "단일"
                        onToggled: {
                            root.workspaceMode = checked
                            root.syncAnalysisVisibility()
                        }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    visible: root.workspaceMode
                    spacing: Math.round(6 * root.uiScale)
                    Label { text: "패널"; color: "#5b6673"; font.pixelSize: Math.round(11 * root.uiScale) }
                    Repeater {
                        model: workspacePanels
                        delegate: Button {
                            required property int index
                            required property string title
                            required property bool opened
                            text: (opened ? "● " : "○ ") + title
                            font.pixelSize: Math.round(12 * root.uiScale)
                            checkable: true
                            checked: opened
                            onClicked: root.togglePanel(index)
                        }
                    }
                    Item { Layout.fillWidth: true }
                    Button { text: "개요+라이브+경보"; font.pixelSize: Math.round(12 * root.uiScale); onClicked: root.setWorkspacePreset(0, 1, 7) }
                    Button { text: "라이브+주기+값"; font.pixelSize: Math.round(12 * root.uiScale); onClicked: root.setWorkspacePreset(1, 3, 4) }
                    Button { text: "라이브+값+그래프"; font.pixelSize: Math.round(12 * root.uiScale); onClicked: root.setWorkspacePreset(1, 4, 5) }
                    Button { text: "보드+제어"; font.pixelSize: Math.round(12 * root.uiScale); onClicked: root.setWorkspacePreset(10, 9, 1) }
                    Button { text: "현재 탭 열기"; font.pixelSize: Math.round(12 * root.uiScale); onClicked: root.enablePanel(tabs.currentIndex) }
                }
            }
        }

        Frame {
            id: compactReplayBar
            Layout.fillWidth: true
            visible: appController.replayLoaded || (!root.workspaceMode && tabs.currentIndex === 2) || (root.workspaceMode && panelVisible(2))
            padding: 0
            implicitHeight: compactReplayContent.implicitHeight + Math.round(12 * root.uiScale)
            Layout.minimumHeight: implicitHeight
            background: Rectangle { color: "#fffaf3"; radius: 10; border.color: "#ead9b5" }

            property real pendingSeekValue: 0.0
            property bool pendingSeekInitialized: false
            property bool seekCommitPending: false

            Connections {
                target: appController
                function onReplayStateChanged() {
                    if (compactReplaySlider.pressed)
                        return
                    compactReplayBar.pendingSeekValue = appController.replayRebuilding ? appController.replayTargetProgress : appController.replayProgress
                    compactReplayBar.pendingSeekInitialized = true
                    compactReplayBar.seekCommitPending = false
                }
            }

            Component.onCompleted: {
                pendingSeekValue = appController.replayProgress
                pendingSeekInitialized = true
            }

            ColumnLayout {
                id: compactReplayContent
                anchors.fill: parent
                anchors.margins: Math.round(6 * root.uiScale)
                spacing: Math.round(4 * root.uiScale)

                Item {
                    Layout.fillWidth: true
                    implicitHeight: replayControlFlow.implicitHeight

                    Flow {
                        id: replayControlFlow
                        width: parent.width
                        spacing: Math.round(5 * root.uiScale)

                        Button {
                            text: "재생 열기"
                            font.pixelSize: Math.round(10.8 * root.uiScale)
                            onClicked: replayDialog.open()
                        }
                        Button {
                            text: "Typed 폴더"
                            font.pixelSize: Math.round(10.8 * root.uiScale)
                            onClicked: typedReplayFolderDialog.open()
                        }
                        Components.StatusBadge {
                            text: appController.replayLoaded ? (appController.replayPlaying ? "▶" : (appController.replayRebuilding ? "…" : "Ⅱ")) : "Replay"
                            kind: appController.replayPlaying ? "warn" : (appController.replayRebuilding ? "info" : "ok")
                            uiScale: Math.max(0.88, root.uiScale * 0.88)
                        }
                        Button { text: "◀"; font.pixelSize: Math.round(10.8 * root.uiScale); enabled: appController.replayLoaded; onClicked: appController.stepReplay(-1) }
                        Button { text: appController.replayPlaying ? "Ⅱ" : "▶"; font.pixelSize: Math.round(11.2 * root.uiScale); enabled: appController.replayLoaded; onClicked: { if (appController.replayPlaying) appController.pauseReplay(); else appController.playReplay(appController.replaySpeed > 0 ? appController.replaySpeed : 1.0) } }
                        Button { text: "■"; font.pixelSize: Math.round(10.8 * root.uiScale); enabled: appController.replayLoaded; onClicked: appController.stopReplay() }
                        Button { text: "▶"; font.pixelSize: Math.round(10.8 * root.uiScale); enabled: appController.replayLoaded; onClicked: appController.stepReplay(1) }
                        ComboBox {
                            Layout.preferredWidth: 84
                            enabled: appController.replayLoaded
                            model: ["0.5x", "1x", "2x"]
                            currentIndex: appController.replaySpeed >= 1.75 ? 2 : (appController.replaySpeed <= 0.75 ? 0 : 1)
                            onActivated: {
                                if (currentIndex === 0)
                                    appController.playReplay(0.5)
                                else if (currentIndex === 1)
                                    appController.playReplay(1.0)
                                else
                                    appController.playReplay(2.0)
                            }
                        }
                        CheckBox { text: "반복"; checked: appController.replayLoop; onToggled: appController.setReplayLoop(checked) }
                        Button { text: "최상위"; font.pixelSize: Math.round(10.2 * root.uiScale); onClicked: root.focusPrimaryIssue() }
                        Button { text: "◀이슈"; font.pixelSize: Math.round(10.0 * root.uiScale); enabled: appController.primaryIssueSeekAvailable; onClicked: root.seekPrimaryIssue(-1) }
                        Button { text: "이슈▶"; font.pixelSize: Math.round(10.0 * root.uiScale); enabled: appController.primaryIssueSeekAvailable; onClicked: root.seekPrimaryIssue(1) }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Math.round(6 * root.uiScale)

                    Label {
                        text: appController.replayLoaded ? (appController.replayCurrentTimeText + " / " + appController.replayDurationText) : "재생 파일 미선택"
                        color: "#35506b"
                        font.pixelSize: Math.round(10.6 * root.uiScale)
                        Layout.preferredWidth: 190
                        elide: Text.ElideRight
                    }
                    Components.PreciseSlider {
                        id: compactReplaySlider
                        Layout.fillWidth: true
                        from: 0
                        to: 1
                        enabled: appController.replayLoaded
                        liveUpdate: false
                        releaseHoldMs: 220
                        value: (compactReplaySlider.visualHoldActive || compactReplayBar.seekCommitPending || appController.replayRebuilding || !compactReplayBar.pendingSeekInitialized)
                               ? compactReplayBar.pendingSeekValue
                               : appController.replayProgress
                        onCommitted: function(nextValue) {
                            if (!appController.replayLoaded)
                                return
                            compactReplayBar.pendingSeekValue = nextValue
                            compactReplayBar.seekCommitPending = true
                            appController.commitSeekReplay(nextValue)
                        }
                    }
                    Label {
                        Layout.preferredWidth: 58
                        horizontalAlignment: Text.AlignRight
                        color: (compactReplaySlider.pressed || compactReplayBar.seekCommitPending || appController.replayRebuilding) ? "#0f4c81" : "#52606d"
                        font.pixelSize: Math.round(10.4 * root.uiScale)
                        font.bold: true
                        text: Math.round((compactReplaySlider.visualHoldActive
                                          ? compactReplaySlider.visualValue
                                          : ((compactReplayBar.seekCommitPending || appController.replayRebuilding)
                                                ? (compactReplayBar.seekCommitPending ? compactReplayBar.pendingSeekValue : appController.replayTargetProgress)
                                                : appController.replayProgress)) * 100) + "%"
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    radius: 8
                    color: "#fffdf8"
                    border.color: "#ead9b5"
                    implicitHeight: replayCursorLabel.implicitHeight + Math.round(12 * root.uiScale)
                    Label {
                        id: replayCursorLabel
                        anchors.fill: parent
                        anchors.margins: 6
                        text: appController.replayLoaded
                              ? (((compactReplaySlider.visualHoldActive || compactReplayBar.seekCommitPending || appController.replayRebuilding)
                                    ? ("탐색 " + appController.replayTimeTextForProgress(compactReplaySlider.visualHoldActive
                                                                                      ? compactReplaySlider.visualValue
                                                                                      : (compactReplayBar.seekCommitPending
                                                                                            ? compactReplayBar.pendingSeekValue
                                                                                            : appController.replayTargetProgress))
                                       + " · ")
                                    : "")
                                 + appController.replayCursorSummary)
                              : "Replay file or typed session을 열면 재생/탐색이 활성화됩니다"
                        color: appController.replayPlaying ? "#15803d" : "#607080"
                        wrapMode: Text.WordWrap
                        font.pixelSize: Math.round(10.4 * root.uiScale)
                    }
                }
            }
        }

        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: tabs.currentIndex
            visible: !root.workspaceMode

            Components.PageViewport { active: !root.workspaceMode && tabs.currentIndex === 0; pageComponent: overviewPageComponent; minPageWidth: root.minimumPageWidthForKey("overview"); uiScale: root.uiScale }
            Components.PageViewport { active: !root.workspaceMode && tabs.currentIndex === 1; pageComponent: livePageComponent; minPageWidth: root.minimumPageWidthForKey("live"); uiScale: root.uiScale }
            Components.PageViewport { active: !root.workspaceMode && tabs.currentIndex === 2; pageComponent: replayPageComponent; minPageWidth: root.minimumPageWidthForKey("replay"); uiScale: root.uiScale }
            Components.PageViewport { active: !root.workspaceMode && tabs.currentIndex === 3; pageComponent: timingPageComponent; minPageWidth: root.minimumPageWidthForKey("timing"); uiScale: root.uiScale }
            Components.PageViewport { active: !root.workspaceMode && tabs.currentIndex === 4; pageComponent: valuePageComponent; minPageWidth: root.minimumPageWidthForKey("value"); uiScale: root.uiScale }
            Components.PageViewport { active: !root.workspaceMode && tabs.currentIndex === 5; pageComponent: graphPageComponent; minPageWidth: root.minimumPageWidthForKey("graph"); uiScale: root.uiScale }
            Components.PageViewport { active: !root.workspaceMode && tabs.currentIndex === 6; pageComponent: graphOverviewPageComponent; minPageWidth: root.minimumPageWidthForKey("graph_overview"); uiScale: root.uiScale }
            Components.PageViewport { active: !root.workspaceMode && tabs.currentIndex === 7; pageComponent: alarmPageComponent; minPageWidth: root.minimumPageWidthForKey("alarm"); uiScale: root.uiScale }
            Components.PageViewport { active: !root.workspaceMode && tabs.currentIndex === 8; pageComponent: settingsPageComponent; minPageWidth: root.minimumPageWidthForKey("settings"); uiScale: root.uiScale }
            Components.PageViewport { active: !root.workspaceMode && tabs.currentIndex === 9; pageComponent: controlPageComponent; minPageWidth: root.minimumPageWidthForKey("control"); uiScale: root.uiScale }
            Components.PageViewport { active: !root.workspaceMode && tabs.currentIndex === 10; pageComponent: boardHealthPageComponent; minPageWidth: root.minimumPageWidthForKey("board"); uiScale: root.uiScale }
        }

        Frame {
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: root.workspaceMode
            padding: 0
            background: Rectangle {
                radius: 14
                gradient: Gradient {
                    GradientStop { position: 0.0; color: "#f6f9fd" }
                    GradientStop { position: 1.0; color: "#edf3fa" }
                }
                border.color: "#d7e0ea"
            }

            Item {
                anchors.fill: parent

                SplitView {
                    anchors.fill: parent
                    anchors.margins: Math.round(8 * root.uiScale)
                    visible: root.visiblePanelCount() > 0
                    orientation: Qt.Horizontal

                    Repeater {
                        model: workspacePanels
                        delegate: Frame {
                            required property int index
                            required property string key
                            required property string title
                            required property bool opened
                            visible: opened
                            enabled: visible
                            implicitWidth: visible ? 320 : 0
                            SplitView.fillWidth: true
                            SplitView.fillHeight: true
                            padding: 0
                            background: Rectangle { color: "#ffffff"; radius: 10; border.color: "#d7e0ea" }

                            ColumnLayout {
                                anchors.fill: parent
                                anchors.margins: Math.round(8 * root.uiScale)
                                spacing: Math.round(8 * root.uiScale)
                                RowLayout {
                                    Layout.fillWidth: true
                                    Label { text: title; font.bold: true; color: "#243447"; font.pixelSize: Math.round(13.5 * root.uiScale) }
                                    Components.StatusBadge {
                                        text: tabs.currentIndex === index ? "현재" : "열림"
                                        kind: tabs.currentIndex === index ? "info" : "ok"
                                        uiScale: Math.max(0.9, root.uiScale * 0.95)
                                    }
                                    Item { Layout.fillWidth: true }
                                    Button { text: "포커스"; font.pixelSize: Math.round(12 * root.uiScale); onClicked: tabs.currentIndex = index }
                                    Button { text: "닫기"; font.pixelSize: Math.round(12 * root.uiScale); onClicked: { workspacePanels.setProperty(index, "opened", false); root.syncAnalysisVisibility() } }
                                }
                                Components.PageViewport {
                                    Layout.fillWidth: true
                                    Layout.fillHeight: true
                                    active: opened
                                    pageComponent: root.componentForKey(key)
                                    minPageWidth: root.minimumPageWidthForKey(key)
                                    uiScale: root.uiScale
                                }
                            }
                        }
                    }
                }

                Rectangle {
                    anchors.fill: parent
                    anchors.margins: 18
                    visible: root.visiblePanelCount() === 0
                    radius: 12
                    color: "#ffffff"
                    border.color: "#d7e0ea"
                    ColumnLayout {
                        anchors.centerIn: parent
                        spacing: 10
                        Label { text: "열린 패널이 없습니다"; color: "#243447"; font.pixelSize: Math.round(22 * root.uiScale); font.bold: true }
                        Label { text: "상단 패널 버튼이나 프리셋 버튼으로 작업 화면을 다시 열 수 있습니다."; color: "#5b6673" }
                    }
                }
            }
        }
    }
}
