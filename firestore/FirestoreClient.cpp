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
                    callback(info);
                    reply->deleteLater();
                    return;
                }

                QJsonObject fields = json["fields"].toObject();

                info.uid        = uid;
                info.name       = fields["name"]      .toObject()["stringValue"] .toString();
                info.email      = fields["email"]     .toObject()["stringValue"] .toString();
                info.role       = fields["role"]      .toObject()["stringValue"] .toString();
                info.libraryId  = fields["libraryId"] .toObject()["stringValue"] .toString();


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
    fields["available"]   = QJsonObject{ {"integerValue", book.available}   };
    fields["category"]    = QJsonObject{ {"stringValue",  book.category}    };
    fields["coverUrl"]    = QJsonObject{ {"stringValue",  book.coverUrl}    };
    fields["description"] = QJsonObject{ {"stringValue",  book.description} };
    fields["rating"]      = QJsonObject{ {"doubleValue",  book.rating}      };
    fields["section"]     = QJsonObject{ {"stringValue",  book.section}     };
    return fields;
}

void FirestoreClient::deleteBook(QString documentId, QString idToken)
{

    QString url = "https://firestore.googleapis.com/v1/projects/" + projectId + "/databases/(default)/documents/books/" + documentId;

    QNetworkRequest request((QUrl(url)));


    request.setRawHeader("Authorization", ("Bearer " + idToken).toUtf8());


    QNetworkReply *reply = networkManager.deleteResource(request);

    connect(reply, &QNetworkReply::finished, [=]() {
        if(reply->error())
            emit requestError(reply->errorString());
        else
            emit requestSuccess("Book deleted");

        reply->deleteLater();
    });
}
void FirestoreClient::fetchBorrowedBooks(QString token, std::function<void(QJsonArray)> callback)
{

    QString url = "https://firestore.googleapis.com/v1/projects/" + projectId + "/databases/(default)/documents/borrowed_books";

    QNetworkRequest request((QUrl(url)));
    request.setRawHeader("Authorization", ("Bearer " + token).toUtf8());


    QNetworkReply *reply = networkManager.get(request);

    connect(reply, &QNetworkReply::finished, [=]() {
        if(reply->error() == QNetworkReply::NoError) {

            QByteArray response = reply->readAll();
            QJsonDocument jsonDoc = QJsonDocument::fromJson(response);
            QJsonObject jsonObj = jsonDoc.object();


            QJsonArray docs = jsonObj.value("documents").toArray();


            callback(docs);
        } else {
            qDebug() << "Error fetching books:" << reply->errorString();
            callback(QJsonArray());
        }
        reply->deleteLater();
    });
}
void FirestoreClient::updateFineInFirestore(QString uid, int fineAmount, QString token)
{

    QString url = "https://firestore.googleapis.com/v1/projects/" + projectId + "/databases/(default)/documents/users/" + uid + "?updateMask.fieldPaths=fineAmount";

    QNetworkRequest request((QUrl(url)));
    request.setRawHeader("Authorization", ("Bearer " + token).toUtf8());
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");


    QJsonObject fineObj;
    fineObj["integerValue"] = QString::number(fineAmount);

    QJsonObject fields;
    fields["fineAmount"] = fineObj;

    QJsonObject body;
    body["fields"] = fields;


    QNetworkReply *reply = networkManager.sendCustomRequest(request, "PATCH", QJsonDocument(body).toJson());

    connect(reply, &QNetworkReply::finished, [=]() {
        if(reply->error() == QNetworkReply::NoError) {
            emit requestSuccess("Fine updated in database");
        } else {

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

            int fine = jsonDoc.object().value("fields").toObject()
                           .value("fineAmount").toObject()
                           .value("integerValue").toString().toInt();
            callback(fine);
        } else {
            callback(0);
        }
        reply->deleteLater();
    });

}


Book FirestoreClient::fieldsToBook(const QString     &docName,
                                   const QJsonObject &fields) const
{
    Book book;

    book.id = docName.section('/', -1);

    book.title       = fields["title"]      .toObject()["stringValue"] .toString();
    book.author      = fields["author"]     .toObject()["stringValue"] .toString();
    book.available   = fields["available"]  .toObject()["integerValue"].toString().toInt();
    book.category    = fields["category"]   .toObject()["stringValue"] .toString();
    book.coverUrl    = fields["coverUrl"]   .toObject()["stringValue"] .toString();
    book.description = fields["description"].toObject()["stringValue"] .toString();
    book.rating      = fields["rating"]     .toObject()["doubleValue"] .toDouble(0.0);
    book.section     = fields["section"]    .toObject()["stringValue"] .toString();

    return book;
}
QString FirestoreClient::buildCandidateId(const QString &name) const
{

    QString firstName = name.split(' ', Qt::SkipEmptyParts).first();
    QString cleaned;
    for (const QChar &ch : firstName)
        if (ch.isLetter()) cleaned += ch;

    if (cleaned.isEmpty()) cleaned = "User";


    cleaned = cleaned.left(1).toUpper() + cleaned.mid(1).toLower();


    cleaned = cleaned.left(10);


    quint32 number = QRandomGenerator::global()->bounded(10000u);
    QString suffix = QString("%1").arg(number, 4, 10, QChar('0'));

    return cleaned + suffix;
}


