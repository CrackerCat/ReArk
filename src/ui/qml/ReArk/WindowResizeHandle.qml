import QtQuick
import QtQuick.Window

MouseArea {
    id: root

    required property Window targetWindow
    required property int edges

    acceptedButtons: Qt.LeftButton
    cursorShape: Qt.ArrowCursor
    enabled: targetWindow && targetWindow.visibility !== Window.Maximized
    hoverEnabled: true
    visible: enabled

    onPressed: function(mouse) {
        if (mouse.button === Qt.LeftButton && targetWindow) {
            targetWindow.startSystemResize(edges)
        }
    }
}
