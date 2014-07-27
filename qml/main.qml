import QtQuick 2.2
import QtQuick.Window 2.1
import QtQuick.Controls 1.2
import Calaos 1.0
import "calaos.js" as Calaos;

Window {
    id: rootWindow
    visible: true
    width: 320 * calaosApp.density

    //iphone4
    height: 480 * calaosApp.density
    //iphone5
    //height: 568

    property bool isLandscape: rootWindow.width > rootWindow.height

    property variant roomModel

    Image {
        source: calaosApp.getPictureSized(isLandscape?
                                              "background_landscape":
                                              "background")
        anchors.fill: parent
        fillMode: Image.PreserveAspectCrop
    }

    Connections {
        target: calaosApp
        onApplicationStatusChanged: {
            if (calaosApp.applicationStatus === Common.LoggedIn)
                stackView.push(homeView)
            else if (calaosApp.applicationStatus === Common.NotConnected)
                stackView.pop(loginView)
        }
    }

    StackView {
        id: stackView
        anchors.fill: parent

        initialItem: LoginView {
            onLoginClicked: calaosApp.login(username, password, hostname)
        }

        // Implements back key navigation
        focus: true
        Keys.onReleased: if ((event.key === Qt.Key_Back || event.key === Qt.Key_Backspace) && stackView.depth > 2) {
                             stackView.pop();
                             event.accepted = true;
                         }
    }

    Component {
        id: homeView
        Item {
            Image {
                source: calaosApp.getPictureSized(isLandscape?
                                                      "background_landscape":
                                                      "background")
                anchors.fill: parent
                fillMode: Image.PreserveAspectCrop
            }
            RoomListView {
                id: listViewRoom
                model: homeModel

                onRoomClicked: {
                    //get room model
                    console.debug("model: " + homeModel)
                    roomModel = homeModel.getRoomModel(idx)
                    stackView.push(roomDetailView)
                }
            }
            ScrollBar {
                width: 10; height: listViewRoom.height
                anchors.right: parent.right
                opacity: 1
                orientation: Qt.Vertical
                wantBackground: false
                position: listViewRoom.visibleArea.yPosition
                pageSize: listViewRoom.visibleArea.heightRatio
            }
        }
    }

    Component {
        id: roomDetailView
        Item {
            Image {
                source: calaosApp.getPictureSized(isLandscape?
                                                      "background_landscape":
                                                      "background")
                anchors.fill: parent
                fillMode: Image.PreserveAspectCrop
            }
            ItemListView {
                id: listViewItems
                model: roomModel
            }
            ScrollBar {
                width: 10; height: listViewItems.height
                anchors.right: parent.right
                opacity: 1
                orientation: Qt.Vertical
                wantBackground: false
                position: listViewItems.visibleArea.yPosition
                pageSize: listViewItems.visibleArea.heightRatio
            }
        }
    }

    Loading {
        z: 9999 //on top of everything
        opacity: calaosApp.applicationStatus === Common.Loading?1:0
    }

    MainMenuBar {
        id: menuBar

        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }

        state: calaosApp.applicationStatus === Common.LoggedIn?"visible":"invisible"

        states: [
            State {
                name: "visible"
                PropertyChanges { target: menuBar; opacity: 1 }
                PropertyChanges { target: menuBar; anchors.bottomMargin: 0 }
            },
            State {
                name: "invisible"
                PropertyChanges { target: menuBar; opacity: 0.2 }
                PropertyChanges { target: menuBar; anchors.bottomMargin: -menuBar.height }
            }
        ]

        transitions: [
            Transition {
                from: "invisible"
                to: "visible"
                NumberAnimation { properties: "opacity,anchors.bottomMargin"; easing.type: Easing.OutExpo; duration: 500 }
            },
            Transition {
                from: "visible"
                to: "invisible"
                NumberAnimation { properties: "opacity,anchors.bottomMargin"; easing.type: Easing.InExpo; duration: 500 }
            }
        ]
    }
}
