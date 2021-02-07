import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Controls.Material 2.12
import net.warsow 2.6

Item {
    id: root

    property var currentValue
    property var selectedValue
    property var model

    Label {
        horizontalAlignment: Qt.AlignHCenter
        font.pointSize: 11
        font.letterSpacing: 1
        font.weight: Font.Medium
        text: "Select an option from the list"
    }

    ListView {
        anchors.fill: parent

        boundsBehavior: Flickable.StopAtBounds
        model: callvotesModel.getOptionsList(selectedVoteArgsHandle)
        spacing: 8

        delegate: MouseArea {
            hoverEnabled: true
            width: root.width
            height: 28

            Label {
                anchors.fill: parent
                horizontalAlignment: Qt.AlignHCenter
                font.weight: Font.Bold
                font.pointSize: 12
                font.capitalization: Font.AllUppercase
                font.letterSpacing: parent.containsMouse ? 2 : 1
                color: parent.containsMouse || modelData["value"] === selectedValue ?
                    Material.accent : Material.foreground
                text: modelData["name"]
            }

            onClicked: selectedValue = modelData["value"]
        }
    }
}