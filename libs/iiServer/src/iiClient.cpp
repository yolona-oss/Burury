#include "iiServer/iiClient.hpp"
#include <qnamespace.h>

iiClient::iiClient(QSslSocket * socket, QObject * parent)
        : SslClientBase(socket, parent),
        _dbAssistant(new API::DriverAssistant),
        _identified(false),
        _ident_tries(0)
{ }

iiClient::~iiClient()
{
        delete _dbAssistant;
}

void
iiClient::doregister(QJsonObject obj)
{
        connect(_dbAssistant, SIGNAL(success(QJsonObject)), this, SIGNAL(registred(QJsonObject)));
        connect(_dbAssistant, SIGNAL(failed(API::HndlError)), this, SIGNAL(registerFailed(API::HndlError)));

        connect(this, SIGNAL(registred(QJsonObject)),
                this, SLOT(on_registred(QJsonObject)));
        connect(this, SIGNAL(registerFailed(API::HndlError)),
                this, SLOT(on_registerFailed(API::HndlError)));

        Q_EMIT logMessage("Attempting register user", Debug);
        Q_EMIT addCommand({
                                  .handler_id = API::HNDL::Config::RegisterWorker,
                                  .caller_role_id = API::Roles::SYS,
                                  .payload = obj,
                                  .waiter = _dbAssistant
                          });
}

void
iiClient::on_registred(QJsonObject obj)
{
        disconnect(_dbAssistant, SIGNAL(success(QJsonObject)), this, SIGNAL(registred(QJsonObject)));
        disconnect(_dbAssistant, SIGNAL(failed(API::HndlError)), this, SIGNAL(registerFailed(API::HndlError)));

        disconnect(this, SIGNAL(registred(QJsonObject)),
                   this, SLOT(on_registred(QJsonObject)));
        disconnect(this, SIGNAL(registerFailed(API::HndlError)),
                   this, SLOT(on_registerFailed(API::HndlError)));

        connect(_dbAssistant, SIGNAL(success(QJsonObject)), this, SLOT(on_requestSuccess(QJsonObject)));
        connect(_dbAssistant, SIGNAL(failed(API::HndlError)),  this, SLOT(on_requestFailed(API::HndlError)));

        _identified = true;

        QJsonDocument doc(obj); // must contain role and basic user information
        _role = obj["role"].toInt();
        QByteArray res =
                iiNPack::pack(doc.toJson(QJsonDocument::Compact), iiNPack::RESPONSE,
                              0, 0);  // login mechanism dont need a stamps

        Q_EMIT logMessage("Registred user: " + obj["name"].toString(), Trace);

        this->sendMessage(res);

        _password.clear();
}

void
iiClient::on_registerFailed(API::HndlError err)
{
        Q_EMIT logMessage("User registration error: " + err.String(), Debug);

        QByteArray res = iiNPack::packError(err.String(), iiNPack::ACCESS_DENIED, 0, 0);
        this->sendMessage(res);
        QTimer::singleShot(5000, [this] { this->disconnectFromHost(); });
}

bool iiClient::identified() const { return _identified; }

void
iiClient::identify(QString login, QString password)
{
        connect(_dbAssistant, SIGNAL(success(QJsonObject)), this, SIGNAL(identified(QJsonObject)));
        connect(_dbAssistant, SIGNAL(failed(API::HndlError)),  this, SIGNAL(identificationFailed(API::HndlError)));

        connect(this, SIGNAL(identified(QJsonObject)),
                this, SLOT(on_identified(QJsonObject)));
        connect(this, SIGNAL(identificationFailed(API::HndlError)),
                this, SLOT(on_identificationFailed(API::HndlError)));

        _login = login;
        _password = password;
        Q_EMIT logMessage("Trying identify user", Debug);
        Q_EMIT addCommand({
                                  .handler_id = API::HNDL::Config::Identify,
                                  .caller_role_id = API::Roles::SYS,
                                  .payload = {
                                          { "login", _login },
                                          { "password", _password },
                                  },
                                  .waiter = _dbAssistant});
}

void
iiClient::on_identified(QJsonObject obj)
{
        disconnect(_dbAssistant, SIGNAL(success(QJsonObject)), this, SIGNAL(identified(QJsonObject)));
        disconnect(_dbAssistant, SIGNAL(failed(API::HndlError)), this, SIGNAL(identificationFailed(API::HndlError)));

        disconnect(this, SIGNAL(identified(QJsonObject)),
                   this, SLOT(on_identified(QJsonObject)));
        disconnect(this, SIGNAL(identificationFailed(API::HndlError)),
                   this, SLOT(on_identificationFailed(API::HndlError)));

        connect(_dbAssistant, SIGNAL(success(QJsonObject)), this, SLOT(on_requestSuccess(QJsonObject)));
        connect(_dbAssistant, SIGNAL(failed(API::HndlError)),  this, SLOT(on_requestFailed(API::HndlError)));

        _identified = true;

        QJsonDocument doc(obj); // must contain role and basic user information
        _role = obj["role"].toInt();
        QByteArray res =
                iiNPack::pack(doc.toJson(QJsonDocument::Compact), iiNPack::RESPONSE,
                              0, 0);  // login mechanism dont need a stamps

        qDebug() << obj;
        Q_EMIT logMessage("Identified user: " + obj["name"].toString(), Trace);

        this->sendMessage(res);

        _password.clear();
}

void
iiClient::on_identificationFailed(API::HndlError err)
{
        /* _ident_tries++; */
        /* if (_ident_tries > _max_tries) { // make configurable */
        /*     this->disconnectFromHost(); */
        /*     // TODO set timeout for connect avalible again */
        /*     return; */
        /* } */

        Q_EMIT logMessage("User identification error: " + err.String(), Debug);

        this->sendMessage(iiNPack::packError(err.String(), iiNPack::ACCESS_DENIED, 0, 0));
        QTimer::singleShot(5000, [this] { this->disconnectFromHost(); });
}

void
iiClient::processRequest(QJsonObject obj, qint64 stamp)
{
        _mtx.lock();
        _curStamp = stamp;
        auto call = API::Handler::parseCall(obj);
        // TODO add checks
        Q_EMIT addCommand({
                                  .handler_id = call.handler_id,
                                  .caller_role_id = _role,
                                  .payload = call.payload,
                                  .waiter = _dbAssistant
                          });
}

void
iiClient::on_requestSuccess(QJsonObject obj)
{
        qint64 stamp = _curStamp;
        _mtx.unlock();

        QJsonDocument doc(obj); // must contain role and basic user information
        this->sendMessage(iiNPack::pack(doc.toJson(QJsonDocument::Compact),
                                        iiNPack::RESPONSE,
                                        QDateTime::currentMSecsSinceEpoch(), stamp));
}

void
iiClient::on_requestFailed(API::HndlError err)
{
        qint64 stamp = _curStamp;
        _mtx.unlock();
        this->sendMessage(iiNPack::packError(err.String(), iiNPack::ResponseError::REQUEST_ERROR,
                                           QDateTime::currentMSecsSinceEpoch(), stamp));
}
