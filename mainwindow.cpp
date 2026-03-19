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
    connect(ui->back_btn, &QPushButton::clicked, this, &MainWindow::handleSharedAction);
    connect(ui->back_btn_2, &QPushButton::clicked, this, &MainWindow::handleSharedAction);

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
                                                                currentRole = role;
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
    QString name     = ui->registerName->text();

    if (name.isEmpty()) {
        QMessageBox::warning(this, "Missing Name", "Please enter your name.");
        return;
    }

    if(email.isEmpty() || password.isEmpty())
    {
        QMessageBox::warning(this,"Error","Enter email and password");
        return;
    }

    authManager->registerUser(email, password,
                              [this, name, email](QString token, QString uid)
                       {
        if (token.isEmpty() || uid.isEmpty()) {
            QMessageBox::warning(this, "Registration Failed",
                                 "Could not create account. "
                                 "Email may already be in use.");
            return;
        }

        // ── 4. Save token & uid for this session ──────────────────────────
        currentToken = token;
        currentUID   = uid;
        firestoreClient->generateUniqueLibraryId(name, token,
        [this, name, email, token, uid]
        (QString libraryId)
        {
            if (libraryId.isEmpty()) {
                QMessageBox::warning(this, "Error",
                                     "Could not generate a Library ID. "
                                     "Please try again.");
                ui->registerButton->setEnabled(true);
                return;
            }

            // ── 5. Build UserInfo and save to Firestore ────────────────────
            UserInfo newUser;
            newUser.uid        = uid;
            newUser.name       = name;
            newUser.email      = email;
            newUser.role       = "user";          // default role
            newUser.libraryId  = libraryId;
            newUser.fineAmount = 0;               // starts at zero

            firestoreClient->createUser(newUser, token);

            // ── 6. Show success & navigate ─────────────────────────────────
            QMessageBox::information(
                this, "Welcome to PocketLib!",
                "Account created!\n\n"
                "Name: "       + name      + "\n"
                             "Library ID: " + libraryId + "\n\n"
                                  "Please keep your Library ID safe."
                );

            ui->registerButton->setEnabled(true);

            // Navigate to home page — adjust index to match your stackedWidget
            // ui->stackedWidget->setCurrentIndex(1);

                                                // Optionally pre-load the user info into the UI right away
            });
    });

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
    currentRole = role;
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


void MainWindow::handleSharedAction() {
    if(currentRole=="admin") ui->stackedWidget->setCurrentWidget(ui->adminDashboardPage);
    else ui->stackedWidget->setCurrentWidget(ui->studentDashboardPage);
}


void MainWindow::on_addBook_btn_clicked()
{
    qDebug() << "Token being sent:" << currentToken.left(30) << "...";
    qDebug() << "Token length:" << currentToken.length();

    if (currentToken.isEmpty()) {
        QMessageBox::warning(this, "Auth Error", "No token — please log in first.");
        return;
    }
    Book book;

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


void MainWindow::on_profile_btn_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->profile_page);

firestoreClient->getUserInfo(currentUID, currentToken,
                                [this](UserInfo info)
    {
        // Guard: if uid is empty the fetch failed
        if (info.uid.isEmpty()) {
            QMessageBox::warning(this, "Error",
                                 "Could not load user information.");
            return;
        }
        ui->name_label->setText(info.name);
        ui->email_label->setText(info.email);
        ui->role_label->setText(info.role);
        ui->libid_label->setText(info.libraryId);
        ui->fine_label->setText("₹ " + QString::number(info.fineAmount));
        currentUserName = info.name;

        // Optional: highlight fine in red if > 0
        if (info.fineAmount > 0) {
            ui->fine_label->setStyleSheet("color: red; font-weight: bold;");
        } else {
            ui->fine_label->setStyleSheet("color: green;");
        }
    });
}


void MainWindow::on_change_name_btn_clicked()
{
    QString newName = ui->changename_in->text();
    ui->change_name_btn->setEnabled(false);
    ui->change_name_btn->setText("Updating…");
    firestoreClient->updateUserName(currentUID, newName, currentToken,
                                   [this, newName](bool success)
                                   {
                                       // Re-enable button regardless of outcome
                                       ui->change_name_btn->setEnabled(true);
                                       ui->change_name_btn->setText("Change Name");

                                       if (!success) {
                                           QMessageBox::warning(this, "Update Failed",
                                                                "Could not update your name. "
                                                                "Please check your connection and try again.");
                                           return;
                                       }

                                       // ── 6. Update in-memory state & all UI labels ──────────────────────
                                       currentUserName = newName;
                                       ui->name_label->setText(newName);   // profile page label

                                       QMessageBox::information(this, "Name Updated",
                                                                "Your name has been changed to:\n" + newName);
                                   });
}