void FirestoreClient::checkLibraryIdExists(
    const QString &candidateId,
    const QString &token,
    std::function<void(bool exists)> callback)
{
    QNetworkRequest request(runQueryUrl());
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", ("Bearer " + token).toUtf8());


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


                bool exists = false;
                if (jsonDoc.isArray()) {
                    QJsonArray arr = jsonDoc.array();

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

                                 callback(candidate);
                             } else {
                                 qDebug() << "Library ID collision:" << candidate << "— retrying…";
                                 tryGenerateId(name, token, attemptsLeft - 1, callback);
                             }
                         });
}


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

void FirestoreClient::updateBook(const Book    &book,
                                 const QString &token,
                                 std::function<void(bool success)> callback)
{
    if (book.id.isEmpty()) {
        qDebug() << "updateBook: book.id must not be empty";
        callback(false);
        return;
    }


    QString url = booksBaseUrl() + "/" + book.id
                  + "?updateMask.fieldPaths=title"
                    "&updateMask.fieldPaths=author"
                    "&updateMask.fieldPaths=available"
                    "&updateMask.fieldPaths=category"
                    "&updateMask.fieldPaths=coverUrl"
                    "&updateMask.fieldPaths=description"
                    "&updateMask.fieldPaths=rating"
                    "&updateMask.fieldPaths=section";

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", ("Bearer " + token).toUtf8());

    QJsonObject body;
    body["fields"] = bookToFields(book);

    QNetworkReply *reply = networkManager.sendCustomRequest(
        request, "PATCH",
        QJsonDocument(body).toJson()
        );

    connect(reply, &QNetworkReply::finished, [reply, callback]()
            {
                QByteArray    response = reply->readAll();
                QJsonDocument jsonDoc  = QJsonDocument::fromJson(response);
                QJsonObject   json     = jsonDoc.object();

                if (json.contains("error")) {
                    QString msg = json["error"].toObject()["message"].toString();
                    qDebug() << "updateBook failed:" << msg;
                    callback(false);
                } else {
                    callback(true);
                }

                reply->deleteLater();
            });
}

