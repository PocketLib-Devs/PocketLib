#include "FirestoreClient.h"

#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>

FirestoreClient::FirestoreClient(QObject *parent)
    : QObject(parent)
{
}

void FirestoreClient::createUser(QString uid, QString email, QString role, QString token)
{
    QString url =
        "https://firestore.googleapis.com/v1/projects/" + projectId +
        "/databases/(default)/documents/users/" + uid;

    QNetworkRequest request(url);

    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", ("Bearer " + token).toUtf8());

    QJsonObject fields;

    fields["email"] = QJsonObject{
        {"stringValue", email}
    };

    fields["role"] = QJsonObject{
        {"stringValue", role}
    };

    QJsonObject body;
    body["fields"] = fields;

    networkManager.sendCustomRequest(
        request,
        "PATCH",
        QJsonDocument(body).toJson()
        );
}

void FirestoreClient::getUserRole(QString uid, QString token,
                                  std::function<void(QString role)> callback)
{
    QString url =
        "https://firestore.googleapis.com/v1/projects/" + projectId +
        "/databases/(default)/documents/users/" + uid;

    QNetworkRequest request(url);
    request.setRawHeader("Authorization", ("Bearer " + token).toUtf8());

    QNetworkReply *reply = networkManager.get(request);

    connect(reply, &QNetworkReply::finished, [reply, callback]()
            {
                QByteArray response = reply->readAll();

                QJsonDocument jsonDoc = QJsonDocument::fromJson(response);
                QJsonObject json = jsonDoc.object();

                QString role =
                    json["fields"].toObject()
                        ["role"].toObject()
                                ["stringValue"].toString();

                callback(role);

                reply->deleteLater();
            });
}

void FirestoreClient::deleteBook(QString documentId)
{
    QNetworkRequest request(baseUrl + "books/" + documentId);

    request.setRawHeader("Authorization",
                         ("Bearer " + idToken).toUtf8());

    QNetworkReply *reply = networkManager.deleteResource(request);

    connect(reply, &QNetworkReply::finished, [=]() {

        if(reply->error())
            emit requestError(reply->errorString());
        else
            emit requestSuccess("Book deleted");

        reply->deleteLater();
    });
}
