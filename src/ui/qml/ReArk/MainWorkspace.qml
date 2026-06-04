import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

Rectangle {
    id: root

    property string fileName: ""
    property string filePath: ""
    property string highlightTheme: "github-dark"
    readonly property bool hasPackage: filePath.length > 0
    readonly property bool darkTheme: Material.theme === Material.Dark
    readonly property color pageColor: darkTheme ? "#171a1f" : "#f5f7f8"
    readonly property color sidebarColor: darkTheme ? "#20242b" : "#ffffff"
    readonly property color editorColor: darkTheme ? "#111419" : "#ffffff"
    readonly property color dividerColor: darkTheme ? "#3a404a" : "#d5dcdf"
    readonly property color hoverColor: darkTheme ? "#2b313a" : "#e8eef0"
    readonly property color selectedColor: darkTheme ? "#33424a" : "#d6e8e7"
    readonly property color secondaryTextColor: darkTheme ? "#aab2bd" : "#5f6872"

    signal openRequested()
    signal fileDropped(url fileUrl)

    color: pageColor

    onFilePathChanged: {
        if (filePath.length > 0) {
            decompilerController.decompileFile(filePath)
        } else {
            decompilerController.clear()
        }
    }

    DropArea {
        id: dropArea
        anchors.fill: parent

        onDropped: function(drop) {
            if (drop.hasUrls && drop.urls.length > 0) {
                root.fileDropped(drop.urls[0])
                drop.accept()
            }
        }
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.preferredWidth: 320
            Layout.fillHeight: true
            color: sidebarColor

            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                Label {
                    Layout.fillWidth: true
                    Layout.leftMargin: 12
                    Layout.rightMargin: 12
                    Layout.topMargin: 12
                    Layout.bottomMargin: 8
                    text: hasPackage ? root.fileName : qsTr("Drop a package to start decompiling")
                    color: hasPackage ? Material.foreground : secondaryTextColor
                    font.pixelSize: 12
                    elide: Text.ElideMiddle
                }

                BusyIndicator {
                    Layout.alignment: Qt.AlignHCenter
                    Layout.topMargin: 18
                    running: decompilerController.busy
                    visible: decompilerController.busy
                }

                ListView {
                    id: fileTree
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: hasPackage ? decompilerController.treeModel : 0
                    currentIndex: decompilerController.selectedIndex

                    delegate: ItemDelegate {
                        width: fileTree.width
                        height: 28
                        leftPadding: 8 + model.depth * 16
                        rightPadding: 10
                        text: model.name
                        highlighted: index === decompilerController.selectedIndex
                        hoverEnabled: true
                        background: Rectangle {
                            color: index === decompilerController.selectedIndex
                                   ? selectedColor
                                   : parent.hovered ? hoverColor : "transparent"
                        }
                        contentItem: RowLayout {
                            spacing: 5

                            Label {
                                Layout.preferredWidth: 10
                                text: model.isDirectory ? (model.expanded ? "▾" : "▸") : ""
                                color: model.isPlaceholder ? secondaryTextColor : Material.foreground
                                opacity: model.isPlaceholder ? 0.75 : 1.0
                                font.pixelSize: 10
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }

                            FileTreeIcon {
                                Layout.preferredWidth: 16
                                Layout.preferredHeight: 16
                                name: model.name
                                kind: model.kind
                                directory: model.isDirectory
                                placeholder: model.isPlaceholder
                            }

                            Label {
                                Layout.fillWidth: true
                                text: model.name
                                color: model.isPlaceholder ? secondaryTextColor : Material.foreground
                                opacity: model.isPlaceholder ? 0.75 : 1.0
                                font.pixelSize: 12
                                font.italic: model.isPlaceholder
                                elide: Text.ElideMiddle
                                verticalAlignment: Text.AlignVCenter
                            }
                        }
                        onClicked: decompilerController.activateIndex(index)
                    }
                }
            }
        }

        Rectangle {
            Layout.preferredWidth: 1
            Layout.fillHeight: true
            color: dividerColor
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: decompilerController.tabsModel.hasTabs ? 36 : 0
                visible: decompilerController.tabsModel.hasTabs
                color: darkTheme ? "#171a1f" : "#eef2f4"
                clip: true

                ListView {
                    id: openTabs
                    anchors.fill: parent
                    orientation: ListView.Horizontal
                    boundsBehavior: Flickable.StopAtBounds
                    clip: true
                    model: decompilerController.tabsModel
                    currentIndex: decompilerController.tabsModel.activeIndex

                    delegate: Rectangle {
                        width: Math.min(220, Math.max(136, tabTitle.implicitWidth + 52))
                        height: openTabs.height
                        color: model.active ? editorColor : (darkTheme ? "#20242b" : "#e4eaed")

                        Rectangle {
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.top: parent.top
                            height: 2
                            color: model.active ? "#4aa3ff" : "transparent"
                        }

                        Rectangle {
                            anchors.right: parent.right
                            width: 1
                            height: parent.height
                            color: dividerColor
                        }

                        Label {
                            id: tabTitle
                            anchors.left: parent.left
                            anchors.right: closeButton.left
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.leftMargin: 12
                            anchors.rightMargin: 4
                            text: model.loading ? model.name + " ..." : model.name
                            color: model.active ? Material.foreground : secondaryTextColor
                            font.pixelSize: 12
                            elide: Text.ElideRight
                            verticalAlignment: Text.AlignVCenter
                        }

                        Rectangle {
                            id: closeButton
                            anchors.right: parent.right
                            anchors.verticalCenter: parent.verticalCenter
                            width: 28
                            height: 28
                            radius: 4
                            color: closeMouse.containsMouse
                                   ? (darkTheme ? "#3a4048" : "#ccd5da")
                                   : "transparent"
                            ToolTip.text: qsTr("Close")
                            ToolTip.visible: closeMouse.containsMouse

                            Label {
                                anchors.centerIn: parent
                                text: "×"
                                color: closeMouse.containsMouse ? Material.foreground : secondaryTextColor
                                font.pixelSize: 16
                                font.weight: Font.Normal
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }

                            MouseArea {
                                id: closeMouse
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: decompilerController.tabsModel.closeTab(index)
                            }
                        }

                        MouseArea {
                            anchors.fill: parent
                            anchors.rightMargin: closeButton.width
                            acceptedButtons: Qt.LeftButton
                            onClicked: decompilerController.tabsModel.activeIndex = index
                        }
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: editorColor

                CodeView {
                    anchors.fill: parent
                    visible: decompilerController.tabsModel.hasTabs
                             && decompilerController.tabsModel.activeContentMode === "text"
                    code: decompilerController.selectedContent
                    highlightTheme: root.highlightTheme
                }

                HexView {
                    anchors.fill: parent
                    visible: decompilerController.tabsModel.hasTabs
                             && decompilerController.tabsModel.activeContentMode === "hex"
                    hexData: decompilerController.selectedContent
                }

                ImageView {
                    anchors.fill: parent
                    visible: decompilerController.tabsModel.hasTabs
                             && decompilerController.tabsModel.activeContentMode === "image"
                    sourceData: decompilerController.selectedContent
                }

                Label {
                    anchors.centerIn: parent
                    visible: !decompilerController.tabsModel.hasTabs && !decompilerController.busy
                    text: !hasPackage
                          ? (dropArea.containsDrag
                             ? qsTr("Release to open package")
                             : qsTr("Open or drop a .hap, .app, or .abc file"))
                          : qsTr("Select a file from the tree")
                    color: secondaryTextColor
                    font.pixelSize: 15
                }
            }
        }
    }
}
