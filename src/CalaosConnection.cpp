#include "CalaosConnection.h"
#include <QJsonDocument>
#include <QDebug>
#include "HardwareUtils.h"

CalaosConnection::CalaosConnection(QObject *parent) :
    QObject(parent)
{
    accessManager = new QNetworkAccessManager(this);
    accessManagerCam = new QNetworkAccessManager(this);
    pollReply = nullptr;
    connect(accessManager, SIGNAL(sslErrors(QNetworkReply*, const QList<QSslError> &)),
            this, SLOT(sslErrors(QNetworkReply*, const QList<QSslError> &)));
    connect(accessManagerCam, SIGNAL(sslErrors(QNetworkReply*, const QList<QSslError> &)),
            this, SLOT(sslErrors(QNetworkReply*, const QList<QSslError> &)));
}

void CalaosConnection::sslErrors(QNetworkReply *reply, const QList<QSslError> &)
{
    reply->ignoreSslErrors();
}

void CalaosConnection::login(QString user, QString pass, QString h)
{
    HardwareUtils::Instance()->showNetworkActivity(true);

    username = user;
    password = pass;
    uuidPolling.clear();

    QJsonObject jroot;
    jroot["cn_user"] = username;
    jroot["cn_pass"] = password;
    jroot["action"] = QStringLiteral("get_home");
    QJsonDocument jdoc(jroot);

    connect(accessManager, SIGNAL(finished(QNetworkReply*)),
            this, SLOT(loginFinished(QNetworkReply*)));

    if (h.startsWith("http://") || h.startsWith("https://"))
        host = h;
    else
        host = QString("https://%1/api.php").arg(h);

    QUrl url(host);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    accessManager->post(request, jdoc.toJson());
}

void CalaosConnection::logout()
{
    HardwareUtils::Instance()->showNetworkActivity(false);

    if (pollReply)
    {
        pollReply->abort();
        pollReply->deleteLater();
        pollReply = nullptr;
    }

    foreach (QNetworkReply *reply, reqReplies)
    {
        reply->abort();
        reply->deleteLater();
    }
    reqReplies.clear();

    uuidPolling.clear();
    emit disconnected();
}

void CalaosConnection::loginFinished(QNetworkReply *reply)
{
    HardwareUtils::Instance()->showNetworkActivity(false);

    disconnect(accessManager, SIGNAL(finished(QNetworkReply*)),
               this, SLOT(loginFinished(QNetworkReply*)));

    reply->deleteLater();
    if (reply->error() != QNetworkReply::NoError)
    {
        qDebug() << "Error in " << reply->url() << ":" << reply->errorString();
        emit loginFailed();
        return;
    }

    QByteArray bytes = reply->readAll();
    QJsonParseError err;
    QJsonDocument jdoc = QJsonDocument::fromJson(bytes, &err);

    qDebug() << "RECV: " << jdoc.toJson();

    if (err.error != QJsonParseError::NoError)
    {
        qDebug() << "JSON parse error " << err.errorString();
        emit loginFailed();
        return;
    }
    QVariantMap jroot = jdoc.object().toVariantMap();

    //start polling
    startJsonPolling();

    emit homeLoaded(jroot);
}

void CalaosConnection::requestFinished()
{
    HardwareUtils::Instance()->showNetworkActivity(false);

    QNetworkReply *reqReply = qobject_cast<QNetworkReply*>(QObject::sender());

    if (!reqReply)
    {
        qWarning() << "Error reqReply is NULL!";
        return;
    }

    reqReply->deleteLater();

    if (reqReply->error() != QNetworkReply::NoError)
    {
        qDebug() << "Error in " << reqReply->url() << ":" << reqReply->errorString();
        return;
    }

    QByteArray bytes = reqReply->readAll();
    QJsonParseError err;
    QJsonDocument jdoc = QJsonDocument::fromJson(bytes, &err);

    if (err.error != QJsonParseError::NoError)
    {
        qDebug() << bytes;
        qDebug() << "JSON parse error at " << err.offset << " : " << err.errorString();
        return;
    }

    qDebug() << "RECV: " << jdoc.toJson();

    reqReplies.removeAll(reqReply);

    QVariantMap jroot = jdoc.object().toVariantMap();

    if (jroot.contains("audio_players") &&
        !jroot["audio_players"].toList().isEmpty())
    {
        //emit event for audio player change
        emit eventAudioStateChange(jroot);
    }

    if (jroot.contains("inputs") &&
        !jroot["inputs"].toList().isEmpty())
    {
        //emit event for specific input change
        emit eventInputStateChange(jroot);
    }

    if (jroot.contains("outputs") &&
        !jroot["outputs"].toList().isEmpty())
    {
        //emit event for output change
        emit eventOutputStateChange(jroot);
    }
}

