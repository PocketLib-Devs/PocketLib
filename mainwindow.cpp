#include "mainwindow.h"

#include <QMessageBox>
#include <QDebug>
#include "ui_mainwindow.h"
#include "config.h"
#include <QProgressDialog>
#include <QThread>
#include <QFileDialog>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QPixmap>
#include <QUrl>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{

    ui->setupUi(this);
    connect(ui->back_btn, &QPushButton::clicked, this, &MainWindow::handleSharedAction);
    connect(ui->back_btn_2, &QPushButton::clicked, this, &MainWindow::handleSharedAction);
    connect(ui->back_btn_3, &QPushButton::clicked, this, &MainWindow::handleSharedAction);
    connect(ui->back_btn_4, &QPushButton::clicked, this, &MainWindow::handleSharedAction);

    connect(ui->seeMoreNew_btn, &QPushButton::clicked, this, [this]() {
        currentDashboardFilter = "new";
        openFilteredPage();
    });

    connect(ui->seeMoreTopRated_btn, &QPushButton::clicked, this, [this]() {
        currentDashboardFilter = "top";
        openFilteredPage();
    });

    connect(ui->seeMoreRecommended_btn, &QPushButton::clicked, this, [this]() {
        currentDashboardFilter = "recommended";
        openFilteredPage();
    });


    ui->stackedWidget->setCurrentWidget(ui->loginPage);

    authManager = new AuthManager(this);
    firestoreClient = new FirestoreClient(this);
    ui->widget->hide();
    ui->genreFilterCombo->addItem("All Genres");
    ui->genreFilterCombo->addItem("Sci-Fi");
    ui->genreFilterCombo->addItem("Engineering");
    ui->forgotPassword_btn->hide();

    connect(ui->dashboardSearch, &QLineEdit::textChanged,
            this, &MainWindow::handleDashboardSearch);

    ui->myBooksGrid->setContextMenuPolicy(Qt::CustomContextMenu);

    ui->sidebarWidget->setGeometry(-250, 0, 250, height());

    sidebarAnim = new QPropertyAnimation(ui->sidebarWidget, "geometry");
    sidebarAnim->setDuration(250);
    sidebarAnim->setEasingCurve(QEasingCurve::OutCubic);

    ui->hamburger_btn->raise();
    ui->hamburger_btn->setAttribute(Qt::WA_TransparentForMouseEvents, false);


}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_loginButton_clicked()
{
    QString email = ui->loginEmail->text();
    QString password = ui->loginPassword->text();

    authManager->loginUser(email, password,
                           [this](QString token, QString uid)
                           {

                               if (token.isEmpty()) {
                                   QMessageBox::warning(this, "Login Failed", "Invalid email or password");

                                   ui->forgotPassword_btn->show();
                                   return;
                               }


                               ui->forgotPassword_btn->hide();


                               this->currentToken = token;
                               this->currentUID = uid;


                               firestoreClient->getUserRole(uid, token, [this](QString role)
                                                            {
                                                                if (role == "admin") {
                                                                    ui->stackedWidget->setCurrentWidget(ui->adminDashboardPage);
                                                                }
                                                                else if (role == "student") {
                                                                    ui->stackedWidget->setCurrentWidget(ui->studentDashboardPage);
                                                                    checkStudentFines(); // Triggers the bell check
                                                                    loadStudentDashboard();
                                                                }
                                                                currentRole = role;
                                                            });
                           });
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


            UserInfo newUser;
            newUser.uid        = uid;
            newUser.name       = name;
            newUser.email      = email;
            newUser.role       = "user";
            newUser.libraryId  = libraryId;
            newUser.fineAmount = 0;

            firestoreClient->createUser(newUser, token);


            QMessageBox::information(
                this, "Welcome to PocketLib!",
                "Account created!\n\n"
                "Name: "       + name      + "\n"
                             "Library ID: " + libraryId + "\n\n"
                                  "Please keep your Library ID safe."
                );

            ui->registerButton->setEnabled(true);


            });
    });

}



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



void MainWindow::on_logoutStudent_clicked()
{
    currentToken.clear();
    currentUID.clear();

    ui->loginEmail->clear();
    ui->loginPassword->clear();

    ui->stackedWidget->setCurrentWidget(ui->loginPage);
}



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


void MainWindow::on_userMonitoring_btn_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->userMonitoringPage);



        firestoreClient->fetchBorrowedBooks(currentToken, [this](QJsonArray books) {
            populateMonitoringTable(books);
        });
    }




void MainWindow::on_backFromMonitoring_btn_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->adminDashboardPage);
}

