#include "stream_reassembler.hh"

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity) : _output(capacity), _capacity(capacity) {}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const uint64_t index, const bool eof) {
    // 步骤 1: 如果 eof 标志存在，记录下来。这是流的总长度。
    if (eof) {
        _eof_flag = true;
        _eof_index = index + data.size();
    }

    // 步骤 2: 获取当前流的状态
    const uint64_t first_unassembled = _output.bytes_written();
    const uint64_t first_unacceptable = first_unassembled + (_capacity - _output.buffer_size());

    // 步骤 3: 裁剪并存储传入的数据
    // 我们只关心那些在容量窗口内 [first_unassembled, first_unacceptable) 的字节
    for (size_t i = 0; i < data.length(); ++i) {
        const size_t current_index = index + i;

        // 如果字节已经组装或超出窗口范围，则忽略
        if (current_index >= first_unacceptable || current_index < first_unassembled) {
            continue;
        }

        // 将有效字节存入缓冲区
        _buffer[current_index] = data[i];
    }

    // 步骤 4: 尝试从缓冲区组装连续的字节
    string assembled_data;
    uint64_t current_assembly_index = first_unassembled;
    while (_buffer.count(current_assembly_index)) {
        assembled_data.push_back(_buffer.at(current_assembly_index));
        _buffer.erase(current_assembly_index);
        current_assembly_index++;
    }

    // 步骤 5: 将组装好的数据写入输出流
    if (!assembled_data.empty()) {
        _output.write(assembled_data);
    }

    // 步骤 6: 最后，检查是否满足 EOF 条件
    // 如果我们已经看到了 eof 标志，并且所有字节（直到 eof_index）都已组装，就关闭流。
    if (_eof_flag && _output.bytes_written() >= _eof_index) {
        _output.end_input();
    }
}

size_t StreamReassembler::unassembled_bytes() const { 
    return this -> _buffer.size();
}

bool StreamReassembler::empty() const { return this -> _buffer.empty(); }
