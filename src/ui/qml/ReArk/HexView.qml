import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

Rectangle {
    id: root

    property string hexData: ""
    readonly property bool darkTheme: Material.theme === Material.Dark
    readonly property color backgroundColor: "#0f1318"
    readonly property color alternateRowColor: "#111820"
    readonly property color headerColor: "#161c23"
    readonly property color dividerColor: "#2f3741"
    readonly property color addressColor: "#7ab7ff"
    readonly property color byteColor: "#d7e0ea"
    readonly property color mutedByteColor: "#66707b"
    readonly property color asciiColor: "#c8d1dc"
    readonly property color cellHoverColor: "#25313b"
    readonly property var document: parseDocument(hexData)
    readonly property var rows: document.rows || []
    readonly property int rowHeight: 22
    readonly property int addressWidth: 92
    readonly property int byteWidth: 34
    readonly property int asciiWidth: 150
    readonly property int tableWidth: addressWidth + byteWidth * 16 + asciiWidth + 44

    color: backgroundColor

    function parseDocument(value) {
        if (value.length === 0) {
            return { path: "", kind: "", size: "", rows: [] }
        }
        try {
            var parsed = JSON.parse(value)
            if (!parsed.rows) {
                parsed.rows = []
            }
            return parsed
        } catch (error) {
            return {
                path: "",
                kind: "",
                size: "",
                rows: [],
                error: String(error)
            }
        }
    }

    function byteAt(row, index) {
        return row.bytes && index < row.bytes.length ? row.bytes[index] : ""
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 44
            color: headerColor

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 14
                anchors.rightMargin: 14
                spacing: 16

                Label {
                    Layout.fillWidth: true
                    text: document.path || qsTr("Binary resource")
                    color: Material.foreground
                    font.pixelSize: 13
                    font.bold: true
                    elide: Text.ElideMiddle
                }

                Label {
                    text: document.kind ? document.kind : ""
                    color: addressColor
                    font.family: "Consolas"
                    font.pixelSize: 12
                }

                Label {
                    text: document.size ? qsTr("%1 bytes").arg(document.size) : ""
                    color: asciiColor
                    font.family: "Consolas"
                    font.pixelSize: 12
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: dividerColor
        }

        Flickable {
            id: horizontalFlick
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            contentWidth: Math.max(root.tableWidth, width)
            contentHeight: height
            flickableDirection: Flickable.HorizontalFlick

            ColumnLayout {
                width: Math.max(root.tableWidth, horizontalFlick.width)
                height: horizontalFlick.height
                spacing: 0

                Rectangle {
                    id: headerRow
                    Layout.fillWidth: true
                    Layout.preferredHeight: 30
                    color: root.backgroundColor

                    Row {
                        anchors.fill: parent
                        anchors.leftMargin: 14
                        anchors.rightMargin: 14
                        spacing: 0

                        Label {
                            width: root.addressWidth
                            height: parent.height
                            text: qsTr("Address")
                            color: root.asciiColor
                            font.family: "Consolas"
                            font.pixelSize: 12
                            verticalAlignment: Text.AlignVCenter
                        }

                        Repeater {
                            model: 16

                            Label {
                                width: root.byteWidth
                                height: headerRow.height
                                text: index.toString(16).toUpperCase().padStart(2, "0")
                                color: root.asciiColor
                                font.family: "Consolas"
                                font.pixelSize: 12
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                        }

                        Label {
                            width: root.asciiWidth
                            height: parent.height
                            text: qsTr("ASCII")
                            color: root.asciiColor
                            font.family: "Consolas"
                            font.pixelSize: 12
                            verticalAlignment: Text.AlignVCenter
                        }
                    }
                }

                ListView {
                    id: rowsList
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    boundsBehavior: Flickable.StopAtBounds
                    model: root.rows
                    reuseItems: true
                    cacheBuffer: 800

                    delegate: Rectangle {
                        required property int index
                        required property var modelData

                        width: rowsList.width
                        height: root.rowHeight
                        color: index % 2 === 0
                               ? root.backgroundColor
                               : root.alternateRowColor

                        Row {
                            anchors.fill: parent
                            anchors.leftMargin: 14
                            anchors.rightMargin: 14
                            spacing: 0

                            Label {
                                width: root.addressWidth
                                height: parent.height
                                text: modelData.address
                                color: root.addressColor
                                font.family: "Consolas"
                                font.pixelSize: 12
                                verticalAlignment: Text.AlignVCenter
                            }

                            Repeater {
                                model: 16

                                Rectangle {
                                    required property int index

                                    width: root.byteWidth
                                    height: root.rowHeight
                                    color: byteMouse.containsMouse ? root.cellHoverColor : "transparent"

                                    Label {
                                        anchors.centerIn: parent
                                        text: root.byteAt(modelData, index)
                                        color: text === "00" ? root.mutedByteColor : root.byteColor
                                        font.family: "Consolas"
                                        font.pixelSize: 12
                                    }

                                    MouseArea {
                                        id: byteMouse
                                        anchors.fill: parent
                                        hoverEnabled: true
                                        acceptedButtons: Qt.NoButton
                                    }
                                }
                            }

                            Label {
                                width: root.asciiWidth
                                height: parent.height
                                text: modelData.ascii || ""
                                color: root.asciiColor
                                font.family: "Consolas"
                                font.pixelSize: 12
                                verticalAlignment: Text.AlignVCenter
                            }
                        }
                    }

                    ScrollBar.vertical: ScrollBar {
                        policy: ScrollBar.AsNeeded
                    }
                }
            }

            ScrollBar.horizontal: ScrollBar {
                policy: ScrollBar.AsNeeded
            }
        }
    }
}
