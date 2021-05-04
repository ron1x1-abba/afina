#ifndef AFINA_NETWORK_ST_NONBLOCKING_CONNECTION_H
#define AFINA_NETWORK_ST_NONBLOCKING_CONNECTION_H

#include <cstring>
#include <memory>
#include <string>
#include <iostream>
#include <unistd.h>
#include <sys/uio.h>
#include <cassert>

#include <afina/execute/Command.h>
#include <spdlog/logger.h>
#include <afina/logging/Service.h>
#include "protocol/Parser.h"
#include <sys/socket.h>
#include <afina/Storage.h>
#include <sys/epoll.h>

namespace Afina {
namespace Network {
namespace STnonblock {

const size_t MAX_OUTPUT = 100;

class Connection {
public:
    Connection(int s, std::shared_ptr<Afina::Storage>& storage, std::shared_ptr<spdlog::logger>& logger) : 
        _socket(s), _storage(storage), _logger(logger) {
        std::memset(&_event, 0, sizeof(struct epoll_event));
        _event.data.ptr = this;
        _data_size = 4096;
        std::memset(_data, 0, 4096);
        _data_offset = 0;
        _out_offset = 0;
    }

    inline bool isAlive() const { return _alive; }

    void Start();

protected:
    void OnError();
    void OnClose();
    void DoRead();
    void DoWrite();

private:
    friend class ServerImpl;

    int _socket;
    struct epoll_event _event;
    std::shared_ptr<spdlog::logger> _logger;
    std::shared_ptr<Afina::Storage> _storage;
    std::size_t arg_remains = 0;
    Protocol::Parser parser;
    std::string argument_for_command;
    std::unique_ptr<Execute::Command> command_to_execute;
    bool _alive = true;
    std::vector<std::string> _output;
    char _data[4096];
    size_t _data_size;
    size_t _data_offset;
    size_t _out_offset;
    bool _write_only = false;
};

} // namespace STnonblock
} // namespace Network
} // namespace Afina

#endif // AFINA_NETWORK_ST_NONBLOCKING_CONNECTION_H
