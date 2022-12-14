#ifndef IINETWORKPACKET_HPP_T3ZVG9UF
#define IINETWORKPACKET_HPP_T3ZVG9UF

/*
 *
 * First packet sended after success connections must be a Cliet Autorization request
 *  Must contain 
 *
 */

#include <QPair>
#include <QJsonDocument>
#include <QByteArray>
#include <QDataStream>
#include <QIODevice>
#include <QDateTime>

/*
 * siisty Network Packet
 */
struct iiNPack {
        public:
                enum PacketType : quint8 {
                        AUTORIZATION_REQUEST,
                        REGISTRATION_REQUEST,
                                // first meesages from remote
                        REQUEST,
                                // request from remote to server
                        RESPONSE,
                                // responce from remote
                        ERROR_MESSAGE,
                                // error message from server
                };

                enum PacketLoadType : quint8 {
                        JSON = 0,
                        XML,
                        RAW,
                };

                // TODO create error map
                enum ResponseError : quint8 {
                        ACCESS_DENIED = 0,
                                // host not identified
                        NETWORK_ERROR,
                                // common network error
                        REQUEST_ERROR,
                                // request command execution error
                        UNSUPPORTED_FORMAT,
                                // request PacketLoadType not supported
                        UNSUPPORTED_TYPE,
                                // request PacketType
                        PARSE_ERROR,
                                // parse remote request payload error
                };

                struct Header {
                        /* 0x0  - 0x3  */ quint32 Size;           /* Overall packet load size in bytes */
                        /* 0x4  - 0x11 */ qint64  ServerStamp;    /* Send time on server; using QDateTime SecsSinceEpoch */
                        /* 0x12 - 0x19 */ qint64  ClientStamp;    /* Send time on client; using QDateTime SecsSinceEpoch */
                        /* 0x20 - 0x21 */ quint8  PacketType;     /* Type of packet; enum class PacketType */
                        /* 0x22 - 0x23 */ quint8  PacketLoadType; /* Load format, see enum class PacketLoadType */
                };

                static const qsizetype HeaderSize;
                static QByteArray pack(const QByteArray& load, const PacketType, qint64 dts, qint64 dtc);
                static QByteArray packError(const QString& errDesc, const ResponseError err, qint64 dts, qint64 dtc);
                        // payload creators

                static Header unpackHeader(QDataStream& input);
                static QByteArray unpackLoad(Header, QDataStream& input);
                static ResponseError extractErrno(const QByteArray& load);
                static QString extractErrStr(const QByteArray& load);

        private:
                iiNPack();
                iiNPack(const iiNPack&) = delete;
                iiNPack(const iiNPack&&) = delete;
                iiNPack& operator = (const iiNPack&) = delete;
};

#endif /* end of include guard: IINETWORKPACKET_HPP_T3ZVG9UF */
