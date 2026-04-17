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

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{

    

    ui->setupUi(this);
    connect(ui->back_btn, &QPushButton::clicked, this, &MainWindow::handleSharedAction);
    connect(ui->back_btn_2, &QPushButton::clicked, this, &MainWindow::handleSharedAction);
    connect(ui->back_btn_3, &QPushButton::clicked, this, &MainWindow::handleSharedAction);

    // Start app at login screen
    ui->stackedWidget->setCurrentWidget(ui->loginPage);
    // 1. Initialize Firebase Managers instead of SQLite
    authManager = new AuthManager(this);
    firestoreClient = new FirestoreClient(this);
    ui->widget->hide();
    ui->genreFilterCombo->addItem("All Genres");
    ui->genreFilterCombo->addItem("Sci-Fi");
    ui->genreFilterCombo->addItem("Engineering");
    ui->forgotPassword_btn->hide();
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
                               // 1. Check if login failed
                               if (token.isEmpty()) {
                                   QMessageBox::warning(this, "Login Failed", "Invalid email or password");
                                   // Show the forgot password button on failure
                                   ui->forgotPassword_btn->show();
                                   return;
                               }

                               // 2. Login succeeded! Hide the button again for next time
                               ui->forgotPassword_btn->hide();

                               // 3. Save current user details
                               this->currentToken = token;
                               this->currentUID = uid;

                               // 4. Fetch the role and redirect to the correct dashboard
                               firestoreClient->getUserRole(uid, token, [this](QString role)
                                                            {
                                                                if (role == "admin") {
                                                                    ui->stackedWidget->setCurrentWidget(ui->adminDashboardPage);
                                                                }
                                                                else if (role == "student") {
                                                                    ui->stackedWidget->setCurrentWidget(ui->studentDashboardPage);
                                                                    checkStudentFines(); // Triggers the bell check
                                                                }
                                                                currentRole = role;
                                                            });
                           });
}
///////////////////////////////////////////////////////////
// REGISTER PAGE OPEN
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

// This function is triggered when the "Back" button on the Book View Page is clicked.
// It switches the stackedWidget back to the student dashboard page.
void MainWindow::on_backButton_clicked()
{
    // stackedWidget contains all the pages of the application.
    // setCurrentWidget() changes the visible page.
    ui->stackedWidget->setCurrentWidget(ui->searchPage);
}


// This function is responsible for opening the Book View Page
// and filling it with the details of the selected book.
void MainWindow::bookViewPage(QString title,
                              QString author,
                              QString category,
                              QString rating,
                              QString description,
                              QIcon coverIcon) // <--- ADD THIS
{
    ui->bookTitleLabel->setText(title);
    ui->bookAuthorLabel->setText(author);
    ui->bookCategoryLabel->setText(category);
    ui->bookRatingLabel->setText(rating);
    ui->bookDescriptionText->setText(description);

    // --- NEW: Set the image ---
    // Extract a nice big pixmap from the icon to fit your label
    QPixmap coverPixmap = coverIcon.pixmap(250, 350);
    ui->bookImage_label->setPixmap(coverPixmap);
    // Ensure the image stretches to fit the label bounds properly
    ui->bookImage_label->setScaledContents(true);
    // --------------------------

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
    book.id          = selectedBookId;           // ← comes from row click
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

    // Confirm before deleting
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

                                       // Clear the edit fields
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

                                    // Hide column 0 — it holds the document ID invisibly
                                    ui->tableWidget_books->setColumnHidden(0, true);

                                    for (const Book &b : books)
                                    {
                                        int row = ui->tableWidget_books->rowCount();
                                        ui->tableWidget_books->insertRow(row);

                                        // Column 0 (hidden) — stores the document ID
                                        ui->tableWidget_books->setItem(row, 0,
                                                                       new QTableWidgetItem(b.id));

                                        // Visible columns
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

    // Auto-fill all edit fields so admin can modify and hit Update
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

    // Show which book is selected
    ui->label_selectedBook->setText(
        "Selected: " + ui->tableWidget_books->item(row, 1)->text()
        + "  [ID: " + selectedBookId + "]");

    qDebug() << "Selected book ID:" << selectedBookId;
}


void MainWindow::on_update_to_add_btn_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->addRemove_page);
}


// --- Navigation ---

void MainWindow::on_search_btn_clicked()
{
    // Switch the stacked widget to the Search Page
    ui->stackedWidget->setCurrentWidget(ui->searchPage);

    // Fetch the live inventory from Firestore and pass it to the grid
    firestoreClient->getAllBooks(currentToken, [this](QList<Book> books) {
        populateBookGrid(books);
    });
}

void MainWindow::on_backFromSearch_btn_clicked()
{
    // Switch back to the admin dashboard
    ui->stackedWidget->setCurrentWidget(ui->adminDashboardPage);
}

// --- Live Search & Filter Logic ---

void MainWindow::on_searchLineEdit_textChanged(const QString &arg1)
{
    // Trigger the filter whenever a letter is typed or deleted
    filterBooks();
}

void MainWindow::on_genreFilterCombo_currentTextChanged(const QString &arg1)
{
    // Trigger the filter whenever a new genre is selected
    filterBooks();
}

