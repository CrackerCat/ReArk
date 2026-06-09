import QtQuick
import QtQuick.Controls.Material

Item {
    id: root

    property string name: ""
    property string kind: ""
    property bool directory: false
    property bool placeholder: false
    readonly property bool darkTheme: Material.theme === Material.Dark
    readonly property bool signatureNode: name === "Package signature" || name === "APK signature"
    readonly property string extension: {
        var dot = name.lastIndexOf(".")
        return dot >= 0 ? name.substring(dot + 1).toLowerCase() : kind.toLowerCase()
    }
    readonly property color folderColor: darkTheme ? "#6aa7c8" : "#2d7ea5"
    readonly property color fileColor: {
        if (placeholder) {
            return darkTheme ? "#7d8792" : "#8a949d"
        }
        if (signatureNode) {
            return "#d6b35d"
        }
        if (name === "Summary") {
            return "#8fb5ff"
        }
        if (extension === "ets" || extension === "ts") {
            return "#46a6d9"
        }
        if (extension === "js") {
            return "#c8b64d"
        }
        if (extension === "json" || extension === "json5") {
            return "#7cc47f"
        }
        if (extension === "txt") {
            return darkTheme ? "#9aa7b5" : "#6f7b86"
        }
        return darkTheme ? "#9aa7b5" : "#6f7b86"
    }

    width: 16
    height: 16
    opacity: placeholder ? 0.65 : 1.0

    Item {
        anchors.fill: parent
        visible: root.directory

        Rectangle {
            x: 2
            y: 4
            width: 7
            height: 3
            radius: 1
            color: root.folderColor
            opacity: 0.9
        }

        Rectangle {
            x: 2
            y: 6
            width: 12
            height: 8
            radius: 1.5
            color: root.folderColor
        }
    }

    Item {
        anchors.fill: parent
        visible: !root.directory

        Rectangle {
            x: 3
            y: 2
            width: 10
            height: 12
            radius: 1
            color: "transparent"
            border.width: 1
            border.color: root.fileColor
        }

        Rectangle {
            x: 9
            y: 2
            width: 4
            height: 4
            color: root.fileColor
            opacity: 0.85
        }

        Rectangle {
            x: 5
            y: 11
            width: 6
            height: 2
            radius: 0.5
            color: root.fileColor
            visible: root.signatureNode || root.name === "Summary"
        }
    }
}
