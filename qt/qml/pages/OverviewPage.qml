import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components" as Components

Item {
    id: root
    property real uiScale: 1.0
    signal openPanel(string key)
    signal focusIssue(string key, string idText)
    signal openReplay()
    signal exportSnapshot()

    function kindFromLevel(level) {
        if (level === "ERR") return "bad"
        if (level === "WARN") return "warn"
        if (level === "OK") return "ok"
        return "info"
    }

    function monitorNote(kind) {
        if (kind === "timing")
            return "현재 활성 ERR " + appController.timingErrCount + " / WARN " + appController.timingWarnCount + " · 누적 이벤트 " + appController.timingCumulativeCount
        if (kind === "value")
            return "현재 활성 ERR " + appController.valueErrCount + " / WARN " + appController.valueWarnCount + " · 누적 값 이상 " + appController.valueCumulativeCount
        return "현재 열린 ERR " + appController.alarmErrCount + " / WARN " + appController.alarmWarnCount + " · 누적 경보 이벤트 " + appController.alarmCumulativeCount
    }

    function monitorValue(kind) {
        if (kind === "timing")
            return appController.timingIssueCount > 0 ? ("활성 " + appController.timingIssueCount + "건 / 누적 " + appController.timingCumulativeCount) : ("정상 / 누적 " + appController.timingCumulativeCount)
        if (kind === "value")
            return appController.valueIssueCount > 0 ? ("활성 " + appController.valueIssueCount + "건 / 누적 " + appController.valueCumulativeCount) : ("정상 / 누적 " + appController.valueCumulativeCount)
        return appController.activeAlarmCount > 0 ? ("열림 " + appController.activeAlarmCount + "건 / 누적 " + appController.alarmCumulativeCount) : ("정상 / 누적 " + appController.alarmCumulativeCount)
    }

    function focusKey(kind) {
        if (kind === "timing") return appController.topTimingId
        if (kind === "value") return appController.topValueId
        return appController.topAlarmId
    }

    ScrollView {
        anchors.fill: parent
        clip: true
        ScrollBar.horizontal.policy: ScrollBar.AsNeeded

        ColumnLayout {
            width: Math.max(root.width - 18, Math.round(1180 * root.uiScale))
            spacing: Math.round(8 * root.uiScale)


            Frame {
                Layout.fillWidth: true
                padding: 0
                background: Rectangle { color: "#ffffff"; radius: 12; border.color: "#dbe5f0" }
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: Math.round(12 * root.uiScale)
                    spacing: 10

                    RowLayout {
                        Layout.fillWidth: true
                        Label { text: "운용 빠른 작업"; font.pixelSize: Math.round(15 * root.uiScale); font.bold: true; color: "#243447" }
                        Item { Layout.fillWidth: true }
                        Components.StatusBadge { text: "입력 " + appController.sourceStateLevel; kind: root.kindFromLevel(appController.sourceStateLevel); uiScale: root.uiScale }
                        Components.StatusBadge { text: "분석 " + appController.analysisModeLevel; kind: root.kindFromLevel(appController.analysisModeLevel); uiScale: root.uiScale }
                        Components.StatusBadge { text: "로그 " + appController.loggingStateLevel; kind: root.kindFromLevel(appController.loggingStateLevel); uiScale: root.uiScale }
                    }

                    Flow {
                        Layout.fillWidth: true
                        spacing: 8
                        Button { text: "라이브"; onClicked: root.openPanel("live") }
                        Button { text: "재생"; onClicked: root.openPanel("replay") }
                        Button { text: "재생 열기"; onClicked: root.openReplay() }
                        Button {
                            text: appController.replayPlaying ? "재생 일시정지" : "재생 시작"
                            enabled: appController.replayLoaded
                            onClicked: {
                                root.openPanel("replay")
                                if (appController.replayPlaying)
                                    appController.pauseReplay()
                                else
                                    appController.playReplay(appController.replaySpeed > 0 ? appController.replaySpeed : 1.0)
                            }
                        }
                        Button {
                            text: appController.logRecordingActive ? "로그 기록 중" : "로그 시작"
                            enabled: appController.connected && !appController.logRecordingActive && !appController.logPendingSave && !appController.logStopping && !appController.logSaving
                            onClicked: {
                                root.openPanel("live")
                                appController.startLog()
                            }
                        }
                        Button {
                            text: appController.logRecordingActive ? "로그 중지·저장" : "저장하기"
                            enabled: (appController.logRecordingActive || appController.logPendingSave) && !appController.logStopping && !appController.logSaving
                            onClicked: {
                                root.openPanel("live")
                                appController.stopLog()
                            }
                        }
                        Button {
                            text: "폐기"
                            enabled: appController.logPendingSave && !appController.logRecordingActive && !appController.logStopping && !appController.logSaving
                            onClicked: appController.discardPendingLog()
                        }
                        Button { text: "스냅샷"; onClicked: root.exportSnapshot() }
                        Button {
                            text: "라이브 복귀"
                            enabled: appController.connected && appController.replayAnalysisHeld
                            onClicked: appController.useLiveAnalysis()
                        }
                    }

                    Label {
                        Layout.fillWidth: true
                        text: "단축키: F6 최상위 보기 · F7/F8 이슈 이동 · Space 재생/정지 · ←/→ 스텝 · Ctrl+L 로그 · Ctrl+R 재생 · Ctrl+E 스냅샷"
                        color: "#607080"
                        font.pixelSize: Math.round(10.6 * root.uiScale)
                        wrapMode: Text.WordWrap
                    }

                    GridLayout {
                        Layout.fillWidth: true
                        columns: width > 1400 ? 4 : (width > 960 ? 2 : 1)
                        rowSpacing: 10
                        columnSpacing: 10

                        Components.InfoCard {
                            Layout.fillWidth: true
                            title: "입력 source"
                            value: appController.sourceStateText
                            note: appController.liveStatsSummary
                            badgeText: "INPUT"
                            kind: root.kindFromLevel(appController.sourceStateLevel)
                            uiScale: root.uiScale
                            clickable: true
                            onClicked: root.openPanel(appController.connected ? "live" : (appController.replayLoaded ? "replay" : "live"))
                        }
                        Components.InfoCard {
                            Layout.fillWidth: true
                            title: "분석 모드"
                            value: appController.analysisModeText
                            note: appController.replaySnapshotSummary
                            badgeText: "ANALYSIS"
                            kind: root.kindFromLevel(appController.analysisModeLevel)
                            uiScale: root.uiScale
                            clickable: true
                            onClicked: root.openPanel(appController.replayAnalysisActive ? "replay" : "live")
                        }
                        Components.InfoCard {
                            Layout.fillWidth: true
                            title: "로그 상태"
                            value: appController.loggingStateText
                            note: appController.logActionText
                            badgeText: "LOG"
                            kind: root.kindFromLevel(appController.loggingStateLevel)
                            uiScale: root.uiScale
                            clickable: true
                            onClicked: root.openPanel("live")
                        }
                        Components.InfoCard {
                            Layout.fillWidth: true
                            title: "재생 상태"
                            value: appController.replayCursorSummary
                            note: appController.replayIssueSummary
                            badgeText: appController.replayLoaded ? "REPLAY" : "NONE"
                            kind: appController.replayLoaded ? (appController.replayPlaying ? "ok" : "warn") : "info"
                            uiScale: root.uiScale
                            clickable: true
                            onClicked: root.openPanel("replay")
                        }
                    }
                }
            }

            Frame {
                Layout.fillWidth: true
                padding: 0
                background: Rectangle { color: "#ffffff"; radius: 12; border.color: "#dbe5f0" }
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: Math.round(12 * root.uiScale)
                    spacing: 10

                    RowLayout {
                        Layout.fillWidth: true
                        Label { text: "운용 헤드라인"; font.pixelSize: Math.round(15 * root.uiScale); font.bold: true; color: "#243447" }
                        Item { Layout.fillWidth: true }
                        Components.StatusBadge { text: "ACTION " + appController.operatorActionLevel; kind: root.kindFromLevel(appController.operatorActionLevel); uiScale: root.uiScale }
                        Components.StatusBadge {
                            text: appController.primaryIssueKind === "none" ? "ISSUE -" : ("ISSUE " + appController.primaryIssueKind.toUpperCase())
                            kind: appController.primaryIssueKind === "bus" ? root.kindFromLevel(appController.busHealthLevel)
                                 : (appController.primaryIssueKind === "alarm" ? root.kindFromLevel(appController.alarmLevel)
                                 : (appController.primaryIssueKind === "value" ? root.kindFromLevel(appController.valueLevel)
                                 : (appController.primaryIssueKind === "timing" ? root.kindFromLevel(appController.timingLevel) : "info")))
                            uiScale: root.uiScale
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        radius: 10
                        color: "#f8fafc"
                        border.color: "#dbe5f0"
                        implicitHeight: headlineLabel.implicitHeight + Math.round(16 * root.uiScale)
                        Label {
                            id: headlineLabel
                            anchors.fill: parent
                            anchors.margins: 8
                            text: appController.operatorHeadline
                            color: "#35506b"
                            wrapMode: Text.WordWrap
                            verticalAlignment: Text.AlignVCenter
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        radius: 10
                        color: appController.operatorActionLevel === "ERR" ? "#fff1f2" : (appController.operatorActionLevel === "WARN" ? "#fffaf0" : "#f8fafc")
                        border.color: appController.operatorActionLevel === "ERR" ? "#fecdd3" : (appController.operatorActionLevel === "WARN" ? "#fcd34d" : "#dbe5f0")
                        implicitHeight: actionLabel.implicitHeight + Math.round(16 * root.uiScale)
                        Label {
                            id: actionLabel
                            anchors.fill: parent
                            anchors.margins: 8
                            text: appController.operatorActionText
                            color: appController.operatorActionLevel === "ERR" ? "#9f1239" : (appController.operatorActionLevel === "WARN" ? "#9a5b00" : "#52606d")
                            wrapMode: Text.WordWrap
                            verticalAlignment: Text.AlignVCenter
                        }
                    }

                    Flow {
                        Layout.fillWidth: true
                        spacing: 8
                        Button {
                            text: "현재 우선 보기"
                            onClicked: {
                                if (appController.primaryIssueTargetTab === "timing")
                                    root.focusIssue("timing", appController.primaryIssueId)
                                else if (appController.primaryIssueTargetTab === "value")
                                    root.focusIssue("value", appController.primaryIssueId)
                                else if (appController.primaryIssueTargetTab === "alarm")
                                    root.focusIssue("alarm", appController.primaryIssueId)
                                else
                                    root.openPanel(appController.primaryIssueTargetTab)
                            }
                        }
                        Button {
                            text: "◀ 현재 이슈"
                            enabled: appController.primaryIssueSeekAvailable
                            onClicked: appController.seekReplayPrimaryIssue(-1)
                        }
                        Button {
                            text: "현재 이슈 ▶"
                            enabled: appController.primaryIssueSeekAvailable
                            onClicked: appController.seekReplayPrimaryIssue(1)
                        }
                        Button {
                            text: "최상위 주기"
                            enabled: appController.topTimingId !== ""
                            onClicked: root.focusIssue("timing", appController.topTimingId)
                        }
                        Button {
                            text: "최상위 값"
                            enabled: appController.topValueId !== ""
                            onClicked: root.focusIssue("value", appController.topValueId)
                        }
                        Button {
                            text: "최상위 경보"
                            enabled: appController.topAlarmId !== ""
                            onClicked: root.focusIssue("alarm", appController.topAlarmId)
                        }
                        Button {
                            text: "설정/진단"
                            onClicked: root.openPanel("settings")
                        }
                    }

                    Label {
                        Layout.fillWidth: true
                        text: appController.operatorFocusText + " · " + ((appController.primaryIssueId !== "" ? appController.primaryIssueId + " · " : "") + appController.primaryIssueSummary)
                        color: "#607080"
                        wrapMode: Text.WordWrap
                    }
                }
            }

            Frame {
                Layout.fillWidth: true
                visible: appController.replayLoaded
                padding: 0
                background: Rectangle { color: "#ffffff"; radius: 12; border.color: "#dbe5f0" }
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: Math.round(12 * root.uiScale)
                    spacing: 10

                    RowLayout {
                        Layout.fillWidth: true
                        Label { text: "재생 탐색 / 문제구간 이동"; font.pixelSize: Math.round(15 * root.uiScale); font.bold: true; color: "#243447" }
                        Item { Layout.fillWidth: true }
                        Components.StatusBadge { text: appController.replayIssueSummary; kind: "info"; uiScale: root.uiScale }
                        Button { text: "재생"; onClicked: root.openPanel("replay") }
                    }

                    GridLayout {
                        Layout.fillWidth: true
                        columns: width > 1200 ? 3 : 1
                        rowSpacing: 10
                        columnSpacing: 10

                        Components.InfoCard {
                            Layout.fillWidth: true
                            title: "주기 문제구간"
                            value: appController.replayTimingMarkerCount > 0 ? ("마커 " + appController.replayTimingMarkerCount) : "없음"
                            note: "재생 막대/버튼으로 실제 이슈 지점 이동"
                            badgeText: appController.replayTimingMarkerCount > 0 ? "T" : "-"
                            kind: appController.replayTimingMarkerCount > 0 ? "info" : "ok"
                            uiScale: root.uiScale
                            clickable: appController.replayTimingMarkerCount > 0
                            onClicked: root.openPanel("replay")
                        }
                        Components.InfoCard {
                            Layout.fillWidth: true
                            title: "값 문제구간"
                            value: appController.replayValueMarkerCount > 0 ? ("마커 " + appController.replayValueMarkerCount) : "없음"
                            note: "실제 값 이상이 찍힌 프레임 기준"
                            badgeText: appController.replayValueMarkerCount > 0 ? "V" : "-"
                            kind: appController.replayValueMarkerCount > 0 ? "warn" : "ok"
                            uiScale: root.uiScale
                            clickable: appController.replayValueMarkerCount > 0
                            onClicked: root.openPanel("replay")
                        }
                        Components.InfoCard {
                            Layout.fillWidth: true
                            title: "경보 문제구간"
                            value: appController.replayAlarmMarkerCount > 0 ? ("마커 " + appController.replayAlarmMarkerCount) : "없음"
                            note: "값·버스 경보 구조 기준"
                            badgeText: appController.replayAlarmMarkerCount > 0 ? "A" : "-"
                            kind: appController.replayAlarmMarkerCount > 0 ? "bad" : "ok"
                            uiScale: root.uiScale
                            clickable: appController.replayAlarmMarkerCount > 0
                            onClicked: root.openPanel("replay")
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        Button { text: "이전 주기"; enabled: appController.replayTimingMarkerCount > 0; onClicked: appController.seekReplayIssue("timing", -1) }
                        Button { text: "다음 주기"; enabled: appController.replayTimingMarkerCount > 0; onClicked: appController.seekReplayIssue("timing", 1) }
                        Button { text: "이전 값"; enabled: appController.replayValueMarkerCount > 0; onClicked: appController.seekReplayIssue("value", -1) }
                        Button { text: "다음 값"; enabled: appController.replayValueMarkerCount > 0; onClicked: appController.seekReplayIssue("value", 1) }
                        Button { text: "이전 경보"; enabled: appController.replayAlarmMarkerCount > 0; onClicked: appController.seekReplayIssue("alarm", -1) }
                        Button { text: "다음 경보"; enabled: appController.replayAlarmMarkerCount > 0; onClicked: appController.seekReplayIssue("alarm", 1) }
                        Item { Layout.fillWidth: true }
                        Label { text: appController.replayCursorSummary; color: "#607080"; Layout.fillWidth: true; wrapMode: Text.WordWrap }
                    }
                }
            }

            Frame {
                Layout.fillWidth: true
                padding: 0
                background: Rectangle { color: "#ffffff"; radius: 12; border.color: "#dbe5f0" }
                RowLayout {
                    anchors.fill: parent
                    anchors.margins: Math.round(10 * root.uiScale)
                    spacing: Math.round(6 * root.uiScale)
                    Components.StatusBadge { text: "SYS " + appController.systemLevel; kind: root.kindFromLevel(appController.systemLevel); uiScale: root.uiScale }
                    Components.StatusBadge { text: "T " + appController.timingLevel; kind: root.kindFromLevel(appController.timingLevel); uiScale: root.uiScale }
                    Components.StatusBadge { text: "V " + appController.valueLevel; kind: root.kindFromLevel(appController.valueLevel); uiScale: root.uiScale }
                    Components.StatusBadge { text: "A " + appController.alarmLevel; kind: root.kindFromLevel(appController.alarmLevel); uiScale: root.uiScale }
                    Label { text: appController.analysisContextText; color: "#607080"; elide: Text.ElideRight; Layout.fillWidth: true }
                }
            }

            GridLayout {
                Layout.fillWidth: true
                columns: width > 1500 ? 5 : (width > 1050 ? 3 : 1)
                rowSpacing: 10
                columnSpacing: 10

                Components.InfoCard {
                    Layout.fillWidth: true
                    title: "포트 / 연결"
                    value: appController.connected ? "연결됨" : "미연결"
                    note: appController.statusText + " · " + appController.liveStatsSummary + " · 활성/누적 T/V/A " + appController.timingIssueCount + "/" + appController.valueIssueCount + "/" + appController.activeAlarmCount + " · " + appController.timingCumulativeCount + "/" + appController.valueCumulativeCount + "/" + appController.alarmCumulativeCount
                    badgeText: appController.connected ? "LIVE" : "OFF"
                    kind: appController.connected ? "ok" : "bad"
                    uiScale: root.uiScale
                    clickable: true
                    onClicked: root.openPanel("live")
                }
                Components.InfoCard {
                    Layout.fillWidth: true
                    title: "모델"
                    value: appController.modelActive ? appController.modelName : "모델 해제"
                    note: appController.modelModeText + " · " + (appController.modelActive ? ((appController.modelVendor !== "" ? appController.modelVendor + " / " : "") + (appController.modelVersion !== "" ? appController.modelVersion + " · " : "") + appController.modelSourceSummary) : "모델 없이도 주기 관찰 + RAW 표시는 가능")
                    badgeText: appController.modelActive ? "MODEL" : "RAW"
                    kind: appController.modelActive ? "info" : "warn"
                    uiScale: root.uiScale
                    clickable: true
                    onClicked: root.openPanel("settings")
                }
                Components.InfoCard {
                    Layout.fillWidth: true
                    title: "수집 / 재생"
                    value: appController.analysisSourceText
                    note: appController.replayCursorSummary + " · " + appController.analysisContextText
                    badgeText: appController.replayAnalysisActive ? "REPLAY" : "LIVE"
                    kind: appController.replayAnalysisActive ? "warn" : "ok"
                    uiScale: root.uiScale
                    clickable: true
                    onClicked: root.openPanel(appController.replayLoaded ? "replay" : "live")
                }
                Components.InfoCard {
                    Layout.fillWidth: true
                    title: "버스 상태"
                    value: appController.busHealthLevel
                    note: appController.busHealthText
                    badgeText: "BUS"
                    kind: root.kindFromLevel(appController.busHealthLevel)
                    uiScale: root.uiScale
                    clickable: true
                    onClicked: root.openPanel("settings")
                }

                Components.InfoCard {
                    Layout.fillWidth: true
                    title: "모델 진단"
                    value: appController.modelDiagnosticsLevel
                    note: appController.modelDiagnosticsSummary
                    badgeText: "CHECK"
                    kind: root.kindFromLevel(appController.modelDiagnosticsLevel)
                    uiScale: root.uiScale
                    clickable: true
                    onClicked: root.openPanel("settings")
                }
                Components.InfoCard {
                    Layout.fillWidth: true
                    title: "해석 신뢰도"
                    value: appController.analysisReliabilityText
                    note: appController.rootCauseSummary + " · " + appController.activeViewStateSummary
                    badgeText: "WHY"
                    kind: root.kindFromLevel(appController.systemLevel)
                    uiScale: root.uiScale
                    clickable: true
                    onClicked: root.openPanel("settings")
                }
                Components.InfoCard {
                    Layout.fillWidth: true
                    title: "표시 행 수"
                    value: String(appController.timingModel.count) + " / " + String(appController.valueModel.count) + " / " + String(appController.alarmModel.count)
                    note: "주기 / 값 / 값·버스 경보 · 관찰 ID L/R " + appController.liveObservedIdCount + "/" + appController.replayObservedIdCount
                    badgeText: "ROWS"
                    kind: "info"
                    uiScale: root.uiScale
                    clickable: true
                    onClicked: root.openPanel("timing")
                }
            }

            Frame {
                Layout.fillWidth: true
                padding: 0
                background: Rectangle { color: "#ffffff"; radius: 12; border.color: "#dbe5f0" }
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: Math.round(12 * root.uiScale)
                    spacing: 10

                    RowLayout {
                        Layout.fillWidth: true
                        Label { text: "핵심 이슈 모니터링"; font.pixelSize: Math.round(15 * root.uiScale); font.bold: true; color: "#243447" }
                        Item { Layout.fillWidth: true }
                        Button { text: "주기"; onClicked: root.openPanel("timing") }
                        Button { text: "값"; onClicked: root.openPanel("value") }
                        Button { text: "경보"; onClicked: root.openPanel("alarm") }
                    }

                    GridLayout {
                        Layout.fillWidth: true
                        columns: width > 1200 ? 3 : 1
                        rowSpacing: 10
                        columnSpacing: 10
                        Repeater {
                            model: [
                                { key: "timing", title: "주기", level: appController.timingLevel },
                                { key: "value", title: "값", level: appController.valueLevel },
                                { key: "alarm", title: "경보(값·버스)", level: appController.alarmLevel }
                            ]
                            delegate: Components.InfoCard {
                                required property var modelData
                                Layout.fillWidth: true
                                preferredHeight: Math.round(118 * root.uiScale)
                                title: modelData.title + " 상태"
                                value: root.monitorValue(modelData.key)
                                note: (root.focusKey(modelData.key) !== "" ? (root.focusKey(modelData.key) + " · ") : "") + root.monitorNote(modelData.key)
                                badgeText: modelData.level === "NONE" ? "-" : modelData.level
                                kind: root.kindFromLevel(modelData.level)
                                uiScale: root.uiScale
                                clickable: true
                                onClicked: {
                                    const idText = root.focusKey(modelData.key)
                                    if (idText !== "") root.focusIssue(modelData.key, idText)
                                    else root.openPanel(modelData.key)
                                }
                            }
                        }
                    }
                }
            }
            Frame {
                Layout.fillWidth: true
                padding: 0
                background: Rectangle { color: "#ffffff"; radius: 12; border.color: "#dbe5f0" }
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: Math.round(12 * root.uiScale)
                    spacing: 10

                    RowLayout {
                        Layout.fillWidth: true
                        Label { text: "최근 상태 변화"; font.pixelSize: Math.round(15 * root.uiScale); font.bold: true; color: "#243447" }
                        Item { Layout.fillWidth: true }
                        Components.StatusBadge { text: "업타임 " + appController.sessionUptimeText; kind: "info"; uiScale: root.uiScale }
                        Components.StatusBadge { text: "RECENT"; kind: "info"; uiScale: root.uiScale }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        radius: 10
                        color: "#f8fafc"
                        border.color: "#dbe5f0"
                        implicitHeight: Math.round(36 * root.uiScale)
                        Label {
                            anchors.fill: parent
                            anchors.margins: 8
                            text: appController.operatorRecentSummary
                            color: "#35506b"
                            wrapMode: Text.WordWrap
                            verticalAlignment: Text.AlignVCenter
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 6
                        Repeater {
                            model: appController.operatorRecentEvents
                            delegate: Rectangle {
                                required property var modelData
                                Layout.fillWidth: true
                                radius: 9
                                color: modelData.level === "ERR" ? "#fff1f2" : (modelData.level === "WARN" ? "#fffaf0" : "#f8fafc")
                                border.color: modelData.level === "ERR" ? "#fecdd3" : (modelData.level === "WARN" ? "#f3d7a1" : "#dbe5f0")
                                implicitHeight: bodyCol.implicitHeight + 14
                                ColumnLayout {
                                    id: bodyCol
                                    anchors.fill: parent
                                    anchors.margins: 7
                                    spacing: 2
                                    RowLayout {
                                        Layout.fillWidth: true
                                        Components.StatusBadge { text: modelData.category; kind: modelData.level === "ERR" ? "bad" : (modelData.level === "WARN" ? "warn" : "info"); uiScale: root.uiScale }
                                        Label { text: modelData.summary; color: "#243447"; font.bold: true; elide: Text.ElideRight; Layout.fillWidth: true }
                                        Label { text: modelData.timeText + " · " + modelData.ageText + " 전"; color: "#607080"; font.pixelSize: Math.round(10.5 * root.uiScale) }
                                    }
                                    Label {
                                        visible: modelData.detail !== ""
                                        text: modelData.detail
                                        color: "#52606d"
                                        wrapMode: Text.WordWrap
                                        Layout.fillWidth: true
                                        font.pixelSize: Math.round(11 * root.uiScale)
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
