import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components" as Components

Item {
    id: root
    property real uiScale: 1.0
    signal exportSnapshot()

    function levelKind(level) {
        if (level === "ERR") return "bad"
        if (level === "WARN") return "warn"
        if (level === "OK") return "ok"
        return "info"
    }

    function columnsFor(widthValue) {
        return widthValue > 1220 ? 3 : (widthValue > 760 ? 2 : 1)
    }

    ScrollView {
        anchors.fill: parent
        clip: true
        ScrollBar.horizontal.policy: ScrollBar.AsNeeded

        ColumnLayout {
            width: Math.max(root.width - 18, Math.round(1180 * root.uiScale))
            spacing: Math.round(12 * uiScale)

            Frame {
                Layout.fillWidth: true
                padding: 0
                background: Rectangle { color: "#ffffff"; radius: 12; border.color: "#d7e0ea" }
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: Math.round(10 * uiScale)
                    spacing: Math.round(8 * uiScale)
                    RowLayout {
                        Layout.fillWidth: true
                        Label { text: "모델/세션"; font.bold: true; color: "#243447" }
                        Item { Layout.fillWidth: true }
                        Components.StatusBadge { text: "SYS " + appController.systemLevel; kind: root.levelKind(appController.systemLevel); uiScale: root.uiScale }
                        Button { text: "스냅샷"; font.pixelSize: Math.round(11.0 * uiScale); onClicked: exportSnapshot() }
                        Button { text: "분석 초기화"; font.pixelSize: Math.round(11.0 * uiScale); onClicked: appController.resetAnalysisContext() }
                        Button { text: "필터 초기화"; font.pixelSize: Math.round(11.0 * uiScale); onClicked: appController.resetAllAnalysisFilters() }
                        Button { text: "세션 초기화"; font.pixelSize: Math.round(11.0 * uiScale); onClicked: appController.clearSavedSession() }
                    }
                    Rectangle {
                        Layout.fillWidth: true
                        radius: 10
                        color: "#f8fbff"
                        border.color: "#dbe5f0"
                        implicitHeight: sessionText.implicitHeight + 20
                        Label {
                            id: sessionText
                            anchors.fill: parent
                            anchors.margins: 10
                            text: appController.sessionSummary
                            color: "#243447"
                            wrapMode: Text.WordWrap
                            font.pixelSize: Math.round(11.3 * uiScale)
                        }
                    }
                }
            }

            Frame {
                Layout.fillWidth: true
                padding: 0
                background: Rectangle { color: "#ffffff"; radius: 12; border.color: "#d7e0ea" }
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: Math.round(10 * uiScale)
                    spacing: Math.round(8 * uiScale)
                    RowLayout {
                        Layout.fillWidth: true
                        Components.StatusBadge { text: "ACTION " + appController.operatorActionLevel; kind: root.levelKind(appController.operatorActionLevel); uiScale: root.uiScale }
                        Label { text: "운용 헤드라인"; font.bold: true; color: "#243447" }
                        Item { Layout.fillWidth: true }
                        Label { text: appController.primaryIssueId !== "" ? (appController.primaryIssueId + " · " + appController.primaryIssueSummary) : appController.primaryIssueSummary; color: "#607080"; elide: Text.ElideRight; Layout.preferredWidth: Math.round(420 * uiScale) }
                    }
                    Rectangle {
                        Layout.fillWidth: true
                        radius: 10
                        color: appController.operatorActionLevel === "ERR" ? "#fff1f2" : (appController.operatorActionLevel === "WARN" ? "#fffaf0" : "#f8fafc")
                        border.color: appController.operatorActionLevel === "ERR" ? "#fecdd3" : (appController.operatorActionLevel === "WARN" ? "#fcd34d" : "#dbe5f0")
                        implicitHeight: Math.round(54 * uiScale)
                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 8
                            spacing: 4
                            Label { text: appController.operatorHeadline; color: "#243447"; font.bold: true; wrapMode: Text.WordWrap; Layout.fillWidth: true }
                            Label { text: appController.operatorActionText; color: "#52606d"; wrapMode: Text.WordWrap; Layout.fillWidth: true }
                        }
                    }
                }
            }

            GridLayout {
                Layout.fillWidth: true
                columns: columnsFor(width)
                rowSpacing: 10
                columnSpacing: 10

                Components.InfoCard {
                    Layout.fillWidth: true
                    title: "시스템 레벨"
                    value: appController.systemLevel
                    note: "주기 " + appController.timingIssueCount + " · 값 " + appController.valueIssueCount + " · 경보(값/버스) " + appController.activeAlarmCount
                    badgeText: "SYS"
                    kind: root.levelKind(appController.systemLevel)
                    uiScale: root.uiScale
                }
                Components.InfoCard {
                    Layout.fillWidth: true
                    title: "모델 상태"
                    value: appController.modelActive ? appController.modelName : "모델 해제"
                    note: appController.modelModeText + " · " + (appController.modelVendor !== "" ? appController.modelVendor + " / " : "") + (appController.modelVersion !== "" ? appController.modelVersion + " · " : "") + appController.modelSourceSummary
                    badgeText: appController.modelActive ? "MODEL" : "OFF"
                    kind: appController.modelActive ? "ok" : "warn"
                    uiScale: root.uiScale
                }
                Components.InfoCard {
                    Layout.fillWidth: true
                    title: "시그널 해석"
                    value: appController.signalDbLoaded ? (String(appController.signalDbMessageCount) + " ID") : "미적용"
                    note: appController.signalDbSummary
                    badgeText: appController.signalDbLoaded ? "READY" : "NONE"
                    kind: appController.signalDbLoaded ? "info" : "warn"
                    uiScale: root.uiScale
                }
                Components.InfoCard {
                    Layout.fillWidth: true
                    title: "모델 메타"
                    value: appController.modelVendor !== "" ? appController.modelVendor : "vendor 미지정"
                    note: (appController.modelVersion !== "" ? appController.modelVersion : "버전 미지정") + " · " + (appController.modelSchema !== "" ? appController.modelSchema : "스키마 미지정")
                    badgeText: "META"
                    kind: appController.modelActive ? "info" : "warn"
                    uiScale: root.uiScale
                }

                Components.InfoCard {
                    Layout.fillWidth: true
                    title: "모델 진단"
                    value: appController.modelDiagnosticsLevel
                    note: appController.modelDiagnosticsSummary
                    badgeText: "CHECK"
                    kind: root.levelKind(appController.modelDiagnosticsLevel)
                    uiScale: root.uiScale
                }
                Components.InfoCard {
                    Layout.fillWidth: true
                    title: "버스 상태"
                    value: appController.busHealthLevel
                    note: appController.busHealthText
                    badgeText: "BUS"
                    kind: root.levelKind(appController.busHealthLevel)
                    uiScale: root.uiScale
                }
                Components.InfoCard {
                    Layout.fillWidth: true
                    title: "라이브 / 재생"
                    value: appController.analysisSourceText
                    note: appController.replayCursorSummary + " · " + appController.activeViewStateSummary
                    badgeText: appController.replayAnalysisActive ? "REPLAY" : (appController.connected ? "LIVE" : "OFF")
                    kind: appController.replayAnalysisActive ? "warn" : (appController.connected ? "ok" : "warn")
                    uiScale: root.uiScale
                }
                Components.InfoCard {
                    Layout.fillWidth: true
                    title: "필터 상태"
                    value: appController.activeViewStateSummary
                    note: "라이브 프레임 " + (appController.liveFrameView.idFilter === "" ? "없음" : appController.liveFrameView.idFilter) + " · 재생 프레임 " + (appController.replayFrameView.idFilter === "" ? "없음" : appController.replayFrameView.idFilter)
                    badgeText: "FILTER"
                    kind: "info"
                    uiScale: root.uiScale
                }
                Components.InfoCard {
                    Layout.fillWidth: true
                    title: "해석 신뢰도"
                    value: appController.analysisReliabilityText
                    note: appController.rootCauseSummary + " · " + appController.analysisContextText
                    badgeText: "WHY"
                    kind: root.levelKind(appController.systemLevel)
                    uiScale: root.uiScale
                }
                Components.InfoCard {
                    Layout.fillWidth: true
                    title: "관찰 ID"
                    value: "LIVE " + appController.liveObservedIdCount + " / REPLAY " + appController.replayObservedIdCount
                    note: appController.analysisContextText
                    badgeText: "IDS"
                    kind: "info"
                    uiScale: root.uiScale
                }
            }

            Frame {
                Layout.fillWidth: true
                padding: 0
                background: Rectangle { color: "#ffffff"; radius: 12; border.color: "#d7e0ea" }
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: Math.round(14 * uiScale)
                    spacing: Math.round(10 * uiScale)

                    Rectangle {
                        Layout.fillWidth: true
                        radius: 10
                        color: "#eef6ff"
                        border.color: "#bfd6ff"
                        implicitHeight: deployText.implicitHeight + 20
                        Label {
                            id: deployText
                            anchors.fill: parent
                            anchors.margins: 10
                            color: "#243447"
                            wrapMode: Text.WordWrap
                            font.pixelSize: Math.round(11.4 * uiScale)
                            text: "빌드 후 DLL을 매번 수동 복사하지 않도록 CMake에 windeployqt 후처리를 넣었습니다. 빌드 시간은 조금 늘 수 있지만 실행 성능이 느려지지는 않습니다."
                        }
                    }

                    GridLayout {
                        Layout.fillWidth: true
                        columns: width > 880 ? 2 : 1
                        rowSpacing: 8
                        columnSpacing: 14

                        Label { text: "모델 파일"; color: "#52606d"; font.bold: true }
                        Label { text: appController.modelPath !== "" ? appController.modelPath : appController.modelSourceSummary; color: "#243447"; wrapMode: Text.WrapAnywhere; Layout.fillWidth: true }

                        Label { text: "모델 이름"; color: "#52606d"; font.bold: true }
                        Label { text: appController.modelName; color: "#243447"; wrapMode: Text.WrapAnywhere; Layout.fillWidth: true }

                        Label { text: "모델 키"; color: "#52606d"; font.bold: true }
                        Label { text: appController.modelKey !== "" ? appController.modelKey : "미지정"; color: "#243447"; wrapMode: Text.WrapAnywhere; Layout.fillWidth: true }

                        Label { text: "모델 버전 / 스키마"; color: "#52606d"; font.bold: true }
                        Label { text: (appController.modelVersion !== "" ? appController.modelVersion : "버전 미지정") + " / " + (appController.modelSchema !== "" ? appController.modelSchema : "스키마 미지정"); color: "#243447"; wrapMode: Text.WrapAnywhere; Layout.fillWidth: true }

                        Label { text: "모델 Vendor"; color: "#52606d"; font.bold: true }
                        Label { text: appController.modelVendor !== "" ? appController.modelVendor : "vendor 미지정"; color: "#243447"; wrapMode: Text.WrapAnywhere; Layout.fillWidth: true }

                        Label { text: "모델 소스"; color: "#52606d"; font.bold: true }
                        Label { text: appController.modelSourceSummary; color: appController.modelActive ? "#118a42" : "#c0392b"; wrapMode: Text.WrapAnywhere; Layout.fillWidth: true }

                        Label { text: "모델 모드"; color: "#52606d"; font.bold: true }
                        Label { text: appController.modelModeText; color: "#243447"; wrapMode: Text.WrapAnywhere; Layout.fillWidth: true }

                        Label { text: "분석 기준"; color: "#52606d"; font.bold: true }
                        Label { text: appController.analysisSourceText; color: "#243447"; wrapMode: Text.WrapAnywhere; Layout.fillWidth: true }

                        Label { text: "분석 컨텍스트"; color: "#52606d"; font.bold: true }
                        Label { text: appController.analysisContextText; color: "#243447"; wrapMode: Text.WrapAnywhere; Layout.fillWidth: true }

                        Label { text: "활성 필터/선택"; color: "#52606d"; font.bold: true }
                        Label { text: appController.activeViewStateSummary; color: "#243447"; wrapMode: Text.WrapAnywhere; Layout.fillWidth: true }

                        Label { text: "재생 커서"; color: "#52606d"; font.bold: true }
                        Label { text: appController.replayCursorSummary; color: "#243447"; wrapMode: Text.WrapAnywhere; Layout.fillWidth: true }

                        Label { text: "실시간 통계"; color: "#52606d"; font.bold: true }
                        Label { text: appController.liveStatsSummary; color: "#243447"; wrapMode: Text.WrapAnywhere; Layout.fillWidth: true }

                        Label { text: "버스 health 요약"; color: "#52606d"; font.bold: true }
                        Label { text: appController.busHealthText; color: "#243447"; wrapMode: Text.WrapAnywhere; Layout.fillWidth: true }

                        Label { text: "해석 신뢰도"; color: "#52606d"; font.bold: true }
                        Label { text: appController.analysisReliabilityText; color: "#243447"; wrapMode: Text.WrapAnywhere; Layout.fillWidth: true }

                        Label { text: "상태 문구"; color: "#52606d"; font.bold: true }
                        Label { text: appController.statusText; color: "#243447"; wrapMode: Text.WrapAnywhere; Layout.fillWidth: true }

                        Label { text: "모델 Notes"; color: "#52606d"; font.bold: true }
                        Label { text: appController.modelNotes !== "" ? appController.modelNotes : "notes 미지정"; color: "#243447"; wrapMode: Text.WordWrap; Layout.fillWidth: true }

                        Label { text: "모델 진단"; color: "#52606d"; font.bold: true }
                        Label { text: appController.modelDiagnosticsLevel + " · " + appController.modelDiagnosticsSummary; color: "#243447"; wrapMode: Text.WordWrap; Layout.fillWidth: true }

                        Label { text: "세션 설정 파일"; color: "#52606d"; font.bold: true }
                        Label { text: appController.sessionFilePath !== "" ? appController.sessionFilePath : "경로 확인 불가"; color: "#243447"; wrapMode: Text.WrapAnywhere; Layout.fillWidth: true }

                        Label { text: "기본 로그 폴더"; color: "#52606d"; font.bold: true }
                        Label { text: appController.defaultLogDirectory !== "" ? appController.defaultLogDirectory : "문서 폴더 확인 불가"; color: "#243447"; wrapMode: Text.WrapAnywhere; Layout.fillWidth: true }

                        Label { text: "기본 스냅샷 폴더"; color: "#52606d"; font.bold: true }
                        Label { text: appController.defaultSnapshotDirectory !== "" ? appController.defaultSnapshotDirectory : "문서 폴더 확인 불가"; color: "#243447"; wrapMode: Text.WrapAnywhere; Layout.fillWidth: true }

                        Label { text: "로그 상태"; color: "#52606d"; font.bold: true }
                        Label { text: appController.logStatusSummary; color: "#243447"; wrapMode: Text.WrapAnywhere; Layout.fillWidth: true }

                        Label { text: "로그 경로"; color: "#52606d"; font.bold: true }
                        Label { text: appController.logPath !== "" ? appController.logPath : "로그 미시작"; color: "#243447"; wrapMode: Text.WrapAnywhere; Layout.fillWidth: true }

                        Label { text: "재생 파일"; color: "#52606d"; font.bold: true }
                        Label { text: appController.replayPath !== "" ? appController.replayPath : "재생 파일 미선택"; color: "#243447"; wrapMode: Text.WrapAnywhere; Layout.fillWidth: true }
                    }
                }
            }
            Frame {
                Layout.fillWidth: true
                padding: 0
                background: Rectangle { color: "#ffffff"; radius: 12; border.color: "#d7e0ea" }
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: Math.round(10 * uiScale)
                    spacing: Math.round(8 * uiScale)
                    RowLayout {
                        Layout.fillWidth: true
                        Label { text: "최근 상태 변화"; font.bold: true; color: "#243447" }
                        Item { Layout.fillWidth: true }
                        Components.StatusBadge { text: "업타임 " + appController.sessionUptimeText; kind: "info"; uiScale: root.uiScale }
                    }
                    Rectangle {
                        Layout.fillWidth: true
                        radius: 10
                        color: "#f8fafc"
                        border.color: "#dbe5f0"
                        implicitHeight: Math.round(34 * uiScale)
                        Label {
                            anchors.fill: parent
                            anchors.margins: 8
                            text: appController.operatorRecentSummary
                            color: "#35506b"
                            elide: Text.ElideRight
                            verticalAlignment: Text.AlignVCenter
                        }
                    }
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 6
                        Repeater {
                            model: appController.operatorRecentEvents
                            delegate: RowLayout {
                                required property var modelData
                                Layout.fillWidth: true
                                spacing: 6
                                Components.StatusBadge { text: modelData.category; kind: modelData.level === "ERR" ? "bad" : (modelData.level === "WARN" ? "warn" : "info"); uiScale: root.uiScale }
                                Label { text: modelData.summary; color: "#243447"; elide: Text.ElideRight; Layout.fillWidth: true }
                                Label { text: modelData.ageText + " 전"; color: "#607080"; font.pixelSize: Math.round(10.6 * uiScale) }
                            }
                        }
                    }
                }
            }

        }
    }
}
