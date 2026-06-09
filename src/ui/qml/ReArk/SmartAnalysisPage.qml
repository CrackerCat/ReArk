import QtQuick
import QtQuick.Controls
import QtQuick.Effects
import QtQuick.Controls.Material
import QtQuick.Layouts

Rectangle {
    id: root

    readonly property bool darkTheme: Material.theme === Material.Dark
    readonly property real scaleUnit: Math.max(0.72, Math.min(1.0, width / 1680))
    readonly property color pageColor: "#e8eef7"
    readonly property color panelColor: "#fbfcff"
    readonly property color primaryTextColor: "#0f172a"
    readonly property color secondaryTextColor: "#748094"
    readonly property color borderColor: "#cfd8e6"
    readonly property color iconColor: "#1f3354"
    readonly property color accentColor: "#5d83f4"
    readonly property color accentHoverColor: "#4e74e4"

    color: pageColor

    Button {
        id: newChatButton

        anchors.top: parent.top
        anchors.right: parent.right
        anchors.topMargin: Math.round(16 * root.scaleUnit)
        anchors.rightMargin: Math.round(24 * root.scaleUnit)
        width: Math.round(Math.max(104, newChatContent.implicitWidth + 30))
        height: Math.round(36 * root.scaleUnit)
        padding: 0
        hoverEnabled: true

        background: Rectangle {
            radius: height / 2
            color: newChatButton.hovered ? "#ffffff" : "#f7f9fd"
            border.width: 1
            border.color: root.borderColor
        }

        contentItem: Row {
            id: newChatContent

            anchors.centerIn: parent
            spacing: 8

            Text {
                text: "\uE8F4"
                color: root.iconColor
                font.family: "Segoe MDL2 Assets"
                font.pixelSize: 11 * root.scaleUnit
                anchors.verticalCenter: parent.verticalCenter
                renderType: Text.NativeRendering
            }

            Text {
                text: qsTr("New Chat")
                color: root.primaryTextColor
                font.pixelSize: 13 * root.scaleUnit
                font.weight: Font.DemiBold
                anchors.verticalCenter: parent.verticalCenter
                renderType: Text.NativeRendering
            }
        }
    }

    ColumnLayout {
        width: Math.min(930, Math.max(600, parent.width * 0.56))
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.verticalCenter: parent.verticalCenter
        anchors.verticalCenterOffset: -Math.round(18 * root.scaleUnit)
        spacing: Math.round(16 * root.scaleUnit)

        Label {
            Layout.fillWidth: true
            text: qsTr("What do you want to protect?")
            color: root.primaryTextColor
            font.pixelSize: Math.round(36 * root.scaleUnit)
            font.weight: Font.Bold
            horizontalAlignment: Text.AlignHCenter
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: Math.round(width * 0.14)
            radius: 8
            color: root.panelColor
            border.width: 1
            border.color: root.borderColor
            layer.enabled: true
            layer.effect: MultiEffect {
                shadowEnabled: true
                shadowBlur: 0.5
                shadowOpacity: 0.15
                shadowVerticalOffset: 4 * root.scaleUnit
            }

            TextEdit {
                id: promptInput

                anchors.left: parent.left
                anchors.right: sendButton.left
                anchors.top: parent.top
                anchors.bottom: toolRow.top
                anchors.leftMargin: Math.round(18 * root.scaleUnit)
                anchors.rightMargin: 16
                anchors.topMargin: Math.round(15 * root.scaleUnit)
                anchors.bottomMargin: 8
                wrapMode: TextEdit.Wrap
                color: root.primaryTextColor
                selectedTextColor: "#ffffff"
                selectionColor: root.accentColor
                font.pixelSize: Math.round(13 * root.scaleUnit)
            }

            Label {
                anchors.left: promptInput.left
                anchors.top: promptInput.top
                text: qsTr("Ask anything about app protection")
                color: root.secondaryTextColor
                font.pixelSize: Math.round(13 * root.scaleUnit)
                visible: promptInput.text.length === 0
            }

            Row {
                id: toolRow

                anchors.left: parent.left
                anchors.bottom: parent.bottom
                anchors.leftMargin: Math.round(24 * root.scaleUnit)
                anchors.bottomMargin: Math.round(20 * root.scaleUnit)
                spacing: Math.round(20 * root.scaleUnit)

                Canvas {
                    width: 14 * root.scaleUnit
                    height: 14 * root.scaleUnit
                    anchors.verticalCenter: parent.verticalCenter
                    onPaint: {
                        const ctx = getContext("2d")
                        ctx.clearRect(0, 0, width, height)
                        ctx.strokeStyle = root.iconColor
                        ctx.lineWidth = Math.max(1.4, 1.8 * root.scaleUnit)
                        ctx.lineCap = "round"
                        ctx.beginPath()
                        ctx.moveTo(width * 0.36, height * 0.72)
                        ctx.lineTo(width * 0.36, height * 0.28)
                        ctx.quadraticCurveTo(width * 0.36, height * 0.08, width * 0.54, height * 0.08)
                        ctx.quadraticCurveTo(width * 0.72, height * 0.08, width * 0.72, height * 0.28)
                        ctx.lineTo(width * 0.72, height * 0.72)
                        ctx.quadraticCurveTo(width * 0.72, height * 0.94, width * 0.5, height * 0.94)
                        ctx.quadraticCurveTo(width * 0.2, height * 0.94, width * 0.2, height * 0.62)
                        ctx.lineTo(width * 0.2, height * 0.28)
                        ctx.stroke()
                    }
                }

                Canvas {
                    width: 15 * root.scaleUnit
                    height: 15 * root.scaleUnit
                    anchors.verticalCenter: parent.verticalCenter
                    onPaint: {
                        const ctx = getContext("2d")
                        ctx.clearRect(0, 0, width, height)
                        ctx.strokeStyle = root.iconColor
                        ctx.lineWidth = Math.max(1.4, 1.8 * root.scaleUnit)
                        ctx.lineJoin = "round"
                        ctx.beginPath()
                        ctx.moveTo(width * 0.5, height * 0.16)
                        ctx.lineTo(width * 0.84, height * 0.5)
                        ctx.lineTo(width * 0.5, height * 0.84)
                        ctx.lineTo(width * 0.16, height * 0.5)
                        ctx.closePath()
                        ctx.stroke()
                    }
                }
            }

            Button {
                id: sendButton

                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.rightMargin: Math.round(16 * root.scaleUnit)
                anchors.bottomMargin: Math.round(13 * root.scaleUnit)
                width: Math.round(40 * root.scaleUnit)
                height: width
                padding: 0
                hoverEnabled: true

                background: Rectangle {
                    radius: width / 2
                    color: sendButton.hovered ? root.accentHoverColor : root.accentColor
                }

                contentItem: Text {
                    text: "\uE74A"
                    color: "#ffffff"
                    font.family: "Segoe MDL2 Assets"
                    font.pixelSize: Math.round(16 * root.scaleUnit)
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    renderType: Text.NativeRendering
                }
            }
        }
    }
}
