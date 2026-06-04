import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import com.reark.app

Rectangle {
    id: root

    property string code: ""
    property string highlightTheme: "github-dark"
    readonly property bool darkTheme: Material.theme === Material.Dark
    readonly property color editorColor: darkTheme ? "#111419" : "#ffffff"

    color: editorColor

    Flickable {
        id: flickable
        anchors.fill: parent
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        contentWidth: Math.max(editor.documentWidth, width)
        contentHeight: Math.max(editor.documentHeight, height)

        CodeEditorItem {
            id: editor
            x: flickable.contentX
            y: flickable.contentY
            height: flickable.height
            width: flickable.width
            text: root.code
            darkTheme: root.darkTheme
            highlightTheme: root.highlightTheme
            scrollX: flickable.contentX
            scrollY: flickable.contentY
        }

        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.RightButton
            onClicked: function(mouse) {
                contextMenu.popup(editor, mouse.x, mouse.y)
            }
        }

        CompactMenu {
            id: contextMenu
            minimumItemWidth: 136

            Action {
                text: qsTr("Copy")
                enabled: editor.hasSelection
                shortcut: StandardKey.Copy
                onTriggered: editor.copySelection()
            }

            Action {
                text: qsTr("Select All")
                shortcut: StandardKey.SelectAll
                onTriggered: editor.selectAll()
            }

            Action {
                text: qsTr("Select Line")
                onTriggered: editor.selectCurrentLine()
            }

            Action {
                text: qsTr("Clear Selection")
                enabled: editor.hasSelection
                onTriggered: editor.clearSelection()
            }
        }

        ScrollBar.vertical: ScrollBar {
            policy: ScrollBar.AsNeeded
        }

        ScrollBar.horizontal: ScrollBar {
            policy: ScrollBar.AsNeeded
        }
    }
}
