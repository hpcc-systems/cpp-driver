/*
  Copyright (c) 2013 Matthew Stump

  This file is part of cassandra.

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

#ifndef CQL_SESSION_IMPL_H_
#define CQL_SESSION_IMPL_H_

#include <istream>
#include <list>
#include <map>
#include <ostream>
#include <string>

#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#include <boost/noncopyable.hpp>
#include <boost/ptr_container/ptr_deque.hpp>
#include <boost/thread/mutex.hpp>
#include "cql/cql.hpp"
#include "cql/cql_connection.hpp"
#include "cql/cql_session.hpp"
#include "cql/cql_builder.hpp"
#include "cql/cql_cluster.hpp"
#include "cql/policies/cql_load_balancing_policy.hpp"


namespace cql {

// Forward declarations
class cql_event_t;
class cql_result_t;
class cql_execute_t;
struct cql_error_t;

class cql_session_callback_info_t {
public:
    cql_session_callback_info_t(
        const cql_session_t::cql_client_callback_t  client  = 0,
        const cql_session_t::cql_ready_callback_t   ready   = 0,
        const cql_session_t::cql_defunct_callback_t defunct = 0,
        const cql_session_t::cql_log_callback_t     log     = 0)
            : _client_callback(client),
              _ready_callback(ready),
              _defunct_callback(defunct),
              _log_callback(log) { }

    inline void 
    set_client_callback(const cql_session_t::cql_client_callback_t& client_callback) {
        _client_callback = client_callback;
    }
    
    inline void
    set_ready_callback(const cql_session_t::cql_ready_callback_t& ready_callback) {
        _ready_callback = ready_callback;
    }
    
    inline void
    set_defunct_callback(const cql_session_t::cql_defunct_callback_t& defunct_callback) {
        _defunct_callback = defunct_callback;
    }
    
    inline void
    set_log_callback(const cql_session_t::cql_log_callback_t& log_callback) {
        _log_callback = log_callback;
    }
    
private:
    cql_session_t::cql_client_callback_t client_callback() const {
        return _client_callback;
    }
    
    cql_session_t::cql_ready_callback_t ready_callback() const {
        return _ready_callback;
    }
    
    cql_session_t::cql_defunct_callback_t defunct_callback() const {
        return _defunct_callback;
    }
    
    cql_session_t::cql_log_callback_t log_callback() const {
        return _log_callback;
    }
    
    friend class cql_session_impl_t;
    
private:
    
    cql_session_t::cql_client_callback_t   _client_callback;
    cql_session_t::cql_ready_callback_t    _ready_callback;
    cql_session_t::cql_defunct_callback_t  _defunct_callback;
    cql_session_t::cql_log_callback_t      _log_callback;
};

class cql_session_impl_t :
    public cql_session_t,
    boost::noncopyable 
{
public:

    cql_session_impl_t(
        const cql_session_callback_info_t& callbacks,
        boost::shared_ptr<cql_configuration_t>  configuration);

    void init();

    boost::shared_ptr<cql_connection_t> 
	connect(
        cql::cql_query_plan_t& query_plan, 
        int& stream_id, 
        std::list<std::string>& tried_hosts);
    
	boost::shared_ptr<cql_connection_t> 
	allocate_connection(const boost::asio::ip::address& address, cql_host_distance_enum distance);
    
	void 
	trashcan_put(boost::shared_ptr<cql_connection_t> connection);
    
	boost::shared_ptr<cql_connection_t> 
	trashcan_recycle(const boost::asio::ip::address& address);
    
	void 
	free_connection(boost::shared_ptr<cql_connection_t> connection);

    virtual 
	~cql_session_impl_t() { }

private:    
    struct client_container_t {
        client_container_t(
            const boost::shared_ptr<cql::cql_connection_t>& c) :
            client(c),
            errors(0)
        {}

        boost::shared_ptr<cql::cql_connection_t> client;
        size_t errors;
    };
    
    typedef 
        boost::ptr_deque<client_container_t> 
        clients_collection_t;
    
    typedef
        std::map<long, boost::shared_ptr<cql_connection_t> >
        connections_collection_t;  
    
    typedef
        std::map<std::string, connections_collection_t>
        connection_pool_t;
    
    cql_session_impl_t(
        cql::cql_session_t::cql_client_callback_t  client_callback,
        cql::cql_session_t::cql_ready_callback_t   ready_callback,
        cql::cql_session_t::cql_defunct_callback_t defunct_callback);

    cql_session_impl_t(
        cql::cql_session_t::cql_client_callback_t  client_callback,
        cql::cql_session_t::cql_ready_callback_t   ready_callback,
        cql::cql_session_t::cql_defunct_callback_t defunct_callback,
        cql::cql_session_t::cql_log_callback_t     log_callback);

    cql_session_impl_t(
        cql::cql_session_t::cql_client_callback_t  client_callback,
        cql::cql_session_t::cql_ready_callback_t   ready_callback,
        cql::cql_session_t::cql_defunct_callback_t defunct_callback,
        cql::cql_session_t::cql_log_callback_t     log_callback,
        size_t                                     reconnect_limit);

    boost::shared_future<cql::cql_future_connection_t>
    add_client(
        const std::string& server,
        unsigned int       port);

    boost::shared_future<cql::cql_future_connection_t>
    add_client(
        const std::string&                      server,
        unsigned int                            port,
        cql::cql_connection_t::cql_event_callback_t event_callback,
        const std::list<std::string>&           events);

    boost::shared_future<cql::cql_future_connection_t>
    add_client(
        const std::string&                        server,
        unsigned int                              port,
        cql::cql_connection_t::cql_event_callback_t   event_callback,
        const std::list<std::string>&             events,
        const std::map<std::string, std::string>& credentials);

    cql::cql_stream_id_t
    query(
        const std::string&                        query,
        cql::cql_consistency_enum				  consistency,
        cql::cql_connection_t::cql_message_callback_t callback,
        cql::cql_connection_t::cql_message_errback_t  errback);

    cql::cql_stream_id_t
    prepare(
        const std::string&                        query,
        cql::cql_connection_t::cql_message_callback_t callback,
        cql::cql_connection_t::cql_message_errback_t  errback);

    cql::cql_stream_id_t
    execute(
        cql::cql_execute_t*                       message,
        cql::cql_connection_t::cql_message_callback_t callback,
        cql::cql_connection_t::cql_message_errback_t  errback);

    boost::shared_future<cql::cql_future_result_t>
    query(
        const std::string& query,
        cql::cql_consistency_enum consistency);

    boost::shared_future<cql::cql_future_result_t>
    prepare(
        const std::string& query);

    boost::shared_future<cql::cql_future_result_t>
    execute(
        cql::cql_execute_t* message);

    bool
    defunct();

    bool
    ready();

    void
    close();

    size_t
    size();

    bool
    empty();

    inline void
    log(
        cql::cql_short_t level,
        const std::string& message);

    void
    connect_callback(
        boost::shared_ptr<boost::promise<cql::cql_future_connection_t> > promise,
        cql::cql_connection_t& client);

    void
    connect_errback(
        boost::shared_ptr<boost::promise<cql::cql_future_connection_t> > promise,
        cql::cql_connection_t& client,
        const cql_error_t& error);

    void
    connect_future_callback(
        boost::shared_ptr<boost::promise<cql::cql_future_connection_t> > promise,
        cql::cql_connection_t&                                               client);

    void
    connect_future_errback(
        boost::shared_ptr<boost::promise<cql::cql_future_connection_t> > promise,
        cql::cql_connection_t&                                               client,
        const cql_error_t&                     error);

    cql::cql_connection_t*
    next_client();

    cql_host_distance_enum
    get_host_distance(boost::shared_ptr<cql_host_t> host);
     
    void
    free_connections(
        connections_collection_t& connections, 
        const std::list<long>& connections_to_remove);

    
    connections_collection_t&
    add_to_connection_pool(
        const boost::asio::ip::address& host_address);
    
    clients_collection_t                         _clients;
    boost::mutex                                 _mutex;
    bool                                         _ready;
    bool                                         _defunct;
    cql::cql_session_t::cql_client_callback_t    _client_callback;
    cql::cql_session_t::cql_ready_callback_t     _ready_callback;
    cql::cql_session_t::cql_defunct_callback_t   _defunct_callback;
    cql::cql_session_t::cql_log_callback_t       _log_callback;
    cql::cql_session_t::cql_connection_errback_t _connect_errback;
    size_t                                       _reconnect_limit;
    
    boost::shared_ptr<cql_configuration_t> _configuration;
    boost::mutex _connection_pool_mtx;
    connection_pool_t _trashcan;
    connection_pool_t _connection_pool;
    std::map<std::string, long> _allocated_connections;
};

} // namespace cql

#endif // CQL_SESSION_IMPL_H_
