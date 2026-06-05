import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Dialogs
import QtQuick.Layouts
import ReArk as RK

ApplicationWindow {
    id: mainWindow
    width: 1180
    height: 760
    minimumWidth: 900
    minimumHeight: 560
    visible: true
    flags: Qt.Window
           | Qt.FramelessWindowHint
           | Qt.WindowSystemMenuHint
           | Qt.WindowMinimizeButtonHint
           | Qt.WindowMaximizeButtonHint
           | Qt.WindowCloseButtonHint
    title: currentFileName.length > 0 ? "ReArk - " + currentFileName : "ReArk"

    property string currentTheme: "dark"
    property string currentHighlightTheme: "GitHub Dark"
    property url currentFileUrl: ""
    readonly property string currentFilePath: decodeURIComponent(currentFileUrl.toString().replace(/^file:\/+/, ""))
    readonly property string currentFileName: currentFilePath.length > 0 ? currentFilePath.split(/[\\/]/).pop() : ""

    Material.theme: currentTheme === "system"
                    ? Material.System
                    : (currentTheme === "light" ? Material.Light : Material.Dark)
    Material.accent: Material.Teal

    footer: RK.StatusBar {
        filePath: decompilerController.status
        version: appVersion
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        RK.WindowTitleBar {
            Layout.fillWidth: true
            targetWindow: mainWindow
            currentTheme: mainWindow.currentTheme
            currentHighlightTheme: mainWindow.currentHighlightTheme
            onOpenRequested: openFileDialog.open()
            onThemeRequested: function(theme) { mainWindow.currentTheme = theme }
            onHighlightThemeRequested: function(theme) { mainWindow.currentHighlightTheme = theme }
            onSystemMenuRequested: function(globalPosition) {
                windowChrome.showSystemMenu(mainWindow, globalPosition)
            }
        }

        RK.MainWorkspace {
            Layout.fillWidth: true
            Layout.fillHeight: true
            fileName: mainWindow.currentFileName
            filePath: mainWindow.currentFilePath
            highlightTheme: mainWindow.currentHighlightTheme
            onOpenRequested: openFileDialog.open()
            onFileDropped: function(url) { mainWindow.currentFileUrl = url }
        }
    }

    RK.WindowResizeHandle {
        targetWindow: mainWindow
        edges: Qt.TopEdge | Qt.LeftEdge
        cursorShape: Qt.SizeFDiagCursor
        width: 8
        height: 8
        anchors.left: parent.left
        anchors.top: parent.top
    }

    RK.WindowResizeHandle {
        targetWindow: mainWindow
        edges: Qt.TopEdge | Qt.RightEdge
        cursorShape: Qt.SizeBDiagCursor
        width: 8
        height: 8
        anchors.right: parent.right
        anchors.top: parent.top
    }

    RK.WindowResizeHandle {
        targetWindow: mainWindow
        edges: Qt.BottomEdge | Qt.LeftEdge
        cursorShape: Qt.SizeBDiagCursor
        width: 8
        height: 8
        anchors.left: parent.left
        anchors.bottom: parent.bottom
    }

    RK.WindowResizeHandle {
        targetWindow: mainWindow
        edges: Qt.BottomEdge | Qt.RightEdge
        cursorShape: Qt.SizeFDiagCursor
        width: 8
        height: 8
        anchors.right: parent.right
        anchors.bottom: parent.bottom
    }

    RK.WindowResizeHandle {
        targetWindow: mainWindow
        edges: Qt.LeftEdge
        cursorShape: Qt.SizeHorCursor
        width: 5
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.topMargin: 8
        anchors.bottomMargin: 8
    }

    RK.WindowResizeHandle {
        targetWindow: mainWindow
        edges: Qt.RightEdge
        cursorShape: Qt.SizeHorCursor
        width: 5
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.topMargin: 8
        anchors.bottomMargin: 8
    }

    RK.WindowResizeHandle {
        targetWindow: mainWindow
        edges: Qt.TopEdge
        cursorShape: Qt.SizeVerCursor
        height: 5
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.leftMargin: 8
        anchors.rightMargin: 8
    }

    RK.WindowResizeHandle {
        targetWindow: mainWindow
        edges: Qt.BottomEdge
        cursorShape: Qt.SizeVerCursor
        height: 5
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.leftMargin: 8
        anchors.rightMargin: 8
    }

    FileDialog {
        id: openFileDialog
        title: qsTr("Open HarmonyOS package or Ark bytecode")
        nameFilters: [
            qsTr("HarmonyOS packages (*.hap *.app *.abc)"),
            qsTr("All files (*)")
        ]
        onAccepted: mainWindow.currentFileUrl = selectedFile
    }

    Component.onCompleted: {
        if (initialFileUrl && initialFileUrl.length > 0) {
            currentFileUrl = initialFileUrl
        }
    }
}
