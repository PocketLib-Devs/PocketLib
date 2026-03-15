#include "FirestoreClient.h"

#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>

FirestoreClient::FirestoreClient(QString projectId, QObject *parent)
    : QObject(parent)
{
    manager = new QNetworkAccessManager(this);

    baseUrl =
    "https://firestore.googleapis.com/v1/projects/" +
    projectId +
    "/databases/(default)/documents/";
}

void FirestoreClient::setAuthToken(QString token)
{
    idToken = token;
}

void FirestoreClient::addBook(QString title, QString author, int copies)
{
    QJsonObject fields;

    fields["title"] =
        QJsonObject{{"stringValue", title}};

    fields["author"] =
        QJsonObject{{"stringValue", author}};

    fields["availableCopies"] =
        QJsonObject{{"integerValue", QString::number(copies)}};

    QJsonObject body;
    body["fields"] = fields;

    QJsonDocument doc(body);

    QNetworkRequest request(baseUrl + "books");

    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization",
                         ("Bearer " + idToken).toUtf8());

    QNetworkReply *reply =
        manager->post(request, doc.toJson());

    connect(reply, &QNetworkReply::finished, [=]() {

        if(reply->error())
            emit requestError(reply->errorString());
        else
            emit requestSuccess("Book added");

        reply->deleteLater();
    });
}

void FirestoreClient::getBooks()
{
    QNetworkRequest request(baseUrl + "books");

    request.setRawHeader("Authorization",
                         ("Bearer " + idToken).toUtf8());

    QNetworkReply *reply = manager->get(request);

    connect(reply, &QNetworkReply::finished, [=]() {

        if(reply->error())
        {
            emit requestError(reply->errorString());
            reply->deleteLater();
            return;
        }

        QByteArray data = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data); // <-- This is the line that went missing!
        QJsonObject jsonObj = doc.object();
        QJsonArray docs = jsonObj.value("documents").toArray();

        emit booksReceived(docs);

        reply->deleteLater();
    });
}

void FirestoreClient::deleteBook(QString documentId)
{
    QNetworkRequest request(baseUrl + "books/" + documentId);

    request.setRawHeader("Authorization",
                         ("Bearer " + idToken).toUtf8());

    QNetworkReply *reply = manager->deleteResource(request);

    connect(reply, &QNetworkReply::finished, [=]() {

        if(reply->error())
            emit requestError(reply->errorString());
        else
            emit requestSuccess("Book deleted");

        reply->deleteLater();
    });
}
