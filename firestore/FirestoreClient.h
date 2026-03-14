#ifndef FIRESTORECLIENT_H
#define FIRESTORECLIENT_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QJsonArray>

class FirestoreClient : public QObject
{
    Q_OBJECT

public:
    explicit FirestoreClient(QString projectId, QObject *parent = nullptr);

    void setAuthToken(QString token);

    void addBook(QString title, QString author, int copies);
    void getBooks();
    void deleteBook(QString documentId);

signals:
    void booksReceived(QJsonArray books);
    void requestSuccess(QString message);
    void requestError(QString error);

private:
    QString baseUrl;
    QString idToken;

    QNetworkAccessManager *manager;
};

#endif