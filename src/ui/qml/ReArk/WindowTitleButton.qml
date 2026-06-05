import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material

ToolButton {
    id: root

    property string buttonType: "minimize"
    readonly property bool darkTheme: Material.theme === Material.Dark
    readonly property string iconGlyph: {
        if (buttonType === "minimize") {
            return "\uE921"
        }
        if (buttonType === "maximize") {
            return "\uE922"
        }
        if (buttonType === "restore") {
            return "\uE923"
        }
        return "\uE8BB"
    }
    readonly property color iconColor: root.hovered && root.buttonType === "close"
                                      ? "#ffffff"
                                      : (root.darkTheme ? "#d9dde3" : "#202020")
    readonly property color hoverColor: root.buttonType === "close"
                                      ? "#c42b1c"
                                      : (root.darkTheme ? "#555555" : "#e5e5e5")

    implicitWidth: 46
    implicitHeight: 32
    display: AbstractButton.IconOnly
    padding: 0

    background: Rectangle {
        color: root.hovered ? root.hoverColor : "transparent"
    }

    contentItem: Text {
        text: root.iconGlyph
        color: root.iconColor
        font.family: "Segoe MDL2 Assets"
        font.pixelSize: 10
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        renderType: Text.NativeRendering
    }
}
