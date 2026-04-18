#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "auth/AuthManager.h"
#include "firestore/FirestoreClient.h"
#include <QListWidgetItem>
#include <QIcon>
#include <QSet>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void bookViewPage(QString title,
                      QString author,
                      QString category,
                      QString rating,
                      QString description,
                      QIcon coverIcon,
                      QString bookId, QString coverUrl, QString Qty);

private slots:

    void handleSharedAction();

    void on_loginButton_clicked();
    void on_openRegisterButton_clicked();
    void on_registerButton_clicked();
    void on_backToLoginButton_clicked();

    void on_logoutStudent_clicked();
    void on_logoutAdmin_clicked();


    void on_sidebar_btn_clicked();

    void on_addBooks_btn_clicked();
    void on_userMonitoring_btn_clicked();
    void on_backFromMonitoring_btn_clicked();
    void on_notificationBell_clicked();

    void on_addBook_btn_clicked();

    void on_profile_btn_clicked();

    void on_change_name_btn_clicked();
    void on_backButton_clicked();

    void on_update_btn_clicked();

    void on_remove_btn_clicked();

    void on_inventory_btn_clicked();

    void on_tableWidget_books_cellClicked(int row, int column);

    void on_update_to_add_btn_clicked();
    void on_search_btn_clicked();
    void on_backFromSearch_btn_clicked();

    void on_searchLineEdit_textChanged(const QString &arg1);
    void on_genreFilterCombo_currentTextChanged(const QString &arg1);
    void on_bookCoverGrid_itemClicked(QListWidgetItem *item);
    void on_browseImage_btn_clicked();
    void on_forgotPassword_btn_clicked();
    void on_changepsd_btn_clicked();
    void on_pushButton_clicked();
    void on_checkout_back_btn_clicked();
    void on_checkout_issue_btn_clicked();

    void on_addToMyBooks_btn_clicked();

    void on_myBooks_btn_clicked();

    void on_myBooksGrid_itemClicked(QListWidgetItem *item);

    void on_rmbook_btn_clicked();

    void on_View_btn_clicked();

private:
    Ui::MainWindow *ui;

    QString currentUserName;
    QString currentRole;

    QString selectedBookId;

    AuthManager *authManager;
    FirestoreClient *firestoreClient;

    QString currentToken;
    QString currentUID;
    void populateMonitoringTable(QJsonArray books);
    void openDashboard(QString role);
    void checkStudentFines();
    void processMockPayment(int amount);
    void filterBooks();
    void populateBookGrid(QList<Book> books);
    QString     currentBookId;
    QString     currentCoverUrl;
    QSet<QString> savedBookIds;
    void on_myBooksGrid_customContextMenuRequested(const QPoint &pos);
    void loadMyBooksGrid();

};

#endif
