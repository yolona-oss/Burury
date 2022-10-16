#include "API/Handlers/EditSelfInfo.hpp"
#include "API/PayloadItem.hpp"

#include <QSqlQuery>
#include <QSqlRecord>
#include <QtSql>

namespace API::HNDL {

EditSelfInfo::EditSelfInfo(QString name, QString desc, const std::initializer_list<Roles::RoleId>& roles)
        : Handler(name, desc, roles)
{ }

EditSelfInfo::~EditSelfInfo()
{ }

HndlError
EditSelfInfo::exec(QJsonObject& req)
{
        HndlError deser_err = Deserialize(req);
        if (!deser_err.Ok()) {
                return deser_err;
        }

        return HndlError();
}

QJsonObject
EditSelfInfo::Serialize() const
{
        return {};
}

HndlError
EditSelfInfo::Deserialize(const QJsonObject& obj)
{

        return HndlError();
}

} /* API::HNDL */