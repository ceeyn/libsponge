#include "tcp_connection.hh"

#include <iostream>

// Dummy implementation of a TCP connection

// For Lab 4, please replace with a real implementation that passes the
// automated checks run by `make check`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

size_t TCPConnection::remaining_outbound_capacity() const { return _receiver.window_size(); }

size_t TCPConnection::bytes_in_flight() const { return _sender.bytes_in_flight(); }

size_t TCPConnection::unassembled_bytes() const { return _receiver.unassembled_bytes(); }

size_t TCPConnection::time_since_last_segment_received() const { return _sender._timer; }

void TCPConnection::segment_received(const TCPSegment &seg) { 
    _receiver.segment_received(seg);
    _sender.ack_received(seg.header().ackno, seg.header().win);
    if (seg.header().rst) {
        _sender.stream_in().set_error();
        _receiver.stream_out().set_error();
    }
 }

bool TCPConnection::active() const { 
    if (_sender.stream_in().error() || _receiver.stream_out().error()) {
        return true;
    }
    if (_sender.stream_in().eof() && _receiver.stream_out().eof() && _sender.bytes_in_flight() == 0
    && !_linger_after_streams_finish) {
        return false;
    }
    return true;
}

size_t TCPConnection::write(const string &data) {
    size_t written_size = _sender.stream_in().write(data);
    _sender.fill_window();
    return written_size;
}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) { 
    _sender.tick(ms_since_last_tick);
    if (_sender.consecutive_retransmissions() >= _cfg.MAX_RETX_ATTEMPTS) {
        _sender.stream_in().set_error();
        _receiver.stream_out().set_error();
        _linger_after_streams_finish = false;
        TCPSegment rst_seg;
        rst_seg.header().rst = true;
        _segments_out.push(rst_seg);
        trans_segments();
    }
 }

void TCPConnection::end_input_stream() {
    _sender.stream_in().end_input();
}

void TCPConnection::connect() {
    _sender.fill_window();
}

TCPConnection::~TCPConnection() {
    try {
        if (active()) {
            cerr << "Warning: Unclean shutdown of TCPConnection\n";

            // Your code here: need to send a RST segment to the peer
            _sender.stream_in().set_error();
            _receiver.stream_out().set_error();
            TCPSegment rst_seg;
            rst_seg.header().rst = true;
            rst_seg.header().seqno = _sender.next_seqno();
            _segments_out.push(rst_seg);

        }
    } catch (const exception &e) {
        std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
    }
}

void TCPConnection::trans_segments() {
    if (_sender.segments_out().empty()) {
        _sender.fill_window();
    } 
    TCPSegment seg = _sender.segments_out().front();
    if (_receiver.ackno().has_value()) {
        seg.header().ackno = *_receiver.ackno();
        seg.header().ack = true;
        seg.header().win = _receiver.window_size();
    }
    _sender.segments_out().pop();
    _segments_out.push(seg);
}