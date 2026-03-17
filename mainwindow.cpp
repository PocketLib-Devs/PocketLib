#include "mainwindow.h"

#include <QMessageBox>
#include <QDebug>
#include "ui_mainwindow.h"
#include "config.h"
#include <QProgressDialog>
#include <QThread>


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

                               this->currentToken = token;
                               this->currentUID = uid;

                               if (token.isEmpty()) {
                                   QMessageBox::warning(this, "Login Failed", "Invalid email or password");
                                   return;
                               }
                               currentToken = token;
                               currentUID   = uid;

                               firestoreClient->getUserRole(uid, token, [this](QString role) {
                                   if (role == "admin") {
                                       ui->stackedWidget->setCurrentWidget(ui->adminDashboardPage);
                                   }
                                   else if (role == "student") {
                                       ui->stackedWidget->setCurrentWidget(ui->studentDashboardPage);
                                       checkStudentFines(); // Triggers the bell check
                                   }
                               });
                           }
                           );
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
//////////////////////////////////////////////////////////////
// OPEN USER MONITORING PAGE
//////////////////////////////////////////////////////////////

void MainWindow::on_userMonitoring_btn_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->userMonitoringPage);


        // Fetch the books and send them to the table!
        firestoreClient->fetchBorrowedBooks(currentToken, [this](QJsonArray books) {
            populateMonitoringTable(books);
        });
    }


//////////////////////////////////////////////////////////////
// BACK FROM USER MONITORING
//////////////////////////////////////////////////////////////

void MainWindow::on_backFromMonitoring_btn_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->adminDashboardPage);
}

void MainWindow::on_notificationBell_clicked()
{
    firestoreClient->getFineAmount(currentUID, currentToken, [this](int fine) {
        if (fine > 0) {
            processMockPayment(fine); // This is the simulation we wrote earlier
        } else {
            QMessageBox::information(this, "Notifications", "Your library record is clear!");
        }
    });
}

void MainWindow::populateMonitoringTable(QJsonArray books)
{
    ui->monitoringTable->setRowCount(0);

    for (int i = 0; i < books.size(); ++i) {
        QJsonObject fields = books[i].toObject().value("fields").toObject();

        // Extracting data (ensure these keys match your Firestore names)
        QString userUID = fields.value("userUID").toObject().value("stringValue").toString();
        QString userName = fields.value("userName").toObject().value("stringValue").toString();
        QString bookName = fields.value("bookName").toObject().value("stringValue").toString();
        QString borrowDate = fields.value("borrowDate").toObject().value("stringValue").toString();
        QString returnDate = fields.value("dueDate").toObject().value("stringValue").toString();

        int row = ui->monitoringTable->rowCount();
        ui->monitoringTable->insertRow(row);

        // Fill 6 columns (Indices 0 to 5)
        ui->monitoringTable->setItem(row, 0, new QTableWidgetItem(QString::number(i + 1))); // Sr. No.
        ui->monitoringTable->setItem(row, 1, new QTableWidgetItem(userName));
        ui->monitoringTable->setItem(row, 2, new QTableWidgetItem(bookName));
        ui->monitoringTable->setItem(row, 3, new QTableWidgetItem(borrowDate));
        ui->monitoringTable->setItem(row, 4, new QTableWidgetItem(returnDate));

        QPushButton *fineBtn = new QPushButton("Impose Fine");

        // Logic check: Is it overdue?
        QDate dueDate = QDate::fromString(returnDate, Qt::ISODate);
        if (QDate::currentDate() > dueDate) {
            fineBtn->setEnabled(true);
            fineBtn->setStyleSheet("background-color: #dc3545; color: white;");
        } else {
            fineBtn->setEnabled(false);
        }

        // Connect button to the NEW Firestore function
        connect(fineBtn, &QPushButton::clicked, [=]() {
            int fineValue = 50; // Set your standard fine amount
            firestoreClient->updateFineInFirestore(userUID, fineValue, authManager->getCurrentToken());
            QMessageBox::information(this, "Action Taken", "Fine of ₹" + QString::number(fineValue) + " imposed.");
        });

        ui->monitoringTable->setCellWidget(row, 5, fineBtn); // Column 5 is "Action"
    }
}
void MainWindow::checkStudentFines()
{
    // Use the current UID and Token stored during login
    firestoreClient->getFineAmount(currentUID, currentToken, [this](int fine) {
        if (fine > 0) {
            QMessageBox msgBox;
            msgBox.setWindowTitle("Library Notification");
            msgBox.setText("<b>Penalty Alert!</b>");
            msgBox.setInformativeText("You have a pending fine of ₹" + QString::number(fine) +
                                      " for an overdue book.");
            msgBox.setIcon(QMessageBox::Warning);

            QPushButton *payBtn = msgBox.addButton("Pay Fine (Simulation)", QMessageBox::ActionRole);
            msgBox.addButton("Close", QMessageBox::RejectRole);

            msgBox.exec();

            if (msgBox.clickedButton() == payBtn) {
                processMockPayment(fine);
            }
        }
    });
}
void MainWindow::processMockPayment(int amount)
{
    // 1. Show a fake processing dialog
    QProgressDialog progress("Connecting to Razorpay...", "Cancel", 0, 100, this);
    progress.setWindowModality(Qt::WindowModal);
    progress.show();

    // Simulate a 2-second delay
    QThread::msleep(2000);
    progress.setValue(100);

    // 2. Tell Firebase the fine is cleared!
    firestoreClient->updateFineInFirestore(currentUID, 0, currentToken);

    QMessageBox::information(this, "Payment Successful", "₹" + QString::number(amount) +
                                                             " paid successfully. Your record is now clear.");
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


// This function is triggered when the "Back" button on the Book View Page is clicked.
// It switches the stackedWidget back to the student dashboard page.
void MainWindow::on_backButton_clicked()
{
    // stackedWidget contains all the pages of the application.
    // setCurrentWidget() changes the visible page.
    ui->stackedWidget->setCurrentWidget(ui->studentDashboardPage);
}


// This function is responsible for opening the Book View Page
// and filling it with the details of the selected book.
void MainWindow::bookViewPage(QString title,
                              QString author,
                              QString category,
                              QString rating,
                              QString description)
{
    // Set the book title label with the title passed to the function
    ui->bookTitleLabel->setText(title);

    // Set the author label
    ui->bookAuthorLabel->setText(author);

    // Set the category label
    ui->bookCategoryLabel->setText(category);

    // Set the rating label
    ui->bookRatingLabel->setText(rating);

    // Set the book description text box
    ui->bookDescriptionText->setText(description);

    // Change the current visible page of the stackedWidget
    // to the Book View Page so the user can see the details
    ui->stackedWidget->setCurrentWidget(ui->bookViewPage);
}
