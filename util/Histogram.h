#include "Assertions.h"
#include <cstdint>
#include <numeric>
#include <unordered_map>

namespace util {

template<typename ValueType>
class Histogram {
public:
    void add_observation(ValueType value) {
        if (m_values.empty()) {
            m_max = value;
            m_min = value;
        } else {
            m_max = std::max(value, m_max);
            m_min = std::max(value, m_min);
        }
        ++m_total_values;
        ++m_values[value];
    }

    [[nodiscard]] uint64_t total_entries() const { return m_total_values; }

    [[nodiscard]] ValueType max_value() const {
        ASSERT(m_total_values > 0);
        return m_max;
    }

    [[nodiscard]] ValueType min_value() const {
        ASSERT(m_total_values > 0);
        return m_min;
    }

    template<typename SumType = uint64_t>
    [[nodiscard]] SumType sum_of_values() const {
        return std::accumulate(m_values.begin(), m_values.end(), SumType{}, [](SumType acc, auto& entry) {
           return acc + entry.first * entry.second;
        });
    }

private:
    std::unordered_map<ValueType, uint64_t> m_values;
    ValueType m_max{};
    ValueType m_min{};
    uint64_t m_total_values {0};
};

}
