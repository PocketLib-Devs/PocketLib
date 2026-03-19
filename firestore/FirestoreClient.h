#ifndef FIRESTORECLIENT_H
#define FIRESTORECLIENT_H


#include <QObject>
#include <QNetworkAccessManager>
#include <functional>
#include <QList>
#include "Book.h"
#include "UserInfo.h"

class FirestoreClient : public QObject
{
    Q_OBJECT

public:
    explicit FirestoreClient(QObject *parent = nullptr);
    
    void createUser(const UserInfo &user, const QString &token);


    void getUserRole(QString uid, QString token,
                     std::function<void(QString role)> callback);

    void getUserInfo(const QString &uid, const QString &token,
                     std::function<void(UserInfo userInfo)> callback);

    void addBook(const Book &book, const QString &token,
                 std::function<void(QString docId)> callback);

    void generateUniqueLibraryId(const QString &name, const QString &token,
                                 std::function<void(QString libraryId)> callback);

    void updateFineAmount(const QString &uid, int amount, const QString &token,
                          std::function<void(bool success)> callback);

    void updateUserName(const QString &uid,
                        const QString &newName,
                        const QString &token,
                        std::function<void(bool success)> callback);



private:

    QJsonObject bookToFields(const Book &book) const;

    Book        fieldsToBook(const QString &docName,
                      const QJsonObject &fields) const;

    QString     buildCandidateId(const QString &name) const;
    void        checkLibraryIdExists(const QString &candidateId,
                              const QString &token,
                              std::function<void(bool exists)> callback);
    void        tryGenerateId(const QString &name, const QString &token,
                       int attemptsLeft,
                       std::function<void(QString)> callback);
    QNetworkAccessManager networkManager;

    const QString projectId = "pocketlib-ea41d";
    QString booksBaseUrl() const
    {
        return "https://firestore.googleapis.com/v1/projects/"
               + projectId
               + "/databases/(default)/documents/books";
    }
    QString usersBaseUrl() const
    {
        return "https://firestore.googleapis.com/v1/projects/"
               + projectId + "/databases/(default)/documents/users";
    }
    QString runQueryUrl() const
    {
        return "https://firestore.googleapis.com/v1/projects/"
               + projectId + "/databases/(default)/documents:runQuery";
    }
};

#endif
