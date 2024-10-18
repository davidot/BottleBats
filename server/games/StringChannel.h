#pragma once
#include "../../util/Process.h"
#include "NamedEnum.h"
#include <charconv>
#include <sstream>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace BBServer {

struct StringWriter;
struct StringCommunicator;

struct StringReaderResult {
    const bool failed;
    const bool can_continue;
    const std::string error;

    StringReaderResult()
        : failed(false)
        , can_continue(true)
        , error("")
    {
    }

    StringReaderResult(bool _continue, std::string _error)
        : failed(true)
        , can_continue(_continue)
        , error(std::move(_error))
    {
    }
};

struct StringReader {

    StringReaderResult read_value(std::string_view& view);

    template<typename T>
    StringReaderResult read_int(T& value, T lower_bound = std::numeric_limits<T>::min(), T upper_bound = std::numeric_limits<T>::max())
    {
        std::string_view current_view;
        if (auto got_value = read_value(current_view); got_value.failed)
            return got_value;

        auto result = std::from_chars(current_view.data(), current_view.data() + current_view.size(), value);

        if (result.ec != std::errc {} || result.ptr != current_view.end())
            return error(std::string("Expected int but failed to find in: ") + std::string { current_view });

        if (value < lower_bound || value > upper_bound)
            return error(std::string("Got ") + std::to_string(value) + " which is outside of range [" + std::to_string(lower_bound) + ", " + std::to_string(upper_bound) + "]");

        return {};
    }

    template<typename T, size_t N>
    StringReaderResult read_mapped_value(T& value, StringMapper<T, N> const& mapper)
    {
        std::string_view current_view;
        if (auto got_value = read_value(current_view); got_value.failed)
            return got_value;

        if (!mapper.from_string(value, current_view))
            return error(std::string("Expected one of: [") + mapper.all_options() + "] got: " + current_view);

        return {};
    }

    StringReaderResult error(std::string error_message);

    StringReaderResult has_line();

    ~StringReader();

private:
    explicit StringReader(std::string value, bool can_continue, std::string_view splitter, StringCommunicator& comm);

    void fill_parts(std::string_view splitter);

    friend struct StringCommunicator;

    bool m_can_continue;
    bool hit_error = false;
    uint32_t m_index = 0;
    StringCommunicator& m_comm;
    // only used to keep string reference alive if needed
    std::string m_value;

    std::vector<std::pair<uint32_t, uint32_t>> m_parts;
};

struct StringCommunicator {
    friend struct StringWriter;

    enum class ConditionalOutput {
        EveryTime,
        OncePerInput
    };

    constexpr static ConditionalOutput OncePerInput = ConditionalOutput::OncePerInput;
    constexpr static ConditionalOutput EveryTime = ConditionalOutput::EveryTime;

    StringWriter output_writer(ConditionalOutput should_output);

    void write_line(std::string_view line, ConditionalOutput should_output = EveryTime);

    void clear_write_once_lock();

    std::optional<std::string> read_line_raw(size_t millis_read_timeout = 1000);

    StringReader input_reader(size_t millis_read_timeout = 1000, std::string const& split = " ");

    ~StringCommunicator();

    StringCommunicator(StringCommunicator&& other)
        : state(std::move(other.state))
    {
    }

    StringCommunicator(std::string& input_buffer, std::vector<std::string>& output_buffer);

    explicit StringCommunicator(std::unique_ptr<util::SubProcess> process);

private:
    void complete_line();

    bool is_owned() const;

    bool wrote_once_since_last_input = false;
    std::string output;

    struct ControlledProcess {
        std::unique_ptr<util::SubProcess> m_process;
    };

    struct ExternalProcess {
        std::vector<std::string>& output_buffer;
        std::string& input_buffer;
    };

    std::variant<ControlledProcess, ExternalProcess> state;
};

struct StringWriter {
    bool will_output() const;

    template<typename T>
    StringWriter& operator<<(T const& val)
    {
        std::ostringstream output;
        output << val;
        m_comm.output += output.view();
        return *this;
    }

    template<typename T>
    StringWriter& write_value(T const& val)
    {
        return this << val;
    }

    friend StringCommunicator;

    ~StringWriter();

private:
    StringWriter(bool will_output, StringCommunicator& comm);

    bool m_will_output;
    StringCommunicator& m_comm;
};

}