void FirestoreClient::removeBook(const QString &bookId,
                                 const QString &token,
                                 std::function<void(bool success)> callback)
{
    QString url = booksBaseUrl() + "/" + bookId;

    QNetworkRequest request(url);
    request.setRawHeader("Authorization", ("Bearer " + token).toUtf8());

    QNetworkReply *reply = networkManager.deleteResource(request);

    connect(reply, &QNetworkReply::finished, [reply, callback]()
            {

                int statusCode =
                    reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

                if (statusCode == 200) {
                    callback(true);
                } else {
                    QByteArray response = reply->readAll();
                    qDebug() << "removeBook failed. Status:" << statusCode
                             << "Body:" << response;
                    callback(false);
                }

                reply->deleteLater();
            });
}
void FirestoreClient::getAllBooks(const QString &token,
                                  std::function<void(QList<Book> books)> callback)
{
    QNetworkRequest request(booksBaseUrl());
    request.setRawHeader("Authorization", ("Bearer " + token).toUtf8());

    QNetworkReply *reply = networkManager.get(request);

    connect(reply, &QNetworkReply::finished, [reply, callback, this]()
            {
                QByteArray    response = reply->readAll();
                QJsonDocument jsonDoc  = QJsonDocument::fromJson(response);
                QJsonObject   json     = jsonDoc.object();

                QList<Book> books;

                if (json.contains("error")) {
                    QString msg = json["error"].toObject()["message"].toString();
                    qDebug() << "getAllBooks failed:" << msg;
                    callback(books);
                    reply->deleteLater();
                    return;
                }


                QJsonArray documents = json["documents"].toArray();
                for (const QJsonValue &val : documents) {
                    QJsonObject doc    = val.toObject();
                    QString     name   = doc["name"].toString();
                    QJsonObject fields = doc["fields"].toObject();
                    books.append(fieldsToBook(name, fields));
                }

                callback(books);
                reply->deleteLater();
            });
}
void FirestoreClient::uploadImageToCloudinary(const QString &localFilePath, std::function<void(QString)> callback)
{

    QFile *file = new QFile(localFilePath);
    if (!file->open(QIODevice::ReadOnly)) {
        qDebug() << "Could not open image file:" << localFilePath;
        callback("");
        delete file;
        return;
    }


    QString cloudName = "dtg3gx9fx";
    QString uploadPreset = "pocketlib_covers";

    QString url = "https://api.cloudinary.com/v1_1/" + cloudName + "/image/upload";
    QNetworkRequest request((QUrl(url)));


    QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);


    QHttpPart presetPart;
    presetPart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"upload_preset\""));
    presetPart.setBody(uploadPreset.toUtf8());
    multiPart->append(presetPart);


    QHttpPart imagePart;
    QFileInfo fileInfo(localFilePath);

    imagePart.setHeader(QNetworkRequest::ContentDispositionHeader,
                        QVariant(QString("form-data; name=\"file\"; filename=\"%1\"").arg(fileInfo.fileName())));
    imagePart.setBodyDevice(file);
    file->setParent(multiPart);
    multiPart->append(imagePart);


    QNetworkReply *reply = networkManager.post(request, multiPart);


    multiPart->setParent(reply);


    connect(reply, &QNetworkReply::finished, [reply, callback]() {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray response = reply->readAll();
            QJsonDocument jsonDoc = QJsonDocument::fromJson(response);


            QString secureUrl = jsonDoc.object().value("secure_url").toString();
            qDebug() << "Upload Successful! URL:" << secureUrl;
            callback(secureUrl);
        } else {
            qDebug() << "Upload Failed:" << reply->errorString();
            qDebug() << reply->readAll();
            callback("");
        }
        reply->deleteLater();
    });
}
void FirestoreClient::addToMyBooks(const QString &uid, const Book &book,
                                   const QString &token,
                                   std::function<void(bool)> callback)
{
    QString url = "https://firestore.googleapis.com/v1/projects/" + projectId
                  + "/databases/(default)/documents/users/" + uid
                  + "/myBooks/" + book.id;

    QNetworkRequest request((QUrl(url)));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", ("Bearer " + token).toUtf8());

    QJsonObject body;
    body["fields"] = bookToFields(book);

    QNetworkReply *reply = networkManager.sendCustomRequest(
        request, "PATCH", QJsonDocument(body).toJson()
        );

    connect(reply, &QNetworkReply::finished, [reply, callback]() {
        QJsonObject json = QJsonDocument::fromJson(reply->readAll()).object();
        if (json.contains("error")) {
            qDebug() << "addToMyBooks failed:"
                     << json["error"].toObject()["message"].toString();
            callback(false);
        } else {
            callback(true);
        }
        reply->deleteLater();
    });
}
void FirestoreClient::getMyBooks(const QString &uid, const QString &token,
                                 std::function<void(QList<Book>)> callback)
{
    QString url = "https://firestore.googleapis.com/v1/projects/" + projectId
                  + "/databases/(default)/documents/users/" + uid + "/myBooks";

    QNetworkRequest request((QUrl(url)));
    request.setRawHeader("Authorization", ("Bearer " + token).toUtf8());

    QNetworkReply *reply = networkManager.get(request);

    connect(reply, &QNetworkReply::finished, [reply, callback, this]() {
        QJsonObject json = QJsonDocument::fromJson(reply->readAll()).object();
        QList<Book> books;

        if (!json.contains("error")) {
            for (const QJsonValue &val : json["documents"].toArray()) {
                QJsonObject doc = val.toObject();
                books.append(fieldsToBook(doc["name"].toString(),
                                          doc["fields"].toObject()));
            }
        } else {
            qDebug() << "getMyBooks failed:"
                     << json["error"].toObject()["message"].toString();
        }

        callback(books);
        reply->deleteLater();
    });
}
void FirestoreClient::removeFromMyBooks(const QString &uid, const QString &bookId,
                                        const QString &token,
                                        std::function<void(bool)> callback)
{
    QString url = "https://firestore.googleapis.com/v1/projects/" + projectId
                  + "/databases/(default)/documents/users/" + uid
                  + "/myBooks/" + bookId;

    QNetworkRequest request((QUrl(url)));
    request.setRawHeader("Authorization", ("Bearer " + token).toUtf8());

    QNetworkReply *reply = networkManager.deleteResource(request);

    connect(reply, &QNetworkReply::finished, [reply, callback]() {
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        callback(status == 200);
          reply->deleteLater();
    });
}

