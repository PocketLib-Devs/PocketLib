#ifndef FIRESTORECLIENT_H
#define FIRESTORECLIENT_H

#include <QObject>
#include <QNetworkAccessManager>

class FirestoreClient : public QObject
{
    Q_OBJECT

public:
    explicit FirestoreClient(QObject *parent = nullptr);

    void getBooks(const QString &idToken);

signals:
    void booksReceived(QString data);
    void firestoreError(QString error);

private:
    QNetworkAccessManager *manager;
    QString projectId = "pocketlib-ea41d";
};

#endif
