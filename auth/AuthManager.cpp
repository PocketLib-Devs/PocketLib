#include "AuthManager.h"

#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QUrl>

AuthManager::AuthManager(QObject *parent)
    : QObject(parent)
{
}

void AuthManager::loginUser(QString email, QString password,
                            std::function<void(QString token, QString uid)> callback)
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

    QNetworkReply *reply =
        networkManager.post(request, QJsonDocument(body).toJson());

    connect(reply, &QNetworkReply::finished, [this, reply, callback]()
            {
                QByteArray response = reply->readAll();

                QJsonDocument jsonDoc = QJsonDocument::fromJson(response);
                QJsonObject json = jsonDoc.object();


                if (json.contains("error")) {
                    QString message =
                        json["error"].toObject()["message"].toString();

                    qDebug() << "Login failed:" << message;

                    callback("", "");
                }
                else {
                    QString token = json["idToken"].toString();
                    QString uid = json["localId"].toString();
                    this->m_token = token;
                    callback(token, uid);
                }

                reply->deleteLater();
            });
}

void AuthManager::registerUser(QString email, QString password,
                               std::function<void(QString token, QString uid)> callback)
{
    QString url =
        "https://identitytoolkit.googleapis.com/v1/accounts:signUp?key="
        + apiKey;

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject body;
    body["email"] = email;
    body["password"] = password;
    body["returnSecureToken"] = true;

    QNetworkReply *reply =
        networkManager.post(request, QJsonDocument(body).toJson());

    connect(reply, &QNetworkReply::finished, [this, reply, callback]()
            {
                QByteArray response = reply->readAll();

                QJsonDocument jsonDoc = QJsonDocument::fromJson(response);
                QJsonObject json = jsonDoc.object();

                if (json.contains("error")) {
                    QString message =
                        json["error"].toObject()["message"].toString();

                    qDebug() << "Registration failed:" << message;

                    callback("", "");
                }
                else {
                    QString token = json["idToken"].toString();
                    QString uid = json["localId"].toString();
                    this->m_token = token;
                    callback(token, uid);
                }

                reply->deleteLater();
            });
}
