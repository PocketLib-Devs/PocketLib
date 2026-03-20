#ifndef USERINFO_H
#define USERINFO_H

#include <QString>

// ─────────────────────────────────────────────────────────────────────────
//  Plain struct that mirrors a document in the "users" collection.
// ─────────────────────────────────────────────────────────────────────────
struct UserInfo
{
    QString uid;
    QString name;
    QString email;
    QString role;
    QString libraryId;   // e.g. "John4782"
    int     fineAmount = 0;
};

#endif // USERINFO_H
