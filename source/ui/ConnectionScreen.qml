import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Controls.Material 2.12
import QtGraphicalEffects 1.12
import net.warsow 2.6

Item {
    id: root
    anchors.fill: parent
    visible: wsw.isShowingConnectionScreen

    RadialGradient {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0.0; color: Qt.darker(Material.background, 1.1) }
            GradientStop { position: 0.9; color: Qt.darker(Material.background, 1.9) }
        }
    }



    Image {
        id: logo
        visible: !wsw.connectionDropReason
        width: Math.min(implicitWidth, parent.width - 32)
        fillMode: Image.PreserveAspectFit
        anchors.centerIn: parent
        source: "logo.webp"
    }

    // TODO: Should we keep the logo within the stack view?
    StackView {
        id: stackView
        anchors.fill: parent
    }

    Connections {
        target: wsw
        onConnectionDropReasonChanged: {
            let reason = wsw.connectionDropReason
            stackView.pop(null)
            if (reason == Wsw.GenericDropReason) {
                stackView.push(genericDropReasonComponent)
            } else if (reason == Wsw.PasswordRequired) {
                stackView.push(passwordRequiredComponent)
            }
        }
    }

    Component {
        id: genericDropReasonComponent

        Rectangle {
            anchors.fill: parent
            color: "red"
        }
    }
    
    Component {
        id: passwordRequiredComponent

        Rectangle {
            anchors.fill: parent
            color: "orange"
        }
    }
}