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
    void on_pushButton_clicked();

    void onLoginSuccess();
    void onLoginFailed(QString error);

private:
    Ui::MainWindow *ui;


    AuthManager *authManager;
    FirestoreClient *dbClient;
};

#endif
