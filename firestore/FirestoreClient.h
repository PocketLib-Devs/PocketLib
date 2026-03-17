#ifndef FIRESTORECLIENT_H
#define FIRESTORECLIENT_H

#include <QObject>
#include <QNetworkAccessManager>
#include <functional>
#include <QJsonArray>

class FirestoreClient : public QObject
{
    Q_OBJECT

public:
    explicit FirestoreClient(QObject *parent = nullptr);

    void createUser(QString uid, QString email, QString role, QString token);

    void getUserRole(QString uid, QString token,
                     std::function<void(QString role)> callback);

    //delete book function
    void deleteBook(QString documentId, QString idToken);



    void fetchBorrowedBooks(QString token, std::function<void(QJsonArray)> callback);

    void updateFineInFirestore(QString uid, int fineAmount, QString token);

    void getFineAmount(QString uid, QString token, std::function<void(int)> callback);

signals:
    void requestError(QString errorMessage);
    void requestSuccess(QString successMessage);

private:
    QNetworkAccessManager networkManager;

    const QString projectId = "pocketlib-ea41d";
};

#endif
