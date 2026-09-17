#include "ProjectSession.h"

#include <QDateTime>

void ProjectSession::addHistory(
    const QString& workspace,
    const QString& action,
    const QString& status,
    const QString& details
)
{
    history.push_back({
        QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss AP"),
        workspace,
        action,
        status,
        details
    });
}