void MainWindow::filterBooks()
{
    // 1. Get the search text, converted to lowercase for case-insensitive matching
    QString searchText = ui->searchLineEdit->text().toLower();

    // 2. Get the currently selected genre
    QString selectedGenre = ui->genreFilterCombo->currentText();

    // 3. Loop through every item in the cover grid
    for (int i = 0; i < ui->bookCoverGrid->count(); ++i) {

        QListWidgetItem *item = ui->bookCoverGrid->item(i);
        if (!item) continue;

        // Get the title (this is the visible text under the icon)
        QString title = item->text().toLower();

        // Retrieve the hidden Author and Genre data we saved inside the item
        QString author = item->data(Qt::UserRole + 1).toString().toLower();
        QString genre = item->data(Qt::UserRole + 2).toString();

        // Check if the title OR the author contains the typed letters
        bool matchesSearch = title.contains(searchText) || author.contains(searchText);

        // THE FIX: Convert selectedGenre to lowercase just for the "All Genres" check
        bool matchesGenre = (selectedGenre.toLower() == "all genres") || (genre == selectedGenre);

        // 4. Show or hide the icon based on the filters
        if (matchesSearch && matchesGenre) {
            item->setHidden(false); // Show it
        } else {
            item->setHidden(true);  // Hide it
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

    // Create a network manager to handle all the image downloads
    QNetworkAccessManager *imageManager = new QNetworkAccessManager(this);

    for (const Book &b : books) {

        // 1. Keep the grey placeholder so the grid loads instantly
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

        ui->bookCoverGrid->addItem(bookItem);

        if (!genresLoaded.contains(b.category) && !b.category.isEmpty()) {
            genresLoaded.append(b.category);
            ui->genreFilterCombo->addItem(b.category);
        }

        // ---------------------------------------------------------
        // 2. NEW: Download the actual image in the background!
        // ---------------------------------------------------------
        if (!b.coverUrl.isEmpty() && b.coverUrl.startsWith("http")) {
            QNetworkRequest request((QUrl(b.coverUrl)));
            QNetworkReply *reply = imageManager->get(request);

            // When this specific image finishes downloading, update the icon
            connect(reply, &QNetworkReply::finished, [reply, bookItem]() {
                if (reply->error() == QNetworkReply::NoError) {
                    QByteArray imageData = reply->readAll();
                    QPixmap pixmap;
                    if (pixmap.loadFromData(imageData)) {
                        // Scale the image so it fits the 100x150 slot perfectly without stretching
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
    // 1. Extract the visible text (Title)
    QString title = item->text();

    // 2. Extract the hidden data
    QString author      = item->data(Qt::UserRole + 1).toString();
    QString category    = item->data(Qt::UserRole + 2).toString();
    QString rating      = item->data(Qt::UserRole + 3).toString();
    QString description = item->data(Qt::UserRole + 4).toString();

    // 3. Grab the actual image we downloaded for the grid
    QIcon coverIcon = item->icon();

    // 4. Pass ALL 6 items to your view page (Notice coverIcon is at the end!)
    bookViewPage(title, author, category, rating, description, coverIcon);
}
void MainWindow::on_browseImage_btn_clicked()
{
    // Open a file dialog to let the user pick an image
    QString imagePath = QFileDialog::getOpenFileName(this, "Select Book Cover", "", "Images (*.png *.jpg *.jpeg)");

    if (imagePath.isEmpty()) return; // User canceled

    // Temporarily disable the Add button so they don't submit before the upload finishes
    ui->addBook_btn->setEnabled(false);
    ui->lineEdit_coverUrl->setText("Uploading...");

    firestoreClient->uploadImageToCloudinary(imagePath, [this](QString url) {
        if (!url.isEmpty()) {
            // Put the final URL into the text box so your existing addBook function can grab it!
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

// 1. Navigation: Go to the checkout page from Admin Dashboard
void MainWindow::on_pushButton_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->adminCheckoutPage);
}

// 2. Navigation: Go back
void MainWindow::on_checkout_back_btn_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->adminDashboardPage);
}

// 3. The Main Execution
void MainWindow::on_checkout_issue_btn_clicked()
{
    QString studentEmail = ui->checkout_studentEmail_in->text().trimmed();
    QString bookId = ui->checkout_bookId_in->text().trimmed();

    if(studentEmail.isEmpty() || bookId.isEmpty()) {
        QMessageBox::warning(this, "Missing Data", "Please enter both the Student's Email and the Book ID.");
        return;
    }

    // Disable the button to prevent double-clicking while waiting for Firebase
    ui->checkout_issue_btn->setEnabled(false);
    ui->checkout_issue_btn->setText("Processing...");

    // Call the heavy lifter function we just created
    firestoreClient->adminCheckoutBook(studentEmail, bookId, currentToken, currentUID,
                                       [this](bool success, QString message) {

                                           // Re-enable the button once Firebase replies
                                           ui->checkout_issue_btn->setEnabled(true);
                                           ui->checkout_issue_btn->setText("Issue Book to Student");

                                           if(success) {
                                               QMessageBox::information(this, "Checkout Successful", message);
                                               // Clear the inputs for the next student in line
                                               ui->checkout_studentEmail_in->clear();
                                               ui->checkout_bookId_in->clear();
                                           } else {
                                               QMessageBox::warning(this, "Checkout Failed", message);
                                           }
                                       });
}
