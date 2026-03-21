#ifndef BOOK_H
#define BOOK_H

#include <QString>

// ─────────────────────────────────────────────
//  Plain data struct that mirrors a Firestore
//  document in the "books" collection.
// ─────────────────────────────────────────────
struct Book
{
    QString id;           // Firestore document ID (auto-generated or custom)
    QString title;
    QString author;
    int     available;
    QString category;
    QString coverUrl;
    QString description;
    double  rating     = 0.0;
    QString section;
};

#endif // BOOK_H
