#ifndef AUTHMANAGER_H
#define AUTHMANAGER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>

class AuthManager : public QObject
{
    Q_OBJECT

public:
    explicit AuthManager(QObject *parent = nullptr);

    void login(const QString &email, const QString &password);
    void registerUser(QString email, QString password);

signals:
    void loginSuccess(QString idToken, QString uid);
    void loginFailed(QString error);

private:
    QNetworkAccessManager *manager;
    QString apiKey = "AIzaSyDd4x5PuwJi8Coi4ii6o93QbcYeBr_ArIg";
};

#endif
