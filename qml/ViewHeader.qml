import QtQuick 2.0

Rectangle {

    property alias headerLabel: txt.text
    property alias iconSource: ic.source

    anchors { left: parent.left; right: parent.right; top: parent.top }
    color: "black"
    height: 45 * calaosApp.density

    Image {
        id: ic

        anchors {
            verticalCenter: parent.verticalCenter
            left: parent.left; leftMargin: 10 * calaosApp.density
        }
    }

    Text {
        id: txt
        color: "#e7e7e7"
        font { bold: false; pointSize: 12 }
        anchors {
            left: iconSource === ""?parent.left:ic.right
            leftMargin: 5 * calaosApp.density
            verticalCenter: parent.verticalCenter
        }
    }

    Rectangle {
        anchors { left: parent.left; right: parent.right; bottom: parent.bottom }
        color: "#333333"
        height: 2 * calaosApp.density
    }
}
