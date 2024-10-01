#pragma once

#include <memory>
#include <thread>

#include <boost/asio.hpp>
#include <grpcpp/grpcpp.h>
#include <p4/v1/p4runtime.grpc.pb.h>

class P4RuntimeMgr;

class StreamClient {
private:
    const P4RuntimeMgr *_manager; // parent p4runtime manager
    grpc::ClientContext _context;
    boost::asio::io_service _ioService;
    std::unique_ptr<std::thread> _recvThread;
    std::unique_ptr<p4::v1::P4Runtime::Stub> _piStub;
    std::unique_ptr<grpc::ClientReaderWriter<p4::v1::StreamMessageRequest,
                                             p4::v1::StreamMessageResponse>>
        _stream;

public:
    StreamClient(const std::shared_ptr<grpc::Channel> &channel,
                 P4RuntimeMgr *manager);

    decltype(_manager) manager() const { return _manager; }

    void masterArbitrationUpdate(int deviceId);
    void recvPacketIn();
    void sendPacketOut(std::string frame);
    void waitForThreads();
    void killAllThreads();
};
