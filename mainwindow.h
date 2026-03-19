#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "auth/AuthManager.h"
#include "firestore/FirestoreClient.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

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

    void on_back_btn_clicked();

    void on_addBook_btn_clicked();

    void on_profile_btn_clicked();

    void on_change_name_btn_clicked();

private:
    Ui::MainWindow *ui;

    QString currentUserName;
    QString currentRole;

    AuthManager *authManager;
    FirestoreClient *firestoreClient;

    QString currentToken;
    QString currentUID;

    void openDashboard(QString role);
};

#endif