void MainWindow::on_notificationBell_clicked()
{
    firestoreClient->getFineAmount(currentUID, currentToken, [this](int fine) {
        if (fine > 0) {
            processMockPayment(fine);
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


        QString userUID = fields.value("userUID").toObject().value("stringValue").toString();
        QString userName = fields.value("userName").toObject().value("stringValue").toString();
        QString bookName = fields.value("bookName").toObject().value("stringValue").toString();
        QString borrowDate = fields.value("borrowDate").toObject().value("stringValue").toString();
        QString returnDate = fields.value("dueDate").toObject().value("stringValue").toString();

        int row = ui->monitoringTable->rowCount();
        ui->monitoringTable->insertRow(row);


        ui->monitoringTable->setItem(row, 0, new QTableWidgetItem(QString::number(i + 1))); // Sr. No.
        ui->monitoringTable->setItem(row, 1, new QTableWidgetItem(userName));
        ui->monitoringTable->setItem(row, 2, new QTableWidgetItem(bookName));
        ui->monitoringTable->setItem(row, 3, new QTableWidgetItem(borrowDate));
        ui->monitoringTable->setItem(row, 4, new QTableWidgetItem(returnDate));

        QPushButton *fineBtn = new QPushButton("Impose Fine");


        QDate dueDate = QDate::fromString(returnDate, Qt::ISODate);
        if (QDate::currentDate() > dueDate) {
            fineBtn->setEnabled(true);
            fineBtn->setStyleSheet("background-color: #dc3545; color: white;");
        } else {
            fineBtn->setEnabled(false);
        }


        connect(fineBtn, &QPushButton::clicked, [=]() {
            int fineValue = 50;
            firestoreClient->updateFineInFirestore(userUID, fineValue, authManager->getCurrentToken());
            QMessageBox::information(this, "Action Taken", "Fine of ₹" + QString::number(fineValue) + " imposed.");
        });

        ui->monitoringTable->setCellWidget(row, 5, fineBtn);
    }
}
void MainWindow::checkStudentFines()
{

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

    QProgressDialog progress("Connecting to Razorpay...", "Cancel", 0, 100, this);
    progress.setWindowModality(Qt::WindowModal);
    progress.show();


    QThread::msleep(2000);
    progress.setValue(100);


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

    book.title       = ui->bookName_in->text();
    book.author      = ui->author_in->text();
    book.category    = ui->categ_in->text();
    book.coverUrl    = ui->lineEdit_coverUrl->text();
    book.description = ui->textEdit_description->toPlainText();
    book.rating      = ui->doubleSpinBox_rating->value();
    book.section     = ui->lineEdit_section->text();
    book.available   = ui->spinBox_qty->value();

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


        if (info.fineAmount > 0) {
            ui->fine_label->setStyleSheet("color: red; font-weight: bold;");
        } else {
            ui->fine_label->setStyleSheet("color: green;");
        }
    });
}

void MainWindow::on_profile_btn_user_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->profile_page);

    firestoreClient->getUserInfo(currentUID, currentToken,
                                 [this](UserInfo info)
                                 {

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

                                       ui->change_name_btn->setEnabled(true);
                                       ui->change_name_btn->setText("Change Name");

                                       if (!success) {
                                           QMessageBox::warning(this, "Update Failed",
                                                                "Could not update your name. "
                                                                "Please check your connection and try again.");
                                           return;
                                       }


                                       currentUserName = newName;
                                       ui->name_label->setText(newName);

                                       QMessageBox::information(this, "Name Updated",
                                                                "Your name has been changed to:\n" + newName);
                                   });
}


void MainWindow::on_backButton_clicked()
{

    ui->stackedWidget->setCurrentWidget(ui->searchPage);
}



void MainWindow::bookViewPage(QString title,
                              QString author,
                              QString category,
                              QString rating,
                              QString description,
                              QIcon coverIcon,
                              QString bookId, QString coverUrl, QString Qty)
{
    ui->bookTitleLabel->setText(title);
    ui->bookAuthorLabel->setText(author);
    ui->bookCategoryLabel->setText(category);
    ui->bookRatingLabel->setText(rating);
    ui->bookDescriptionText->setText(description);
    ui->availabilityLabel->setText(Qty);


    QPixmap coverPixmap = coverIcon.pixmap(250, 350);
    ui->bookImage_label->setPixmap(coverPixmap);

    ui->bookImage_label->setScaledContents(true);

    currentBookId  = bookId;
    currentCoverUrl = coverUrl;


    if (savedBookIds.contains(bookId)) {
        ui->addToMyBooks_btn->setText("✔  Already in My Books");
        ui->addToMyBooks_btn->setEnabled(false);
    } else {
        ui->addToMyBooks_btn->setText("＋  Add to My Books");
        ui->addToMyBooks_btn->setEnabled(true);
    }
    if (savedBookIds.contains(bookId)) {
        ui->addToMyBooks_btn->setEnabled(false);
        ui->addToMyBooks_btn->setText("✔  Already in My Books");
        ui->rmbook_btn->setVisible(true);
    } else {

        ui->addToMyBooks_btn->setText("＋  Add to My Books");
        ui->addToMyBooks_btn->setEnabled(true);
        ui->rmbook_btn->setVisible(false);
    }
    ui->stackedWidget->setCurrentWidget(ui->bookViewPage);


}

