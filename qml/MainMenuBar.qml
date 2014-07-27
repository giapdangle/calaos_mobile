import QtQuick 2.0
import Calaos 1.0

Image {

    height: menuType === Common.MenuMain?
                98 / 2 * calaosApp.density:
                48 * calaosApp.density

    fillMode: Image.TileHorizontally
    verticalAlignment: Image.AlignLeft

    source: "qrc:/img/menu_footer_background.png"

    property int menuType: Common.MenuMain

    signal buttonHomeClicked()
    signal buttonMediaClicked()
    signal buttonScenariosClicked()
    signal buttonConfigClicked()
    signal buttonBackClicked()

    function unselectAll(bt) {
        if (bt !== btHome) btHome.selected = false
        if (bt !== btMedia) btMedia.selected = false
        if (bt !== btScenario) btScenario.selected = false
        if (bt !== btConfig) btConfig.selected = false
    }

    ButtonFooter {

        opacity: menuType === Common.MenuBack?1:0
        Behavior on opacity { NumberAnimation {} }
        visible: opacity > 0

        icon: calaosApp.getPictureSized("icon_exit")
        buttonLabel: qsTr("Back")
        onButtonClicked: {
            unselectAll()
            buttonBackClicked()
        }
        anchors {
            verticalCenter: parent.verticalCenter
            left: parent.left
            leftMargin: 10 * calaosApp.density
        }
    }

    Row {

        opacity: menuType === Common.MenuMain?1:0
        Behavior on opacity { NumberAnimation {} }
        visible: opacity > 0

        height: btHome.height
        width: btHome.width * 4
        anchors {
            centerIn: parent
            verticalCenterOffset: 1
        }

        MainMenuButton {
            id: btHome

            onButtonClicked: {
                unselectAll(btHome)
                buttonHomeClicked()
            }

            iconBase: "qrc:/img/button_home.png"
            iconGlow: "qrc:/img/button_home_glow.png"
            iconBloom: "qrc:/img/button_home_bloom.png"
        }

        MainMenuButton {
            id: btMedia

            onButtonClicked: {
                unselectAll(btMedia)
                buttonMediaClicked()
            }

            iconBase: "qrc:/img/button_media.png"
            iconGlow: "qrc:/img/button_media_glow.png"
            iconBloom: "qrc:/img/button_media_bloom.png"
        }

        MainMenuButton {
            id: btScenario

            onButtonClicked: {
                unselectAll(btScenario)
                buttonScenariosClicked()
            }

            iconBase: "qrc:/img/button_scenarios.png"
            iconGlow: "qrc:/img/button_scenarios_glow.png"
            iconBloom: "qrc:/img/button_scenarios_bloom.png"
        }

        MainMenuButton {
            id: btConfig

            onButtonClicked: {
                unselectAll(btConfig)
                buttonConfigClicked()
            }

            iconBase: "qrc:/img/button_configuration.png"
            iconGlow: "qrc:/img/button_configuration_glow.png"
            iconBloom: "qrc:/img/button_configuration_bloom.png"
        }
    }

}
