#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    // 1. 获取seg的header，playroad
    const TCPHeader &header = seg.header();
    const Buffer &payload = seg.payload();
    if (this->_isn.has_value() == false) {
        if (header.syn == false) {
            return;
        }
        this->_isn = header.seqno;
    }
    const uint64_t asn = unwrap(header.seqno, this->_isn.value(), stream_out().bytes_written());
    uint64_t index = asn > 0 ? asn - 1 : 0;
    this->_reassembler.push_substring(payload.copy(), index, header.fin);
    // 2. 获取header的sn，提取本地记录的isn，给出asn，index
    // 3. 将seg的payload加入到reassembler中
    // 4.更新ackno
    _ackno = wrap(stream_out().bytes_written() + (header.fin ? 1 : 0), this->_isn.value());
}

optional<WrappingInt32> TCPReceiver::ackno() const { return {}; }

size_t TCPReceiver::window_size() const { return {}; }
