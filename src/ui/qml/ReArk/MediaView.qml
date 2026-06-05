import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import QtMultimedia

Rectangle {
    id: root

    property string sourceUrl: ""
    property string fileName: ""
    readonly property bool darkTheme: Material.theme === Material.Dark
    readonly property color backgroundColor: darkTheme ? "#111419" : "#f5f7f8"
    readonly property color panelColor: darkTheme ? "#171d24" : "#ffffff"
    readonly property color dividerColor: darkTheme ? "#2f3741" : "#cfd8de"
    readonly property color secondaryTextColor: darkTheme ? "#aab2bd" : "#5f6872"

    color: backgroundColor

    function formatDuration(value) {
        var totalSeconds = Math.floor(Math.max(0, value) / 1000)
        var seconds = totalSeconds % 60
        var minutes = Math.floor(totalSeconds / 60) % 60
        var hours = Math.floor(totalSeconds / 3600)
        var mmss = minutes.toString().padStart(2, "0") + ":" + seconds.toString().padStart(2, "0")
        return hours > 0 ? hours + ":" + mmss : mmss
    }

    MediaPlayer {
        id: player
        source: root.visible ? root.sourceUrl : ""
        videoOutput: videoOutput
        audioOutput: AudioOutput {
            muted: muteButton.checked
            volume: volumeSlider.value
        }

        onErrorOccurred: errorLabel.text = player.errorString
    }

    onSourceUrlChanged: {
        errorLabel.text = ""
        if (visible && sourceUrl.length > 0) {
            player.play()
        }
    }

    onVisibleChanged: {
        errorLabel.text = ""
        if (visible && sourceUrl.length > 0) {
            player.play()
        } else {
            player.stop()
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: root.backgroundColor

            VideoOutput {
                id: videoOutput
                anchors.fill: parent
                anchors.margins: 16
                fillMode: VideoOutput.PreserveAspectFit
            }

            Label {
                id: errorLabel
                anchors.centerIn: parent
                width: Math.min(480, parent.width - 48)
                visible: text.length > 0
                color: root.secondaryTextColor
                font.pixelSize: 13
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.Wrap
            }

            Label {
                anchors.centerIn: parent
                width: Math.min(480, parent.width - 48)
                visible: root.sourceUrl.length === 0
                text: qsTr("No media selected")
                color: root.secondaryTextColor
                font.pixelSize: 13
                horizontalAlignment: Text.AlignHCenter
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: root.dividerColor
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 44
            color: root.panelColor

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12
                anchors.rightMargin: 12
                spacing: 10

                MediaIconButton {
                    iconName: player.playbackState === MediaPlayer.PlayingState ? "pause" : "play"
                    enabled: root.sourceUrl.length > 0
                    ToolTip.text: player.playbackState === MediaPlayer.PlayingState ? qsTr("Pause") : qsTr("Play")
                    ToolTip.visible: hovered
                    onClicked: {
                        if (player.playbackState === MediaPlayer.PlayingState) {
                            player.pause()
                        } else {
                            player.play()
                        }
                    }
                }

                Slider {
                    Layout.fillWidth: true
                    enabled: player.seekable && player.duration > 0
                    from: 0
                    to: Math.max(1, player.duration)
                    value: player.position
                    onMoved: player.setPosition(value)
                }

                Label {
                    text: player.duration > 0
                          ? root.formatDuration(player.position) + " / " + root.formatDuration(player.duration)
                          : "00:00 / 00:00"
                    color: root.secondaryTextColor
                    font.family: "Consolas"
                    font.pixelSize: 12
                }

                MediaIconButton {
                    id: muteButton
                    checkable: true
                    iconName: checked ? "muted" : "sound"
                    ToolTip.text: checked ? qsTr("Muted") : qsTr("Sound")
                    ToolTip.visible: hovered
                }

                Slider {
                    id: volumeSlider
                    Layout.preferredWidth: 96
                    from: 0
                    to: 1
                    value: 0.8
                }
            }
        }
    }
}
