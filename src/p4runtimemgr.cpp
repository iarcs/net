#include "p4runtimemgr.hpp"

#include <PI/proto/p4info_to_and_from_proto.h>
#include <boost/filesystem.hpp>
#include <google/protobuf/text_format.h>
#include <grpcpp/grpcpp.h>
#include <p4/tmp/p4config.grpc.pb.h>
#include <p4/v1/p4runtime.grpc.pb.h>

#include "controller.hpp"
#include "logger.hpp"
#include "network.hpp"
#include "switch.hpp"

using namespace std;
using grpc::CreateChannel;
using grpc::InsecureChannelCredentials;
namespace fs = boost::filesystem;

P4RuntimeMgr::P4RuntimeMgr(const Switch *sw) : _sw(sw), _p4info(nullptr) {}

P4RuntimeMgr::~P4RuntimeMgr() {
    if (this->_p4info) {
        pi_destroy_config(this->_p4info);
    }
}

void P4RuntimeMgr::logMessage(const google::protobuf::Message &msg) {
    if (this->_p4rtLogger) {
        this->_p4rtLogger->info("message:\n" + msg.DebugString());
    }
}

void P4RuntimeMgr::connect() {
    // Connect to the p4runtime server (in bmv2)
    const string servAddr = "127.0.0.1:" + to_string(this->_sw->grpcPort());
    auto channel = CreateChannel(servAddr, InsecureChannelCredentials());
    this->_piStub = p4::v1::P4Runtime::NewStub(channel);
    this->_streamClient = make_unique<StreamClient>(channel, this);

    // Send master arbitration update message to establish this controller as
    // master for stream messages
    this->_streamClient->masterArbitrationUpdate(this->_sw->deviceId());

    // Enable p4runtime message logging
    auto logFn = fs::path(this->_sw->network()->controller()->outputDir()) /
                 ("logs/" + this->_sw->name() + "-p4runtime.log");
    this->_p4rtLogger = make_unique<Logger>(this->_sw->name() + "-p4rt");
    this->_p4rtLogger->enable_file_logging(logFn.string());
}

void P4RuntimeMgr::startProcessPacketIn() {
    // Start receiving packets from the P4 switch
    this->_streamClient->recvPacketIn();
}

void P4RuntimeMgr::waitForThreads() {
    this->_streamClient->waitForThreads();
}

void P4RuntimeMgr::killAllThreads() {
    this->_streamClient->killAllThreads();
}

void P4RuntimeMgr::setForwardingPipelineConfig(const string &p4config,
                                               const string &p4info) {
    // Build p4info protobuf message
    p4::config::v1::P4Info p4infoProto;
    if (p4info.empty()) {
        assert(pi_add_config(p4config.c_str(), PI_CONFIG_TYPE_BMV2_JSON,
                             &this->_p4info) == PI_STATUS_SUCCESS);
        p4infoProto = pi::p4info::p4info_serialize_to_proto(this->_p4info);
    } else {
        assert(google::protobuf::TextFormat::ParseFromString(p4info,
                                                             &p4infoProto));
        assert(pi::p4info::p4info_proto_reader(p4infoProto, &this->_p4info));
    }

    // Build config request protobuf message
    p4::v1::SetForwardingPipelineConfigRequest request;
    request.set_device_id(this->_sw->deviceId());
    request.set_action(
        p4::v1::SetForwardingPipelineConfigRequest_Action_VERIFY_AND_COMMIT);
    p4::v1::ForwardingPipelineConfig *config = request.mutable_config();
    config->set_allocated_p4info(&p4infoProto);
    p4::tmp::P4DeviceConfig devConfig;
    devConfig.set_reassign(true);
    devConfig.set_device_data(p4config);
    devConfig.SerializeToString(config->mutable_p4_device_config());

    // Send the config request
    p4::v1::SetForwardingPipelineConfigResponse rep;
    grpc::ClientContext context;
    auto status =
        this->_piStub->SetForwardingPipelineConfig(&context, request, &rep);
    config->release_p4info();
    if (!status.ok()) {
        logger.error("SetForwardingPipelineConfig failed: " +
                     to_string(status.error_code()) + ": " +
                     status.error_message());
    }
}

void P4RuntimeMgr::writeTableEntry(const P4TableEntry &entry) {
    p4::v1::TableEntry entryPB = entry.protobufMsg(this->_p4info);

    p4::v1::WriteRequest request;
    request.set_device_id(this->_sw->deviceId());
    auto update = request.add_updates();
    update->set_type(p4::v1::Update_Type_INSERT);
    auto entity = update->mutable_entity();
    entity->set_allocated_table_entry(&entryPB);
    this->logMessage(request);

    p4::v1::WriteResponse rep;
    grpc::ClientContext context;
    auto status = this->_piStub->Write(&context, request, &rep);
    entity->release_table_entry();
    if (!status.ok()) {
        logger.error("Write failed: " + to_string(status.error_code()) + ": " +
                     status.error_message());
    }
}

void P4RuntimeMgr::deleteTableEntry(const P4TableEntry &entry) {
    p4::v1::TableEntry entryPB = entry.protobufMsg(this->_p4info);

    p4::v1::WriteRequest request;
    request.set_device_id(this->_sw->deviceId());
    auto update = request.add_updates();
    update->set_type(p4::v1::Update_Type_DELETE);
    auto entity = update->mutable_entity();
    entity->set_allocated_table_entry(&entryPB);
    this->logMessage(request);

    p4::v1::WriteResponse rep;
    grpc::ClientContext context;
    auto status = this->_piStub->Write(&context, request, &rep);
    entity->release_table_entry();
    if (!status.ok()) {
        logger.error("Write failed: " + to_string(status.error_code()) + ": " +
                     status.error_message());
    }
}

