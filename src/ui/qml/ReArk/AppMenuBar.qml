import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

Rectangle {
    id: root

    property string currentTheme: "dark"
    property string currentHighlightTheme: "github-dark"
    readonly property bool darkTheme: Material.theme === Material.Dark

    signal openRequested()
    signal themeRequested(string theme)
    signal highlightThemeRequested(string theme)

    implicitHeight: 30
    height: 30
    color: darkTheme ? "#3f3f3f" : "#f3f3f3"

    function showMenu(source, menu) {
        if (fileMenu !== menu) {
            fileMenu.close()
        }
        if (viewMenu !== menu) {
            viewMenu.close()
        }
        if (helpMenu !== menu) {
            helpMenu.close()
        }
        if (!menu.visible) {
            menu.popup(source, 0, source.height)
        }
    }

    RowLayout {
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        spacing: 0

        MenuBarButton {
            text: "File"
            menu: fileMenu
            onMenuRequested: function(source, menu) { root.showMenu(source, menu) }
        }

        MenuBarButton {
            text: "View"
            menu: viewMenu
            onMenuRequested: function(source, menu) { root.showMenu(source, menu) }
        }

        MenuBarButton {
            text: "Help"
            menu: helpMenu
            onMenuRequested: function(source, menu) { root.showMenu(source, menu) }
        }
    }

    CompactMenu {
        id: fileMenu
        minimumItemWidth: 200

        Action {
            text: qsTr("Open...")
            shortcut: StandardKey.Open
            onTriggered: root.openRequested()
        }

        CompactMenuSeparator {}

        CompactMenu {
            title: qsTr("Preferences")
            minimumItemWidth: 184

            CompactMenu {
                title: qsTr("Theme")
                minimumItemWidth: 150

                Action {
                    text: qsTr("Dark")
                    onTriggered: root.themeRequested("dark")
                }

                Action {
                    text: qsTr("Light")
                    onTriggered: root.themeRequested("light")
                }

                Action {
                    text: qsTr("System")
                    onTriggered: root.themeRequested("system")
                }
            }
        }

        CompactMenuSeparator {}

        Action {
            text: qsTr("Exit")
            shortcut: StandardKey.Quit
            onTriggered: Qt.quit()
        }
    }

    CompactMenu {
        id: viewMenu

        Action {
            text: qsTr("GitHub Dark")
            checkable: true
            checked: root.currentHighlightTheme === "github-dark"
            onTriggered: root.highlightThemeRequested("github-dark")
        }

        Action {
            text: qsTr("GitHub Light")
            checkable: true
            checked: root.currentHighlightTheme === "github-light"
            onTriggered: root.highlightThemeRequested("github-light")
        }

        Action {
            text: qsTr("Darcula")
            checkable: true
            checked: root.currentHighlightTheme === "darcula"
            onTriggered: root.highlightThemeRequested("darcula")
        }

        Action {
            text: qsTr("Monokai")
            checkable: true
            checked: root.currentHighlightTheme === "monokai"
            onTriggered: root.highlightThemeRequested("monokai")
        }
    }

    CompactMenu {
        id: helpMenu

        Action {
            text: qsTr("About ReArk")
            onTriggered: {
                var factory = Qt.createComponent("qrc:/ReArk/AboutWindow.qml")
                if (factory.status === Component.Ready) {
                    var aboutWindow = factory.createObject(null, {
                        "currentTheme": root.currentTheme
                    })
                    aboutWindow.show()
                } else {
                    console.error(factory.errorString())
                }
            }
        }
    }
}
