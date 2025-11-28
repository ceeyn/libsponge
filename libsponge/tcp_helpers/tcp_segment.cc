#include "tcp_segment.hh"

#include "parser.hh"
#include "util.hh"

#include <variant>

using namespace std;

//! \param[in] buffer string/Buffer to be parsed
//! \param[in] datagram_layer_checksum pseudo-checksum from the lower-layer protocol
ParseResult TCPSegment::parse(const Buffer buffer, const uint32_t datagram_layer_checksum) {
    InternetChecksum check(datagram_layer_checksum);
    check.add(buffer);
    if (check.value()) {
        return ParseResult::BadChecksum;
    }

    NetParser p{buffer};
    _header.parse(p);
    _payload = p.buffer();
    return p.get_error();
}
/**
 * 为什么和 payload 有关

- 序列号空间的目的是让 ACK 能统一表示“下一个期望字节的序列号”。这个空间既包含真实数据字节，也包含控制位（ SYN/FIN ）各自的一个单位。
- 一个段可能同时携带 SYN 和数据（虽然不常见，但协议允许）。这时：
  - SYN 占用 1 个序列号单位；
  - 载荷中的每个字节各占用 1 个序列号单位；
  - ACK 必须前进两者之和，这就是为什么和 payload 有关。
 */
size_t TCPSegment::length_in_sequence_space() const {
    return payload().str().size() + (header().syn ? 1 : 0) + (header().fin ? 1 : 0);
}

//! \param[in] datagram_layer_checksum pseudo-checksum from the lower-layer protocol
BufferList TCPSegment::serialize(const uint32_t datagram_layer_checksum) const {
    TCPHeader header_out = _header;
    header_out.cksum = 0;

    // calculate checksum -- taken over entire segment
    InternetChecksum check(datagram_layer_checksum);
    check.add(header_out.serialize());
    check.add(_payload);
    header_out.cksum = check.value();

    BufferList ret;
    ret.append(header_out.serialize());
    ret.append(_payload);

    return ret;
}
