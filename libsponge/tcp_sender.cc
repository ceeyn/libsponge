#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <random>

// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
    , _stream(capacity) {}

 size_t TCPSender::bytes_in_flight() const { return _bytes_in_flight;; }

void TCPSender::fill_window() {
    // 1。找到当前窗口大小，发送范围是[next_seq, _ack + windowsize]
    size_t window_size = _window_size > 0 ? _window_size : 1;
    TCPSegment seg;
    // 2。每次取TCPConfig::MAX_PAYLOAD_SIZE，但保证next_seq+payload_size <= _ack + windowsize
    while (bytes_in_flight() < window_size)
    {
        size_t playload_size = min(static_cast<size_t>(window_size - bytes_in_flight()), TCPConfig::MAX_PAYLOAD_SIZE);
        seg.payload() = stream_in().read(playload_size);
        seg.header().seqno = wrap(_next_seqno, _isn);
        seg.header().syn = _next_seqno == 0;
        seg.header().fin = stream_in().eof() && bytes_in_flight() + playload_size <= window_size;
        _segments_out.push(seg);
        _outstanding_segments.push(seg);
        _next_seqno += seg.length_in_sequence_space();
        _bytes_in_flight += seg.length_in_sequence_space();
    }
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) { 
    uint64_t recv_ackno = unwrap(ackno, _isn, _next_seqno);
    // 1. 检查ack必须大于等于当前ack
    if (recv_ackno < _ackno) {
        return;
    }
    // 2. ack有效则更新当前ack和窗口大小
    _window_size = window_size;
    _ackno = recv_ackno;
    uint64_t cur_left_seg_ack = 0;
    while (cur_left_seg_ack < _ackno && !_outstanding_segments.empty()) {
        TCPSegment cur = _outstanding_segments.front();
        cur_left_seg_ack = unwrap(cur.header().seqno, _isn, _next_seqno);
        if (cur_left_seg_ack <= _ackno) {
            _outstanding_segments.pop();
        } else {
            break;
        }
        _bytes_in_flight -= cur.length_in_sequence_space();
    }
    _last_tick_total_time = 0;
    _consecutive_retransmissions = 0;
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) { 
    // 1. 检查是否有超时的段
    _last_tick_total_time += ms_since_last_tick;
    if (_last_tick_total_time <= _initial_retransmission_timeout * pow(2, _consecutive_retransmissions)) {
        return;
    }
    // 2. 若有，从_outstanding_segments弹出，发送，更新超时时间，增加重传次数
    if (!_outstanding_segments.empty()) {
        _segments_out.push(_outstanding_segments.front());
        _consecutive_retransmissions++;
        _last_tick_total_time = 0;
    }
    // 3. 重置重新传输计时器，并启动它
 }

unsigned int TCPSender::consecutive_retransmissions() const { return _consecutive_retransmissions; }

void TCPSender::send_empty_segment() {
    TCPSegment seg;
    seg.header().seqno = wrap(_next_seqno, _isn);
    _segments_out.push(seg);
}
