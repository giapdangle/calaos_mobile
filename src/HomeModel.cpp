#include "HomeModel.h"
#include <QDebug>
#include "RoomModel.h"

HomeModel::HomeModel(QQmlApplicationEngine *eng, CalaosConnection *con, ScenarioModel *scModel, LightOnModel *lModel, QObject *parent) :
    QStandardItemModel(parent),
    engine(eng),
    connection(con),
    scenarioModel(scModel),
    lightOnModel(lModel)
{
    QHash<int, QByteArray> roles;
    roles[RoleType] = "roomType";
    roles[RoleHits] = "roomHits";
    roles[RoleName] = "roomName";
    roles[RoleLightsCount] = "lights_on_count";
    setItemRoleNames(roles);

    update_lights_on_count(0);
}

void HomeModel::load(QVariantMap &homeData)
{
    clear();
    scenarioModel->clear();
    IOCache::Instance().clearCache();

    if (!homeData.contains("home"))
    {
        qDebug() << "no home entry";
        return;
    }

    QVariantList rooms = homeData["home"].toList();
    QVariantList::iterator it = rooms.begin();
    for (;it != rooms.end();it++)
    {
        QVariantMap r = it->toMap();
        RoomItem *room = new RoomItem(engine, connection);
        connect(room, SIGNAL(sig_light_on(IOBase*)), this, SLOT(newlight_on(IOBase*)));
        connect(room, SIGNAL(sig_light_off(IOBase*)), this, SLOT(newlight_off(IOBase*)));

        room->update_roomName(r["name"].toString());
        room->update_roomType(r["type"].toString());
        room->update_roomHits(r["hits"].toString().toInt());
        room->load(r, scenarioModel, RoomModel::LoadNormal);
        appendRow(room);
    }
}

QObject *HomeModel::getRoomModel(int idx) const
{
    RoomItem *it = dynamic_cast<RoomItem *>(item(idx));
    if (!it) return nullptr;
    return it->getRoomModel();
}

void HomeModel::newlight_on(IOBase *io)
{
    update_lights_on_count(get_lights_on_count() + 1);

    lightOnModel->addLight(io);
}

void HomeModel::newlight_off(IOBase *io)
{
    int l = get_lights_on_count() - 1;
    if (l < 0) l = 0;
    update_lights_on_count(l);

    lightOnModel->removeLight(io);
}

RoomItem::RoomItem(QQmlApplicationEngine *eng, CalaosConnection *con):
    QStandardItem(),
    engine(eng),
    connection(con)
{
    update_lights_on_count(0);
    room = new RoomModel(engine, connection, this);
    connect(room, SIGNAL(sig_light_on(IOBase*)), this, SLOT(newlight_on(IOBase*)));
    connect(room, SIGNAL(sig_light_off(IOBase*)), this, SLOT(newlight_off(IOBase*)));
    engine->setObjectOwnership(room, QQmlEngine::CppOwnership);
}

QObject *RoomItem::getRoomModel() const
{
    return room;
}

void RoomItem::load(QVariantMap &roomData, ScenarioModel *scenarioModel, int load_flag)
{
    room->load(roomData, scenarioModel, load_flag);
}

void RoomItem::newlight_on(IOBase *io)
{
    update_lights_on_count(get_lights_on_count() + 1);
    emit sig_light_on(io);
}

void RoomItem::newlight_off(IOBase *io)
{
    int l = get_lights_on_count() - 1;
    if (l < 0) l = 0;
    update_lights_on_count(l);
    emit sig_light_off(io);
}

LightOnModel::LightOnModel(QQmlApplicationEngine *eng, CalaosConnection *con, QObject *parent):
    QStandardItemModel(parent),
    engine(eng),
    connection(con)
{
    QHash<int, QByteArray> roles;
    roles[RoleType] = "ioType";
    roles[RoleHits] = "ioHits";
    roles[RoleName] = "ioName";
    roles[RoleId] = "ioId";
    roles[RoleRoomName] = "roomName";
    setItemRoleNames(roles);
}

QObject *LightOnModel::getItemModel(int idx)
{
    IOBase *obj = dynamic_cast<IOBase *>(item(idx));
    if (obj) engine->setObjectOwnership(obj, QQmlEngine::CppOwnership);
    return obj;
}

void LightOnModel::addLight(IOBase *io)
{
    appendRow(io->cloneIO());
}

void LightOnModel::removeLight(IOBase *io)
{
    for (int i = 0;i < rowCount();i++)
    {
        IOBase *cur = dynamic_cast<IOBase *>(item(i));
        if (cur->get_ioId() == io->get_ioId())
        {
            removeRow(i);
            break;
        }
    }
}
