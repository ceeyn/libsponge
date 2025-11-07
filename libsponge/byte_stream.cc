#include "byte_stream.hh"
#include <algorithm>  // std::min

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

ByteStream::ByteStream(const size_t capacity)
    : _capacity(capacity),
      _buffer(),
      _written_size(0),
      _read_size(0),
      _is_ended(false),
      _error(false) {}
// ... existing code ...
size_t ByteStream::write(const std::string &data) {
    if (_is_ended || remaining_capacity() == 0) return 0;
    size_t writing_size = std::min(data.size(), remaining_capacity());
    _buffer.insert(_buffer.end(), data.begin(), data.begin() + writing_size);
    _written_size += writing_size;
    return writing_size;
}
// ... existing code ...
string ByteStream::peek_output(const size_t len) const {
    return std::string(_buffer.begin(), _buffer.begin() + std::min(len, _buffer.size()));
}
// ... existing code ...
void ByteStream::pop_output(const size_t len) {
    size_t remove_count = std::min(len, _buffer.size());
    _buffer.erase(_buffer.begin(), _buffer.begin() + remove_count);
    _read_size += remove_count;
}
// ... existing code ...
std::string ByteStream::read(const size_t len) {
    std::string result = peek_output(len);
    pop_output(result.size());
    return result;
}
// ... existing code ...
void ByteStream::end_input() {
    _is_ended = true;
}
// ... existing code ...
bool ByteStream::input_ended() const {
    return _is_ended;
}
// ... existing code ...
size_t ByteStream::buffer_size() const {
    return _buffer.size();
}
// ... existing code ...
bool ByteStream::buffer_empty() const {
    return _buffer.empty();
}
// ... existing code ...
bool ByteStream::eof() const {
    return input_ended() && buffer_empty();
}
// ... existing code ...
size_t ByteStream::bytes_written() const {
    return _written_size;
}
// ... existing code ...
size_t ByteStream::bytes_read() const {
    return _read_size;
}
// ... existing code ...
size_t ByteStream::remaining_capacity() const {
    return _capacity - _buffer.size();
}