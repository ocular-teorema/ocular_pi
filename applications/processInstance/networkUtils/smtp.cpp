// Copyright (c) 2013 Raivis Strogonovs
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
// The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE

#include "smtp.h"

Smtp::Smtp() : QObject()
{
    m_pSocket = NULL;
    m_pTxtStream = NULL;
}

Smtp::~Smtp()
{

}

void Smtp::sendMail(
    const QString &host,
    const QString &user,
    const QString &pass,
    const QString &to,
    const QString &subject,
    const QString &body,
    int   port,
    int   timeout
)
{
    this->user = user;
    this->pass = pass;
    this->rcpt = to;
    this->port = port;
    this->timeout = timeout;

    message = "To: " + to + "\n";
    message.append("From: " + user + "\n");
    message.append("Subject: " + subject + "\n");
    message.append(body);
    message.replace(QString::fromLatin1("\n"), QString::fromLatin1("\r\n"));
    message.replace(QString::fromLatin1("\r\n.\r\n"), QString::fromLatin1("\r\n..\r\n"));

    // Create socket and text stream
    m_pSocket = new QSslSocket();
    // Create text stream
    m_pTxtStream = new QTextStream(m_pSocket);

    // Connect signals and connect to host
    connect(m_pSocket, SIGNAL(readyRead()), this, SLOT(readyRead()));
    connect(m_pSocket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(errorReceived(QAbstractSocket::SocketError)));

    if (port == 465)
    {
        m_pSocket->connectToHostEncrypted(host, port); //"smtp.gmail.com" and 465 for gmail TLS
    }
    else
    {
        m_pSocket->connectToHost(host, port);
    }

    if (!m_pSocket->waitForConnected(timeout))
    {
        ERROR_MESSAGE0(ERR_TYPE_ERROR, "Smtp", m_pSocket->errorString().toUtf8().constData());
    }

    // Init protocol state
    state = Init;
    ERROR_MESSAGE0(ERR_TYPE_MESSAGE, "EventNotifier", "Sending email notification");
}

void Smtp::errorReceived(QAbstractSocket::SocketError socketError)
{
    Q_UNUSED(socketError);
    ERROR_MESSAGE1(ERR_TYPE_ERROR, "Smtp", "Socket error received: %s", m_pSocket->errorString().toUtf8().constData());
}

void Smtp::readyRead()
{
    DEBUG_MESSAGE0("Smtp", "Smtp ready read");

    QString responseLine; // SMTP is line-oriented

    do
    {
        responseLine = m_pSocket->readLine();
        response += responseLine;
    }
    while (m_pSocket->canReadLine() && responseLine[3] != ' ');

    responseLine.truncate(3);

    DEBUG_MESSAGE1("Smtp", "Server response code: %s", responseLine.toUtf8().constData());
    DEBUG_MESSAGE1("Smtp", "Server response: %s", response.toUtf8().constData());

    if (state == Init && responseLine == "220")
    {
        // banner was okay, let's go on
        *m_pTxtStream << "EHLO localhost" <<"\r\n";
        m_pTxtStream->flush();
        state = HandShake;
    }
    else if (state == HandShake && responseLine == "250")
    {
        if (port == 465) // assuming port 465 for SSL connection
        {
            m_pSocket->startClientEncryption();
            if(!m_pSocket->waitForEncrypted(timeout))
            {
                ERROR_MESSAGE1(ERR_TYPE_ERROR,
                               "Smtp",
                               "Socket error received: %s",
                               m_pSocket->errorString().toUtf8().constData());
                state = Close;
            }
            //Send EHLO once again but now encrypted
        }
        *m_pTxtStream << "EHLO localhost" << "\r\n";
        m_pTxtStream->flush();
        state = Auth;
    }
    else if (state == Auth && responseLine == "250")
    {
        // Trying AUTH
        *m_pTxtStream << "AUTH LOGIN" << "\r\n";
        m_pTxtStream->flush();
        state = User;
    }
    else if (state == User && responseLine == "334")
    {
        //Trying User
        //GMAIL is using XOAUTH2 protocol, which basically means that password and username has to be sent in base64 coding
        //https://developers.google.com/gmail/xoauth2_protocol
        *m_pTxtStream << QByteArray().append(user).toBase64()  << "\r\n";
        m_pTxtStream->flush();
        state = Pass;
    }
    else if (state == Pass && responseLine == "334")
    {
        //Trying pass
        *m_pTxtStream << QByteArray().append(pass).toBase64() << "\r\n";
        m_pTxtStream->flush();
        state = Mail;
    }
    else if (state == Mail && responseLine == "235")
    {
        // HELO response was okay (well, it has to be)
        //Apperantly for Google it is mandatory to have MAIL FROM and RCPT email formated the following way -> <email@gmail.com>
        *m_pTxtStream << "MAIL FROM:<" << user << ">\r\n";
        m_pTxtStream->flush();
        state = Rcpt;
    }
    else if (state == Rcpt && responseLine == "250")
    {
        //Apperantly for Google it is mandatory to have MAIL FROM and RCPT email formated the following way -> <email@gmail.com>
        *m_pTxtStream << "RCPT TO:<" << rcpt << ">\r\n";
        m_pTxtStream->flush();
        state = Data;
    }
    else if (state == Data && responseLine == "250")
    {
        *m_pTxtStream << "DATA\r\n";
        m_pTxtStream->flush();
        state = Body;
    }
    else if (state == Body && responseLine == "354")
    {
        *m_pTxtStream << message << "\r\n.\r\n";
        m_pTxtStream->flush();
        state = Quit;
    }
    else if (state == Quit && responseLine == "250")
    {
        *m_pTxtStream << "QUIT\r\n";
        m_pTxtStream->flush();
        // here, we just close.
        state = Close;
        ERROR_MESSAGE0(ERR_TYPE_MESSAGE, "EventNotifier", "Email sent");
    }
    else if (state == Close)
    {
        closeConnection();
        return;
    }
    else
    {
        // something broke
        ERROR_MESSAGE1(ERR_TYPE_ERROR, "Smtp", "Unexpected reply from SMTP server: %s", response.toUtf8().constData());
        state = Close;
    }
    response = "";
}

void Smtp::closeConnection()
{
    m_pTxtStream->reset();
    m_pTxtStream->resetStatus();
    m_pSocket->disconnect();
    if (!m_pSocket->waitForDisconnected(2000))
    {
        ERROR_MESSAGE0(ERR_TYPE_ERROR, "Smtp", m_pSocket->errorString().toUtf8().constData());
    }

    disconnect(m_pSocket, SIGNAL(readyRead()), this, SLOT(readyRead()));
    disconnect(m_pSocket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(errorReceived(QAbstractSocket::SocketError)));

    SAFE_DELETE(m_pSocket);
    SAFE_DELETE(m_pTxtStream);
    ERROR_MESSAGE0(ERR_TYPE_MESSAGE, "EventNotifier", "Connection closed. Socket and text stream deleted");
}
