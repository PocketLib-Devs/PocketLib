#ifndef FIRESTORECLIENT_H
#define FIRESTORECLIENT_H


#include <QObject>
#include <QNetworkAccessManager>
#include <functional>
#include <QJsonArray>
#include <QList>
#include "Book.h"
#include "UserInfo.h"
#include <QHttpMultiPart>
#include <QHttpPart>
#include <QFile>
#include <QFileInfo>

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

    void adminCheckoutBook(QString studentLibraryId, QString bookId, QString token, QString adminUID, std::function<void(bool, QString)> callback);

    void deleteBook(QString documentId, QString idToken);

    void fetchBorrowedBooks(QString token, std::function<void(QJsonArray)> callback);

    void updateFineInFirestore(QString uid, int fineAmount, QString token);

    void getFineAmount(QString uid, QString token, std::function<void(int)> callback);

    void updateBook(const Book &book, const QString &token,
                    std::function<void(bool success)> callback);

    void removeBook(const QString &bookId, const QString &token,
                    std::function<void(bool success)> callback);

    void getAllBooks(const QString &token,
                     std::function<void(QList<Book> books)> callback);

    void uploadImageToCloudinary(const QString &localFilePath, std::function<void(QString secureUrl)> callback);

    void addToMyBooks(const QString &uid, const Book &book,
                     const QString &token,
                     std::function<void(bool)> callback);

    void getMyBooks(const QString &uid, const QString &token,
                   std::function<void(QList<Book>)> callback);

    void removeFromMyBooks(const QString &uid, const QString &bookId,
                          const QString &token,
                          std::function<void(bool)> callback);

signals:
    void requestError(QString errorMessage);
    void requestSuccess(QString successMessage);

    void deleteBook(QString documentId);


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

    QString baseUrl;
    QString idToken;

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
