#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "config.h"
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // 1. Initialize Firebase Managers instead of SQLite
    authManager = new AuthManager(FIREBASE_API_KEY, this);
    dbClient = new FirestoreClient(FIREBASE_PROJECT_ID, this);

    // 2. Connect the AuthManager internet signals to your local slots
    connect(authManager, &AuthManager::loginSuccess, this, &MainWindow::onLoginSuccess);
    connect(authManager, &AuthManager::loginFailed, this, &MainWindow::onLoginFailed);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_pushButton_clicked()
{
    // 1. Grab the text from the UI boxes
    // IMPORTANT: Firebase requires an EMAIL (e.g., admin@pocketlib.com), not just "Admin01"
    QString enteredEmail = ui->usernameBox->text();
    QString enteredPassword = ui->passwordBox->text();

    // 2. Ask Firebase to authenticate this user over the internet
    // This function will run, and the app will keep working while it waits for a reply.
    authManager->login(enteredEmail, enteredPassword);
}

// 3. This runs automatically when Firebase says "Yes, password is correct!"
void MainWindow::onLoginSuccess()
{
    // Give the database client the security token so it can fetch books later
    dbClient->setAuthToken(authManager->getToken());

    // Route to the dashboard (Using the UI name from your screenshot)
    ui->stackedWidget->setCurrentIndex(1);
}

// 4. This runs automatically if the password/email is wrong
void MainWindow::onLoginFailed(QString error)
{
    QMessageBox::warning(this, "Login Failed", "Incorrect Email or Password.\n" + error);
}
