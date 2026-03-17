#ifndef AUTHMANAGER_H
#define AUTHMANAGER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QJsonObject>
#include <functional>

class AuthManager : public QObject
{
    Q_OBJECT

public:
    explicit AuthManager(QObject *parent = nullptr);

    void loginUser(QString email, QString password,
                   std::function<void(QString token, QString uid)> callback);

    void registerUser(QString email, QString password,
                      std::function<void(QString token, QString uid)> callback);

private:
    QNetworkAccessManager networkManager;

    const QString apiKey = "AIzaSyDd4x5PuwJi8Coi4ii6o93QbcYeBr_ArIg";
};

#endif
