#include "FirestoreClient.h"

#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>
#include <QUuid>
#include <QDebug>
#include <QRandomGenerator>

FirestoreClient::FirestoreClient(QObject *parent)
    : QObject(parent)
{
}
void FirestoreClient::createUser(const UserInfo &user, const QString &token)
{
    QString url = usersBaseUrl() + "/" + user.uid;

    QNetworkRequest request(url);

    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", ("Bearer " + token).toUtf8());

    QJsonObject fields;

    fields["name"]       = QJsonObject{ {"stringValue",  user.name}                    };
    fields["email"]      = QJsonObject{ {"stringValue",  user.email}                   };
    fields["role"]       = QJsonObject{ {"stringValue",  user.role}                    };
    fields["libraryId"]  = QJsonObject{ {"stringValue",  user.libraryId}               };
    fields["fineAmount"] = QJsonObject{ {"integerValue",  QString::number(user.fineAmount)} };

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
void FirestoreClient::getUserInfo(const QString &uid, const QString &token,
                                  std::function<void(UserInfo)> callback)
{
    QNetworkRequest request(usersBaseUrl() + "/" + uid);
    request.setRawHeader("Authorization", ("Bearer " + token).toUtf8());

    QNetworkReply *reply = networkManager.get(request);

    connect(reply, &QNetworkReply::finished, [reply, uid, callback]()
            {
                QByteArray    response = reply->readAll();
                QJsonDocument jsonDoc  = QJsonDocument::fromJson(response);
                QJsonObject   json     = jsonDoc.object();

                UserInfo info;

                if (json.contains("error")) {
                    qDebug() << "getUserInfo failed:"
                             << json["error"].toObject()["message"].toString();
                    callback(info);            // uid will be empty → caller can check
                    reply->deleteLater();
                    return;
                }

                QJsonObject fields = json["fields"].toObject();

                info.uid        = uid;
                info.name       = fields["name"]      .toObject()["stringValue"] .toString();
                info.email      = fields["email"]     .toObject()["stringValue"] .toString();
                info.role       = fields["role"]      .toObject()["stringValue"] .toString();
                info.libraryId  = fields["libraryId"] .toObject()["stringValue"] .toString();

                // fineAmount is stored as integerValue (Firestore sends it as a string)
                QString fineStr = fields["fineAmount"].toObject()["integerValue"].toString();
                info.fineAmount = fineStr.toInt();

                callback(info);
                reply->deleteLater();
            });
}
void FirestoreClient::updateFineAmount(const QString &uid, int amount,
                                       const QString &token,
                                       std::function<void(bool)> callback)
{
    QString url = usersBaseUrl() + "/" + uid
                  + "?updateMask.fieldPaths=fineAmount";

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", ("Bearer " + token).toUtf8());

    QJsonObject fields;
    fields["fineAmount"] = QJsonObject{
        {"integerValue", QString::number(amount)}
    };

    QJsonObject body;
    body["fields"] = fields;

    QNetworkReply *reply = networkManager.sendCustomRequest(
        request, "PATCH", QJsonDocument(body).toJson()
        );

    connect(reply, &QNetworkReply::finished, [reply, callback]()
            {
                QByteArray    response = reply->readAll();
                QJsonDocument jsonDoc  = QJsonDocument::fromJson(response);
                QJsonObject   json     = jsonDoc.object();

                if (json.contains("error")) {
                    qDebug() << "updateFineAmount failed:"
                             << json["error"].toObject()["message"].toString();
                    callback(false);
                } else {
                    callback(true);
                }
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

/*
 *  docName is the full resource name returned by Firestore, e.g.:
 *  "projects/pocketlib-ea41d/databases/(default)/documents/books/abc123"
 *  We extract only the last path segment as the document ID.
 */
Book FirestoreClient::fieldsToBook(const QString     &docName,
                                   const QJsonObject &fields) const
{
    Book book;

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
QString FirestoreClient::buildCandidateId(const QString &name) const
{
    // Take the first word of the name, keep only letters
    QString firstName = name.split(' ', Qt::SkipEmptyParts).first();
    QString cleaned;
    for (const QChar &ch : firstName)
        if (ch.isLetter()) cleaned += ch;

    if (cleaned.isEmpty()) cleaned = "User";

    // Capitalise first letter, lowercase the rest (consistent format)
    cleaned = cleaned.left(1).toUpper() + cleaned.mid(1).toLower();

    // Clamp to 10 characters so the full ID stays readable
    cleaned = cleaned.left(10);

    // 4-digit suffix: 0000 – 9999
    quint32 number = QRandomGenerator::global()->bounded(10000u);
    QString suffix = QString("%1").arg(number, 4, 10, QChar('0'));

    return cleaned + suffix;
}

/*
 *  Uses Firestore's runQuery endpoint to find any user document whose
 *  libraryId field equals candidateId.  Callback: true = already taken.
 */
void FirestoreClient::checkLibraryIdExists(
    const QString &candidateId,
    const QString &token,
    std::function<void(bool exists)> callback)
{
    QNetworkRequest request(runQueryUrl());
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", ("Bearer " + token).toUtf8());

    // Structured query: SELECT * FROM users WHERE libraryId == candidateId LIMIT 1
    QJsonObject fieldFilter;
    fieldFilter["field"]  = QJsonObject{ {"fieldPath", "libraryId"} };
    fieldFilter["op"]     = "EQUAL";
    fieldFilter["value"]  = QJsonObject{ {"stringValue", candidateId} };

    QJsonObject where;
    where["fieldFilter"] = fieldFilter;

    QJsonObject structuredQuery;
    structuredQuery["from"]  = QJsonArray{
        QJsonObject{ {"collectionId", "users"} }
    };
    structuredQuery["where"] = where;
    structuredQuery["limit"] = 1;

    QJsonObject body;
    body["structuredQuery"] = structuredQuery;

    QNetworkReply *reply = networkManager.post(
        request, QJsonDocument(body).toJson()
        );

    connect(reply, &QNetworkReply::finished, [reply, callback]()
            {
                QByteArray    response = reply->readAll();
                QJsonDocument jsonDoc  = QJsonDocument::fromJson(response);

                // runQuery returns a JSON array; an empty document entry means no match
                bool exists = false;
                if (jsonDoc.isArray()) {
                    QJsonArray arr = jsonDoc.array();
                    // If there's a real document (has "document" key), ID is taken
                    for (const QJsonValue &val : arr) {
                        if (val.toObject().contains("document")) {
                            exists = true;
                            break;
                        }
                    }
                }

                callback(exists);
                reply->deleteLater();
            });
}

/*
 *  Recursive helper that retries up to `attemptsLeft` times.
 */
void FirestoreClient::tryGenerateId(const QString &name,
                                    const QString &token,
                                    int attemptsLeft,
                                    std::function<void(QString)> callback)
{
    if (attemptsLeft <= 0) {
        qDebug() << "generateUniqueLibraryId: max attempts reached.";
        callback("");
        return;
    }

    QString candidate = buildCandidateId(name);

    checkLibraryIdExists(candidate, token, [=](bool exists)
                         {
                             if (!exists) {
                                 // Found a unique one
                                 callback(candidate);
                             } else {
                                 qDebug() << "Library ID collision:" << candidate << "— retrying…";
                                 tryGenerateId(name, token, attemptsLeft - 1, callback);
                             }
                         });
}

// ═══════════════════════════════════════════════════════════════════════════
//  LIBRARY ID — public entry point
// ═══════════════════════════════════════════════════════════════════════════
void FirestoreClient::generateUniqueLibraryId(
    const QString &name, const QString &token,
    std::function<void(QString libraryId)> callback)
{
    tryGenerateId(name, token, /*maxAttempts=*/10, callback);
}


void FirestoreClient::addBook(const Book    &book,
                              const QString &token,
                              std::function<void(QString docId)> callback)
{
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

                    QString returnedId = json["name"].toString().section('/', -1);
                    callback(returnedId.isEmpty() ? docId : returnedId);
                }

                reply->deleteLater();
            });
}
void FirestoreClient::updateUserName(const QString &uid,
                                     const QString &newName,
                                     const QString &token,
                                     std::function<void(bool success)> callback)
{
    // updateMask ensures ONLY the 'name' field is overwritten
    QString url = usersBaseUrl() + "/" + uid
                  + "?updateMask.fieldPaths=name";

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", ("Bearer " + token).toUtf8());

    QJsonObject fields;
    fields["name"] = QJsonObject{ {"stringValue", newName} };

    QJsonObject body;
    body["fields"] = fields;

    QNetworkReply *reply = networkManager.sendCustomRequest(
        request, "PATCH", QJsonDocument(body).toJson()
        );

    connect(reply, &QNetworkReply::finished, [reply, callback]()
            {
                QJsonObject json =
                    QJsonDocument::fromJson(reply->readAll()).object();

                if (json.contains("error")) {
                    qDebug() << "updateUserName failed:"
                             << json["error"].toObject()["message"].toString();
                    callback(false);
                } else {
                    callback(true);
                }
                reply->deleteLater();
            });
}

