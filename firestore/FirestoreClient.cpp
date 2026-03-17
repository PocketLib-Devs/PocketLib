#include "FirestoreClient.h"

#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>
#include <QUuid>
#include <QDebug>

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
QJsonObject FirestoreClient::bookToFields(const Book &book) const
{
    QJsonObject fields;

    fields["title"]       = QJsonObject{ {"stringValue",  book.title}       };
    fields["author"]      = QJsonObject{ {"stringValue",  book.author}      };
    fields["available"]   = QJsonObject{ {"booleanValue", book.available}   };
    fields["category"]    = QJsonObject{ {"stringValue",  book.category}    };
    fields["coverUrl"]    = QJsonObject{ {"stringValue",  book.coverUrl}    };
    fields["description"] = QJsonObject{ {"stringValue",  book.description} };
    fields["rating"]      = QJsonObject{ {"doubleValue",  book.rating}      };
    fields["section"]     = QJsonObject{ {"stringValue",  book.section}     };

    return fields;
}

/*
 *  docName is the full resource name returned by Firestore, e.g.:
 *  "projects/pocketlib-ea41d/databases/(default)/documents/books/abc123"
 *  We extract only the last path segment as the document ID.
 */
Book FirestoreClient::fieldsToBook(const QString     &docName,
                                   const QJsonObject &fields) const
{
    Book book;

    // Extract document ID from the resource name
    book.id = docName.section('/', -1);

    book.title       = fields["title"]      .toObject()["stringValue"] .toString();
    book.author      = fields["author"]     .toObject()["stringValue"] .toString();
    book.available   = fields["available"]  .toObject()["booleanValue"].toBool(true);
    book.category    = fields["category"]   .toObject()["stringValue"] .toString();
    book.coverUrl    = fields["coverUrl"]   .toObject()["stringValue"] .toString();
    book.description = fields["description"].toObject()["stringValue"] .toString();
    book.rating      = fields["rating"]     .toObject()["doubleValue"] .toDouble(0.0);
    book.section     = fields["section"]    .toObject()["stringValue"] .toString();

    return book;
}

// ═══════════════════════════════════════════════════════════════════════════
//  addBook  –  PATCH  /books/{id}   (creates or overwrites)
// ═══════════════════════════════════════════════════════════════════════════
void FirestoreClient::addBook(const Book    &book,
                              const QString &token,
                              std::function<void(QString docId)> callback)
{
    // Use the provided id, or generate a new UUID
    QString docId = book.id.isEmpty()
                        ? QUuid::createUuid().toString(QUuid::WithoutBraces)
                        : book.id;

    QString url = booksBaseUrl() + "/" + docId;

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", ("Bearer " + token).toUtf8());

    QJsonObject body;
    body["fields"] = bookToFields(book);

    QNetworkReply *reply = networkManager.sendCustomRequest(
        request, "PATCH",
        QJsonDocument(body).toJson()
        );

    connect(reply, &QNetworkReply::finished, [reply, callback, docId]()
            {
                QByteArray    response = reply->readAll();
                QJsonDocument jsonDoc  = QJsonDocument::fromJson(response);
                QJsonObject   json     = jsonDoc.object();

                if (json.contains("error")) {
                    QString msg = json["error"].toObject()["message"].toString();
                    qDebug() << "addBook failed:" << msg;
                    callback("");
                } else {
                    // Firestore echoes back the document name
                    QString returnedId = json["name"].toString().section('/', -1);
                    callback(returnedId.isEmpty() ? docId : returnedId);
                }

                reply->deleteLater();
            });
}