vector<P4TableEntry> P4RuntimeMgr::readTableEntries() {
    p4::v1::ReadRequest request;
    request.set_device_id(this->_sw->deviceId());
    auto entity = request.add_entities();
    auto table_entry = entity->mutable_table_entry();
    table_entry->set_table_id(0);
    this->logMessage(request);

    vector<P4TableEntry> results;

    p4::v1::ReadResponse rep;
    grpc::ClientContext context;
    auto reader = this->_piStub->Read(&context, request);
    while (reader->Read(&rep)) {
        for (const auto &entity : rep.entities()) {
            P4TableEntry entry(entity.table_entry(), this->_p4info);
            results.emplace_back(std::move(entry));
        }
    }
    auto status = reader->Finish();
    if (!status.ok()) {
        logger.error("Read failed: " + to_string(status.error_code()) + ": " +
                     status.error_message());
    }

    return results;
}

vector<P4CounterEntry> P4RuntimeMgr::readCounters() {
    p4::v1::ReadRequest request;
    request.set_device_id(this->_sw->deviceId());
    auto entity = request.add_entities();
    auto counter_entry = entity->mutable_counter_entry();
    counter_entry->set_counter_id(0);
    this->logMessage(request);

    vector<P4CounterEntry> results;

    p4::v1::ReadResponse rep;
    grpc::ClientContext context;
    auto reader = this->_piStub->Read(&context, request);
    while (reader->Read(&rep)) {
        for (const auto &entity : rep.entities()) {
            P4CounterEntry entry(entity.counter_entry(), this->_p4info);
            results.emplace_back(std::move(entry));
        }
    }
    auto status = reader->Finish();
    if (!status.ok()) {
        logger.error("Read failed: " + to_string(status.error_code()) + ": " +
                     status.error_message());
    }

    return results;
}

// def ReadDirectCounter(self, table_id=False, dry_run=False):
//     request = p4runtime_pb2.ReadRequest()
//     request.device_id = self.device_id
//     entity = request.entities.add()
//     direct_counter_entry = entity.direct_counter_entry
//     # assign
//     if table_id is not None:
//         direct_counter_entry.table_entry.table_id = table_id
//     else:
//         direct_counter_entry.table_entry.table_id = 0
//     if dry_run:
//         print("P4Runtime Read Direct Counters:", request)
//     else:
//         for response in self.client_stub.Read(request):
//             yield response

// # Read Register
// def ReadRegister(self, register_id=None, index=None, dry_run=False):
//     request = p4runtime_pb2.ReadRequest()
//     request.device_id = self.device_id
//     entity = request.entities.add()
//     register_entry = entity.register_entry

//     if register_id is not None:
//         register_entry.register_id = register_id
//     else:
//         register_entry.register_id = 0
//     if index is not None:
//         register_entry.index.index = index
//     if dry_run:
//         print("P4Runtime Read Register: ", request)
//     else:
//         for response in self.client_stub.Read(request):
//             yield response

// def WritePREEntry(self, pre_entry, dry_run=False):
//     request = p4runtime_pb2.WriteRequest()
//     request.device_id = self.device_id
//     request.election_id.low = 1
//     update = request.updates.add()
//     update.type = p4runtime_pb2.Update.INSERT
//     update.entity.packet_replication_engine_entry.CopyFrom(pre_entry)
//     if dry_run:
//         print("P4Runtime Write:", request)
//     else:
//         self.client_stub.Write(request)

// def PacketOut(self, packet, dry_run=False, **kwargs):
//     request = p4runtime_pb2.StreamMessageRequest()
//     request.packet.CopyFrom(packet)
//     if dry_run:
//         print("P4Runtime PacketOut: ", request)
//     else:
//         self.requests_stream.put(request)
//         #for item in self.stream_msg_resp:
//         #    return item

// def PacketIn(self, dry_run=False, **kwargs):
//     request = p4runtime_pb2.StreamMessageRequest()
//     if dry_run:
//         print("P4Runtime PacketIn: ", request)
//     else:
//         self.requests_stream.put(request)
//         for item in self.stream_msg_resp:
//             return item

// # Digest
// def WriteDigestEntry(self, digest_entry, dry_run=False):
//     request = p4runtime_pb2.WriteRequest()
//     request.device_id = self.device_id
//     request.election_id.low = 1
//     update = request.updates.add()
//     update.type = p4runtime_pb2.Update.INSERT
//     update.entity.digest_entry.CopyFrom(digest_entry)

//     if dry_run:
//         print("P4Runtime write DigestEntry: ", request)
//     else:
//         self.client_stub.Write(request)

// def DigestListAck(self, digest_ack, dry_run=False, **kwargs):
//     request = p4runtime_pb2.StreamMessageRequest()
//     request.digest_ack.CopyFrom(digest_ack)
//     if dry_run:
//         print("P4 Runtime DigestListAck: ", request)
//     else:
//         self.requests_stream.put(request)
//         for item in self.stream_msg_resp:
//             return item

// def DigestList(self, dry_run=False, **kwargs):
//     request = p4runtime_pb2.StreamMessageRequest()
//     if dry_run:
//         print("P4 Runtime DigestList Response: ", request)
//     else:
//         self.requests_stream.put(request)
//         for item in self.stream_msg_resp:
//             return item
