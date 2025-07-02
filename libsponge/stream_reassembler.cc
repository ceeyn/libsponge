#include "stream_reassembler.hh"

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity)
    : _output(capacity),
      _capacity(capacity),
      _next_byte_index(0),
      _last_reassembler_size(0),
      _unassembled(),
      _eof_flag(false),
      _eof_index(0) {}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const std::string &data, const size_t index, const bool eof) {
    size_t max_pos = _output.bytes_read() + _capacity;
    size_t min_pos = _output.bytes_written();
    if ((!data.length() == 0 && index + data.length() <= min_pos) || index >= max_pos) {
        return;
    }
    if (eof) {
        _eof_index = index + data.length();
        _eof_flag = true;
    }
    size_t start = std::max(index, min_pos);
    size_t end = std::min(index + data.length(), max_pos);
    for (size_t i = start; i < end; i++) {
        if (!_unassembled.count(i)) {
            _unassembled[i] = data[i-index];
        }
    }
    // size_t firstUnReassemble = _next_byte_index;
    string curAssemble;
    size_t cur = _output.bytes_written();
    while (_unassembled.count(cur)) {
        curAssemble += _unassembled[cur];
        _unassembled.erase(cur);
        cur++;
    }
    if (!curAssemble.empty()) {
        _output.write(curAssemble);
    }
    if (_eof_flag && _output.bytes_written() == _eof_index)
        _output.end_input();
}
size_t StreamReassembler::unassembled_bytes() const { return _unassembled.size(); }

bool StreamReassembler::empty() const { return _unassembled.empty(); }
