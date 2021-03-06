#include "Connection.h"

namespace Afina {
namespace Network {
namespace STnonblock {

// See Connection.h
void Connection::Start() {
    _logger->debug("Starting new connection socket : {}", _socket);
    _event.events |= EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLRDHUP;
}

// See Connection.h
void Connection::OnError() {
    _logger->debug("Error in Connection on socket : {}", _socket);
    _alive = false; // isAlive() -> false
}

// See Connection.h
void Connection::OnClose() {
    _logger->debug("Connection is closing on socket : {}", _socket);
    _alive = false; // clear all tmp information, isAlive() -> false
}

// See Connection.h
void Connection::DoRead() {
    assert(_alive);
    // Process new connection:
    // - read commands until socket alive
    // - execute each command
    // - send response
    _logger->debug("Reading in Connection on socket : {}", _socket);
    try {
        int readed_bytes = -1;
        // while ((readed_bytes = read(_socket, client_buffer, sizeof(client_buffer))) > 0) {
        //while ((readed_bytes = read(_socket, _data + _data_offset, sizeof(_data) - _data_offset)) > 0) {
        if ((readed_bytes = read(_socket, _data + _data_offset, _data_size - _data_offset)) > 0) {
            _logger->debug("Got {} bytes from socket", readed_bytes);
            _data_offset += readed_bytes;
            // Single block of data readed from the socket could trigger inside actions a multiple times,
            // for example:
            // - read#0: [<command1 start>]
            // - read#1: [<command1 end> <argument> <command2> <argument for command 2> <command3> ... ]
            while (_data_offset > 0) {
                _logger->debug("Process {} bytes", _data_offset);
                // There is no command yet
                if (!command_to_execute) {
                    std::size_t parsed = 0;
                    if (parser.Parse(_data, _data_offset, parsed)) {
                        // There is no command to be launched, continue to parse input stream
                        // Here we are, current chunk finished some command, process it
                        _logger->debug("Found new command: {} in {} bytes", parser.Name(), parsed);
                        command_to_execute = parser.Build(arg_remains);
                        if (arg_remains > 0) {
                            arg_remains += 2;
                        }
                    }

                    // Parsed might fails to consume any bytes from input stream. In real life that could happens,
                    // for example, because we are working with UTF-16 chars and only 1 byte left in stream
                    if (parsed == 0) {
                        break;
                    } else {
                        std::memmove(_data, _data + parsed, _data_offset - parsed);
                        _data_offset -= parsed;
                    }
                }

                // There is command, but we still wait for argument to arrive...
                if (command_to_execute && arg_remains > 0) {
                    _logger->debug("Fill argument: {} bytes of {}", _data_offset, arg_remains);
                    // There is some parsed command, and now we are reading argument
                    std::size_t to_read = std::min(arg_remains, std::size_t(_data_offset));
                    argument_for_command.append(_data, to_read);

                    std::memmove(_data, _data + to_read, _data_offset - to_read);
                    arg_remains -= to_read;
                    _data_offset -= to_read;
                }

                // Thre is command & argument - RUN!
                if (command_to_execute && arg_remains == 0) {
                    _logger->debug("Start command execution");

                    std::string result;
                    if (argument_for_command.size()) {
                        argument_for_command.resize(argument_for_command.size() - 2);
                    }
                    command_to_execute->Execute(*_storage, argument_for_command, result);

                    // Send response
                    result += "\r\n";
                    _event.events |= EPOLLOUT;
                    _output.push_back(std::move(result));
                    if (_output.size() > MAX_OUTPUT)
                        _event.events &= ~EPOLLIN;

                    // Prepare for the next command
                    command_to_execute.reset();
                    argument_for_command.resize(0);
                    parser.Reset();
                }
            } // while (readed_bytes)
        }

        if (readed_bytes == 0 && errno != EAGAIN) { // eof in socket
            _logger->debug("Connection closed by client on socket : {}", _socket);
            _write_only = true;
        }
        else if (errno != EAGAIN){
            throw std::runtime_error(std::string(strerror(errno)));
        }
    } catch (std::runtime_error &ex) {
        _logger->error("Failed to process connection on descriptor {}: {}", _socket, ex.what());
        OnClose();
    }
}

// See Connection.h
void Connection::DoWrite() {
    assert(_alive);
    _logger->debug("Writing in Connection on socket : {}", _socket);
    iovec out[_output.size()] = {};
    for(size_t i = 0; i < _output.size(); ++i) {
        out[i].iov_base = &(_output[i][0]);
        out[i].iov_len = _output[i].size();
    }
    out[0].iov_base = static_cast<char*>(out[0].iov_base) + _out_offset;
    out[0].iov_len -= _out_offset;
    int ret = writev(_socket, &out[0], _output.size());
    if(ret == -1 && errno != EAGAIN) {
        OnClose();
        _logger->debug("Connection failed to write to socket : {}", _socket);
        return;
    }
    for(size_t i = 0; i < _output.size() && ret >= _output[i].size(); ++i) {
        ret -= _output[i].size();
        _output.erase(_output.begin(), _output.begin() + 1);
    }
    _out_offset = ret;
    if(_output.empty() && _write_only) {
        OnClose();
    }
    else if(_output.empty()){
        _event.events &= ~EPOLLOUT;
    }
    else if(_output.size() < MAX_OUTPUT - MAX_OUTPUT / 10) {
        _event.events |= EPOLLIN;
    }
}

} // namespace STnonblock
} // namespace Network
} // namespace Afina
