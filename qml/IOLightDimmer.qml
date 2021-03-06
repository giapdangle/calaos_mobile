import QtQuick 2.0
import QtQuick.Controls 1.2

BorderImage {
    property variant modelData

    source: calaosApp.getPictureSized("back_items_home")
    border.left: 5 * calaosApp.density; border.top: 5 * calaosApp.density
    border.right: 5 * calaosApp.density; border.bottom: 5 * calaosApp.density

    width: parent.width
    height: 80 * calaosApp.density

    AnimatedIcon {
        id: icon

        countImage: 9
        imageFilenameOn: "icon_light_00%1"
        imageFilenameOff: "icon_light_off"

        anchors {
            left: parent.left; leftMargin: 8 * calaosApp.density
            verticalCenter: parent.verticalCenter
            verticalCenterOffset: -18 * calaosApp.density
        }

        iconState: modelData.stateInt > 0
    }

    Text {
        color: icon.iconState?"#ffda5a":"#3ab4d7"
        font { bold: false; pointSize: 12 }
        text: modelData.ioName
        clip: true
        elide: Text.ElideMiddle
        anchors {
            left: icon.right; leftMargin: 8 * calaosApp.density
            right: btoff.left; rightMargin: 8 * calaosApp.density
            verticalCenter: parent.verticalCenter
            verticalCenterOffset: -18 * calaosApp.density
        }

        Behavior on color { ColorAnimation { duration: 200 } }
    }

    ItemButtonAction {
        id: bton
        anchors {
            right: parent.right; rightMargin: 8 * calaosApp.density
            verticalCenter: parent.verticalCenter
            verticalCenterOffset: -18 * calaosApp.density
        }
        imageSource: "button_light_on"

        onButtonClicked: modelData.sendTrue()
    }

    ItemButtonAction {
        id: btoff
        anchors {
            right: bton.left; rightMargin: 8 * calaosApp.density
            verticalCenter: parent.verticalCenter
            verticalCenterOffset: -18 * calaosApp.density
        }
        imageSource: "button_light_off"

        onButtonClicked: modelData.sendFalse()
    }

    Slider {
        id: slider
        anchors {
            left: parent.left; leftMargin: 8 * calaosApp.density
            right: parent.right; rightMargin: 8 * calaosApp.density
            verticalCenter: parent.verticalCenter
            verticalCenterOffset: 18 * calaosApp.density
            //bottom: parent.bottom; bottomMargin: 14 * calaosApp.density
        }
        updateValueWhileDragging: false
        maximumValue: 100
        value: modelData.stateInt
        onValueChanged: {
            console.log("slider value changed")
            if (value !== modelData.stateInt)
                modelData.sendIntValue(Math.round(value))
        }
    }
}
