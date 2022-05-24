#pragma once

#include "GeneratorSettings.h"

#include <deque>
#include <sstream>
#include <optional>

namespace Elevated {

class OutputSettings : public GeneratorSettings {
private:
    enum class SeparatorType { SEPARATOR_AFTER, IS_SEPARATOR, NOTHING };

    size_t m_groups = 0;
    std::ostringstream m_strm;
    std::optional<std::string> m_lastError;
    bool m_separatorNeeded = false;

    template<typename Value>
    void write(Value& value, SeparatorType separator = SeparatorType::SEPARATOR_AFTER);

public:
    void varargsValue(std::string_view name,
                      const std::function<void(GeneratorSettings&, size_t, bool)>& func,
                      size_t& count) override;
    void boolValue(std::string_view name, bool& value) override;
    void stringValue(std::string_view name, std::string& value) override;
    void optionValue(std::string_view name, size_t& value,
                     const std::vector<std::string>& options) override;
    bool hasFailed() override;

    const std::optional<std::string>& error();

    [[nodiscard]] std::string value() const;

    void clearValue();

protected:
    void startGroup(const std::string&) override;
    void endGroup(const std::string&) override;
    void getIntegerValue(std::string_view name, int& value, int min, int max) override;
    void getUnsignedValue(std::string_view name, uint32_t& value, int min, int max) override;
    void getDoubleValue(std::string_view name, double& value, double min, double max) override;
    void addError(const std::string& message, bool error) override;
};

template<typename WrappedType>
class ProxyGeneratorSettings : public WrappedType {
    static_assert(std::is_base_of_v<GeneratorSettings, WrappedType>);

private:
    GeneratorSettings& m_settings;
    std::deque<GeneratorSettings::GeneratorGroup> m_groups;
    size_t m_varargCount = 0;

public:
    explicit ProxyGeneratorSettings(GeneratorSettings& mSettings) : m_settings(mSettings) {}

    void varargsValue(std::string_view name,
                      const std::function<void(GeneratorSettings&, size_t, bool)>& func,
                      size_t& count) override {
        if (!m_varargCount) {
            m_settings.varargsValue(name, func, count);
        }
        m_varargCount++;
        ASSERT(m_varargCount);
        WrappedType::varargsValue(name, func, count);
        ASSERT(m_varargCount > 0);
        m_varargCount--;
    }

    void boolValue(std::string_view name, bool& value) override {
        if (!m_varargCount) {
            m_settings.boolValue(name, value);
        }
        WrappedType::boolValue(name, value);
    }

    void stringValue(std::string_view name, std::string& value) override {
        if (!m_varargCount) {
            m_settings.stringValue(name, value);
        }
        WrappedType::stringValue(name, value);
    }
    void optionValue(std::string_view name, size_t& value,
                     const std::vector<std::string>& options) override {
        if (!m_varargCount) {
            m_settings.optionValue(name, value, options);
        }
        WrappedType::optionValue(name, value, options);
    }
    bool hasFailed() override {
        if (m_settings.hasFailed()) {
            return true;
        } else {
            return WrappedType::hasFailed();
        }
    }

protected:
    void startGroup(const std::string& name) override {
        if (!m_varargCount) {
            m_groups.emplace_back(m_settings, name);
        }
        WrappedType::startGroup(name);
    }

    void endGroup(const std::string& name) override {
        if (!m_varargCount) {
            m_groups.pop_back();
        }
        WrappedType::endGroup(name);
    }
    void getIntegerValue(std::string_view name, int& value, int min, int max) override {
        if (!m_varargCount) {
            m_settings.integerValue(name, value, min, max);
        }
        WrappedType::getIntegerValue(name, value, min, max);
    }
    void getUnsignedValue(std::string_view name, uint32_t& value, int min, int max) override {
        if (!m_varargCount) {
            m_settings.unsignedValue(name, value, min, max);
        }
        WrappedType::getUnsignedValue(name, value, min, max);
    }
    void getDoubleValue(std::string_view name, double& value, double min, double max) override {
        if (!m_varargCount) {
            m_settings.doubleValue(name, value, min, max);
        }
        WrappedType::getDoubleValue(name, value, min, max);
    }

    void addError(const std::string& message, bool error) override {
        if (!m_varargCount) {
            m_settings.encounteredError(message, error);
        }
        WrappedType::addError(message, error);
    }
};

using ProxyOutputSettings = ProxyGeneratorSettings<OutputSettings>;

}
