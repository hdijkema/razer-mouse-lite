#include "hiddescriptor.h"


int HidDescriptor::featureReportLength(const Bytes& descriptor)
{
    struct State { std::uint32_t size = 0, count = 0, id = 0; } state;
    std::vector<State> stack;
    std::uint32_t bits = 0;
    for (std::size_t offset = 0; offset < descriptor.size();) {
        const auto prefix = static_cast<unsigned char>(descriptor[offset++]);
        if (prefix == 0xfe) {
            if (offset + 2 > descriptor.size()) return 0;
            const auto length = static_cast<unsigned char>(descriptor[offset]);
            offset += 2 + length;
            if (offset > descriptor.size()) return 0;
            continue;
        }
        const int length = (prefix & 3) == 3 ? 4 : (prefix & 3);
        if (offset + length > descriptor.size()) return 0;
        std::uint32_t value = 0;
        for (int i = 0; i < length; ++i) value |= std::uint32_t(static_cast<unsigned char>(descriptor[offset++])) << (8 * i);
        switch (prefix & 0xfc) {
        case 0x74: state.size = value; break;
        case 0x94: state.count = value; break;
        case 0x84: if (value == 0 || value > 255) return 0; state.id = value; break;
        case 0xa4: if (stack.size() >= 32) return 0; stack.push_back(state); break;
        case 0xb4: if (stack.empty()) return 0; state = stack.back(); stack.pop_back(); break;
        case 0xb0:
            if (state.id != 0) break;
            if (state.size > 8192 || state.count > 8192 || std::uint64_t(state.size) * state.count + bits > 32768) return 0;
            bits += state.size * state.count;
            break;
        default: break;
        }
    }
    return bits == 0 ? 0 : int((bits + 7) / 8 + 1);
}