void CalaosConnection::requestCamFinished(QNetworkReply *reqReply, const QString &camid)
{
    if (!reqReply)
    {
        qWarning() << "Error reqReply is NULL!";
        return;
    }

    reqReply->deleteLater();

    if (reqReply->error() != QNetworkReply::NoError)
    {
        qDebug() << "Error in " << reqReply->url() << ":" << reqReply->errorString();
        emit cameraPictureFailed(camid);
        return;
    }

    QByteArray bytes = reqReply->readAll();
    QJsonParseError err;
    QJsonDocument jdoc = QJsonDocument::fromJson(bytes, &err);

    if (err.error != QJsonParseError::NoError)
    {
        qDebug() << bytes;
        qDebug() << "JSON parse error at " << err.offset << " : " << err.errorString();
        emit cameraPictureFailed(camid);
        return;
    }

    //qDebug() << "RECV: " << jdoc.toJson();

    reqReplies.removeAll(reqReply);

    QVariantMap jroot = jdoc.object().toVariantMap();

    if (jroot.contains("error"))
    {
        qWarning() << "Error getting camera picture";
        emit cameraPictureFailed(camid);
        return;
    }

    if (jroot.contains("contenttype") &&
        jroot.contains("data") &&
        jroot.contains("encoding"))
    {
        qDebug() << "New camera picture for cam: " << camid;

        //we have a new picture
        emit cameraPictureDownloaded(
                    camid,
                    jroot["data"].toString(),
                    jroot["encoding"].toString(),
                    jroot["contenttype"].toString());
        return;
    }

    emit cameraPictureFailed(camid);
}

void CalaosConnection::requestError(QNetworkReply::NetworkError code)
{
    Q_UNUSED(code)
    qDebug() << "Request error!";
    logout();
    return;
}

void CalaosConnection::sendCommand(QString id, QString value, QString type, QString action)
{
    HardwareUtils::Instance()->showNetworkActivity(true);

    QJsonObject jroot;
    jroot["cn_user"] = username;
    jroot["cn_pass"] = password;
    jroot["action"] = action;
    jroot["type"] = type;
    if (type == "audio")
        jroot["player_id"] = id;
    else
        jroot["id"] = id;
    jroot["value"] = value;
    QJsonDocument jdoc(jroot);

    qDebug() << "SEND: " << jdoc.toJson();

    QUrl url(host);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QNetworkReply *reqReply = accessManager->post(request, jdoc.toJson());

    connect(reqReply, SIGNAL(finished()), this, SLOT(requestFinished()));
    connect(reqReply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(requestError(QNetworkReply::NetworkError)));

    reqReplies.append(reqReply);
}

void CalaosConnection::queryState(QStringList inputs, QStringList outputs, QStringList audio_players)
{
    HardwareUtils::Instance()->showNetworkActivity(true);

    QJsonObject jroot;
    jroot["cn_user"] = username;
    jroot["cn_pass"] = password;
    jroot["action"] = QString("get_state");
    jroot["inputs"] = QJsonValue::fromVariant(inputs);
    jroot["outputs"] = QJsonValue::fromVariant(outputs);
    jroot["audio_players"] = QJsonValue::fromVariant(audio_players);
    QJsonDocument jdoc(jroot);

    qDebug() << "SEND: " << jdoc.toJson();

    QUrl url(host);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QNetworkReply *reqReply = accessManager->post(request, jdoc.toJson());

    connect(reqReply, SIGNAL(finished()), this, SLOT(requestFinished()));
    connect(reqReply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(requestError(QNetworkReply::NetworkError)));

    reqReplies.append(reqReply);
}

