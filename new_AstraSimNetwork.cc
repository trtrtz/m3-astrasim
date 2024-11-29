#include <json/json.hpp>
#include "astra-sim/system/AstraNetworkAPI.hh"
#include "astra-sim/system/Sys.hh"
#include "extern/remote_memory_backend/analytical/AnalyticalRemoteMemory.hh"

#include <execinfo.h>
#include <stdio.h>
#include <unistd.h>
#include <fstream>
#include <iostream>
#include <queue>
#include <string>
#include <thread>
#include <vector>
#include "entry.h"
#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"

using namespace std;
using namespace ns3;
using json = nlohmann::json;

class ASTRASimNetwork : public AstraSim::AstraNetworkAPI {
 public:
  ASTRASimNetwork(int rank) : AstraNetworkAPI(rank) {}

  ~ASTRASimNetwork() {}

  int sim_finish() {
    for (auto it = node_to_bytes_sent_map.begin();
         it != node_to_bytes_sent_map.end();
         it++) {
      pair<int, int> p = it->first;
      if (p.second == 0) {
        cout << "All data sent from node " << p.first << " is " << it->second
             << "\n";
      } else {
        cout << "All data received by node " << p.first << " is " << it->second
             << "\n";
      }
    }
    return 0;
  }

  double sim_time_resolution() {
    return 0;
  }

  void handleEvent(int dst, int cnt) {}

  AstraSim::timespec_t sim_get_time() {
    AstraSim::timespec_t timeSpec;
    timeSpec.time_res = AstraSim::NS;
    timeSpec.time_val = 0; // Return zero as no simulation time is needed
    return timeSpec;
  }

  virtual void sim_schedule(
      AstraSim::timespec_t delta,
      void (*fun_ptr)(void* fun_arg),
      void* fun_arg) {
    // Remove scheduling since no simulation will occur
    return;
  }

  virtual int sim_send(
      void* buffer,
      uint64_t message_size,
      int type,
      int dst_id,
      int tag,
      AstraSim::sim_request* request,
      void (*msg_handler)(void* fun_arg),
      void* fun_arg) {
    int src_id = rank;

    // Use send_flow_collective for the entire workload instead of segmented events
    send_flow_collective(src_id, dst_id, message_size, msg_handler, fun_arg, tag);
    return 0;
  }

  virtual int sim_recv(
      void* buffer,
      uint64_t message_size,
      int type,
      int src_id,
      int tag,
      AstraSim::sim_request* request,
      void (*msg_handler)(void* fun_arg),
      void* fun_arg) {
    int dst_id = rank;
    MsgEvent recv_event =
        MsgEvent(src_id, dst_id, 1, message_size, fun_arg, msg_handler);
    MsgEventKey recv_event_key =
        make_pair(tag, make_pair(recv_event.src_id, recv_event.dst_id));

    // Handle receive events without simulation
    if (received_msg_standby_hash.find(recv_event_key) !=
        received_msg_standby_hash.end()) {
      int received_msg_bytes = received_msg_standby_hash[recv_event_key];
      if (received_msg_bytes == message_size) {
        received_msg_standby_hash.erase(recv_event_key);
        recv_event.callHandler();
      } else if (received_msg_bytes > message_size) {
        received_msg_standby_hash[recv_event_key] =
            received_msg_bytes - message_size;
        recv_event.callHandler();
      } else {
        received_msg_standby_hash.erase(recv_event_key);
        recv_event.remaining_msg_bytes -= received_msg_bytes;
        sim_recv_waiting_hash[recv_event_key] = recv_event;
      }
    } else {
      if (sim_recv_waiting_hash.find(recv_event_key) ==
          sim_recv_waiting_hash.end()) {
        sim_recv_waiting_hash[recv_event_key] = recv_event;
      } else {
        int expecting_msg_bytes =
            sim_recv_waiting_hash[recv_event_key].remaining_msg_bytes;
        recv_event.remaining_msg_bytes += expecting_msg_bytes;
        sim_recv_waiting_hash[recv_event_key] = recv_event;
      }
    }
    return 0;
  }
};

// Command line arguments and default values.
string workload_configuration;
string system_configuration;
string network_configuration;
string memory_configuration;
string comm_group_configuration;
string logical_topology_configuration;
int num_queues_per_dim = 1;
double comm_scale = 1;
double injection_scale = 1;
bool rendezvous_protocol = false;
auto logical_dims = vector<int>();
int num_npus = 1;
auto queues_per_dim = vector<int>();

void read_logical_topo_config(
    string network_configuration,
    vector<int>& logical_dims) {
  ifstream inFile;
  inFile.open(network_configuration);
  if (!inFile) {
    cerr << "Unable to open file: " << network_configuration << endl;
    exit(1);
  }

  json j;
  inFile >> j;
  if (j.contains("logical-dims")) {
    vector<string> logical_dims_str_vec = j["logical-dims"];
    for (auto logical_dims_str : logical_dims_str_vec) {
      logical_dims.push_back(stoi(logical_dims_str));
    }
  }

  for (auto num_npus_per_dim : logical_dims) {
    num_npus *= num_npus_per_dim;
  }
}

// Read command line arguments.
void parse_args(int argc, char* argv[]) {
  CommandLine cmd;
  cmd.AddValue(
      "workload-configuration",
      "Workload configuration file.",
      workload_configuration);
  cmd.AddValue(
      "system-configuration",
      "System configuration file",
      system_configuration);
  cmd.AddValue(
      "network-configuration",
      "Network configuration file",
      network_configuration);
  cmd.AddValue(
      "remote-memory-configuration",
      "Memory configuration file",
      memory_configuration);
  cmd.AddValue(
      "comm-group-configuration",
      "Communicator group configuration file",
      comm_group_configuration);
  cmd.AddValue(
      "logical-topology-configuration",
      "Logical topology configuration file",
      logical_topology_configuration);
  cmd.AddValue(
      "num-queues-per-dim",
      "Number of queues per each dimension",
      num_queues_per_dim);
  cmd.AddValue("comm-scale", "Communication scale", comm_scale);
  cmd.AddValue("injection-scale", "Injection scale", injection_scale);
  cmd.AddValue(
      "rendezvous-protocol",
      "Whether to enable rendezvous protocol",
      rendezvous_protocol);

  cmd.Parse(argc, argv);
}

int main(int argc, char* argv[]) {
  cout << "ASTRA-sim + NS3" << endl;

  parse_args(argc, argv);
  read_logical_topo_config(logical_topology_configuration, logical_dims);

  vector<ASTRASimNetwork*> networks(num_npus, nullptr);
  vector<AstraSim::Sys*> systems(num_npus, nullptr);
  Analytical::AnalyticalRemoteMemory* mem =
      new Analytical::AnalyticalRemoteMemory(memory_configuration);

  for (int npu_id = 0; npu_id < num_npus; npu_id++) {
    networks[npu_id] = new ASTRASimNetwork(npu_id);
    systems[npu_id] = new AstraSim::Sys(
        npu_id,
        workload_configuration,
        comm_group_configuration,
        system_configuration,
        mem,
        networks[npu_id],
        logical_dims,
        queues_per_dim,
        injection_scale,
        comm_scale,
        rendezvous_protocol);
  }

  if (auto ok = setup_ns3_simulation(network_configuration); ok == -1) {
    std::cerr << "Fail to setup ns3 simulation." << std::endl;
    return -1;
  }

  return 0;
}