void MainWindow::on_update_btn_clicked()
{
    if (selectedBookId.isEmpty()) {
        QMessageBox::warning(this, "No Book Selected",
                             "Please click a book in the table first.");
        return;
    }

    Book book;
    book.id          = selectedBookId;
    book.title       = ui->bookName_in->text();
    book.author      = ui->author_in->text();
    book.category    = ui->categ_in->text();
    book.coverUrl    = ui->lineEdit_coverUrl->text();
    book.description = ui->textEdit_description->toPlainText();
    book.rating      = ui->doubleSpinBox_rating->value();
    book.section     = ui->lineEdit_section->text();
    book.available   = ui->spinBox_qty->value();

    firestoreClient->updateBook(book, currentToken, [this](bool success) {
        if (success) {
            QMessageBox::information(this, "Success", "Book updated.");

            selectedBookId.clear();
        } else {
            QMessageBox::warning(this, "Failed", "Could not update book.");
        }
    });
}


void MainWindow::on_remove_btn_clicked()
{

    if (selectedBookId.isEmpty()) {
        QMessageBox::warning(this, "No Book Selected",
                             "Please click a book in the table first.");
        return;
    }


    auto confirm = QMessageBox::question(
        this, "Confirm Delete",
        "Are you sure you want to remove this book?\n\n"
            + ui->bookName_in->text(),
        QMessageBox::Yes | QMessageBox::No
        );
    if (confirm != QMessageBox::Yes) return;

    firestoreClient->removeBook(selectedBookId, currentToken,
                               [this](bool success)
                               {
                                   if (success) {
                                       QMessageBox::information(this, "Removed", "Book removed.");

                                       selectedBookId.clear();


                                       ui->bookName_in->clear();
                                       ui->author_in->clear();
                                       ui->categ_in->clear();
                                       ui->label_selectedBook->setText("No book selected.");
                                   } else {
                                       QMessageBox::warning(this, "Failed", "Could not remove book.");
                                   }
                               });

}


void MainWindow::on_inventory_btn_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->inventory);
    firestoreClient->getAllBooks(currentToken, [this](QList<Book> books)
                                {
                                    ui->tableWidget_books->setRowCount(0);
                                    ui->tableWidget_books->setColumnCount(9);
                                    ui->tableWidget_books->setHorizontalHeaderLabels(
                                        {"", "Title", "Author", "Category", "Rating", "Available","Description","Section","Cover URL"});


                                    ui->tableWidget_books->setColumnHidden(0, true);

                                    for (const Book &b : books)
                                    {
                                        int row = ui->tableWidget_books->rowCount();
                                        ui->tableWidget_books->insertRow(row);


                                        ui->tableWidget_books->setItem(row, 0,
                                                                       new QTableWidgetItem(b.id));


                                        ui->tableWidget_books->setItem(row, 1,
                                                                       new QTableWidgetItem(b.title));
                                        ui->tableWidget_books->setItem(row, 2,
                                                                       new QTableWidgetItem(b.author));
                                        ui->tableWidget_books->setItem(row, 3,
                                                                       new QTableWidgetItem(b.category));
                                        ui->tableWidget_books->setItem(row, 4,
                                                                       new QTableWidgetItem(QString::number(b.rating)));
                                        ui->tableWidget_books->setItem(row, 5,
                                                                       new QTableWidgetItem(QString::number(b.available)));
                                        ui->tableWidget_books->setItem(row, 6,
                                                                       new QTableWidgetItem(b.description));
                                        ui->tableWidget_books->setItem(row, 7,
                                                                      new QTableWidgetItem(b.section));
                                        ui->tableWidget_books->setItem(row, 8,
                                                                      new QTableWidgetItem(b.coverUrl));
                                    }
                                });
}



