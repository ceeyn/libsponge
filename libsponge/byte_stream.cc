#include "byte_stream.hh"

// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

ByteStream::ByteStream(const size_t capacity)
    : _capacity(capacity),
     _buffer(),           // ✅ 先初始化 _buffer
     _bytes_written(0),
     _bytes_read(0),
     _input_ended(false), // ✅ 再初始化 _input_ended
     _error(false) {}

size_t ByteStream::write(const string &data) {
    if (_input_ended || remaining_capacity() == 0) return 0;
    size_t write_len = min(data.size(), remaining_capacity());
    // 批量插入：O(1) 时间复杂度
    _buffer.insert(_buffer.end(), data.begin(), data.begin() + write_len);
    _bytes_written += write_len;
    return write_len;
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    return std::string(_buffer.begin(), _buffer.begin() + std::min(len, _buffer.size()));
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) {
    size_t remove_count = min(len, _buffer.size());
    _buffer.erase(_buffer.begin(), _buffer.begin() + remove_count); // O(1)删除
    _bytes_read += remove_count;
}
//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
    std::string result = peek_output(len);
    pop_output(result.size());
    return result;
}

void ByteStream::end_input() {
    _input_ended = true;
}

bool ByteStream::input_ended() const {
    return _input_ended;
}

size_t ByteStream::buffer_size() const { 
    return _buffer.size(); 
}


bool ByteStream::buffer_empty() const { 
    return _buffer.empty();
}


bool ByteStream::eof() const { 
    return input_ended() && buffer_empty(); 
}


size_t ByteStream::bytes_written() const { 
    return _bytes_written; 
}


size_t ByteStream::bytes_read() const { 
    return _bytes_read; 
}


size_t ByteStream::remaining_capacity() const { 
    return _capacity - buffer_size();
}