import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import CanMonitor
import "../components" as Components

Item {
    id: pageRoot
    property real uiScale: 1.0
    property real savedSignalScrollY: 0
    property string signalAnchorKey: ""
    property bool restoringSignalListPosition: false
    property bool pendingSignalListRestore: false

    property real dragAnchorMs: -1
    property real dragCurrentMs: -1
    property bool draggingSelection: false
    property real selectionStartMs: -1
    property real selectionEndMs: -1
    property int detailYAxisMode: 1
    property var detailSeriesCache: []
    property real rootSelectionStartMs: -1
    property real rootSelectionEndMs: -1
    property var selectionHistory: []

    property real detailDragAnchorMs: -1
    property real detailDragCurrentMs: -1
    property bool detailDraggingSelection: false

    readonly property real overviewWindowMs: (appController.graphOverviewSeries.length > 0 && appController.graphOverviewSeries[0].windowMs !== undefined)
                                             ? Number(appController.graphOverviewSeries[0].windowMs) : 0
    readonly property bool selectionActive: selectionStartMs >= 0 && selectionEndMs > selectionStartMs + 1
    readonly property real visualSelectionStartMs: draggingSelection ? Math.min(dragAnchorMs, dragCurrentMs) : selectionStartMs
    readonly property real visualSelectionEndMs: draggingSelection ? Math.max(dragAnchorMs, dragCurrentMs) : selectionEndMs
    readonly property real detailWindowMs: selectionActive ? Math.max(1, selectionEndMs - selectionStartMs) : 0
    readonly property real visualDetailSelectionStartMs: detailDraggingSelection ? Math.min(detailDragAnchorMs, detailDragCurrentMs) : -1
    readonly property real visualDetailSelectionEndMs: detailDraggingSelection ? Math.max(detailDragAnchorMs, detailDragCurrentMs) : -1
    readonly property bool canZoomBack: selectionHistory && selectionHistory.length > 0

    function kindFromLevel(level) {
        if (level === "ERR") return "bad"
        if (level === "WARN") return "warn"
        if (level === "OK") return "ok"
        return "info"
    }

    function presetIndexFor(key) {
        const list = appController.graphPresets
        for (let i = 0; i < list.length; ++i) {
            if (list[i].key === key)
                return i
        }
        return 0
    }

    function syncPresetIndex() {
        const idx = presetIndexFor(appController.graphPresetKey)
        if (presetCombo.currentIndex !== idx)
            presetCombo.currentIndex = idx
    }

    function signalIndexForKey(key) {
        const list = appController.graphCatalog
        for (let i = 0; i < list.length; ++i) {
            if (list[i].key === key)
                return i
        }
        return -1
    }

    function clampSignalListY(y) {
        return Math.max(0, Math.min(y, Math.max(0, signalList.contentHeight - signalList.height)))
    }

    function rememberSignalListPosition(key) {
        savedSignalScrollY = signalList.contentY
        signalAnchorKey = key
        pendingSignalListRestore = true
    }

    function plotLeftPad(viewport) { return viewport ? viewport.plotLeftPadding : Math.round(60 * uiScale) }
    function plotRightPad(viewport) { return viewport ? viewport.plotRightPadding : Math.round(28 * uiScale) }
    function plotWidth(viewport, totalWidth) { return Math.max(1, totalWidth - plotLeftPad(viewport) - plotRightPad(viewport)) }
    function xToOverviewMs(x, width) {
        if (overviewWindowMs <= 0)
            return 0
        const left = plotLeftPad(overviewViewport)
        const usable = plotWidth(overviewViewport, width)
        const clamped = Math.max(left, Math.min(x, left + usable))
        return ((clamped - left) / usable) * overviewWindowMs
    }

    function xToDetailLocalMs(x, width) {
        if (detailWindowMs <= 0)
            return 0
        const left = plotLeftPad(detailViewport)
        const usable = plotWidth(detailViewport, width)
        const clamped = Math.max(left, Math.min(x, left + usable))
        return ((clamped - left) / usable) * detailWindowMs
    }

    function durationText(ms) {
        if (ms <= 0)
            return "-"
        const totalMs = Math.round(ms)
        const totalSec = Math.floor(totalMs / 1000)
        const millis = totalMs % 1000
        const hours = Math.floor(totalSec / 3600)
        const minutes = Math.floor((totalSec % 3600) / 60)
        const seconds = totalSec % 60
        if (hours > 0)
            return hours + ":" + String(minutes).padStart(2, "0") + ":" + String(seconds).padStart(2, "0") + "." + String(millis).padStart(3, "0")
        return minutes + ":" + String(seconds).padStart(2, "0") + "." + String(millis).padStart(3, "0")
    }

    function secondsText(ms) {
        return (ms / 1000.0).toFixed(3) + "s"
    }

    function shortSeriesLabel(labelText) {
        let text = String(labelText || "")
        text = text.replace("Motor ", "")
        text = text.replace("Encoder", "Enc")
        text = text.replace("Steering", "Str")
        text = text.replace("Actual ", "")
        text = text.replace("[Acount]", "")
        text = text.replace("[Account]", "")
        text = text.trim()
        if (text.length > 18)
            return text.slice(0, 18) + "..."
        return text
    }

    function refreshDetailSeries() {
        detailSeriesCache = selectionActive ? appController.graphOverviewDetailSeries(selectionStartMs, selectionEndMs) : []
    }

    function resetOverviewDragState() {
        draggingSelection = false
        dragAnchorMs = -1
        dragCurrentMs = -1
    }

    function resetDetailDragState() {
        detailDraggingSelection = false
        detailDragAnchorMs = -1
        detailDragCurrentMs = -1
    }

    function clampRangeToWindow(startMs, endMs, windowMs) {
        if (!(windowMs > 0))
            return null
        const start = Math.max(0, Math.min(startMs, endMs))
        const end = Math.max(start, Math.min(Math.max(startMs, endMs), windowMs))
        if (!(end > start + 1))
            return null
        return { startMs: start, endMs: end }
    }

    function normalizeSelectionState() {
        if (!(overviewWindowMs > 0) || appController.graphOverviewSeries.length === 0) {
            clearSelection()
            return
        }

        if (!selectionActive) {
            resetDetailDragState()
            refreshDetailSeries()
            return
        }

        const currentRange = clampRangeToWindow(selectionStartMs, selectionEndMs, overviewWindowMs)
        if (!currentRange) {
            clearSelection()
            return
        }

        selectionStartMs = currentRange.startMs
        selectionEndMs = currentRange.endMs

        const rootRange = clampRangeToWindow(rootSelectionStartMs, rootSelectionEndMs, overviewWindowMs)
        if (rootRange) {
            rootSelectionStartMs = rootRange.startMs
            rootSelectionEndMs = rootRange.endMs
        } else {
            rootSelectionStartMs = currentRange.startMs
            rootSelectionEndMs = currentRange.endMs
        }

        const nextHistory = []
        if (selectionHistory) {
            for (let i = 0; i < selectionHistory.length; ++i) {
                const entry = selectionHistory[i]
                const range = clampRangeToWindow(Number(entry.startMs), Number(entry.endMs), overviewWindowMs)
                if (!range)
                    continue
                nextHistory.push(range)
            }
        }
        selectionHistory = nextHistory
        resetDetailDragState()
        refreshDetailSeries()
    }

    function clearSelection() {
        resetOverviewDragState()
        resetDetailDragState()
        selectionStartMs = -1
        selectionEndMs = -1
        rootSelectionStartMs = -1
        rootSelectionEndMs = -1
        selectionHistory = []
        refreshDetailSeries()
    }

    function applyOverviewSelection(startMs, endMs) {
        const range = clampRangeToWindow(startMs, endMs, overviewWindowMs)
        if (!range) {
            clearSelection()
            return
        }
        selectionStartMs = range.startMs
        selectionEndMs = range.endMs
        rootSelectionStartMs = range.startMs
        rootSelectionEndMs = range.endMs
        selectionHistory = []
        resetDetailDragState()
        refreshDetailSeries()
    }

    function applyNestedSelection(startMs, endMs) {
        const start = Math.max(selectionStartMs, Math.min(startMs, endMs))
        const end = Math.min(selectionEndMs, Math.max(startMs, endMs))
        if (end <= start)
            return
        const nextHistory = selectionHistory ? selectionHistory.slice(0) : []
        nextHistory.push({ startMs: selectionStartMs, endMs: selectionEndMs })
        selectionHistory = nextHistory
        selectionStartMs = start
        selectionEndMs = end
        detailDraggingSelection = false
        detailDragAnchorMs = -1
        detailDragCurrentMs = -1
        refreshDetailSeries()
    }

    function zoomBackOneLevel() {
        if (!canZoomBack)
            return
        const nextHistory = selectionHistory.slice(0)
        const prev = nextHistory.pop()
        selectionHistory = nextHistory
        selectionStartMs = Number(prev.startMs)
        selectionEndMs = Number(prev.endMs)
        detailDraggingSelection = false
        detailDragAnchorMs = -1
        detailDragCurrentMs = -1
        refreshDetailSeries()
    }

    function zoomBackToRoot() {
        if (rootSelectionEndMs <= rootSelectionStartMs)
            return
        selectionStartMs = rootSelectionStartMs
        selectionEndMs = rootSelectionEndMs
        selectionHistory = []
        detailDraggingSelection = false
        detailDragAnchorMs = -1
        detailDragCurrentMs = -1
        refreshDetailSeries()
    }

    function detailCursorMs() {
        if (!selectionActive)
            return -1
        const absolute = Number(appController.graphOverviewCursorMs)
        if (!isFinite(absolute) || absolute < selectionStartMs || absolute > selectionEndMs)
            return -1
        return absolute - selectionStartMs
    }

    Component.onCompleted: Qt.callLater(function() {
        syncPresetIndex()
        refreshDetailSeries()
    })

    Connections {
        target: appController
        function restoreSignalListPosition() {
            const keepY = savedSignalScrollY
            restoringSignalListPosition = true
            pendingSignalListRestore = true
            Qt.callLater(function() {
                signalList.forceLayout()
                signalList.contentY = clampSignalListY(keepY)
                Qt.callLater(function() {
                    signalList.forceLayout()
                    signalList.contentY = clampSignalListY(keepY)
                    pendingSignalListRestore = false
                    restoringSignalListPosition = false
                })
            })
        }
        function onGraphCatalogChanged() { syncPresetIndex(); restoreSignalListPosition() }
        function onGraphSelectionChanged() {
            syncPresetIndex()
            restoreSignalListPosition()
            if (appController.graphSelectedKeys.length === 0) {
                clearSelection()
                return
            }
            refreshDetailSeries()
        }
        function onGraphOverviewChanged() {
            normalizeSelectionState()
        }
    }

    ColumnLayout {
        id: pageColumn
        anchors.fill: parent
        spacing: 6

        Frame {
            Layout.fillWidth: true
            background: Rectangle { color: "#ffffff"; radius: 10; border.color: "#d7e0ea" }
            RowLayout {
                anchors.fill: parent
                anchors.margins: 6
                spacing: 8

                Label {
                    text: "프리셋"
                    color: "#5b6673"
                    font.bold: true
                }
                ComboBox {
                    id: presetCombo
                    Layout.preferredWidth: 240
                    model: appController.graphPresets
                    textRole: "title"
                    onActivated: {
                        const entry = appController.graphPresets[currentIndex]
                        if (entry)
                            appController.setGraphPresetKey(entry.key)
                    }
                }
                Button { text: "프리셋 해제"; onClicked: appController.setGraphPresetKey("manual") }
                Button { text: "선택 지우기"; onClicked: appController.clearGraphSelection() }
                Item { Layout.fillWidth: true }
                Label {
                    Layout.fillWidth: true
                    text: appController.graphOverviewSourceSummary + " · " + appController.graphOverviewRangeSummary
                    color: "#35506b"
                    elide: Text.ElideRight
                }
            }
        }

        Frame {
            Layout.fillWidth: true
            background: Rectangle { color: "#ffffff"; radius: 10; border.color: "#d7e0ea" }
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 6
                spacing: 4
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    Components.StatusBadge { text: appController.replayLoaded ? "REPLAY READY" : "REPLAY 없음"; kind: appController.replayLoaded ? "ok" : "info"; uiScale: uiScale }
                    Components.StatusBadge { text: appController.graphOverviewBuilding ? "PREPARING" : (appController.graphOverviewReady ? "READY" : "IDLE"); kind: appController.graphOverviewBuilding ? "warn" : (appController.graphOverviewReady ? "ok" : "info"); uiScale: uiScale }
                    Components.StatusBadge { text: "SYS " + appController.systemLevel; kind: kindFromLevel(appController.systemLevel); uiScale: uiScale }
                    Label { text: "시작 " + appController.graphOverviewStartText; color: "#607080" }
                    Label { text: "끝 " + appController.graphOverviewEndText; color: "#607080" }
                    Label {
                        text: "전체 " + appController.graphOverviewDurationText + (overviewWindowMs > 0 ? (" (" + secondsText(overviewWindowMs) + ")") : "")
                        color: "#35506b"
                        font.bold: true
                    }
                    Item { Layout.fillWidth: true }
                    ProgressBar {
                        Layout.preferredWidth: 160
                        from: 0
                        to: 1
                        value: appController.graphOverviewBuildProgress
                        visible: appController.graphOverviewBuilding
                    }
                }
                Rectangle {
                    Layout.fillWidth: true
                    radius: 8
                    color: "#f8fafc"
                    border.color: "#dbe5f0"
                    implicitHeight: Math.round(30 * uiScale)
                    Label {
                        id: helpLabel
                        anchors.fill: parent
                        anchors.margins: 6
                        text: appController.graphOverviewBuilding
                              ? (appController.graphOverviewBuildText + " · 재생 중에는 현재 위치선만 이동하고 전체 데이터는 다시 계산하지 않습니다.")
                              : "이 탭은 재생 파일 전체 시간을 한 번에 보는 고정 화면입니다. 상단 overview에서 드래그하면 하단 확대 구간이 열리고, 전체 조감도는 그대로 유지됩니다."
                        color: "#35506b"
                        elide: Text.ElideRight
                        verticalAlignment: Text.AlignVCenter
                    }
                }
            }
        }

        SplitView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            orientation: Qt.Horizontal

            Frame {
                SplitView.preferredWidth: 300
                SplitView.minimumWidth: 250
                background: Rectangle { color: "#ffffff"; radius: 10; border.color: "#d7e0ea" }

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 6
                    spacing: 6

                    RowLayout {
                        Layout.fillWidth: true
                        Label { text: "전체 그래프 신호"; font.bold: true; color: "#243447" }
                        Item { Layout.fillWidth: true }
                        Label { text: "최대 4선 · 로드시 자동 준비"; color: "#5b6673" }
                    }

                    ListView {
                        id: signalList
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        spacing: 2
                        reuseItems: true
                        cacheBuffer: 220
                        boundsBehavior: Flickable.StopAtBounds
                        onContentYChanged: {
                            if (!restoringSignalListPosition && !pendingSignalListRestore)
                                savedSignalScrollY = contentY
                        }
                        model: appController.graphCatalog
                        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
                        delegate: Rectangle {
                            property var entry: modelData
                            width: ListView.view.width
                            radius: 7
                            color: entry.selected ? "#eff6ff" : "#f8fafc"
                            border.color: entry.selected ? "#93c5fd" : "#dbe5f0"
                            implicitHeight: Math.round(26 * uiScale)

                            RowLayout {
                                anchors.fill: parent
                                anchors.margins: 2
                                spacing: 4

                                CheckBox {
                                    checked: entry.selected
                                    onClicked: {
                                        rememberSignalListPosition(entry.key)
                                        appController.toggleGraphSignal(entry.key)
                                    }
                                }

                                Item {
                                    Layout.fillWidth: true
                                    Layout.fillHeight: true

                                    TapHandler {
                                        onTapped: {
                                            rememberSignalListPosition(entry.key)
                                            appController.toggleGraphSignal(entry.key)
                                        }
                                    }

                                    Label {
                                        anchors.left: parent.left
                                        anchors.right: parent.right
                                        anchors.verticalCenter: parent.verticalCenter
                                        text: entry.label
                                        elide: Text.ElideRight
                                        color: "#243447"
                                        font.bold: entry.selected
                                        font.pixelSize: Math.round(10.6 * uiScale)
                                    }
                                }
                            }
                        }
                    }
                }
            }

            Frame {
                SplitView.fillWidth: true
                background: Rectangle { color: "#ffffff"; radius: 10; border.color: "#d7e0ea" }

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 6
                    spacing: 6

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8
                        Label { text: "재생 전체 시간범위"; font.bold: true; color: "#243447"; font.pixelSize: Math.round(11.0 * uiScale) }
                        Label {
                            Layout.fillWidth: true
                            text: selectionActive
                                  ? ("선택 " + durationText(selectionStartMs) + " ~ " + durationText(selectionEndMs) + " · Δ " + durationText(selectionEndMs - selectionStartMs) + " · " + (selectionHistory.length + 1) + "단")
                                  : "상단 overview에서 드래그해 확대 구간을 선택하세요."
                            color: selectionActive ? "#0f4c81" : "#64748b"
                            elide: Text.ElideRight
                        }
                        Button { text: "한 단계 뒤로"; enabled: canZoomBack; onClicked: zoomBackOneLevel() }
                        Button { text: "처음 선택"; enabled: selectionActive && (selectionHistory.length > 0); onClicked: zoomBackToRoot() }
                        Button { text: "선택 해제"; enabled: selectionActive; onClicked: clearSelection() }
                    }

                    Flickable {
                        id: legendRail
                        Layout.fillWidth: true
                        Layout.preferredHeight: Math.max(Math.round(28 * uiScale), legendRow.implicitHeight)
                        clip: true
                        boundsBehavior: Flickable.StopAtBounds
                        flickableDirection: Flickable.HorizontalFlick
                        contentWidth: legendRow.implicitWidth
                        contentHeight: legendRow.implicitHeight
                        interactive: contentWidth > width + 1

                        Row {
                            id: legendRow
                            spacing: 10

                            Repeater {
                                model: appController.graphOverviewSeries
                                delegate: Row {
                                    property var entry: modelData
                                    spacing: 4
                                    Rectangle { width: 12; height: 12; radius: 6; color: entry.color }
                                    Label {
                                        text: shortSeriesLabel(entry.label) + " " + entry.latestText + (entry.unit ? (" " + entry.unit) : "")
                                        color: "#4b5563"
                                        font.pixelSize: Math.round(10.4 * uiScale)
                                    }
                                }
                            }
                        }

                        ScrollBar.horizontal: ScrollBar {
                            policy: legendRail.interactive ? ScrollBar.AsNeeded : ScrollBar.AlwaysOff
                        }
                    }

                    Rectangle {
                        id: overviewCard
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.minimumHeight: Math.round(selectionActive ? 280 * uiScale : 390 * uiScale)
                        radius: 12
                        color: "#f8fbff"
                        border.color: "#dbe5f0"

                        GraphViewport {
                            id: overviewViewport
                            anchors.fill: parent
                            anchors.margins: 1
                            visible: appController.graphOverviewSeries.length > 0
                            series: appController.graphOverviewSeries
                            viewStartMs: 0
                            viewEndMs: overviewWindowMs
                            cursorMs: appController.graphOverviewCursorMs
                            selectionStartMs: visualSelectionStartMs
                            selectionEndMs: visualSelectionEndMs
                            uiScale: pageRoot.uiScale
                        }

                        MouseArea {
                            anchors.fill: parent
                            enabled: appController.graphOverviewSeries.length > 0 && !appController.graphOverviewBuilding && overviewWindowMs > 0
                            acceptedButtons: Qt.LeftButton
                            hoverEnabled: true
                            onPressed: function(mouse) {
                                dragAnchorMs = xToOverviewMs(mouse.x, width)
                                dragCurrentMs = dragAnchorMs
                                draggingSelection = true
                            }
                            onPositionChanged: function(mouse) {
                                if (draggingSelection)
                                    dragCurrentMs = xToOverviewMs(mouse.x, width)
                            }
                            onReleased: function(mouse) {
                                if (!draggingSelection)
                                    return
                                dragCurrentMs = xToOverviewMs(mouse.x, width)
                                draggingSelection = false
                                const startMs = Math.min(dragAnchorMs, dragCurrentMs)
                                const endMs = Math.max(dragAnchorMs, dragCurrentMs)
                                if ((endMs - startMs) < Math.max(overviewWindowMs * 0.003, 30)) {
                                    clearSelection()
                                } else {
                                    applyOverviewSelection(startMs, endMs)
                                }
                            }
                            onDoubleClicked: clearSelection()
                        }

                        Column {
                            anchors.centerIn: parent
                            spacing: 6
                            visible: appController.graphOverviewSeries.length === 0 && !appController.graphOverviewBuilding
                            Label { text: appController.replayLoaded ? (appController.graphSelectedKeys.length > 0 ? "표시 가능한 전체 그래프 데이터가 없습니다" : "전체 그래프 신호를 선택하세요") : "재생 파일을 먼저 로드하세요"; color: "#475569"; font.bold: true }
                            Label { text: appController.graphOverviewBuildText; color: "#64748b" }
                        }

                        Rectangle {
                            anchors.centerIn: parent
                            width: Math.min(parent.width * 0.58, 520)
                            height: Math.round(86 * uiScale)
                            radius: 12
                            color: "#fffbeb"
                            border.color: "#f3d7a1"
                            visible: appController.graphOverviewBuilding
                            ColumnLayout {
                                anchors.fill: parent
                                anchors.margins: 10
                                spacing: 6
                                Label { text: "전체 그래프 준비 중"; color: "#9a5b00"; font.bold: true }
                                ProgressBar { Layout.fillWidth: true; from: 0; to: 1; value: appController.graphOverviewBuildProgress }
                                Label { Layout.fillWidth: true; text: appController.graphOverviewBuildText; color: "#7c5a10"; elide: Text.ElideRight }
                            }
                        }
                    }

                    Frame {
                        visible: selectionActive && appController.graphOverviewSeries.length > 0
                        Layout.fillWidth: true
                        Layout.preferredHeight: Math.round(240 * uiScale)
                        Layout.minimumHeight: Math.round(230 * uiScale)
                        background: Rectangle { color: "#fbfdff"; radius: 10; border.color: "#dbe5f0" }

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 6
                            spacing: 6

                            RowLayout {
                                Layout.fillWidth: true
                                Label { text: "선택구간 확대"; font.bold: true; color: "#243447" }
                                Label { text: durationText(selectionStartMs) + " ~ " + durationText(selectionEndMs); color: "#607080" }
                                Label { text: "폭 " + durationText(selectionEndMs - selectionStartMs) + " (상대시간 확대)"; color: "#35506b"; font.bold: true }
                                Item { Layout.fillWidth: true }
                                Label { text: "Y축"; color: "#5b6673"; font.bold: true }
                                ComboBox {
                                    id: detailYAxisCombo
                                    model: ["전체고정", "자동", "피크", "0대칭"]
                                    currentIndex: detailYAxisMode
                                    onActivated: detailYAxisMode = currentIndex
                                }
                            }

                            Label {
                                Layout.fillWidth: true
                                text: detailYAxisMode === 0 ? "전체 그래프와 같은 값축을 유지합니다. X축은 선택 시작 기준 상대시간으로 확대 표시됩니다." : (detailYAxisMode === 1 ? "선택구간 min/max 기준 자동값축입니다. X축은 선택 폭만큼만 확대 표시됩니다." : (detailYAxisMode === 2 ? "피크 관찰용으로 값축 패딩을 더 좁혔습니다. X축은 선택 폭 기준 상대시간입니다." : "부호 비교용 0대칭 값축입니다. X축은 선택 폭 기준 상대시간입니다."))
                                color: "#64748b"
                                wrapMode: Text.WordWrap
                                font.pixelSize: Math.round(10.4 * uiScale)
                            }

                            Rectangle {
                                id: detailCard
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                Layout.minimumHeight: Math.round(180 * uiScale)
                                radius: 10
                                color: "#ffffff"
                                border.color: "#dbe5f0"

                                GraphViewport {
                                    id: detailViewport
                                    anchors.fill: parent
                                    anchors.margins: 1
                                    series: detailSeriesCache
                                    viewStartMs: 0
                                    viewEndMs: detailWindowMs
                                    cursorMs: detailCursorMs()
                                    yAxisMode: detailYAxisMode
                                    selectionStartMs: visualDetailSelectionStartMs
                                    selectionEndMs: visualDetailSelectionEndMs
                                    uiScale: pageRoot.uiScale
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    enabled: selectionActive && detailWindowMs > 0 && detailSeriesCache.length > 0
                                    acceptedButtons: Qt.LeftButton
                                    hoverEnabled: true
                                    onPressed: function(mouse) {
                                        detailDragAnchorMs = xToDetailLocalMs(mouse.x, width)
                                        detailDragCurrentMs = detailDragAnchorMs
                                        detailDraggingSelection = true
                                    }
                                    onPositionChanged: function(mouse) {
                                        if (detailDraggingSelection)
                                            detailDragCurrentMs = xToDetailLocalMs(mouse.x, width)
                                    }
                                    onReleased: function(mouse) {
                                        if (!detailDraggingSelection)
                                            return
                                        detailDragCurrentMs = xToDetailLocalMs(mouse.x, width)
                                        detailDraggingSelection = false
                                        const localStart = Math.min(detailDragAnchorMs, detailDragCurrentMs)
                                        const localEnd = Math.max(detailDragAnchorMs, detailDragCurrentMs)
                                        if ((localEnd - localStart) < Math.max(detailWindowMs * 0.01, 10)) {
                                            detailDragAnchorMs = -1
                                            detailDragCurrentMs = -1
                                            return
                                        }
                                        applyNestedSelection(selectionStartMs + localStart, selectionStartMs + localEnd)
                                    }
                                    onDoubleClicked: zoomBackOneLevel()
                                }
                            }

                            Rectangle {
                                Layout.fillWidth: true
                                radius: 8
                                color: "#eff6ff"
                                border.color: "#bfdbfe"
                                implicitHeight: Math.round(24 * uiScale)
                                Label {
                                    anchors.fill: parent
                                    anchors.margins: 6
                                    text: "하단 확대뷰도 드래그하면 추가 확대됩니다. 더블클릭하면 한 단계 뒤로 돌아갑니다."
                                    color: "#35506b"
                                    verticalAlignment: Text.AlignVCenter
                                    font.pixelSize: Math.round(10.0 * uiScale)
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
