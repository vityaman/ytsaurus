//
// HTTPSession.h
//
// Library: Net
// Package: HTTP
// Module:  HTTPSession
//
// Definition of the HTTPSession class.
//
// Copyright (c) 2005-2006, Applied Informatics Software Engineering GmbH.
// and Contributors.
//
// SPDX-License-Identifier:	BSL-1.0
//


#ifndef DB_Net_HTTPSession_INCLUDED
#define DB_Net_HTTPSession_INCLUDED


#include <ios>
#include "DBPoco/Any.h"
#include "DBPoco/Buffer.h"
#include "DBPoco/Exception.h"
#include "DBPoco/Net/Net.h"
#include "DBPoco/Net/StreamSocket.h"
#include "DBPoco/Timespan.h"


namespace DBPoco
{
namespace Net
{


    class Net_API HTTPSession
    /// HTTPSession implements basic HTTP session management
    /// for both HTTP clients and HTTP servers.
    ///
    /// HTTPSession implements buffering for HTTP connections, as well
    /// as specific support for the various HTTP stream classes.
    ///
    /// This class can not be instantiated. HTTPClientSession or
    /// HTTPServerSession must be used instead.
    {
    public:
        void setKeepAlive(bool keepAlive);
        /// Sets the keep-alive flag for this session.
        ///
        /// If the keep-alive flag is enabled, persistent
        /// HTTP/1.1 connections are supported.

        bool getKeepAlive() const;
        /// Returns the value of the keep-alive flag for
        /// this session.

        void setTimeout(const DBPoco::Timespan & timeout);
        /// Sets the timeout for the HTTP session.

        void
        setTimeout(const DBPoco::Timespan & connectionTimeout, const DBPoco::Timespan & sendTimeout, const DBPoco::Timespan & receiveTimeout);
        /// Sets different timeouts for the HTTP session.

        DBPoco::Timespan getTimeout() const;
        /// Returns the timeout for the HTTP session.

        DBPoco::Timespan getConnectionTimeout() const;
        /// Returns connection timeout for the HTTP session.

        DBPoco::Timespan getSendTimeout() const;
        /// Returns send timeout for the HTTP session.

        DBPoco::Timespan getReceiveTimeout() const;
        /// Returns receive timeout for the HTTP session.

        bool connected() const;
        /// Returns true if the underlying socket is connected.

        virtual void abort();
        /// Aborts a session in progress by shutting down
        /// and closing the underlying socket.

        const DBPoco::Exception * networkException() const;
        /// If sending or receiving data over the underlying
        /// socket connection resulted in an exception, a
        /// pointer to this exception is returned.
        ///
        /// Otherwise, NULL is returned.

        void attachSessionData(const DBPoco::Any & data);
        /// Allows to attach an application-specific data
        /// item to the session.
        ///
        /// On the server side, this can be used to manage
        /// data that must be maintained over the entire
        /// lifetime of a persistent connection (that is,
        /// multiple requests sent over the same connection).

        const DBPoco::Any & sessionData() const;
        /// Returns the data attached with attachSessionData(),
        /// or an empty DBPoco::Any if no user data has been
        /// attached.

        enum
        {
            HTTP_PORT = 80
        };

        StreamSocket detachSocket();
        /// Detaches the socket from the session.
        ///
        /// The socket is returned, and a new, uninitialized socket is
        /// attached to the session.

        StreamSocket & socket();
        /// Returns a reference to the underlying socket.

        void drainBuffer(DBPoco::Buffer<char> & buffer);
        /// Copies all bytes remaining in the internal buffer to the
        /// given DBPoco::Buffer, resizing it as necessary.
        ///
        /// This is usually used together with detachSocket() to
        /// obtain any data already read from the socket, but not
        /// yet processed.

    protected:
        HTTPSession();
        /// Creates a HTTP session using an
        /// unconnected stream socket.

        HTTPSession(const StreamSocket & socket);
        /// Creates a HTTP session using the
        /// given socket. The session takes ownership
        /// of the socket and closes it when it's no
        /// longer used.

        HTTPSession(const StreamSocket & socket, bool keepAlive);
        /// Creates a HTTP session using the
        /// given socket. The session takes ownership
        /// of the socket and closes it when it's no
        /// longer used.

        virtual ~HTTPSession();
        /// Destroys the HTTPSession and closes the
        /// underlying socket.

        int get();
        /// Returns the next byte in the buffer.
        /// Reads more data from the socket if there are
        /// no bytes left in the buffer.

        int peek();
        /// Peeks at the next character in the buffer.
        /// Reads more data from the socket if there are
        /// no bytes left in the buffer.

        virtual int read(char * buffer, std::streamsize length);
        /// Reads up to length bytes.
        ///
        /// If there is data in the buffer, this data
        /// is returned. Otherwise, data is read from
        /// the socket to avoid unnecessary buffering.

        virtual int write(const char * buffer, std::streamsize length);
        /// Writes data to the socket.

        int receive(char * buffer, int length);
        /// Reads up to length bytes.

        int buffered() const;
        /// Returns the number of bytes in the buffer.

        void refill();
        /// Refills the internal buffer.

        virtual void connect(const SocketAddress & address);
        /// Connects the underlying socket to the given address
        /// and sets the socket's receive timeout.

        void attachSocket(const StreamSocket & socket);
        /// Attaches a socket to the session, replacing the
        /// previously attached socket.

        void close();
        /// Closes the underlying socket.

        void setException(const DBPoco::Exception & exc);
        /// Stores a clone of the exception.

        void clearException();
        /// Clears the stored exception.

    private:
        enum
        {
            HTTP_DEFAULT_TIMEOUT = 60000000,
            HTTP_DEFAULT_CONNECTION_TIMEOUT = 30000000
        };

        HTTPSession(const HTTPSession &);
        HTTPSession & operator=(const HTTPSession &);

        StreamSocket _socket;
        std::unique_ptr<char[]> _pBuffer;
        char * _pCurrent;
        char * _pEnd;
        bool _keepAlive;
        DBPoco::Timespan _connectionTimeout;
        DBPoco::Timespan _receiveTimeout;
        DBPoco::Timespan _sendTimeout;
        DBPoco::Exception * _pException;
        DBPoco::Any _data;

        friend class HTTPStreamBuf;
        friend class HTTPHeaderStreamBuf;
        friend class HTTPFixedLengthStreamBuf;
        friend class HTTPChunkedStreamBuf;
    };


    //
    // inlines
    //
    inline bool HTTPSession::getKeepAlive() const
    {
        return _keepAlive;
    }

    inline DBPoco::Timespan HTTPSession::getTimeout() const
    {
        return _receiveTimeout;
    }

    inline DBPoco::Timespan HTTPSession::getConnectionTimeout() const
    {
        return _connectionTimeout;
    }

    inline DBPoco::Timespan HTTPSession::getSendTimeout() const
    {
        return _sendTimeout;
    }

    inline DBPoco::Timespan HTTPSession::getReceiveTimeout() const
    {
        return _receiveTimeout;
    }

    inline StreamSocket & HTTPSession::socket()
    {
        return _socket;
    }


    inline const DBPoco::Exception * HTTPSession::networkException() const
    {
        return _pException;
    }


    inline int HTTPSession::buffered() const
    {
        return static_cast<int>(_pEnd - _pCurrent);
    }


    inline const DBPoco::Any & HTTPSession::sessionData() const
    {
        return _data;
    }


}
} // namespace DBPoco::Net


#endif // DB_Net_HTTPSession_INCLUDED
