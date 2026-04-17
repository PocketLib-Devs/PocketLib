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

void FirestoreClient::updateBook(const Book    &book,
                                 const QString &token,
                                 std::function<void(bool success)> callback)
{
    if (book.id.isEmpty()) {
        qDebug() << "updateBook: book.id must not be empty";
        callback(false);
        return;
    }

    // Build the updateMask query string so Firestore only touches these fields
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
                // Firestore returns an empty JSON object {} on successful delete
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
                    callback(books);           // return empty list
                    reply->deleteLater();
                    return;
                }

                // Firestore returns: { "documents": [ { "name":..., "fields":... }, ... ] }
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
    // 1. Setup the File
    QFile *file = new QFile(localFilePath);
    if (!file->open(QIODevice::ReadOnly)) {
        qDebug() << "Could not open image file:" << localFilePath;
        callback("");
        delete file;
        return;
    }

    // Replace these with YOUR actual Cloud Name and Preset Name!
    QString cloudName = "dtg3gx9fx";
    QString uploadPreset = "pocketlib_covers";

    QString url = "https://api.cloudinary.com/v1_1/" + cloudName + "/image/upload";
    QNetworkRequest request((QUrl(url)));

    // 2. Create the MultiPart form data
    QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

    // Part A: The Upload Preset (Tells Cloudinary where to put it)
    QHttpPart presetPart;
    presetPart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"upload_preset\""));
    presetPart.setBody(uploadPreset.toUtf8());
    multiPart->append(presetPart);

    // Part B: The Image File itself
    QHttpPart imagePart;
    QFileInfo fileInfo(localFilePath);
    // Give the file a name in the form data
    imagePart.setHeader(QNetworkRequest::ContentDispositionHeader,
                        QVariant(QString("form-data; name=\"file\"; filename=\"%1\"").arg(fileInfo.fileName())));
    imagePart.setBodyDevice(file);
    file->setParent(multiPart); // Ensure the file is deleted when the multiPart is deleted
    multiPart->append(imagePart);

    // 3. Send the POST Request
    QNetworkReply *reply = networkManager.post(request, multiPart);

    // Attach the multiPart to the reply so it isn't deleted before the upload finishes
    multiPart->setParent(reply);

    // 4. Handle the Response
    connect(reply, &QNetworkReply::finished, [reply, callback]() {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray response = reply->readAll();
            QJsonDocument jsonDoc = QJsonDocument::fromJson(response);

            // Cloudinary returns the public, permanent URL as "secure_url"
            QString secureUrl = jsonDoc.object().value("secure_url").toString();
            qDebug() << "Upload Successful! URL:" << secureUrl;
            callback(secureUrl);
        } else {
            qDebug() << "Upload Failed:" << reply->errorString();
            qDebug() << reply->readAll(); // Prints the exact error from Cloudinary
            callback("");
        }
        reply->deleteLater();
    });
}


void FirestoreClient::adminCheckoutBook(QString studentLibraryId, QString bookId, QString token, QString adminUID, std::function<void(bool, QString)> callback)
{
    // STEP 1: Query the user by libraryId
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

    // Use the class's built-in networkManager
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

        // STEP 2: Fetch the book to check stock
        QUrl bookUrl("https://firestore.googleapis.com/v1/projects/pocketlib-ea41d/databases/(default)/documents/books/" + bookId);
        QNetworkRequest bookReq(bookUrl);
        bookReq.setRawHeader("Authorization", ("Bearer " + token).toUtf8());

        // Use the class's built-in networkManager again
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

            // STEP 3: Write the new Borrow Record
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

            // Fire and forget the post
            networkManager.post(borrowReq, QJsonDocument(borrowDoc).toJson());

            // STEP 4: Decrement the Book Stock
            QUrl updateBookUrl("https://firestore.googleapis.com/v1/projects/pocketlib-ea41d/databases/(default)/documents/books/" + bookId + "?updateMask.fieldPaths=available");
            QNetworkRequest updateReq(updateBookUrl);
            updateReq.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
            updateReq.setRawHeader("Authorization", ("Bearer " + token).toUtf8());

            QJsonObject updateFields;
            updateFields["available"] = QJsonObject{{"integerValue", QString::number(available - 1)}};
            QJsonObject updateDoc{{"fields", updateFields}};

            // Fire and forget the patch
            networkManager.sendCustomRequest(updateReq, "PATCH", QJsonDocument(updateDoc).toJson());

            // Finally, trigger success
            callback(true, "Book issued successfully to " + studentName + "!");
            bookReply->deleteLater();
        });

        reply->deleteLater();
    });
}
