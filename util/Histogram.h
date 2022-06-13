#pragma once
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

    double avg_value() const {
        ASSERT(m_total_values != 0);
        return sum_of_values<double>() / (double) m_total_values;
    }

private:
    std::unordered_map<ValueType, uint64_t> m_values;
    ValueType m_max{};
    ValueType m_min{};
    uint64_t m_total_values {0};
};

template<typename KeyType, typename ValueType>
class StoringHistogram {
public:
    void add_to_observation(KeyType key, ValueType value) {
        m_sum += value;
        if (m_values.empty()) {
            m_max = value;
            m_min = value;
            m_values[key] = value;
        } else if (auto value_or_end = m_values.find(key); value_or_end != m_values.end()) {
            auto result = value_or_end->second += value;
            value = result;
        } else {
            m_values[key] = value;
        }
        if (value > m_max)
            m_max = value;
        if (value < m_min)
            m_min = value;
    }

    [[nodiscard]] ValueType max_value() const {
        return m_max;
    }

    [[nodiscard]] ValueType min_value() const {
        return m_min;
    }

    [[nodiscard]] ValueType sum_value() const {
        return m_sum;
    }

    template<typename SumType = uint64_t>
    [[nodiscard]] SumType sum_of_values() const {
        return std::accumulate(m_values.begin(), m_values.end(), SumType{}, [](SumType acc, auto& entry) {
            return acc + entry.second;
        });
    }

    double avg_value() const {
        if (m_values.empty())
            return 0.0f;
        return sum_of_values<double>() / (double) m_values.size();
    }

private:
    std::unordered_map<KeyType, ValueType> m_values;
    ValueType m_max{};
    ValueType m_min{};
    ValueType m_sum{};
};

}
