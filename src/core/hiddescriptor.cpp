#include "hiddescriptor.h"
#include <QList>

int HidDescriptor::featureReportLength(const QByteArray &descriptor)
{
    struct State { quint32 size = 0, count = 0, id = 0; } state;
    QList<State> stack;
    quint32 bits = 0;
    for (qsizetype offset = 0; offset < descriptor.size();) {
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
        quint32 value = 0;
        for (int i = 0; i < length; ++i) value |= quint32(static_cast<unsigned char>(descriptor[offset++])) << (8 * i);
        switch (prefix & 0xfc) {
        case 0x74: state.size = value; break;
        case 0x94: state.count = value; break;
        case 0x84: if (value == 0 || value > 255) return 0; state.id = value; break;
        case 0xa4: if (stack.size() >= 32) return 0; stack.append(state); break;
        case 0xb4: if (stack.isEmpty()) return 0; state = stack.takeLast(); break;
        case 0xb0:
            if (state.id != 0) break;
            if (state.size > 8192 || state.count > 8192 || quint64(state.size) * state.count + bits > 32768) return 0;
            bits += state.size * state.count;
            break;
        default: break;
        }
    }
    return bits == 0 ? 0 : int((bits + 7) / 8 + 1);
}
