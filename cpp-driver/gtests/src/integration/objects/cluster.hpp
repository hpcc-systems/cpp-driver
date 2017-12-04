/*
  Copyright (c) DataStax, Inc.

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

#ifndef __TEST_CLUSTER_HPP__
#define __TEST_CLUSTER_HPP__
#include "cassandra.h"

#include "object_base.hpp"

#include "session.hpp"

#include <string>

#include <gtest/gtest.h>

namespace test {
namespace driver {

/**
 * Wrapped cluster object (builder)
 */
class Cluster : public Object<CassCluster, cass_cluster_free> {
public:
  /**
   * Create the cluster for the builder object
   */
  Cluster()
    : Object<CassCluster, cass_cluster_free>(cass_cluster_new()) { }

  /**
   * Create the cluster for the builder object
   *
   * @param cluster Already defined cluster object to utilize
   */
  Cluster(CassCluster* cluster)
    : Object<CassCluster, cass_cluster_free>(cluster) { }

  /**
   * Create the cluster object from a shared reference
   *
   * @param cluster Shared reference
   */
  Cluster(Ptr cluster)
    : Object<CassCluster, cass_cluster_free>(cluster) { }

  /**
   * Destroy the cluster
   */
  virtual ~Cluster() {};

  /**
   * Build/Create the cluster
   *
   * @return Cluster object
   */
  static Cluster build() {
    return Cluster();
  }

  /**
   * Use the newest beta protocol version
   *
   * @param enable True if beta protocol should be enable; false the highest
   *               non-beta protocol will be used (unless set) (default: false)
   * @return Cluster object
   */
  Cluster& with_beta_protocol(bool enable = false) {
    EXPECT_EQ(CASS_OK, cass_cluster_set_use_beta_protocol_version(get(),
              (enable == true ? cass_true : cass_false)));
    return *this;
  }

  /**
   * Sets the amount of time between heartbeat messages and controls the amount
   * of time the connection must be idle before sending heartbeat messages. This
   * is useful for preventing intermediate network devices from dropping
   * connections
   *
   * @param interval_s Heartbeat time interval (in seconds); 0 to disable
   *                   heartbeat messages (default: 30s)
   * @return Cluster object
   */
  Cluster& with_connection_heartbeat_interval(unsigned int interval_s = 30u) {
    cass_cluster_set_connection_heartbeat_interval(get(), interval_s);
    return *this;
  }

  /**
   * Assign/Append the contact points; passing an empty string will clear
   * the contact points
   *
   * @param contact_points A comma delimited list of hosts (addresses or
   *                       names)
   * @return Cluster object
   */
  Cluster& with_contact_points(const std::string& contact_points) {
    EXPECT_EQ(CASS_OK, cass_cluster_set_contact_points(get(),
      contact_points.c_str()));
    return *this;
  }

  /**
   * Assign the number of connections made to each node/server for each
   * connections thread
   *
   * NOTE: One extra connection is established (the control connection)
   *
   * @param connections Number of connection per host (default: 1)
   * @return Cluster object
   */
  Cluster& with_core_connections_per_host(unsigned int connections = 1u) {
    EXPECT_EQ(CASS_OK, cass_cluster_set_core_connections_per_host(get(),
      connections));
    return *this;
  }

  /**
   * Set/Add a execution profile
   *
   * @param name Name for the execution profile
   * @param profile Execution profile to add to the cluster
   * @return Cluster object
   */
  Cluster& with_execution_profile(const std::string& name,
                                  ExecutionProfile profile) {
    EXPECT_EQ(CASS_OK, cass_cluster_set_execution_profile(get(),
              name.c_str(), profile.get()));
    return *this;
  }

  /**
   * Enable/Disable the use of hostname resolution
   *
   * This is useful for authentication (Kerberos) or encryption (SSL)
   * services that require a valid hostname for verification.
   *
   * NOTE: Not available if using libuv 0.10.x or earlier
   *
   * @param enable True if hostname resolution should be enabled; false
   *               otherwise (default: true)
   * @return Cluster object
   */
  Cluster& with_hostname_resolution(bool enable = true) {
    EXPECT_EQ(CASS_OK, cass_cluster_set_use_hostname_resolution(get(),
      (enable == true ? cass_true : cass_false)));
    return *this;
  }

  /**
   * Enable round robin load balance policy for statement/batch execution
   *
   * @return Cluster object
   */
  Cluster& with_load_balance_round_robin() {
    cass_cluster_set_load_balance_round_robin(get());
    return *this;
  }

  /**
   * Assign the use of a particular binary protocol version; driver will
   * automatically downgrade to the lowest server supported version on
   * connection
   *
   * @param protocol_version Binary protocol version
   * @return Cluster object
   */
  Cluster& with_protocol_version(int protocol_version) {
    EXPECT_EQ(CASS_OK, cass_cluster_set_protocol_version(get(),
      protocol_version));
    return *this;
  }

  /**
   * Enable/Disable the randomization of the contact points list
   *
   * @param enable True if contact points should be randomized false otherwise
   *               (default: true)
   * @return Cluster object
   */
  Cluster& with_randomized_contact_points(bool enable = true) {
    cass_cluster_set_use_randomized_contact_points(get(),
      (enable == true ? cass_true : cass_false));
    return *this;
  }

  /**
   * Sets the timeout (in milliseconds) for waiting for a response from a node
   *
   * @param timeout_ms Request timeout in milliseconds; 0 to disable timeout
   *                   (default: 12s)
   */
  Cluster& with_request_timeout(unsigned int timeout_ms = 12000u) {
    cass_cluster_set_request_timeout(get(), timeout_ms);
    return *this;
  }

  /**
   * Sets the retry policy used for all requests unless overridden by setting
   * a retry policy on a statement or a batch.
   *
   * @param retry_policy Retry policy to assign to the cluster profile
   */
  Cluster& with_retry_policy(RetryPolicy retry_policy) {
    cass_cluster_set_retry_policy(get(), retry_policy.get());
    return *this;
  }

  /**
   * Enable/Disable the schema metadata
   *
   * If disabled this allows the driver to skip over retrieving and
   * updating schema metadata, but it also disables the usage of token-aware
   * routing and session->schema() will always return an empty object. This
   * can be useful for reducing the startup overhead of short-lived sessions
   *
   * @param enable True if schema metada should be enabled; false otherwise
   *               (default: true)
   * @return Cluster object
   */
  Cluster& with_schema_metadata(bool enable = true) {
    cass_cluster_set_use_schema(get(), (enable == true ? cass_true : cass_false));
    return *this;
  }

  /**
   * Create a new session and establish a connection to the server;
   * synchronously
   *
   * @param keyspace Keyspace to use (default: None)
   * @return Session object
   * @throws Session::Exception If session could not be established
   */
  Session connect(const std::string& keyspace = "", bool assert_ok = true) {
    return Session::connect(get(), keyspace, assert_ok);
  }
};

} // namespace driver
} // namespace test

#endif // __TEST_CLUSTER_HPP__