void MainWindow::on_tableWidget_books_cellClicked(int row, int column)
{
    selectedBookId = ui->tableWidget_books->item(row, 0)->text();


    Book b;
    ui->bookName_in->setText(
        ui->tableWidget_books->item(row, 1)->text());
    ui->author_in->setText(
        ui->tableWidget_books->item(row, 2)->text());
    ui->categ_in->setText(
        ui->tableWidget_books->item(row, 3)->text());
    ui->doubleSpinBox_rating->setValue(
        (ui->tableWidget_books->item(row, 4)->text()).toDouble());
    ui->spinBox_qty->setValue(
        (ui->tableWidget_books->item(row, 5)->text()).toInt());
    ui->textEdit_description->setText(
        ui->tableWidget_books->item(row, 6)->text());
    ui->lineEdit_section->setText(
        ui->tableWidget_books->item(row, 7)->text());
    ui->lineEdit_coverUrl->setText(
        ui->tableWidget_books->item(row, 8)->text());


    ui->bookTitleLabel->setText(
        ui->tableWidget_books->item(row, 1)->text());
    ui->bookAuthorLabel->setText(
        ui->tableWidget_books->item(row, 2)->text());
    ui->bookCategoryLabel->setText(
        ui->tableWidget_books->item(row, 3)->text());
    ui->bookRatingLabel->setText(
        (ui->tableWidget_books->item(row, 4)->text()));
    ui->availabilityLabel->setText("Available copies: "+
        (ui->tableWidget_books->item(row, 5)->text()));


    ui->label_selectedBook->setText(
        "Selected: " + ui->tableWidget_books->item(row, 1)->text()
        + "  [ID: " + selectedBookId + "]");

    qDebug() << "Selected book ID:" << selectedBookId;
}


void MainWindow::on_update_to_add_btn_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->addRemove_page);
}




void MainWindow::on_search_btn_clicked()
{

    ui->stackedWidget->setCurrentWidget(ui->searchPage);


    firestoreClient->getAllBooks(currentToken, [this](QList<Book> books) {
        populateBookGrid(books);
    });
}

void MainWindow::on_backFromSearch_btn_clicked()
{
    if (currentRole == "admin")
        ui->stackedWidget->setCurrentWidget(ui->adminDashboardPage);
    else
        ui->stackedWidget->setCurrentWidget(ui->studentDashboardPage);
}



void MainWindow::on_searchLineEdit_textChanged(const QString &arg1)
{

    filterBooks();
}

void MainWindow::on_genreFilterCombo_currentTextChanged(const QString &arg1)
{

    filterBooks();
}

void MainWindow::filterBooks()
{

    QString searchText = ui->searchLineEdit->text().toLower();


    QString selectedGenre = ui->genreFilterCombo->currentText();


    for (int i = 0; i < ui->bookCoverGrid->count(); ++i) {

        QListWidgetItem *item = ui->bookCoverGrid->item(i);
        if (!item) continue;


        QString title = item->text().toLower();


        QString author = item->data(Qt::UserRole + 1).toString().toLower();
        QString genre = item->data(Qt::UserRole + 2).toString();


        bool matchesSearch = title.contains(searchText) || author.contains(searchText);


        bool matchesGenre = (selectedGenre.toLower() == "all genres") || (genre == selectedGenre);


        if (matchesSearch && matchesGenre) {
            item->setHidden(false);
        } else {
            item->setHidden(true);
        }
    }
}


