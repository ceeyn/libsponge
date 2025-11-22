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
        _isn=WrappingInt32(seg.header().seqno);
    }
    const uint64_t asn = unwrap(header.seqno, this->_isn.value(), _reassembler.stream_out().bytes_written());
    uint64_t index = header.syn ? 0: asn - 1;
    this->_reassembler.push_substring(payload.copy(), index, header.fin);
    // 2. 获取header的sn，提取本地记录的isn，给出asn，index
    // 3. 将seg的payload加入到reassembler中
    // 4.更新ackno
    // ackno 不能用当前段的 header.fin 来决定是否 +1；必须用流状态 input_ended() （只有当 FIN 已按序组装到末尾时才 +1）。否则遇到乱序 FIN 会把 ack 提前一位，正好对应你看到的“期望比实际小 1”。
    _ackno = wrap(_reassembler.stream_out().bytes_written() + (_reassembler.stream_out().input_ended() ? 2 : 1), this->_isn.value());
}

optional<WrappingInt32> TCPReceiver::ackno() const { return this->_ackno; }

size_t TCPReceiver::window_size() const { return _capacity - _reassembler.stream_out().buffer_size(); }
