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

void FirestoreClient::deleteBook(QString documentId, QString idToken)
{
    // 1. Construct the correct URL using your projectId
    QString url = "https://firestore.googleapis.com/v1/projects/" + projectId + "/databases/(default)/documents/books/" + documentId;

    QNetworkRequest request((QUrl(url)));

    // 2. Add the authentication token
    request.setRawHeader("Authorization", ("Bearer " + idToken).toUtf8());

    // 3. Use your class's actual networkManager (not 'manager')
    QNetworkReply *reply = networkManager.deleteResource(request);

    connect(reply, &QNetworkReply::finished, [=]() {
        if(reply->error())
            emit requestError(reply->errorString()); // Now emits properly!
        else
            emit requestSuccess("Book deleted");     // Now emits properly!

        reply->deleteLater();
    });
}
void FirestoreClient::fetchBorrowedBooks(QString token, std::function<void(QJsonArray)> callback)
{
    // Point this to your 'borrowed_books' collection
    QString url = "https://firestore.googleapis.com/v1/projects/" + projectId + "/databases/(default)/documents/borrowed_books";

    QNetworkRequest request((QUrl(url)));
    request.setRawHeader("Authorization", ("Bearer " + token).toUtf8());

    // Use a GET request to retrieve data
    QNetworkReply *reply = networkManager.get(request);

    connect(reply, &QNetworkReply::finished, [=]() {
        if(reply->error() == QNetworkReply::NoError) {
            // Read the JSON response from Firebase
            QByteArray response = reply->readAll();
            QJsonDocument jsonDoc = QJsonDocument::fromJson(response);
            QJsonObject jsonObj = jsonDoc.object();

            // Extract the array of documents
            QJsonArray docs = jsonObj.value("documents").toArray();

            // Send the array back to the MainWindow!
            callback(docs);
        } else {
            qDebug() << "Error fetching books:" << reply->errorString();
            callback(QJsonArray()); // Send empty array if it fails
        }
        reply->deleteLater();
    });
}
void FirestoreClient::updateFineInFirestore(QString uid, int fineAmount, QString token)
{
    // Path to the specific user's document
    QString url = "https://firestore.googleapis.com/v1/projects/" + projectId + "/databases/(default)/documents/users/" + uid + "?updateMask.fieldPaths=fineAmount";

    QNetworkRequest request((QUrl(url)));
    request.setRawHeader("Authorization", ("Bearer " + token).toUtf8());
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    // Construct the JSON body
    QJsonObject fineObj;
    fineObj["integerValue"] = QString::number(fineAmount);

    QJsonObject fields;
    fields["fineAmount"] = fineObj;

    QJsonObject body;
    body["fields"] = fields;

    // Send the PATCH request
    QNetworkReply *reply = networkManager.sendCustomRequest(request, "PATCH", QJsonDocument(body).toJson());

    connect(reply, &QNetworkReply::finished, [=]() {
        if(reply->error() == QNetworkReply::NoError) {
            emit requestSuccess("Fine updated in database");
        } else {
            // THIS WILL REVEAL THE EXACT PROBLEM
            qDebug() << "FIREBASE REJECTED THE UPDATE!";
            qDebug() << "ERROR:" << reply->errorString();
            qDebug() << "DETAILS:" << reply->readAll();

            emit requestError("Failed to update fine: " + reply->errorString());
        }
        reply->deleteLater();
    });
}
void FirestoreClient::getFineAmount(QString uid, QString token, std::function<void(int)> callback)
{
    QString url = "https://firestore.googleapis.com/v1/projects/" + projectId + "/databases/(default)/documents/users/" + uid;

    QNetworkRequest request((QUrl(url)));
    request.setRawHeader("Authorization", ("Bearer " + token).toUtf8());

    QNetworkReply *reply = networkManager.get(request);

    connect(reply, &QNetworkReply::finished, [=]() {
        if(reply->error() == QNetworkReply::NoError) {
            QJsonDocument jsonDoc = QJsonDocument::fromJson(reply->readAll());
            // Extract the fineAmount field
            int fine = jsonDoc.object().value("fields").toObject()
                           .value("fineAmount").toObject()
                           .value("integerValue").toString().toInt();
            callback(fine);
        } else {
            callback(0); // If error, assume no fine for safety
        }
        reply->deleteLater();
    });
}
