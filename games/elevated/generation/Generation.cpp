#include "Generation.h"
#include "../../../util/Assertions.h"
#include <set>

namespace Elevated {

NextRequests NextRequests::done() {
    return {Type::Done, 0};
}

NextRequests NextRequests::unknown() {
    return {Type::Unknown, 0};
}

NextRequests NextRequests::at(Time time) {
    return {Type::At, time};
}

std::strong_ordering NextRequests::operator<=>(NextRequests const& rhs) const {
    if (type == rhs.type) {
        if (type != Type::At)
            return std::strong_ordering::equal;

        return next_request_time <=> rhs.next_request_time;
    }
    if (type == Type::At)
        return std::strong_ordering::less;
    if (type == Type::Done)
        return std::strong_ordering::greater;
    ASSERT(type == Type::Unknown);
    if (rhs.type == Type::At)
        return std::strong_ordering::greater;

    ASSERT(rhs.type == Type::Done);
    return std::strong_ordering::less;
}

}
