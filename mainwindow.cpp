#include "mainwindow.h"
#include <QDebug>
#include <QMessageBox>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include "./ui_mainwindow.h"
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("pocketlib.db");

    if (!db.open()) {
        qDebug() << "CRITICAL ERROR: Database failed to open!";
        qDebug() << db.lastError().text();
        return;
    } else {
        qDebug() << "SUCCESS: PocketLIB Database is online!";
    }

    // 2. Create the Tables based on the PocketLIB requirements
    QSqlQuery query;

    // Table 1: Users (Handles login, roles, and points)
    query.exec("CREATE TABLE IF NOT EXISTS Users ("
               "LibraryID TEXT PRIMARY KEY, "
               "Password TEXT, "
               "Role TEXT, "
               "PointsBalance INTEGER DEFAULT 0)");

    // Table 2: Books (Handles inventory) [cite: 24, 25, 26, 53, 54]
    query.exec("CREATE TABLE IF NOT EXISTS Books ("
               "BookID INTEGER PRIMARY KEY AUTOINCREMENT, "
               "Title TEXT, "
               "Category TEXT, "
               "Quantity INTEGER, "
               "AvailableStatus TEXT)");

    // Table 3: Transactions (Handles borrowing and fines) [cite: 30, 34, 35, 58, 65]
    query.exec("CREATE TABLE IF NOT EXISTS Transactions ("
               "LoanID INTEGER PRIMARY KEY AUTOINCREMENT, "
               "LibraryID TEXT, "
               "BookID INTEGER, "
               "DueDate TEXT, "
               "FineAmount INTEGER DEFAULT 0)");

    query.exec("INSERT OR IGNORE INTO Users (LibraryID, Password, Role) "
               "VALUES ('Admin01', 'adminpass', 'Admin')");

    query.exec("INSERT OR IGNORE INTO Users (LibraryID, Password, Role) "
               "VALUES ('User01', 'userpass', 'Consumer')");
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_pushButton_clicked()
{
    // 1. Grab the text from the UI boxes
    QString enteredID = ui->usernameBox->text();
    QString enteredPassword = ui->passwordBox->text();

    // 2. Ask the database if this user exists
    QSqlQuery query;
    // We use placeholders (:) to safely pass variables into SQL
    query.prepare("SELECT Role FROM Users WHERE LibraryID = :id AND Password = :password");
    query.bindValue(":id", enteredID);
    query.bindValue(":password", enteredPassword);

    // 3. Run the search
    if (query.exec()) {
        if (query.next()) {
            // SUCCESS! The database found a matching row.
            QString userRole = query.value(0).toString(); // Grab the 'Role' text

            // 4. Route to the correct screen based on the Role
            if (userRole == "Admin") {
                ui->stackedWidget->setCurrentIndex(1); // Go to Admin Dashboard
            } else if (userRole == "Consumer") {
                ui->stackedWidget->setCurrentIndex(2); // Go to User Dashboard
            }

        } else {
            // FAILED! No match was found.
            QMessageBox::warning(this, "Login Failed", "Incorrect Library ID or Password.");
        }
    } else {
        qDebug() << "Database query error!";
    }
}
