#include "firestoreclient.h"
#include <QNetworkRequest>
#include <QNetworkReply>

FirestoreClient::FirestoreClient(QObject *parent)
    : QObject(parent)
{
    manager = new QNetworkAccessManager(this);
}

void FirestoreClient::getBooks(const QString &idToken)
{
    QString url =
        "https://firestore.googleapis.com/v1/projects/" +
        projectId +
        "/databases/(default)/documents/books";

    QNetworkRequest request(url);
    request.setRawHeader("Authorization", ("Bearer " + idToken).toUtf8());

    QNetworkReply *reply = manager->get(request);

    connect(reply, &QNetworkReply::finished, [=]() {

        if(reply->error())
        {
            emit firestoreError(reply->errorString());
            reply->deleteLater();
            return;
        }

        QByteArray data = reply->readAll();
        emit booksReceived(data);

        reply->deleteLater();
    });
}