void FirestoreClient::adminCheckoutBook(QString studentLibraryId, QString bookId, QString token, QString adminUID, std::function<void(bool, QString)> callback)
{

    QUrl queryUrl("https://firestore.googleapis.com/v1/projects/pocketlib-ea41d/databases/(default)/documents:runQuery");
    QNetworkRequest request(queryUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", ("Bearer " + token).toUtf8());

    QJsonObject fieldFilter;
    fieldFilter["field"] = QJsonObject{{"fieldPath", "libraryId"}};
    fieldFilter["op"] = "EQUAL";
    fieldFilter["value"] = QJsonObject{{"stringValue", studentLibraryId}};

    QJsonObject whereObj{{"fieldFilter", fieldFilter}};
    QJsonObject fromObj{{"collectionId", "users"}};
    QJsonArray fromArray; fromArray.append(fromObj);

    QJsonObject structuredQuery;
    structuredQuery["from"] = fromArray;
    structuredQuery["where"] = whereObj;
    QJsonObject queryPayload{{"structuredQuery", structuredQuery}};


    QNetworkReply *reply = networkManager.post(request, QJsonDocument(queryPayload).toJson());

    connect(reply, &QNetworkReply::finished, [=]() {
        if (reply->error() != QNetworkReply::NoError) {
            callback(false, "Network error while searching for user.");
            reply->deleteLater(); return;
        }

        QJsonArray docs = QJsonDocument::fromJson(reply->readAll()).array();
        if (docs.isEmpty() || !docs[0].toObject().contains("document")) {
            callback(false, "Student Library ID not found in database.");
            reply->deleteLater(); return;
        }

        QJsonObject userDoc = docs[0].toObject().value("document").toObject();
        QString docName = userDoc.value("name").toString();
        QString studentUID = docName.split("/").last();
        QString studentName = userDoc.value("fields").toObject().value("name").toObject().value("stringValue").toString();


        QUrl bookUrl("https://firestore.googleapis.com/v1/projects/pocketlib-ea41d/databases/(default)/documents/books/" + bookId);
        QNetworkRequest bookReq(bookUrl);
        bookReq.setRawHeader("Authorization", ("Bearer " + token).toUtf8());


        QNetworkReply *bookReply = networkManager.get(bookReq);

        connect(bookReply, &QNetworkReply::finished, [=]() {
            if (bookReply->error() != QNetworkReply::NoError) {
                callback(false, "Book ID not found in database.");
                bookReply->deleteLater(); return;
            }

            QJsonObject bookData = QJsonDocument::fromJson(bookReply->readAll()).object().value("fields").toObject();
            int available = bookData.value("available").toObject().value("integerValue").toString().toInt();
            QString bookTitle = bookData.value("title").toObject().value("stringValue").toString();

            if (available <= 0) {
                callback(false, "Book is currently out of stock!");
                bookReply->deleteLater(); return;
            }


            QUrl borrowUrl("https://firestore.googleapis.com/v1/projects/pocketlib-ea41d/databases/(default)/documents/borrowed_books");
            QNetworkRequest borrowReq(borrowUrl);
            borrowReq.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
            borrowReq.setRawHeader("Authorization", ("Bearer " + token).toUtf8());

            QJsonObject borrowFields;
            borrowFields["bookId"] = QJsonObject{{"stringValue", bookId}};
            borrowFields["bookName"] = QJsonObject{{"stringValue", bookTitle}};
            borrowFields["userUID"] = QJsonObject{{"stringValue", studentUID}};
            borrowFields["userName"] = QJsonObject{{"stringValue", studentName}};
            borrowFields["borrowDate"] = QJsonObject{{"stringValue", QDate::currentDate().toString(Qt::ISODate)}};
            borrowFields["dueDate"] = QJsonObject{{"stringValue", QDate::currentDate().addDays(14).toString(Qt::ISODate)}};
            borrowFields["status"] = QJsonObject{{"stringValue", "Active"}};
            borrowFields["issuedBy"] = QJsonObject{{"stringValue", adminUID}};

            QJsonObject borrowDoc{{"fields", borrowFields}};


            networkManager.post(borrowReq, QJsonDocument(borrowDoc).toJson());


            QUrl updateBookUrl("https://firestore.googleapis.com/v1/projects/pocketlib-ea41d/databases/(default)/documents/books/" + bookId + "?updateMask.fieldPaths=available");
            QNetworkRequest updateReq(updateBookUrl);
            updateReq.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
            updateReq.setRawHeader("Authorization", ("Bearer " + token).toUtf8());

            QJsonObject updateFields;
            updateFields["available"] = QJsonObject{{"integerValue", QString::number(available - 1)}};
            QJsonObject updateDoc{{"fields", updateFields}};


            networkManager.sendCustomRequest(updateReq, "PATCH", QJsonDocument(updateDoc).toJson());


            callback(true, "Book issued successfully to " + studentName + "!");
            bookReply->deleteLater();
        });

        reply->deleteLater();
    });
}
