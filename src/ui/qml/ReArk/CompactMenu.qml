import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material

Menu {
    id: root

    readonly property bool darkTheme: Material.theme === Material.Dark
    property int minimumItemWidth: 168

    delegate: MenuItem {
        implicitHeight: 28
        padding: 12
        verticalPadding: 4
        spacing: 12
        font.pixelSize: 13
    }

    background: Rectangle {
        implicitWidth: root.minimumItemWidth
        color: root.darkTheme ? "#3f3f3f" : "#ffffff"
        radius: 3
        border.width: 0
    }
}
