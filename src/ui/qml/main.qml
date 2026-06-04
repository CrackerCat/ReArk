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
    title: currentFileName.length > 0 ? "ReArk - " + currentFileName : "ReArk"

    property string currentTheme: "dark"
    property string currentHighlightTheme: "github-dark"
    property url currentFileUrl: ""
    readonly property string currentFilePath: decodeURIComponent(currentFileUrl.toString().replace(/^file:\/+/, ""))
    readonly property string currentFileName: currentFilePath.length > 0 ? currentFilePath.split(/[\\/]/).pop() : ""

    Material.theme: currentTheme === "system"
                    ? Material.System
                    : (currentTheme === "light" ? Material.Light : Material.Dark)
    Material.accent: Material.Teal

    menuBar: RK.AppMenuBar {
        currentTheme: mainWindow.currentTheme
        currentHighlightTheme: mainWindow.currentHighlightTheme
        onOpenRequested: openFileDialog.open()
        onThemeRequested: function(theme) { mainWindow.currentTheme = theme }
        onHighlightThemeRequested: function(theme) { mainWindow.currentHighlightTheme = theme }
    }

    footer: RK.StatusBar {
        filePath: decompilerController.status
        version: appVersion
    }

    RK.MainWorkspace {
        anchors.fill: parent
        fileName: mainWindow.currentFileName
        filePath: mainWindow.currentFilePath
        highlightTheme: mainWindow.currentHighlightTheme
        onOpenRequested: openFileDialog.open()
        onFileDropped: function(url) { mainWindow.currentFileUrl = url }
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
