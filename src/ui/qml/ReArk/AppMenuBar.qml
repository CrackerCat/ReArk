import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import com.reark.app

Rectangle {
    id: root

    property string currentTheme: "dark"
    property string currentHighlightTheme: "GitHub Dark"
    property bool embedded: false
    property bool menuNavigationActive: false
    readonly property bool darkTheme: Material.theme === Material.Dark

    signal openRequested()
    signal themeRequested(string theme)
    signal highlightThemeRequested(string theme)

    implicitWidth: menuRow.implicitWidth
    implicitHeight: embedded ? 32 : 30
    height: implicitHeight
    color: embedded ? "transparent" : (darkTheme ? "#3f3f3f" : "#f3f3f3")

    function showMenu(source, menu, toggle) {
        if (toggle && menu.visible) {
            menu.close()
            return
        }

        menuNavigationActive = true
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

    function anyMenuVisible() {
        return fileMenu.visible || viewMenu.visible || helpMenu.visible
    }

    function leaveMenuNavigationWhenClosed() {
        Qt.callLater(function() {
            if (!root.anyMenuVisible()) {
                root.menuNavigationActive = false
            }
        })
    }

    RowLayout {
        id: menuRow

        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        spacing: 0

        MenuBarButton {
            id: fileButton

            text: "File"
            menu: fileMenu
            embedded: root.embedded
            menuNavigationActive: root.menuNavigationActive
            onMenuRequested: function(source, menu, toggle) { root.showMenu(source, menu, toggle) }
        }

        MenuBarButton {
            id: viewButton

            text: "View"
            menu: viewMenu
            embedded: root.embedded
            menuNavigationActive: root.menuNavigationActive
            onMenuRequested: function(source, menu, toggle) { root.showMenu(source, menu, toggle) }
        }

        MenuBarButton {
            id: helpButton

            text: "Help"
            menu: helpMenu
            embedded: root.embedded
            menuNavigationActive: root.menuNavigationActive
            onMenuRequested: function(source, menu, toggle) { root.showMenu(source, menu, toggle) }
        }
    }

    Shortcut {
        sequence: "Alt+F"
        onActivated: root.showMenu(fileButton, fileMenu, false)
    }

    Shortcut {
        sequence: "Alt+V"
        onActivated: root.showMenu(viewButton, viewMenu, false)
    }

    Shortcut {
        sequence: "Alt+H"
        onActivated: root.showMenu(helpButton, helpMenu, false)
    }

    CompactMenu {
        id: fileMenu
        minimumItemWidth: 200
        onClosed: root.leaveMenuNavigationWhenClosed()

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
        onClosed: root.leaveMenuNavigationWhenClosed()

        SyntaxThemeProvider {
            id: syntaxThemeProvider
        }

        CompactMenu {
            id: syntaxHighlightMenu

            title: qsTr("Syntax Highlight")
            minimumItemWidth: 210
            delegate: MenuItem {
                id: syntaxThemeItem

                implicitHeight: 28
                padding: 12
                verticalPadding: 4
                spacing: 12
                font.pixelSize: 13
                indicator: null

                contentItem: RowLayout {
                    spacing: 12

                    Label {
                        Layout.fillWidth: true
                        text: syntaxThemeItem.text
                        color: syntaxThemeItem.enabled ? Material.foreground : Material.hintTextColor
                        font: syntaxThemeItem.font
                        elide: Text.ElideRight
                        verticalAlignment: Text.AlignVCenter
                    }

                    Label {
                        Layout.preferredWidth: 18
                        text: syntaxThemeItem.checked ? "✓" : ""
                        color: syntaxThemeItem.enabled ? Material.foreground : Material.hintTextColor
                        font: syntaxThemeItem.font
                        horizontalAlignment: Text.AlignRight
                        verticalAlignment: Text.AlignVCenter
                    }
                }
            }

            Instantiator {
                model: syntaxThemeProvider.themes

                delegate: Action {
                    text: modelData
                    checkable: true
                    checked: root.currentHighlightTheme === modelData
                    onTriggered: root.highlightThemeRequested(modelData)
                }

                onObjectAdded: function(index, object) {
                    syntaxHighlightMenu.insertAction(index, object)
                }

                onObjectRemoved: function(index, object) {
                    syntaxHighlightMenu.removeAction(object)
                }
            }
        }
    }

    CompactMenu {
        id: helpMenu
        onClosed: root.leaveMenuNavigationWhenClosed()

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
