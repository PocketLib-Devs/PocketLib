#ifndef FIRESTORECLIENT_H
#define FIRESTORECLIENT_H

#include <QObject>
#include <QNetworkAccessManager>
#include <functional>
#include <QJsonArray>
#include <QList>
#include "Book.h"

class FirestoreClient : public QObject
{
    Q_OBJECT

public:
    explicit FirestoreClient(QObject *parent = nullptr);

    void createUser(QString uid, QString email, QString role, QString token);

    void getUserRole(QString uid, QString token,
                     std::function<void(QString role)> callback);
    void addBook(const Book &book, const QString &token,
                 std::function<void(QString docId)> callback);


    //delete book function
    void deleteBook(QString documentId, QString idToken);



    void fetchBorrowedBooks(QString token, std::function<void(QJsonArray)> callback);

    void updateFineInFirestore(QString uid, int fineAmount, QString token);

    void getFineAmount(QString uid, QString token, std::function<void(int)> callback);

signals:
    void requestError(QString errorMessage);
    void requestSuccess(QString successMessage);

private:
    // ── helpers ─────────────────────────────────────────────────────────────
    /** Converts a Book struct → Firestore REST "fields" JSON object. */
    QJsonObject bookToFields(const Book &book) const;

    /** Converts a Firestore REST "fields" JSON object → Book struct. */
    Book        fieldsToBook(const QString &docName,
                      const QJsonObject &fields) const;
    QNetworkAccessManager networkManager;

    const QString projectId = "pocketlib-ea41d";
    QString booksBaseUrl() const
    {
        return "https://firestore.googleapis.com/v1/projects/"
               + projectId
               + "/databases/(default)/documents/books";
    }
};

#endif
