#include "authmanager.h"
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>

AuthManager::AuthManager(QObject *parent)
    : QObject(parent)
{
    manager = new QNetworkAccessManager(this);
}

void AuthManager::login(const QString &email, const QString &password)
{
    QString url =
        "https://identitytoolkit.googleapis.com/v1/accounts:signInWithPassword?key="
        + apiKey;

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject json;
    json["email"] = email;
    json["password"] = password;
    json["returnSecureToken"] = true;

    QJsonDocument doc(json);

    QNetworkReply *reply = manager->post(request, doc.toJson());

    connect(reply, &QNetworkReply::finished, [=]() {

        if(reply->error())
        {
            emit loginFailed(reply->errorString());
            reply->deleteLater();
            return;
        }

        QByteArray response = reply->readAll();
        QJsonDocument resDoc = QJsonDocument::fromJson(response);
        QJsonObject obj = resDoc.object();

        QString idToken = obj["idToken"].toString();
        QString uid = obj["localId"].toString();

        emit loginSuccess(idToken, uid);

        reply->deleteLater();
    });
}


void AuthManager::registerUser(QString email, QString password)
{
    QString url =
        "https://identitytoolkit.googleapis.com/v1/accounts:signUp?key="
        + apiKey;

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject json;
    json["email"] = email;
    json["password"] = password;
    json["returnSecureToken"] = true;

    QJsonDocument doc(json);

    QNetworkReply *reply = manager->post(request, doc.toJson());

    connect(reply, &QNetworkReply::finished, [=]() {

        if(reply->error())
        {
            emit loginFailed(reply->errorString());
            reply->deleteLater();
            return;
        }

        QByteArray response = reply->readAll();
        QJsonDocument resDoc = QJsonDocument::fromJson(response);

        QString idToken = resDoc.object()["idToken"].toString();
        QString uid = resDoc.object()["localId"].toString();

        emit loginSuccess(idToken, uid);

        reply->deleteLater();
    });
}