void MainWindow::populateBookGrid(QList<Book> books)
{
    ui->bookCoverGrid->clear();
    ui->genreFilterCombo->clear();
    ui->genreFilterCombo->addItem("All genres");
    QStringList genresLoaded;

    ui->bookCoverGrid->setIconSize(QSize(100, 150));
    ui->bookCoverGrid->setGridSize(QSize(140, 200));


    QNetworkAccessManager *imageManager = new QNetworkAccessManager(this);

    for (const Book &b : books) {


        QPixmap placeholderCover(100, 150);
        placeholderCover.fill(QColor("#DCE1E6"));
        QListWidgetItem *bookItem = new QListWidgetItem(QIcon(placeholderCover), b.title);

        bookItem->setForeground(QBrush(Qt::black));
        bookItem->setTextAlignment(Qt::AlignHCenter | Qt::AlignBottom);

        bookItem->setData(Qt::UserRole, b.id);
        bookItem->setData(Qt::UserRole + 1, b.author.toLower());
        bookItem->setData(Qt::UserRole + 2, b.category);
        bookItem->setData(Qt::UserRole + 3, QString::number(b.rating));
        bookItem->setData(Qt::UserRole + 4, b.description);
        bookItem->setData(Qt::UserRole + 5, b.coverUrl);
        bookItem->setData(Qt::UserRole + 6, b.available);

        ui->bookCoverGrid->addItem(bookItem);

        if (!genresLoaded.contains(b.category) && !b.category.isEmpty()) {
            genresLoaded.append(b.category);
            ui->genreFilterCombo->addItem(b.category);
        }


        if (!b.coverUrl.isEmpty() && b.coverUrl.startsWith("http")) {
            QNetworkRequest request((QUrl(b.coverUrl)));
            QNetworkReply *reply = imageManager->get(request);


            connect(reply, &QNetworkReply::finished, [reply, bookItem]() {
                if (reply->error() == QNetworkReply::NoError) {
                    QByteArray imageData = reply->readAll();
                    QPixmap pixmap;
                    if (pixmap.loadFromData(imageData)) {

                        QPixmap scaledPixmap = pixmap.scaled(100, 150, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
                        bookItem->setIcon(QIcon(scaledPixmap));
                    }
                }
                reply->deleteLater();
            });
        }
    }
}

void MainWindow::on_bookCoverGrid_itemClicked(QListWidgetItem *item)
{

    QString title = item->text();


    QString author      = item->data(Qt::UserRole + 1).toString();
    QString category    = item->data(Qt::UserRole + 2).toString();
    QString rating      = item->data(Qt::UserRole + 3).toString();
    QString description = item->data(Qt::UserRole + 4).toString();
    QString bookId      = item->data(Qt::UserRole).toString();
    QString coverUrl    = item->data(Qt::UserRole + 5).toString();
    QString Qty         = item->data(Qt::UserRole + 6).toString();


    QIcon coverIcon = item->icon();


    bookViewPage(title, author, category, rating, description, coverIcon, bookId, coverUrl, Qty);
}
void MainWindow::on_browseImage_btn_clicked()
{

    QString imagePath = QFileDialog::getOpenFileName(this, "Select Book Cover", "", "Images (*.png *.jpg *.jpeg)");

    if (imagePath.isEmpty()) return;


    ui->addBook_btn->setEnabled(false);
    ui->lineEdit_coverUrl->setText("Uploading...");

    firestoreClient->uploadImageToCloudinary(imagePath, [this](QString url) {
        if (!url.isEmpty()) {

            ui->lineEdit_coverUrl->setText(url);
        } else {
            ui->lineEdit_coverUrl->setText("Upload Failed");
        }
        ui->addBook_btn->setEnabled(true);
    });
}

void MainWindow::on_forgotPassword_btn_clicked()
{
    QMessageBox msgBox(this);
    msgBox.setWindowTitle("Password Reset");
    msgBox.setText("<b>Account Locked / Forgot Password</b>");
    msgBox.setInformativeText("Because PocketLib uses internal Library IDs, automated email resets are disabled.\n\n"
                              "Please visit the Library Help Desk or contact the System Administrator to verify your identity and reset your password.");
    msgBox.setIcon(QMessageBox::Information);
    msgBox.setStandardButtons(QMessageBox::Ok);

    msgBox.exec();
}

void MainWindow::on_changepsd_btn_clicked()
{
    QMessageBox msgBox(this);
    msgBox.setWindowTitle("Change Password");
    msgBox.setText("<b>Password Change Restricted</b>");
    msgBox.setInformativeText("Because PocketLib uses internal Library IDs, automated password changes are currently disabled in the app.\n\n"
                              "Please contact the System Administrator or visit the Library Help Desk to request a new password.");
    msgBox.setIcon(QMessageBox::Information);
    msgBox.setStandardButtons(QMessageBox::Ok);

    msgBox.exec();
}


void MainWindow::loadStudentDashboard()
{
    firestoreClient->getAllBooks(currentToken, [this](QList<Book> books)
                                 {
                                     QList<Book> newBooks;
                                     QList<Book> topRated;
                                     QList<Book> recommended;

                                     for (const Book &b : books)
                                     {

                                         newBooks.append(b);


                                         if (b.rating >= 4.5)
                                             topRated.append(b);


                                         if (b.category == "Romance")
                                             recommended.append(b);
                                     }


                                     populateSection(ui->newBooksLayout, newBooks.mid(0, 6));
                                     populateSection(ui->topRatedLayout, topRated.mid(0, 6));
                                     populateSection(ui->recommendedLayout, recommended.mid(0, 6));
                                 });
}

void MainWindow::populateSection(QHBoxLayout *layout, QList<Book> books)
{

    QLayoutItem *child;
    while ((child = layout->takeAt(0)) != nullptr) {
        delete child->widget();
        delete child;
    }

    if (books.isEmpty()) {
        QLabel *empty = new QLabel("No books available");
        empty->setStyleSheet(R"(
    color: gray;
    font-size: 12px;
    padding: 10px;
)");
        empty->setAlignment(Qt::AlignCenter);

        layout->addWidget(empty);
        layout->addStretch();
        return;
    }


    layout->setSpacing(15);
    layout->setContentsMargins(10, 5, 10, 5);

    for (const Book &b : books)
    {

        QWidget *card = new QWidget();
        card->setFixedSize(120, 200);
        card->setCursor(Qt::PointingHandCursor);

        card->setStyleSheet(R"(
    QWidget {
        background: white;
        border-radius: 12px;
    }
    QWidget:hover {
        background: #f8f9fa;
        border: 1px solid #ddd;
    }
)");

        QVBoxLayout *v = new QVBoxLayout(card);
        v->setSpacing(5);


        QLabel *cover = new QLabel();
        cover->setFixedSize(100, 140);
        cover->setAlignment(Qt::AlignCenter);

        QPixmap placeholder(100, 140);
        placeholder.fill(Qt::lightGray);
        cover->setPixmap(placeholder);
        cover->setScaledContents(true);


        QLabel *title = new QLabel(b.title);
        title->setWordWrap(true);
        title->setAlignment(Qt::AlignCenter);
        title->setStyleSheet("font-size: 11px; color: black;");

        v->addWidget(cover);
        v->addWidget(title);

        layout->addWidget(card);


        card->setProperty("title", b.title);
        card->setProperty("author", b.author);
        card->setProperty("category", b.category);
        card->setProperty("rating", QString::number(b.rating));
        card->setProperty("description", b.description);
        card->setProperty("section", b.section);
        card->setProperty("availability", QString::number(b.available));
        card->setProperty("bookId", b.id);


        card->installEventFilter(this);


        if (!b.coverUrl.isEmpty() && b.coverUrl.startsWith("http"))
        {
            QNetworkAccessManager *manager = new QNetworkAccessManager(card);

            QNetworkReply *reply = manager->get(QNetworkRequest(QUrl(b.coverUrl)));

            connect(reply, &QNetworkReply::finished, [reply, cover, card]() {
                if (reply->error() == QNetworkReply::NoError)
                {
                    QByteArray data = reply->readAll();

                    QPixmap pix;
                    pix.loadFromData(data);

                    QPixmap scaled = pix.scaled(
                        100, 140,
                        Qt::KeepAspectRatioByExpanding,
                        Qt::SmoothTransformation
                        );

                    cover->setPixmap(scaled);


                    card->setProperty("coverPixmap", scaled);

                    if (pix.isNull()) {
                        pix = QPixmap(100,140);
                        pix.fill(Qt::lightGray);
                    }
                }
                reply->deleteLater();
            });
        }
    }


    layout->addStretch();


}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonPress)
    {
        QWidget *card = qobject_cast<QWidget*>(obj);

        if (card)
        {
            QString title = card->property("title").toString();

            if (!title.isEmpty())
            {

                QPixmap pix = card->property("coverPixmap").value<QPixmap>();
                QIcon icon(pix);

                bookViewPage(
                    card->property("title").toString(),
                    card->property("author").toString(),
                    card->property("category").toString(),
                    card->property("rating").toString(),
                    card->property("description").toString(),
                    icon,
                    card->property("section").toString(),
                    card->property("availability").toString(),
                    card->property("bookId").toString()
                    );

                return true;
            }
        }
    }

    return QMainWindow::eventFilter(obj, event);
}

