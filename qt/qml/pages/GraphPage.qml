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
    property bool signalRestoreScheduled: false

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

    Component.onCompleted: Qt.callLater(syncPresetIndex)

    Connections {
        target: appController
        function restoreSignalListPosition() {
            if (signalRestoreScheduled)
                return
            const keepY = savedSignalScrollY
            const keepKey = signalAnchorKey
            signalRestoreScheduled = true
            restoringSignalListPosition = true
            pendingSignalListRestore = true
            Qt.callLater(function() {
                signalList.forceLayout()
                const keepIndex = signalIndexForKey(keepKey)
                if (keepIndex >= 0)
                    signalList.positionViewAtIndex(keepIndex, ListView.Contain)
                signalList.contentY = clampSignalListY(keepY)
                Qt.callLater(function() {
                    signalList.forceLayout()
                    if (keepIndex >= 0)
                        signalList.positionViewAtIndex(keepIndex, ListView.Contain)
                    signalList.contentY = clampSignalListY(keepY)
                    Qt.callLater(function() {
                        signalList.forceLayout()
                        signalList.contentY = clampSignalListY(keepY)
                        pendingSignalListRestore = false
                        restoringSignalListPosition = false
                        signalRestoreScheduled = false
                    })
                })
            })
        }
        function onGraphCatalogChanged() { syncPresetIndex(); restoreSignalListPosition() }
        function onGraphSelectionChanged() { syncPresetIndex(); restoreSignalListPosition() }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 4

        Frame {
            Layout.fillWidth: true
            background: Rectangle { color: "#ffffff"; radius: 10; border.color: "#d7e0ea" }
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 5
                spacing: 4

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 6
                    Label { text: "프리셋"; color: "#5b6673"; font.bold: true }
                    ComboBox {
                        id: presetCombo
                        Layout.preferredWidth: 220
                        model: appController.graphPresets
                        textRole: "title"
                        onActivated: {
                            const entry = appController.graphPresets[currentIndex]
                            if (entry)
                                appController.setGraphPresetKey(entry.key)
                        }
                    }
                    Label { text: "창"; color: "#5b6673"; font.bold: true }
                    ComboBox {
                        id: windowCombo
                        model: [
                            { title: "5초", ms: 5000 },
                            { title: "10초", ms: 10000 },
                            { title: "15초", ms: 15000 },
                            { title: "30초", ms: 30000 },
                            { title: "60초", ms: 60000 }
                        ]
                        textRole: "title"
                        Component.onCompleted: {
                            for (let i = 0; i < model.length; ++i) {
                                if (model[i].ms === appController.graphWindowMs) {
                                    currentIndex = i
                                    break
                                }
                            }
                        }
                        onActivated: appController.setGraphWindowMs(model[currentIndex].ms)
                    }
                    Button { text: "프리셋 해제"; onClicked: appController.setGraphPresetKey("manual") }
                    Button { text: "선택 지우기"; onClicked: appController.clearGraphSelection() }
                    CheckBox {
                        text: "미세확대"
                        checked: appController.graphDetailZoom
                        onToggled: appController.setGraphDetailZoom(checked)
                    }
                    Label {
                        text: appController.graphDetailZoomSummary
                        color: appController.graphDetailZoom ? "#2563eb" : "#5b6673"
                        font.bold: appController.graphDetailZoom
                    }
                    Item { Layout.fillWidth: true }
                    Label { text: appController.graphSourceSummary; color: "#5b6673"; elide: Text.ElideRight }
                    Label {
                        text: appController.graphRangeSummary
                        color: "#35506b"
                        elide: Text.ElideRight
                        Layout.maximumWidth: 560
                    }
                }
            }
        }

        Frame {
            Layout.fillWidth: true
            background: Rectangle { color: "#ffffff"; radius: 10; border.color: "#d7e0ea" }
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 6
                spacing: 6
                RowLayout {
                    Layout.fillWidth: true
                    Components.StatusBadge { text: "SYS " + appController.systemLevel; kind: kindFromLevel(appController.systemLevel); uiScale: uiScale }
                    Components.StatusBadge { text: "BUS " + appController.busHealthLevel; kind: kindFromLevel(appController.busHealthLevel); uiScale: uiScale }
                    Components.StatusBadge { text: appController.graphDetailZoom ? "ZOOM LOCK" : "FIXED AXIS"; kind: appController.graphDetailZoom ? "warn" : "info"; uiScale: uiScale }
                    Item { Layout.fillWidth: true }
                    Label { text: appController.graphRangeSummary; color: "#607080"; elide: Text.ElideRight; Layout.preferredWidth: Math.round(430 * uiScale) }
                }
                Rectangle {
                    Layout.fillWidth: true
                    radius: 8
                    color: "#f8fafc"
                    border.color: "#dbe5f0"
                    implicitHeight: Math.round(32 * uiScale)
                    Label { anchors.fill: parent; anchors.margins: 6; text: appController.operatorHeadline + " · " + appController.graphDetailZoomSummary; color: "#35506b"; wrapMode: Text.WordWrap; verticalAlignment: Text.AlignVCenter }
                }
            }
        }

        SplitView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            orientation: Qt.Horizontal

            Frame {
                SplitView.preferredWidth: 300
                SplitView.minimumWidth: 260
                background: Rectangle { color: "#ffffff"; radius: 10; border.color: "#d7e0ea" }

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 6
                    spacing: 6

                    RowLayout {
                        Layout.fillWidth: true
                        Label { text: "그래프 신호"; font.bold: true; color: "#243447" }
                        Item { Layout.fillWidth: true }
                        Label { text: "최대 4선 · 엔코더 증분 기본은 절대값"; color: "#5b6673" }
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

                                Rectangle {
                                    Layout.preferredWidth: Math.round(10 * uiScale)
                                    Layout.preferredHeight: Math.round(18 * uiScale)
                                    radius: Math.round(3 * uiScale)
                                    color: entry.color || "#94a3b8"
                                    opacity: entry.selected ? 1.0 : 0.55
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
                        Label { text: "시계열 비교"; font.bold: true; color: "#243447"; font.pixelSize: Math.round(11.0 * uiScale) }
                        Item { Layout.fillWidth: true }
                        Repeater {
                            model: appController.graphSeries
                            delegate: RowLayout {
                                property var entry: modelData
                                spacing: 4
                                Rectangle { width: 12; height: 12; radius: 6; color: entry.color }
                                Label {
                                    text: entry.label + "  현재 " + entry.latestText + (entry.unit ? (" " + entry.unit) : "") + "  [" + entry.renderModeLabel + "]"
                                    color: "#4b5563"
                                }
                            }
                        }
                    }

                    Label {
                        Layout.fillWidth: true
                        text: appController.graphDetailZoom ? "판정은 RAW 기준으로 유지하고, 그래프는 고정축 기반 표시 위에서 현재 구간을 잠근 폭으로만 확대합니다." : "판정은 RAW 기준으로 유지합니다. 엔코더 4선 증분 기본 프리셋은 절대값 비교이며, 방향 비교가 필요하면 부호유지 프리셋을 쓰면 됩니다."
                        color: "#64748b"
                        font.pixelSize: Math.round(10.5 * uiScale)
                        wrapMode: Text.WordWrap
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        radius: 12
                        color: "#f8fbff"
                        border.color: "#dbe5f0"

                        GraphViewport {
                            id: graphViewport
                            anchors.fill: parent
                            series: appController.graphSeries
                            uiScale: pageRoot.uiScale
                        }
                    }
                }
            }
        }
    }
}
