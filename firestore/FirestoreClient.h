#ifndef FIRESTORECLIENT_H
#define FIRESTORECLIENT_H

#include <QObject>
#include <QNetworkAccessManager>
#include <functional>

class FirestoreClient : public QObject
{
    Q_OBJECT

public:
    explicit FirestoreClient(QObject *parent = nullptr);

    void createUser(QString uid, QString email, QString role, QString token);

    void getUserRole(QString uid, QString token,
                     std::function<void(QString role)> callback);

    void deleteBook(QString documentId);

signals:
    void requestSuccess(QString message);
    void requestError(QString error);

private:
    QNetworkAccessManager networkManager;

    QString baseUrl;
    QString idToken;

    const QString projectId = "pocketlib-ea41d";
};

#endif