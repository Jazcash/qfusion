import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Controls.Material 2.12
import QtQuick.Layouts 1.12
import net.warsow 2.6

Item {
    id: root

    StackView {
        id: stackView
        width: root.width - 16
        anchors.top: parent.top
        anchors.topMargin: 8
        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        initialItem: voteSelectionComponent

        onCurrentItemChanged: {
            if (currentItem.stage === 1) {
                chosenValue = undefined
            }
        }
    }

    property int selectedVoteIndex
    property string selectedVoteName
    property string selectedVoteDesc
    property int selectedVoteArgsHandle
    property int selectedVoteArgsKind
    property string selectedVoteCurrent

    property var chosenValue: undefined

    Connections {
        target: callvotesModel
        onCurrentChanged: {
            if (index === selectedVoteIndex) {
                selectedVoteCurrent = value
            }
        }
    }

    Component {
        id: voteSelectionComponent
        ListView {
            id: list

            model: callvotesModel
            spacing: 12

            readonly property int stage: 1

            delegate: MouseArea {
                id: mouseArea
                hoverEnabled: true
                width: Math.min(list.width, Math.max(150, label.width))
                height: 24

                onClicked: {
                    selectedVoteIndex = index
                    selectedVoteName = name
                    selectedVoteDesc = desc
                    selectedVoteArgsHandle = argsHandle
                    selectedVoteArgsKind = argsKind
                    selectedVoteCurrent = current
                    stackView.replace(argsSelectionComponent)
                }

                Label {
                    id: label
                    width: list.width
                    horizontalAlignment: Qt.AlignHCenter
                    text: name
                    font.pointSize: 12
                    font.weight: Font.Bold
                    font.letterSpacing: mouseArea.containsMouse ? 2 : 1
                    font.capitalization: Font.AllUppercase
                    color: mouseArea.containsMouse ? Material.accent : Material.foreground
                }
            }
        }
    }

    Component {
        id: argsSelectionComponent

        ColumnLayout {
            spacing: 20

            readonly property int stage: 2

            Component.onCompleted: {
                if (selectedVoteArgsKind === CallvotesModel.Boolean) {
                    argsSelectionStack.push(booleanSelectorComponent)
                } else if (selectedVoteArgsKind === CallvotesModel.Number) {
                    argsSelectionStack.push(numberSelectorComponent)
                } else if (selectedVoteArgsKind === CallvotesModel.Player) {
                    argsSelectionStack.push(playerSelectorComponent)
                } else if (selectedVoteArgsKind === CallvotesModel.Minutes) {
                    argsSelectionStack.push(minutesSelectorComponent)
                } else if (selectedVoteArgsKind === CallvotesModel.Options) {
                    argsSelectionStack.push(optionsSelectorComponent)
                } else {
                    argsSelectionStack.push(noArgsSelectorComponent)
                }
            }

            Component {
                id: booleanSelectorComponent
                Label { text: "Boolean" }
            }

            Component {
                id: numberSelectorComponent
                Label { text: "Number" }
            }

            Component {
                id: minutesSelectorComponent
                Label { text: "Minutes" }
            }

            Component {
                id: playerSelectorComponent
                InGameCallvotePlayerSelector {}
            }

            Component {
                id: optionsSelectorComponent
                InGameCallvoteOptionSelector {}
            }

            Component {
                id: noArgsSelectorComponent
                Label {
                    horizontalAlignment: Qt.AlignHCenter
                    verticalAlignment: Qt.AlignVCenter
                    font.pointSize: 11
                    font.letterSpacing: 1
                    text: "This vote has no arguments"
                }
            }

            Label {
                Layout.preferredWidth: root.width
                horizontalAlignment: Qt.AlignHCenter
                font.pointSize: 16
                font.capitalization: Font.AllUppercase
                font.weight: Font.Medium
                text: selectedVoteName
            }
            Label {
                Layout.preferredWidth: root.width
                horizontalAlignment: Qt.AlignHCenter
                wrapMode: Text.WordWrap
                font.pointSize: 11
                font.letterSpacing: 1
                text: selectedVoteDesc
            }

            // TODO: This could be just a loader !!!!!!!!!!!!!!!!!!!!!
            StackView {
                id: argsSelectionStack
                Layout.topMargin: 16
                Layout.bottomMargin: 16
                Layout.preferredWidth: root.width
                Layout.fillHeight: true
            }

            RowLayout {
                Layout.preferredWidth: root.width
                Button {
                    flat: true
                    text: "back"
                    Layout.preferredWidth: 64
                    Material.theme: Material.Dark
                    onClicked: stackView.replace(voteSelectionComponent)
                }
                Item {
                    Layout.fillWidth: true
                }
                Button {
                    flat: true
                    text: "opcall"
                    Layout.preferredWidth: 72
                    Material.theme: Material.Dark
                }
                Button {
                    flat: true
                    highlighted: true
                    text: "vote"
                    Layout.preferredWidth: 72
                }
            }
        }
    }

    function handleKeyEvent(event) {
        if (event.key === Qt.Key_Escape || event.key === Qt.Key_Back) {
            if (stackView.currentItem.stage === 2) {
                stackView.replace(voteSelectionComponent)
                event.accepted = true
                return true
            }
        }
        return false
    }
}