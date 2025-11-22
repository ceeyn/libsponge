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
   uint64_t tmp =
        (n.raw_value() >= isn.raw_value()) ? n.raw_value() - isn.raw_value() : (1ul<<32) - (isn.raw_value() - n.raw_value());
    uint32_t dv = checkpoint / (1ul << 32);
    uint64_t cnt1 = dv * (1ul << 32) + tmp;
    uint64_t cnt2 = (dv - 1) * (1ul << 32) + tmp;
    uint64_t cnt3 = (dv + 1) * (1ul << 32) + tmp;
    uint64_t c1 = (checkpoint > cnt1) ? (checkpoint - cnt1) : (cnt1 - checkpoint);
    uint64_t c2 = (checkpoint > cnt2) ? (checkpoint - cnt2) : (cnt2 - checkpoint);
    uint64_t c3 = (checkpoint > cnt3) ? (checkpoint - cnt3) : (cnt3 - checkpoint);
    if (c1 <= c2 && c1 <= c3) {
        return cnt1;
    }
    else if (c2 <= c1 && c2 <= c3){
        return cnt2;
    }
    else{
        return cnt3;
    }
}
