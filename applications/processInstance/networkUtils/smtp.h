// Copyright (c) 2013 Raivis Strogonovs
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
// The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#ifndef SMTP_H
#define SMTP_H

#include <QString>
#include <QByteArray>
#include <QTextStream>
#include <QtNetwork/QSslSocket>
#include <QtWidgets/QMessageBox>
#include <QtNetwork/QAbstractSocket>

#include "cameraPipelineCommon.h"

class Smtp : public QObject
{
    Q_OBJECT
public:

    Smtp();
    ~Smtp();

    void sendMail(
        const QString &host,
        const QString &user,
        const QString &pass,
        const QString &to,
        const QString &subject,
        const QString &body,
        int   port = 25,
        int   timeout = 10000);

private slots:
    void errorReceived(QAbstractSocket::SocketError socketError);
    void readyRead();
    void closeConnection();

private:
    QTextStream *m_pTxtStream;
    QSslSocket  *m_pSocket;
    QString     response;
    QString     message;
    QString     rcpt;
    QString     user;
    QString     pass;
    int         port;
    int         timeout;
    int         state;
    enum        states {Tls, HandShake, Auth, User, Pass, Rcpt, Mail, Data, Init, Body, Quit, Close};
};
#endif
