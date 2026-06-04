import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

Rectangle {
    id: root

    required property string text
    required property Menu menu
    readonly property bool darkTheme: Material.theme === Material.Dark

    signal menuRequested(var source, var menu)

    Layout.preferredWidth: label.implicitWidth + 22
    Layout.fillHeight: true
    color: mouse.containsMouse || menu.visible
           ? (darkTheme ? "#555555" : "#e5e5e5")
           : "transparent"

    Label {
        id: label
        anchors.centerIn: parent
        text: root.text
        color: root.darkTheme ? "#f2f2f2" : "#202020"
        font.pixelSize: 13
    }

    MouseArea {
        id: mouse
        anchors.fill: parent
        hoverEnabled: true
        onEntered: root.menuRequested(root, root.menu)
        onClicked: root.menuRequested(root, root.menu)
    }
}
