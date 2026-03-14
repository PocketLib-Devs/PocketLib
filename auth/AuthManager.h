#ifndef AUTHMANAGER_H
#define AUTHMANAGER_H

#include <QObject>
#include <QNetworkAccessManager>

class AuthManager : public QObject
{
    Q_OBJECT

public:
    explicit AuthManager(QString apiKey, QObject *parent = nullptr);

    void login(QString email, QString password);
    QString getToken();

signals:
    void loginSuccess();
    void loginFailed(QString error);

private:
    QString apiKey;
    QString idToken;
    QNetworkAccessManager *manager;
};

#endif