void CalaosConnection::getCameraPicture(const QString &camid)
{
    QJsonObject jroot;
    jroot["cn_user"] = username;
    jroot["cn_pass"] = password;
    jroot["action"] = QString("get_camera_pic");
    jroot["camera_id"] = camid;
    QJsonDocument jdoc(jroot);

    qDebug() << "SEND: " << jdoc.toJson();

    QUrl url(host);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QNetworkReply *reqReply = accessManagerCam->post(request, jdoc.toJson());

    connect(reqReply, &QNetworkReply::finished, [=]() { requestCamFinished(reqReply, camid); });
    connect(reqReply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(requestError(QNetworkReply::NetworkError)));

    reqReplies.append(reqReply);
}

void CalaosConnection::startJsonPolling()
{
    if (uuidPolling.isEmpty())
        qDebug() << "Start polling...";

    QJsonObject jroot;
    jroot["cn_user"] = username;
    jroot["cn_pass"] = password;
    jroot["action"] = QString("poll_listen");
    if (uuidPolling.isEmpty())
        jroot["type"] = QString("register");
    else
    {
        jroot["type"] = QString("get");
        jroot["uuid"] = uuidPolling;
    }
    QJsonDocument jdoc(jroot);

    QUrl url(host);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    pollReply = accessManager->post(request, jdoc.toJson());

    connect(pollReply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(requestError(QNetworkReply::NetworkError)));
    connect(pollReply, &QNetworkReply::finished, [=]()
    {
        pollReply->deleteLater();
        if (pollReply->error() != QNetworkReply::NoError)
        {
            qDebug() << "Error in " << pollReply->url() << ":" << pollReply->errorString();
            logout();
            return;
        }

        QByteArray bytes = pollReply->readAll();
        pollReply = nullptr;

        QJsonParseError err;
        QJsonDocument jdoc = QJsonDocument::fromJson(bytes, &err);

        if (err.error != QJsonParseError::NoError)
        {
            qDebug() << "JSON parse error " << err.errorString();
            emit disconnected();
            return;
        }
        QVariantMap jroot = jdoc.object().toVariantMap();

        if (jroot.contains("uuid") && uuidPolling.isEmpty())
        {
            uuidPolling = jroot["uuid"].toString();
            startJsonPolling();
            return;
        }

        if (jroot["success"].toString() != "true")
        {
            qDebug() << "Failed to get events";
            emit disconnected();
            return;
        }

        QVariantList events = jroot["events"].toList();
        foreach (QVariant v, events)
            processEvents(v.toString());

        QTimer::singleShot(200, this, SLOT(startJsonPolling()));
    });
}

void CalaosConnection::processEvents(QString msg)
{
    if (msg == "") return;

    qDebug() << "Received: " << msg;

    QStringList spl = msg.split(' ');

    if (spl.at(0) == "output" || spl.at(0) == "input")
    {
        if (spl.size() < 3) return;

        QString id = QUrl::fromPercentEncoding(spl.at(1).toLocal8Bit());
        QStringList s = QUrl::fromPercentEncoding(spl.at(2).toLocal8Bit()).split(':');
        QString val;
        if (s.size() > 1) val = s.at(1);

        if (spl.at(0) == "input")
            emit eventInputChange(id, s.at(0), val);
        else
            emit eventOutputChange(id, s.at(0), val);
    }
    else if (spl.at(0) == "audio_volume")
    {
        if (spl.count() < 4) return;

        emit eventAudioVolumeChange(spl.at(1), spl.at(3).toDouble());
    }
    else if (spl.at(0) == "audio_status")
    {
        emit eventAudioStatusChange(spl.at(1), spl.at(2));
    }
    else if (spl.at(0) == "audio")
    {
        if (spl.count() > 2 &&
            spl.at(2) == "songchanged")
            emit eventAudioChange(spl.at(1));
    }

    //TODO all other event types
}
