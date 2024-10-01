#include "streamclient.hpp"

#include <sstream>
#include <thread>

#include "controller.hpp"
#include "logger.hpp"
#include "network.hpp"
#include "p4runtimemgr.hpp"
#include "packethandler.hpp"
#include "switch.hpp"

using namespace std;

// static inline string tid_to_str(thread::id tid) {
//     ostringstream ss;
//     ss << tid;
//     return ss.str();
// }

StreamClient::StreamClient(const shared_ptr<grpc::Channel> &channel,
                           P4RuntimeMgr *manager)
    : _manager(manager), _piStub(p4::v1::P4Runtime::NewStub(channel)),
      _stream(_piStub->StreamChannel(&_context)) {}

void StreamClient::masterArbitrationUpdate(int deviceId) {
    p4::v1::StreamMessageRequest request;
    auto arbitration = request.mutable_arbitration();
    arbitration->set_device_id(deviceId);

    if (!this->_stream->Write(request)) {
        logger.error("masterArbitrationUpdate failed");
    }
}

void StreamClient::recvPacketIn() {
    // Spawn the packet_in receive thread
    this->_recvThread = make_unique<std::thread>([this]() {
        p4::v1::StreamMessageResponse resp;

        while (this->_stream->Read(&resp)) {
            if (!resp.has_packet()) {
                continue;
            }

            const auto &packet = resp.packet();
            // Extract the controller_header("packet_in") information
            string invId = packet.metadata(0).value();
            // L2 frame
            string frame = packet.payload();
            auto manager = this->_manager;
            auto sw = manager->sw();
            auto controller = sw->network()->controller();

            // Synchronous processing
            PacketHandler(controller, sw, manager, std::move(invId),
                          std::move(frame))();
            // Async IO
            // https://studiofreya.com/cpp/boost/c-boost-asio-introduction-tutorial/
            // this->_ioService.post(PacketHandler(
            //     controller, sw, manager, std::move(invId),
            //     std::move(frame)));
        }

        auto status = this->_stream->Finish();
        if (!status.ok()) {
            logger.warn("recvPacketIn: " + to_string(status.error_code()) +
                        ": " + status.error_message());
        }
    });
}

void StreamClient::sendPacketOut(string frame) {
    logger.info("Sending packet out");

    p4::v1::StreamMessageRequest request;
    auto packet = request.mutable_packet();
    // controller_header("packet_out") is currently empty. Otherwise,
    // packet->add_metadata() can be used to populate the packet_out header.
    packet->set_payload(std::move(frame));

    if (!this->_stream->Write(request)) {
        logger.error("sendPacketOut failed");
    }
}

void StreamClient::waitForThreads() {
    // There is only one thread at the moment.
    if (this->_recvThread && this->_recvThread->joinable()) {
        this->_recvThread->join();
    }
}

void StreamClient::killAllThreads() {
    this->_stream->WritesDone();
}
