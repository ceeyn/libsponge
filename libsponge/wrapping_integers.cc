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
WrappingInt32 wrap(uint64_t n, WrappingInt32 isn) {
    // DUMMY_CODE(n, isn);
    return isn + static_cast<uint32_t>(n);
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
uint64_t unwrap(WrappingInt32 n, WrappingInt32 isn, uint64_t checkpoint) {
    uint32_t offset = n - isn;  // uint32_t subtraction: wraps correctly
    // Get base = checkpoint aligned to 2^32
    uint64_t base = (checkpoint & 0xFFFFFFFF00000000ull);
    uint64_t candidate = base + offset;

    // Try three candidates: candidate, candidate - 2^32, candidate + 2^32
    // Select the one closest to checkpoint
    uint64_t best = candidate;
    if (candidate >= (1ul << 32)) {
        uint64_t cand2 = candidate - (1ul << 32);
        if (abs(static_cast<int64_t>(cand2 - checkpoint)) < abs(static_cast<int64_t>(best - checkpoint))) {
            best = cand2;
        }
    }

    uint64_t cand3 = candidate + (1ul << 32);
    if (abs(static_cast<int64_t>(cand3 - checkpoint)) < abs(static_cast<int64_t>(best - checkpoint))) {
        best = cand3;
    }

    return best;
}
