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

 size_t TCPSender::bytes_in_flight() const { return _bytes_in_flight; }

void TCPSender::fill_window() {
    size_t current_window = _window_size > 0 ? _window_size : 1;

    while (_bytes_in_flight < current_window) {
        TCPSegment seg;
        if (_next_seqno == 0) {
            seg.header().syn = true;
        }
        size_t payload_size = min(TCPConfig::MAX_PAYLOAD_SIZE, current_window - _bytes_in_flight);
        std::string payload = _stream.read(payload_size);
        seg.payload() = Buffer(std::move(payload));
        if (!_fin_sent && _stream.eof() && (seg.length_in_sequence_space() + _bytes_in_flight < current_window)) {
            seg.header().fin = true;
            _fin_sent = true;
        }
        if (seg.length_in_sequence_space() == 0) {
            break;
        }
        if (_outstanding_segments.empty()) {
            _timer = 0;
        }
        seg.header().seqno = wrap(_next_seqno, _isn);
        _next_seqno += seg.length_in_sequence_space();
        _bytes_in_flight += seg.length_in_sequence_space();
        _outstanding_segments.push(seg);
        _segments_out.push(seg);
    }
}

void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    uint64_t abs_ackno = unwrap(ackno, _isn, _next_seqno);
    if (abs_ackno > _next_seqno) {
        return;
    }

    _window_size = window_size;

    bool acked_something = false;
    while (!_outstanding_segments.empty()) {
        TCPSegment &seg = _outstanding_segments.front();
        uint64_t seg_end_seqno = unwrap(seg.header().seqno, _isn, _next_seqno) + seg.length_in_sequence_space();

        if (abs_ackno >= seg_end_seqno) {
            _bytes_in_flight -= seg.length_in_sequence_space();
            _outstanding_segments.pop();
            acked_something = true;
        } else {
            break;
        }
    }

    if (acked_something) {
        _timer = 0;
        _consecutive_retransmissions = 0;
        fill_window();
    } else if (_outstanding_segments.empty()) {
        _timer = 0;
    }
}

void TCPSender::tick(const size_t ms_since_last_tick) {
    if (_outstanding_segments.empty()) {
        return;
    }

    _timer += ms_since_last_tick;
    unsigned int current_rto = _initial_retransmission_timeout * (1 << _consecutive_retransmissions);

    if (_timer >= current_rto) {
        _segments_out.push(_outstanding_segments.front());
        if (_window_size > 0) {
            _consecutive_retransmissions++;
        }
        _timer = 0;
    }
}

unsigned int TCPSender::consecutive_retransmissions() const { return _consecutive_retransmissions; }

void TCPSender::send_empty_segment() {
    TCPSegment seg;
    seg.header().seqno = wrap(_next_seqno, _isn);
    _segments_out.push(seg);
}