void MainWindow::openFilteredPage()
{
    ui->stackedWidget->setCurrentWidget(ui->searchPage);

    firestoreClient->getAllBooks(currentToken, [this](QList<Book> books)
                                 {
                                     QList<Book> filtered;

                                     for (const Book &b : books)
                                     {
                                         if (currentDashboardFilter == "new")
                                         {
                                             filtered.append(b);
                                         }
                                         else if (currentDashboardFilter == "top")
                                         {
                                             if (b.rating >= 4.5)
                                                 filtered.append(b);
                                         }
                                         else if (currentDashboardFilter == "recommended")
                                         {
                                             if (b.category == "Engineering")
                                                 filtered.append(b);
                                         }
                                     }

                                     populateBookGrid(filtered);
                                 });
}

void MainWindow::handleDashboardSearch(const QString &text)
{

    if (text.trimmed().isEmpty()) {
        ui->stackedWidget->setCurrentWidget(ui->studentDashboardPage);
        loadStudentDashboard();
        return;
    }


    ui->stackedWidget->setCurrentWidget(ui->searchPage);


    firestoreClient->getAllBooks(currentToken, [this, text](QList<Book> books)
                                 {
                                     QList<Book> filtered;

                                     QString query = text.toLower();

                                     for (const Book &b : books)
                                     {
                                         if (b.title.toLower().contains(query) ||
                                             b.author.toLower().contains(query) ||
                                             b.category.toLower().contains(query))
                                         {
                                             filtered.append(b);
                                         }
                                     }

                                     populateBookGrid(filtered);
                                 });
}

