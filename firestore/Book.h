#ifndef BOOK_H
#define BOOK_H

#include <QString>


struct Book
{
    QString id;
    QString title;
    QString author;
    int     available;
    QString category;
    QString coverUrl;
    QString description;
    double  rating     = 0.0;
    QString section;
};

#endif
