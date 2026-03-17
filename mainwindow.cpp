#include "mainwindow.h"

#include <QMessageBox>
#include <QDebug>
#include "ui_mainwindow.h"
#include "config.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{

    

    ui->setupUi(this);

    // Start app at login screen
    ui->stackedWidget->setCurrentWidget(ui->loginPage);
    // 1. Initialize Firebase Managers instead of SQLite
    authManager = new AuthManager(this);
    firestoreClient = new FirestoreClient(this);
    ui->widget->hide();

}

MainWindow::~MainWindow()
{
    delete ui;
}

//////////////////////////////////////////////////////////////
// LOGIN
//////////////////////////////////////////////////////////////

void MainWindow::on_loginButton_clicked()
{
    QString email = ui->loginEmail->text();
    QString password = ui->loginPassword->text();

    authManager->loginUser(email, password,
                           [this](QString token, QString uid)
                           {
                               // If login failed
                               if (token.isEmpty()) {
                                   QMessageBox::warning(this, "Login Failed", "Invalid email or password");
                                   return;
                               }
                               currentToken = token;
                               currentUID   = uid;

                               // If login succeeded → check role from Firestore
                               firestoreClient->getUserRole(uid, token,
                                                            [this](QString role)
                                                            {
                                                                if (role == "admin") {
                                                                    ui->stackedWidget->setCurrentWidget(ui->adminDashboardPage);
                                                                }
                                                                else if (role == "student") {
                                                                    ui->stackedWidget->setCurrentWidget(ui->studentDashboardPage);
                                                                }
                                                            });
                           });
}

//////////////////////////////////////////////////////////////
// REGISTER PAGE OPEN
//////////////////////////////////////////////////////////////

void MainWindow::on_openRegisterButton_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->registerPage);
}

//////////////////////////////////////////////////////////////
// BACK TO LOGIN
//////////////////////////////////////////////////////////////

void MainWindow::on_backToLoginButton_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->loginPage);
}

//////////////////////////////////////////////////////////////
// REGISTER USER
//////////////////////////////////////////////////////////////

void MainWindow::on_registerButton_clicked()
{
    QString email = ui->registerEmail->text();
    QString password = ui->registerPassword->text();

    if(email.isEmpty() || password.isEmpty())
    {
        QMessageBox::warning(this,"Error","Enter email and password");
        return;
    }

    authManager->registerUser(email, password,
                      [this, email](QString token, QString uid)
                      {
                          // Create user record in Firestore
                          firestoreClient->createUser(uid, email, "student", token);

                          QMessageBox::information(this,"Success","Account created successfully!");

                          ui->stackedWidget->setCurrentIndex(0);
                      }
                      );
}

//////////////////////////////////////////////////////////////
// OPEN DASHBOARD BASED ON ROLE
//////////////////////////////////////////////////////////////

void MainWindow::openDashboard(QString role)
{
    if(role == "admin")
    {
        ui->stackedWidget->setCurrentWidget(ui->adminDashboardPage);
    }
    else
    {
        ui->stackedWidget->setCurrentWidget(ui->studentDashboardPage);
    }
}

//////////////////////////////////////////////////////////////
// LOGOUT (STUDENT)
//////////////////////////////////////////////////////////////

void MainWindow::on_logoutStudent_clicked()
{
    currentToken.clear();
    currentUID.clear();

    ui->loginEmail->clear();
    ui->loginPassword->clear();

    ui->stackedWidget->setCurrentWidget(ui->loginPage);
}

//////////////////////////////////////////////////////////////
// LOGOUT (ADMIN)
//////////////////////////////////////////////////////////////

void MainWindow::on_logoutAdmin_clicked()
{
    currentToken.clear();
    currentUID.clear();

    ui->loginEmail->clear();
    ui->loginPassword->clear();

    ui->stackedWidget->setCurrentWidget(ui->loginPage);
}

void MainWindow::on_sidebar_btn_clicked()
{
    bool isSidebarVisible = ui->widget->isVisible();
    if(!isSidebarVisible) ui->widget->setVisible(true);
    else ui->widget->setVisible(false);
}


void MainWindow::on_addBooks_btn_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->addRemove_page);
}


void MainWindow::on_back_btn_clicked()
{
    if(ui->addRemove_page->isVisible()) ui->stackedWidget->setCurrentWidget(ui->adminDashboardPage);
    else ui->stackedWidget->setCurrentWidget(ui->studentDashboardPage);
}

//example
void MainWindow::on_addBook_btn_clicked()
{
    qDebug() << "Token being sent:" << currentToken.left(30) << "...";
    qDebug() << "Token length:" << currentToken.length();

    if (currentToken.isEmpty()) {
        QMessageBox::warning(this, "Auth Error", "No token — please log in first.");
        return;
    }
    Book book;
    // book.id left empty → a UUID will be generated automatically
    book.title       = ui->bookName_in->text();
    book.author      = ui->author_in->text();
    book.category    = ui->categ_in->text();
    book.coverUrl    = ui->lineEdit_coverUrl->text();
    book.description = ui->textEdit_description->toPlainText();
    book.rating      = ui->doubleSpinBox_rating->value();
    book.section     = ui->lineEdit_section->text();
    book.available   = ui->checkBox->isChecked();

    firestoreClient->addBook(book, currentToken,
                            [this](QString docId)
                            {
                                if (docId.isEmpty()) {
                                    QMessageBox::warning(this, "Error", "Failed to add book.");
                                } else {
                                    QMessageBox::information(this, "Success",
                                                             "Book added! ID: " + docId);
                                }
                            });
}