void MainWindow::on_addToMyBooks_btn_clicked()
{
    if (currentBookId.isEmpty()) return;


    Book book;
    book.id          = currentBookId;
    book.title       = ui->bookTitleLabel->text();
    book.author      = ui->bookAuthorLabel->text();
    book.category    = ui->bookCategoryLabel->text();
    book.rating      = ui->bookRatingLabel->text().toDouble();
    book.description = ui->bookDescriptionText->toPlainText();
    book.coverUrl    = currentCoverUrl;
    book.available   = ui->availabilityLabel->text().toInt();

    ui->addToMyBooks_btn->setEnabled(false);
    ui->addToMyBooks_btn->setText("Saving…");

    firestoreClient->addToMyBooks(currentUID, book, currentToken,
                                  [this, book](bool success)
                                  {
                                      if (!success) {
                                          QMessageBox::warning(this, "Error", "Could not save book. Try again.");
                                          ui->addToMyBooks_btn->setEnabled(true);
                                          ui->addToMyBooks_btn->setText("＋  Add to My Books");
                                          return;
                                      }


                                      savedBookIds.insert(book.id);

                                      ui->addToMyBooks_btn->setText("✔  Already in My Books");


                                      QMessageBox::information(this, "Saved!",
                                                               "\"" + book.title + "\" added to My Books.");
                                  });
    ui->rmbook_btn->setVisible(true);
}


void MainWindow::on_myBooks_btn_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->myBooksPage);
    loadMyBooksGrid();
}

void MainWindow::on_myBooks_btn_user_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->myBooksPage);
    loadMyBooksGrid();
}

void MainWindow::loadMyBooksGrid()
{
    ui->myBooksGrid->clear();
    ui->myBooksGrid->setViewMode(QListWidget::IconMode);
    ui->myBooksGrid->setIconSize(QSize(100, 150));
    ui->myBooksGrid->setGridSize(QSize(140, 200));
    ui->myBooksGrid->setResizeMode(QListWidget::Adjust);
    ui->myBooksGrid->setMovement(QListWidget::Static);
    ui->myBooksGrid->setWordWrap(true);
    ui->myBooksGrid->setSpacing(8);

    firestoreClient->getMyBooks(currentUID, currentToken,
                                [this](QList<Book> books)
                                {

                                    savedBookIds.clear();

                                    if (books.isEmpty()) {

                                        QListWidgetItem *empty = new QListWidgetItem("📚\nNo books\nsaved yet");
                                        empty->setTextAlignment(Qt::AlignCenter);
                                        empty->setFlags(Qt::NoItemFlags);
                                        empty->setSizeHint(QSize(140, 200));
                                        ui->myBooksGrid->addItem(empty);
                                        return;
                                    }

                                    QNetworkAccessManager *imageManager = new QNetworkAccessManager(this);

                                    for (const Book &b : books) {

                                        savedBookIds.insert(b.id);


                                        QPixmap placeholder(100, 150);
                                        placeholder.fill(QColor("#1C2333"));

                                        QListWidgetItem *item = new QListWidgetItem(QIcon(placeholder), b.title);
                                        item->setTextAlignment(Qt::AlignHCenter | Qt::AlignBottom);
                                        item->setToolTip(b.title + "\n" + b.author);
                                        item->setSizeHint(QSize(140, 200));


                                        item->setData(Qt::UserRole,     b.id);
                                        item->setData(Qt::UserRole + 1, b.author);
                                        item->setData(Qt::UserRole + 2, b.category);
                                        item->setData(Qt::UserRole + 3, QString::number(b.rating));
                                        item->setData(Qt::UserRole + 4, b.description);
                                        item->setData(Qt::UserRole + 5, b.coverUrl);
                                        item->setData(Qt::UserRole + 6, b.available);

                                        ui->myBooksGrid->addItem(item);


                                        if (!b.coverUrl.isEmpty() && b.coverUrl.startsWith("http")) {
                                            QNetworkReply *reply =
                                                imageManager->get(QNetworkRequest(QUrl(b.coverUrl)));

                                            connect(reply, &QNetworkReply::finished, [reply, item]() {
                                                if (reply->error() == QNetworkReply::NoError) {
                                                    QPixmap pixmap;
                                                    if (pixmap.loadFromData(reply->readAll())) {
                                                        item->setIcon(QIcon(
                                                            pixmap.scaled(100, 150,
                                                                          Qt::KeepAspectRatioByExpanding,
                                                                          Qt::SmoothTransformation)
                                                            ));
                                                    }
                                                }
                                                reply->deleteLater();
                                            });
                                        }
                                    }
                                });
}

