#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QMessageBox>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    authManager = new AuthManager(this);
    firestoreClient = new FirestoreClient(this);

    connect(authManager, &AuthManager::loginSuccess,
            this, [=](QString token, QString uid){

                qDebug() << "Login successful!";
                qDebug() << "UID:" << uid;

                currentToken = token;

                ui->stackedWidget->setCurrentWidget(ui->dashboardPage);

                firestoreClient->getBooks(currentToken);
            });

    connect(authManager, &AuthManager::loginFailed,
            this, [=](QString error){

                QMessageBox::warning(this,"Login Failed",error);

            });

    connect(firestoreClient, &FirestoreClient::booksReceived,
            this, [=](QString data){

                qDebug() << "Books data:";
                qDebug() << data;

            });

    connect(firestoreClient, &FirestoreClient::firestoreError,
            this, [=](QString error){

                qDebug() << "Firestore error:" << error;

            });
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_loginButton_clicked()
{
    QString email = ui->usernameBox->text();
    QString password = ui->passwordBox->text();

    if(email.isEmpty() || password.isEmpty())
    {
        QMessageBox::warning(this,"Error","Please enter credentials");
        return;
    }

    authManager->login(email, password);
}

void MainWindow::on_logoutButton_clicked()
{
    QMessageBox::StandardButton reply;

    reply = QMessageBox::question(this,
                                  "Logout",
                                  "Are you sure you want to logout?",
                                  QMessageBox::Yes | QMessageBox::No);

    if(reply == QMessageBox::Yes)
    {
        currentToken.clear();

        ui->usernameBox->clear();
        ui->passwordBox->clear();

        ui->stackedWidget->setCurrentWidget(ui->loginPage);
    }
}

void MainWindow::on_openRegisterButton_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->registerPage);
}

void MainWindow::on_backToLoginButton_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->loginPage);
}

void MainWindow::on_registerButton_clicked()
{
    QString email = ui->registerEmailBox->text();
    QString password = ui->registerPasswordBox->text();

    if(email.isEmpty() || password.isEmpty())
    {
        QMessageBox::warning(this,"Error","Enter all details");
        return;
    }

    authManager->registerUser(email,password);
}

