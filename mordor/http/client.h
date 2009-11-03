#ifndef __MORDOR_HTTP_CLIENT_H__
#define __MORDOR_HTTP_CLIENT_H__
// Copyright (c) 2009 - Decho Corp.

#include <list>
#include <set>

#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>

#include "connection.h"
#include "mordor/fiber.h"
#include "mordor/streams/stream.h"
#include "multipart.h"

namespace Mordor {

class Scheduler;

namespace HTTP {

class ClientConnection;
class ClientRequest : public boost::enable_shared_from_this<ClientRequest>, boost::noncopyable
{
private:
    friend class ClientConnection;

    enum State {
        WAITING,
        HEADERS,
        INFLIGHT,
        COMPLETE,
        ERROR
    };
public:
    typedef boost::shared_ptr<ClientRequest> ptr;
    typedef boost::weak_ptr<ClientRequest> weak_ptr;

private:
    ClientRequest(boost::shared_ptr<ClientConnection> conn, const Request &request);

public:
    ~ClientRequest();

    const Request &request();
    Stream::ptr requestStream();
    Multipart::ptr requestMultipart();
    EntityHeaders &requestTrailer();

    const Response &response();
    bool hasResponseBody();
    Stream::ptr responseStream();
    Multipart::ptr responseMultipart();
    const EntityHeaders &responseTrailer() const;

    Stream::ptr stream();

    void cancel(bool abort = false);
    void finish();
    void ensureResponse();

private:
    void doRequest();
    void requestMultipartDone();
    void requestDone();
    void responseDone();

private:
    boost::shared_ptr<ClientConnection> m_conn;
    Scheduler *m_scheduler;
    Fiber::ptr m_fiber;
    Request m_request;
    Response m_response;
    EntityHeaders m_requestTrailer, m_responseTrailer;
    State m_requestState, m_responseState;
    bool m_cancelled, m_aborted, m_badResponse, m_incompleteResponse, m_badTrailer, m_incompleteTrailer, m_hasResponseBody;
    Stream::ptr m_requestStream;
    boost::weak_ptr<Stream> m_responseStream;
    Multipart::ptr m_requestMultipart;
    boost::weak_ptr<Multipart> m_responseMultipart;
};

class ClientConnection : public Connection, public boost::enable_shared_from_this<ClientConnection>, boost::noncopyable
{
private:
    friend class ClientRequest;

public:
    typedef boost::shared_ptr<ClientConnection> ptr;

public:
    ClientConnection(Stream::ptr stream);

    ClientRequest::ptr request(const Request &requestHeaders);

    bool newRequestsAllowed();

private:
    void scheduleNextRequest(ClientRequest *currentRequest);
    void scheduleNextResponse(ClientRequest *currentRequest);
    void scheduleAllWaitingRequests();
    void scheduleAllWaitingResponses();

private:
    boost::mutex m_mutex;
    std::list<ClientRequest *> m_pendingRequests;
    std::list<ClientRequest *>::iterator m_currentRequest;
    std::set<ClientRequest *> m_waitingResponses;
    bool m_allowNewRequests;
    bool m_priorRequestFailed, m_priorResponseFailed, m_priorResponseClosed;

    void invariant() const;
};

}}

#endif