void MainWindow::on_myBooksGrid_itemClicked(QListWidgetItem *item)
{
    QString title = item->text();


    QString author      = item->data(Qt::UserRole + 1).toString();
    QString category    = item->data(Qt::UserRole + 2).toString();
    QString rating      = item->data(Qt::UserRole + 3).toString();
    QString description = item->data(Qt::UserRole + 4).toString();
    QString bookId      = item->data(Qt::UserRole).toString();
    QString coverUrl    = item->data(Qt::UserRole + 5).toString();
    QString Qty         = item->data(Qt::UserRole + 6).toString();



    QIcon coverIcon = item->icon();


    bookViewPage(title, author, category, rating, description, coverIcon, bookId, coverUrl, Qty);
}

void MainWindow::on_rmbook_btn_clicked()
{
    if (currentBookId.isEmpty()) return;

    auto confirm = QMessageBox::question(
        this, "Remove Book",
        "Remove \"" + ui->bookTitleLabel->text() + "\" from My Books?",
        QMessageBox::Yes | QMessageBox::No
        );
    if (confirm != QMessageBox::Yes) return;

    firestoreClient->removeFromMyBooks(currentUID, currentBookId, currentToken,
                                       [this](bool success)
                                       {
                                           if (success) {
                                               savedBookIds.remove(currentBookId);


                                               ui->rmbook_btn->setEnabled(false);
                                               ui->addToMyBooks_btn->setText("＋  Add to My Books");
                                               ui->addToMyBooks_btn->setEnabled(true);


                                               loadMyBooksGrid();

                                               QMessageBox::information(this, "Removed",
                                                                        "Book removed from My Books.");
                                           } else {
                                               QMessageBox::warning(this, "Error", "Could not remove book.");
                                           }
                                       });

}


void MainWindow::on_View_btn_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->bookViewPage);
}


void MainWindow::on_pushButton_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->adminCheckoutPage);
}


void MainWindow::on_checkout_back_btn_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->adminDashboardPage);
}


void MainWindow::on_checkout_issue_btn_clicked()
{
    QString studentEmail = ui->checkout_studentEmail_in->text().trimmed();
    QString bookId = ui->checkout_bookId_in->text().trimmed();

    if(studentEmail.isEmpty() || bookId.isEmpty()) {
        QMessageBox::warning(this, "Missing Data", "Please enter both the Student's Email and the Book ID.");
        return;
    }


    ui->checkout_issue_btn->setEnabled(false);
    ui->checkout_issue_btn->setText("Processing...");


    firestoreClient->adminCheckoutBook(studentEmail, bookId, currentToken, currentUID,
                                       [this](bool success, QString message) {


                                           ui->checkout_issue_btn->setEnabled(true);
                                           ui->checkout_issue_btn->setText("Issue Book to Student");

                                           if(success) {
                                               QMessageBox::information(this, "Checkout Successful", message);

                                               ui->checkout_studentEmail_in->clear();
                                               ui->checkout_bookId_in->clear();
                                           } else {
                                               QMessageBox::warning(this, "Checkout Failed", message);
                                           }
                                       });

}

void MainWindow::on_hamburger_btn_clicked()
{
    QRect startRect;
    QRect endRect;

    if (ui->sidebarWidget->x() < 0)
    {

        startRect = QRect(-250, 30, 250, height());
        endRect   = QRect(0, 30, 250, height());
    }
    else
    {

        startRect = QRect(0, 30, 250, height());
        endRect   = QRect(-250, 30, 250, height());
    }

    sidebarAnim->stop();
    sidebarAnim->setStartValue(startRect);
    sidebarAnim->setEndValue(endRect);
    sidebarAnim->start();


    ui->hamburger_btn->raise();
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);


    QRect geo = ui->sidebarWidget->geometry();
    ui->sidebarWidget->setGeometry(geo.x(), 0, geo.width(), height());
}
