#include "OutputSettings.h"


namespace Elevated {


template<typename Value>
void OutputSettings::write(Value& value, SeparatorType separator) {
    if (m_separatorNeeded) {
        m_separatorNeeded = false;
        if (separator != SeparatorType::IS_SEPARATOR) {
            m_strm << ", ";
        }
    }
    m_strm << value;
    if (separator == SeparatorType::SEPARATOR_AFTER) {
        m_separatorNeeded = true;
    }
}

void OutputSettings::varargsValue(std::string_view,
                                  const std::function<void(GeneratorSettings&, size_t, bool)>& func,
                                  size_t& count) {
    write(count);
    for (size_t i = 0; i < count; i++) {
        func(*this, i, false);
    }
}

void OutputSettings::boolValue(std::string_view, bool& value) {
    if (value) {
        write("true");
    } else {
        write("false");
    }
}

void OutputSettings::stringValue(std::string_view, std::string& value) {
    write(value);
}

void OutputSettings::optionValue(std::string_view, size_t& value,
                                 const std::vector<std::string>& options) {
    // TODO: change interface to not require dynamic allocation!
    if (value < options.size()) {
        write(options[value]);
    } else {
        write("???");
    }
}

bool OutputSettings::hasFailed() {
    //    spdlog::warn("Current state: group {}, error {}", m_groups,
    //                 m_lastError.has_value() ? m_lastError.value() : "-");
    return m_groups > 0 || m_lastError.has_value();
}

void OutputSettings::startGroup(const std::string&) {
    write("(", SeparatorType::IS_SEPARATOR);
    m_groups++;
}

void OutputSettings::endGroup(const std::string&) {
    if (m_groups == 0) {
        ASSERT_NOT_REACHED();
    }
    write(")", SeparatorType::IS_SEPARATOR);
    write("", SeparatorType::SEPARATOR_AFTER);
    m_groups--;
}

void OutputSettings::getIntegerValue(std::string_view, int& value, int, int) {
    write(value);
}

void OutputSettings::getUnsignedValue(std::string_view, uint32_t& value, int, int) {
    write(value);
}

void OutputSettings::getDoubleValue(std::string_view, double& value, double, double) {
    write(value);
}

void OutputSettings::addError(const std::string& message, bool error) {
    // should not have any errors?
    if (error) {
        m_lastError = message;
    }
}

std::string OutputSettings::value() const {
    return m_strm.str();
}

const std::optional<std::string>& OutputSettings::error() {
    return m_lastError;
}

void OutputSettings::clearValue() {
    m_separatorNeeded = false;
    m_lastError = {};
    m_groups = 0;
    m_strm.str(std::string());
    m_strm.clear();
}

}
