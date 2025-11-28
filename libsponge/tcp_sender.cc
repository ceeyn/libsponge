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
    // 可用窗口：若对端通告为 0，允许发送一个字节（或 SYN）
    const size_t window = _window_size > 0 ? _window_size : 1;

    // 若当前已在飞字节已用满窗口，则不再发送
    while (bytes_in_flight() < window) {
        const bool need_syn = (_next_seqno == 0);
        // 控制位占用（先只考虑 SYN；FIN 在载荷确定后再判断）
        const size_t ctrl_cost_syn = need_syn ? 1 : 0;

        // 剩余可用于载荷的窗口空间（扣除 SYN 的占用）
        const size_t window_left_for_payload = window - bytes_in_flight() - ctrl_cost_syn;
        if (window_left_for_payload == 0 && !need_syn) {
            // 没空间放载荷且不需要 SYN，退出
            break;
        }

        // 载荷最大不超过 MSS 和窗口剩余
        const size_t payload_size = std::min(TCPConfig::MAX_PAYLOAD_SIZE, window_left_for_payload);
        Buffer payload = stream_in().read(payload_size);

        // 只有在输入结束且段总长度不越窗时才可附加 FIN
        const bool can_fin = stream_in().eof() &&
                             (bytes_in_flight() + ctrl_cost_syn + payload.size() + 1 /* FIN */ <= window);

        // 若既没有载荷也不需要 SYN/FIN，说明没东西可发，退出
        if (payload.size() == 0 && !need_syn && !can_fin) {
            break;
        }

        // 构造并发送段
        TCPSegment seg;
        seg.header().seqno = wrap(_next_seqno, _isn);
        seg.header().syn = need_syn;
        seg.header().fin = can_fin;
        seg.payload() = std::move(payload);

        // 入发送队列与未确认队列
        _segments_out.push(seg);
        _outstanding_segments.push(seg);

        // 更新序列号与在飞字节
        const size_t seg_len = seg.length_in_sequence_space();
        _next_seqno += seg_len;
        _bytes_in_flight += seg_len;

        // 启动/重置定时器（首次有未确认段时）
        if (_outstanding_segments.size() == 1) {
            _last_tick_total_time = 0;
        }

        // 若窗口刚好用尽，退出循环
        if (bytes_in_flight() >= window) {
            break;
        }
    }
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    const uint64_t ack_abs = unwrap(ackno, _isn, _next_seqno);

    // 过滤无效 ACK：不可超过已发送的下一个序号
    if (ack_abs > _next_seqno) {
        return;
    }
    // 重复/过时 ACK：不推进前沿
    if (ack_abs <= _ackno) {
        _window_size = window_size;
        return;
    }

    // 有效 ACK：更新窗口与前沿
    _window_size = window_size;
    _ackno = ack_abs;

    bool progressed = false;
    // 移除所有“已完全确认”的最早未确认段
    while (!_outstanding_segments.empty()) {
        const TCPSegment &seg = _outstanding_segments.front();
        const uint64_t seg_start = unwrap(seg.header().seqno, _isn, _next_seqno);
        const uint64_t seg_end = seg_start + seg.length_in_sequence_space();

        if (seg_end <= _ackno) {
            _bytes_in_flight -= seg.length_in_sequence_space();
            _outstanding_segments.pop();
            progressed = true;
        } else {
            break;
        }
    }

    // 若确认前沿推进：重置定时器与退避计数
    if (progressed) {
        _last_tick_total_time = 0;
        _consecutive_retransmissions = 0;
    }

    // 尝试继续填充窗口
    fill_window();
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) {
    // 无未确认段：定时器不运行
    if (_outstanding_segments.empty()) {
        return;
    }

    _last_tick_total_time += ms_since_last_tick;

    // 当前 RTO = 初始 RTO * 2^(连续重传次数)
    const unsigned int rto_multiplier = 1u << _consecutive_retransmissions;
    const unsigned int current_rto = _initial_retransmission_timeout * rto_multiplier;

    if (_last_tick_total_time >= current_rto) {
        // 超时：重传最早未确认段
        const TCPSegment &seg = _outstanding_segments.front();
        _segments_out.push(seg);

        // 指数退避
        _consecutive_retransmissions += 1;

        // 重启定时器
        _last_tick_total_time = 0;
    }
}

unsigned int TCPSender::consecutive_retransmissions() const { return _consecutive_retransmissions; }

void TCPSender::send_empty_segment() {
    TCPSegment seg;
    seg.header().seqno = wrap(_next_seqno, _isn);
    _segments_out.push(seg);
}