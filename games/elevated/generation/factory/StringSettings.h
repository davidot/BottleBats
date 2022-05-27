#pragma once

#include "../Generation.h"
#include "GeneratorSettings.h"

#include <deque>
#include <sstream>
#include <tuple>
#include <vector>

namespace Elevated {

struct ScenarioResult {
    std::unique_ptr<ScenarioGenerator> generator;
    std::vector<std::string> errors;
};

ScenarioResult parse_scenario(const std::string& value, long initial_seed);

class StringSettings : public GeneratorSettings {
private:
    std::string m_value;
    size_t m_location = 0;

public:
    struct ErrorMessage {
        std::string message;
        bool error;
        size_t from = 0;
        size_t to = 0;
    };

private:
    std::vector<ErrorMessage> m_errors;

    std::deque<std::tuple<size_t, char, std::string>> m_groups;

    template<typename T>
    bool readFromPartialString(T& ref, size_t to);

    template<typename T>
    bool readSimpleValue(T& ref);

    char peek(size_t offset = 0);

    size_t findFirst(char c);

    // should return lower case only
    char consume();

    void consumeWhiteSpace();

    size_t nextSeparator();

    void consumeSeparator(size_t);

    void stringError(const std::string& message, bool error = true, size_t from = 0, size_t to = 0);

    bool hasFailedAfter(size_t location);

public:
    explicit StringSettings(std::string value);

protected:
    void startGroup(const std::string& name) override;
    void endGroup(const std::string& name) override;
    void getIntegerValue(std::string_view name, int& value, int min, int max) override;
    void getDoubleValue(std::string_view name, double& value, double min, double max) override;
    void addError(const std::string& message, bool error) override;
    void getUnsignedValue(std::string_view name, uint32_t& value, int min, int max) override;

public:
    void varargsValue(std::string_view name,
                      const std::function<void(GeneratorSettings&, size_t, bool)>& func,
                      size_t& count) override;

    void optionValue(std::string_view name, size_t& value,
                     const std::vector<std::string>& options) override;

    void boolValue(std::string_view name, bool& value) override;

    void stringValue(std::string_view name, std::string& value) override;

    bool hasFailed() override;

    [[nodiscard]] const std::vector<ErrorMessage>& errors() const { return m_errors; }
    [[nodiscard]] bool eof() const;
};

}
