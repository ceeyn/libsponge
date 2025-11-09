#include "wrapping_integers.hh"

// Dummy implementation of a 32-bit wrapping integer

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! Transform an "absolute" 64-bit sequence number (zero-indexed) into a WrappingInt32
//! \param n The input absolute 64-bit sequence number
//! \param isn The initial sequence number
// (isn + asn) % 2^32 = sn
WrappingInt32 wrap(uint64_t n, WrappingInt32 isn) {
    constexpr uint64_t WINDOW = 1ULL << 32;
    const uint32_t raw = static_cast<uint32_t>((n + isn.raw_value()) % WINDOW);
    return WrappingInt32{raw};
}

//! Transform a WrappingInt32 into an "absolute" 64-bit sequence number (zero-indexed)
//! \param n The relative sequence number
//! \param isn The initial sequence number
//! \param checkpoint A recent absolute 64-bit sequence number
//! \returns the 64-bit sequence number that wraps to `n` and is closest to `checkpoint`
//!
//! \note Each of the two streams of the TCP connection has its own ISN. One stream
//! runs from the local TCPSender to the remote TCPReceiver and has one ISN,
//! and the other stream runs from the remote TCPSender to the local TCPReceiver and
//! has a different ISN.
// // n - isn + k * 2^32 closest to checkpoint
// checkpoint & 0xFFFFFFFF00000000 + (n - isn) % 2^32
// checkpoint & 0xFFFFFFFF00000000 + (n - isn) % 2^32 + 2^32
// checkpoint & 0xFFFFFFFF00000000 + (n - isn) % 2^32 - 2^32
uint64_t unwrap(WrappingInt32 n, WrappingInt32 isn, uint64_t checkpoint) {
    const uint32_t N = 1u << 32;
    const uint64_t WINDOW = 1ULL << 32;
    const uint64_t HALF   = 1ULL << 31;

    // 计算 32 位内的偏移量：offset = (n - isn) mod 2^32
    const uint32_t n_raw   = n.raw_value();
    const uint32_t isn_raw = isn.raw_value();
    const uint32_t offset  = static_cast<uint32_t>(n_raw - isn_raw); // 32 位环绕差

    // 以 checkpoint 的高 32 位为基准对齐
    uint64_t candidate = (checkpoint & ~0xFFFFFFFFULL) + offset;

    // 距离超过半个窗口则向邻近窗口调整
    if (candidate > checkpoint && candidate - checkpoint > HALF) {
        // 避免下溢
        if (candidate >= WINDOW) candidate -= WINDOW;
    } else if (candidate < checkpoint && checkpoint - candidate > HALF) {
        candidate += WINDOW;
    }

    return candidate;
}
