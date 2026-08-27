#include "SqlUtil.h"

namespace SqlUtil {

QString escapeLike(const QString &raw)
{
    QString s = raw;
    s.replace('!', "!!");
    s.replace('%', "!%");
    s.replace('_', "!_");
    return s;
}

QString likePattern(const QString &raw)
{
    return "%" + escapeLike(raw) + "%";
}

QString likeCond(const QString &field, const QString &placeholder)
{
    return field + " LIKE " + placeholder + " ESCAPE '!'";
}

QString likeConds(const QStringList &fields, const QString &placeholder)
{
    QStringList parts;
    for (const QString &f : fields)
        parts << likeCond(f, placeholder);
    return "(" + parts.join(" OR ") + ")";
}

}
