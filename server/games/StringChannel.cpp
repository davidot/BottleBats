#include "StringChannel.h"
#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace BBServer {

StringReader::StringReader(std::string value, bool can_continue, std::string_view splitter, StringCommunicator& comm)
    : m_can_continue(can_continue)
    , m_comm(comm)
    , m_value(std::move(value))
{
    fill_parts(splitter);
};

void StringReader::fill_parts(std::string_view splitter)
{
    m_parts.reserve(8);

    size_t end_last = 0;
    std::string_view view { m_value };

    size_t pos_found = view.find(splitter);

    while (pos_found != std::string::npos) {
        m_parts.emplace_back(end_last, pos_found);
        end_last = pos_found + splitter.size();

        pos_found = view.find(splitter, end_last);
    }

    m_parts.emplace_back(end_last, view.size());
}

StringReaderResult StringReader::read_value(std::string_view& view)
{
    if (m_index >= m_parts.size())
        return error(std::string("Expected another part but only have ") + std::to_string(m_parts.size()) + " parts");

    auto [start, end] = m_parts[m_index];

    view = std::string_view { m_value }.substr(start, start - end);
    ++m_index;
    return {};
}

StringWriter::~StringWriter()
{
    if (m_will_output)
        m_comm.complete_line();
}

StringWriter StringCommunicator::output_writer(StringCommunicator::ConditionalOutput should_output)
{
    bool valid = true;
    if (should_output == ConditionalOutput::OncePerInput) {
        if (wrote_once_since_last_input)
            valid = false;

        wrote_once_since_last_input = true;
    }

    return { valid, *this };
}

void StringCommunicator::write_line(std::string_view line, StringCommunicator::ConditionalOutput should_output)
{
    if (should_output == ConditionalOutput::OncePerInput) {
        if (wrote_once_since_last_input)
            return;

        wrote_once_since_last_input = true;
    }

    output += line;
    complete_line();
}

std::optional<std::string> StringCommunicator::read_line_raw(size_t millis_read_timeout)
{
    if (!is_owned()) {
        auto& external_process = std::get<ExternalProcess>(state);
        auto& input_buffer = external_process.input_buffer;
        auto end_of_line = input_buffer.find('\n');
        if (end_of_line == std::string::npos)
            return std::nullopt;

        std::string line = input_buffer.substr(0, end_of_line);
        input_buffer.erase(0, end_of_line + 1);
        return line;
    }

    auto& controlled_process = std::get<ControlledProcess>(state);
    ASSERT(controlled_process.m_process);

    util::SubProcess& process = *(controlled_process.m_process);

    // Owned process returns empty line if we get none (probably bot misbehaved)
    if (output.empty()) {
        std::string line;

        bool got_line = process.readLineWithTimeout(line, millis_read_timeout);

        if (!got_line)
            return "";

        if (line.ends_with('\n'))
            line.pop_back();

        return line;
    } else {
        auto result = process.sendAndWaitForResponse(output, millis_read_timeout);
        output.clear();

        if (!result.has_value())
            return "";

        while (result->ends_with('\n'))
            result->pop_back();
        return result;
    }
}

StringReader StringCommunicator::input_reader(size_t millis_read_timeout, std::string const& split)
{
    // TODO: Can optimize the string lifetimes (not owned) only clearing the
    //       string data after StringReader is done
    //       For the owned one we could attempt to write on the same string
    auto line_or_nothing = read_line_raw(millis_read_timeout);
    if (!line_or_nothing.has_value())
        return StringReader("", !is_owned(), split, *this);

    return StringReader(std::move(*line_or_nothing), !is_owned(), split, *this);
}

StringReaderResult StringReader::has_line()
{
    // TODO: If optimized (see StringCommunicator::input_reader) we must store / whether a line was present
    if (!m_value.empty())
        return {};

    if (m_can_continue)
        return error("");

    return error("No output line!");
}

void StringCommunicator::complete_line()
{
    if (is_owned()) {
        output += '\n';
        return;
    }

    auto& external_process = std::get<ExternalProcess>(state);

    external_process.output_buffer.emplace_back(output);
    output.clear();
}

StringCommunicator::~StringCommunicator()
{
    if (is_owned()) {
        auto& controlled_process = std::get<ControlledProcess>(state);

        controlled_process.m_process.reset();
        ASSERT(controlled_process.m_process.get() == nullptr);
    }
}

StringCommunicator::StringCommunicator(std::string& input_buffer, std::vector<std::string>& output_buffer)
    : state(ExternalProcess { output_buffer, input_buffer })
{
}

StringCommunicator::StringCommunicator(std::unique_ptr<util::SubProcess> process)
    : state(ControlledProcess { std::move(process) })
{
}

bool StringCommunicator::is_owned() const
{
    return state.index() == 0;
}

StringReaderResult StringReader::error(std::string error_message)
{
    hit_error = true;
    return { m_can_continue, std::move(error_message) };
}

StringReader::~StringReader()
{
    if (!hit_error && !m_parts.empty())
        m_comm.clear_write_once_lock();
}

bool StringWriter::will_output() const
{
    return m_will_output;
}

StringWriter::StringWriter(bool will_output, StringCommunicator& comm)
    : m_will_output(will_output)
    , m_comm(comm)
{
}

void StringCommunicator::clear_write_once_lock()
{
    wrote_once_since_last_input = false;
}

}