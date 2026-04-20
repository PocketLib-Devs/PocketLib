#ifndef USERINFO_H
#define USERINFO_H

#include <QString>


struct UserInfo
{
    QString uid;
    QString name;
    QString email;
    QString role;
    QString libraryId;
    int     fineAmount = 0;
};

#endif
