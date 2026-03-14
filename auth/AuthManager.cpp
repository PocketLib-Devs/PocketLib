#include "AuthManager.h"

#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonObject>
#include <QJsonDocument>

AuthManager::AuthManager(QString apiKey, QObject *parent)
    : QObject(parent), apiKey(apiKey)
{
    manager = new QNetworkAccessManager(this);
}

QString AuthManager::getToken()
{
    return idToken;
}

void AuthManager::login(QString email, QString password)
{
    QString url =
    "https://identitytoolkit.googleapis.com/v1/accounts:signInWithPassword?key="
    + apiKey;

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject body;
    body["email"] = email;
    body["password"] = password;
    body["returnSecureToken"] = true;

    QJsonDocument doc(body);

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

        idToken = resDoc.object()["idToken"].toString();

        emit loginSuccess();

        reply->deleteLater();
    });
}