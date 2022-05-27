#include "StringSettings.h"
#include "../../../../util/Assertions.h"
#include "../Generation.h"
#include "FactoryFactory.h"
#include "OutputSettings.h"

#include <iostream>
#include <map>

namespace Elevated {

static const std::locale& customLocale()
{
    static auto locale = std::locale::classic();
    return locale;
}

bool isOpeningParen(char c)
{
    return c == '(' || c == '[' || c == '{';
}

bool isClosingParen(char c)
{
    return c == ')' || c == ']' || c == '}';
}

const std::string trueChars = "true";
const std::string falseChars = "false";

StringSettings::StringSettings(std::string value)
    : m_value(std::move(value))
{
    // immediately discard any leading whitespace
    consumeWhiteSpace();
}

//  STRING IDEA:
// integer read everything until space or comma
// double read everything until space or comma
// bool value read everything until space or comma and if t/true or f/false -> valid
// string value if no " to consume read until space NOT COMMA!
//              else read until " not prepended with \ also
//                    \[anything] has to escape to be [anything] in case you want to end a
// group start-> consume (,[,{ not sure if options are better?
//                    string with \ also, maybe warn if \[^"\] because you should not do that?

// end same with ),],}

template<typename T>
bool StringSettings::readFromPartialString(T& ref, size_t to)
{
    ASSERT(to >= m_location);
    ASSERT(to <= m_value.length());
    if (to == m_location) {
        stringError("No more characters left to read from", true, m_location);
        return false;
    }
    std::string value = m_value.substr(m_location, to - m_location);
    std::istringstream stream(value);
    stream.imbue(customLocale());
    stream >> ref;

    bool succeeded = !stream.fail();
    //    if (!succeeded) {
    //        stringError(fmt::format("Failed to read {} from '{}'", typeid(T).name(), value), true,
    //        m_location, to);
    //    }
    if (!stream.eof()) {
        std::string leftOverString;
        size_t currentIndex = static_cast<size_t>(stream.tellg());
        std::getline(stream, leftOverString);
        stringError("Unused input, skipping over: '" + leftOverString + "'", true,
            m_location + currentIndex, to);
    }
    consumeSeparator(to);
    return succeeded;
}

template<typename T>
bool StringSettings::readSimpleValue(T& ref)
{
    // consume prepending whitespace
    consumeWhiteSpace();
    return readFromPartialString(ref, nextSeparator());
}

char StringSettings::peek(size_t offset)
{
    ASSERT(m_location + offset < m_value.length());
    return m_value[m_location + offset];
}

void StringSettings::getIntegerValue(std::string_view name, int& value, int, int)
{
    size_t preLocation = m_location;
    if (!readSimpleValue(value)) {
        stringError("Failed read int value for '" + std::string(name) + '\'', true, preLocation,
            m_location);
    }
    //    if (value < min || value > max) {
    //        stringError(fmt::format("Integer '{}' value out of bounds ({} to {})", name, min,
    //        max),
    //                    false);
    //    }
}

void StringSettings::getDoubleValue(std::string_view name, double& value, double, double)
{
    size_t preLocation = m_location;
    if (!readSimpleValue(value)) {
        stringError("Failed read double value for '" + std::string(name) + '\'', true, preLocation,
            m_location);
    }
    //    if (value < min || value > max) {
    //        stringError(fmt::format("Double value out of bounds '{}'", name), false);
    //    }
}

void StringSettings::boolValue(std::string_view name, bool& value)
{
    consumeWhiteSpace();
    size_t endOfWord = nextSeparator();

    if (m_location == endOfWord) {
        stringError("Failed read bool value for '" + std::string(name) + '\'', true);
        return;
    }

    size_t separator = endOfWord;

    enum BoolState {
        Failed,
        ReadingTrue,
        ReadingFalse,
    } state;
    const std::string& expectedChars = [&state, &separator, &name, this, first = consume()] {
        ASSERT(!std::isupper(first));
        if (first == 'f') {
            state = ReadingFalse;
            return falseChars;
        } else if (first == 't') {
            state = ReadingTrue;
            return trueChars;
        }

        stringError("Failed to read bool value for '" + std::string(name) + '\'', true, m_location,
            separator);
        consumeSeparator(separator);
        state = Failed;
        return trueChars;
    }();

    if (state == Failed) {
        return;
    }

    if ((endOfWord - m_location) > (expectedChars.size() - 1)) {
        size_t startOverflowLocation = m_location + expectedChars.size() - 1;
        stringError(
            "Unused input for bool '" + std::string(name) + "', skipping over: '" + m_value.substr(startOverflowLocation, endOfWord - startOverflowLocation) + '\'',
            true, m_location, endOfWord);
        endOfWord = startOverflowLocation;
    }

    size_t indexString = 1;

    while (m_location < endOfWord) {
        char c = consume();
        ASSERT(!std::isupper(c));
        ASSERT(std::islower(expectedChars[indexString]));
        if (c != expectedChars[indexString]) {
            state = Failed;
            stringError("Unknown bool value for '" + std::string(name) + '\'', true, m_location,
                separator);
            break;
        }
        indexString++;
    }

    consumeSeparator(separator);

    switch (state) {
    case ReadingTrue:
        value = true;
        break;
    case ReadingFalse:
        value = false;
        break;
    case Failed:
        break;
    }
}

bool StringSettings::hasFailed()
{
    return std::any_of(m_errors.begin(), m_errors.end(),
        [](ErrorMessage& error) { return error.error; });
}

bool StringSettings::hasFailedAfter(size_t location)
{
    return std::any_of(m_errors.begin(), m_errors.end(), [&location](ErrorMessage& error) {
        return error.error && error.from >= location;
    });
}

void StringSettings::stringValue(std::string_view name, std::string& value)
{
    static const constexpr char quote = '"';
    static const constexpr char escape = '\\';

    size_t startLocation = m_location;

    consumeWhiteSpace();

    if (eof()) {
        stringError("No characters to read string", true, m_location);
        return;
    }

    if (char first = peek(); first == quote) {
        consume();

        std::string collectedString;

        size_t nextQuote = findFirst(quote);
        collectedString.reserve(nextQuote - m_location);

        enum StringReadState {
            ReadingChars,
            ReadingEscape,
            ReadingQuote,
        } state
            = ReadingChars;

        while (state != ReadingQuote && !eof()) {
            char c = consume();
            switch (state) {
            case ReadingChars:
                switch (c) {
                case quote:
                    state = ReadingQuote;
                    break;
                case escape:
                    state = ReadingEscape;
                    break;
                default:
                    collectedString.push_back(c);
                }
                break;
            case ReadingEscape:
                collectedString.push_back(c);
                if (c == quote) {
                    nextQuote = findFirst(quote);
                    collectedString.reserve(value.size() + nextQuote - m_location);
                }
                if (c != quote && c != escape) {
                    stringError(R"(Warning unknown escape if you want a \ use \\)",
                        false, m_location - 1);
                }
                state = ReadingChars;
                break;
            case ReadingQuote:
                ASSERT_NOT_REACHED();
            }
        }

        if (m_location == m_value.size() && state != ReadingQuote) {
            stringError("String value '" + std::string(name) + "' not closed", false, startLocation,
                m_location);
        } else {
            value = std::move(collectedString);
        }
        size_t end = nextSeparator();
        consumeSeparator(end);
    } else {
        size_t end = nextSeparator();
        value = m_value.substr(m_location, end - m_location);
        std::transform(value.begin(), value.end(), value.begin(),
            [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        consumeSeparator(end);
    }
}

void StringSettings::startGroup(const std::string& name)
{
    static std::map<char, char> openToClose = {
        { '(', ')' },
        { '[', ']' },
        { '{', '}' },
    };
    consumeWhiteSpace();
    if (eof()) {
        stringError("No opening group character", true, m_location);
        return;
    }
    char open = peek();

    switch (open) {
    case '(':
    case '[':
    case '{':
        consume();
        ASSERT(openToClose.count(open) > 0);
        m_groups.emplace_back(m_location, openToClose[open], name);
        break;
    default:
        stringError("No opening group character", false, m_location);
    }
}

void StringSettings::endGroup(const std::string& name)
{
    consumeWhiteSpace();
    if (eof()) {
        stringError("No closing group character", true, m_location);
        return;
    }
    char close = peek();
    switch (close) {
    case ')':
    case ']':
    case '}':
        consume();
        if (m_groups.empty()) {
            stringError("No group opened", true, m_location);
            return;
        }
        if (auto& [pos, paren, cName] = m_groups.back(); paren != close) {
            stringError("Mismatching open and close parenthesis", false, pos, m_location);
            if (cName != name) {
                stringError("Mismatched group name, should be closing: " + cName + " but closing: " + name,
                    true, pos, m_location);
            }
        }
        m_groups.pop_back();

        // this is for cases like aaa(xxx), <- trailing comma
        if (!eof() && peek() == ',') {
            consume();
        }

        break;
    default:
        stringError("No closing group character", false, m_location);
    }
}

void StringSettings::addError(const std::string& message, bool error)
{
    stringError(message, error);
}

void StringSettings::stringError(const std::string& message, bool error, size_t from, size_t to)
{
    if (from == to && from == 0) {
        // probably nothing
        from = m_location;
    }
    std::stringstream ss;

    ss << message;

    for (auto it = m_groups.rbegin(); it != m_groups.rend(); it++) {
        ss << " while processing'" << std::get<2>(*it) << "' ";
    }

    m_errors.push_back(ErrorMessage { ss.str(), error, from, to });
}

void StringSettings::varargsValue(std::string_view name,
    const std::function<void(GeneratorSettings&, size_t, bool)>& func,
    size_t& count)
{
    size_t oldCount = count;
    size_t preLocation = m_location;
    if (!readSimpleValue(count)) {
        stringError("No count preceding varargs argument: " + std::string(name), true, preLocation,
            m_location);
        count = 0;
    }

    size_t afterCount = m_location;

    for (size_t i = 0; i < count; i++) {
        if (eof()) {
            stringError("No more input left to read next value!", false);
        }
        if (hasFailedAfter(afterCount)) {
            stringError("Varargs abort because of error", false);
            count = i;
            break;
        }
        func(*this, i, false);
    }

    for (size_t i = count; i < oldCount; i++) {
        func(*this, i, true);
    }
}

void StringSettings::optionValue(std::string_view name, size_t& value,
    const std::vector<std::string>& options)
{
    ASSERT(std::all_of(options.begin(), options.end(),
        [](const std::string& str) {
            return std::none_of(str.begin(), str.end(),
                [](unsigned char c) { return std::isupper(c); });
        }));
    size_t preIndex = m_location;
    std::string option;
    stringValue("Option value: " + std::string(name), option);
    auto it = std::find(options.begin(), options.end(), option);
    if (it == options.end()) {
        stringError("Unknown option '" + option + "' for: " + std::string(name), false, preIndex,
            m_location);
        value = options.size();
    } else {
        value = std::distance(options.begin(), it);
    }
}

void StringSettings::getUnsignedValue(std::string_view name, uint32_t& value, int, int)
{
    size_t preLocation = m_location;
    if (!readSimpleValue(value)) {
        stringError("Failed read unsigned value for '" + std::string(name) + '\'', true, preLocation,
            m_location);
    }
    //    if (value < min || value > max) {
    //        stringError(fmt::format("Integer '{}' value out of bounds ({} to {})", name, min,
    //        max),
    //                    false);
    //    }
}

size_t StringSettings::findFirst(char c)
{
    size_t offset = 0;
    while (m_location + offset < m_value.size() && peek(offset) != c) {
        offset++;
    }
    return m_location + offset;
}

char StringSettings::consume()
{
    ASSERT(!eof());

    char c = m_value[m_location++];
    if (c == '0') {
        stringError("Premature? end of string", true, m_location);
        m_location = m_value.size();
        return '0';
    }
    return static_cast<char>(std::tolower(c));
}

void StringSettings::consumeWhiteSpace()
{
    while (!eof() && std::isspace(peek())) {
        consume();
    }
}

bool StringSettings::eof() const
{
    return m_location >= m_value.size();
}

size_t StringSettings::nextSeparator()
{
    //    spdlog::info("Looking for seperator, stamrting at: {} [{}]", m_location,
    //    m_value[m_location]);
    size_t offset = 0;
    while ((m_location + offset) < m_value.size()) {
        char c = peek(offset);
        if (std::isspace(c) || c == ',' || isOpeningParen(c) || isClosingParen(c)) {
            break;
        }
        offset++;
    }
    ASSERT((m_location + offset) <= m_value.size());
    return m_location + offset;
}

void StringSettings::consumeSeparator(size_t loc)
{
    ASSERT(loc <= m_value.size());

    if (loc >= m_value.size()) {
        m_location = m_value.size();
    } else if (char v = m_value[loc]; isOpeningParen(v) || isClosingParen(v)) {
        if (isClosingParen(v) && m_groups.empty()) {
            stringError("Group closed without ever being opened", true, loc);
            m_location = loc + 1;
        } else {
            //            spdlog::info("separator [{}]", v);
            m_location = loc;
        }
    } else {
        ASSERT((std::isspace(m_value[loc]) || m_value[loc] == ','));
        m_location = loc + 1;
    }
}

ScenarioResult parse_scenario(const std::string& value, long initial_seed)
{
    StringSettings bSettings { value };
    ProxyOutputSettings settings { bSettings };
    settings.set_initial_seed(initial_seed);
    auto factory = scenarioFactories().createGenerator("root");
    ScenarioResult result{};
    result.generator = factory->visit(settings);
    if (bSettings.hasFailed())
        result.generator = nullptr;

    if (settings.hasFailed()) {
        ASSERT((bSettings.hasFailed() || settings.error().has_value()));
        if (settings.error().has_value())
            result.errors.push_back("Could not construct scenario from string: " + value + ", last error: " + settings.error().value());
    }
    for (auto& error : bSettings.errors()) {
        if (error.error) {
            ASSERT(result.generator == nullptr);
            result.errors.push_back("Error: " + error.message + " (" + std::to_string(error.from) + " to " + std::to_string(error.to) + ')');
        } else {
            result.errors.push_back("Warning: " + error.message + " (" + std::to_string(error.from) + " to " + std::to_string(error.to) + ')');
        }

//        size_t context = 10;
//        auto loc = error.from;
//        if (error.to != 0 && error.to > error.from) {
//            size_t from = std::max(size_t(0), loc - context);
//            size_t to = std::min(value.size(), error.to + context + 1);
//
//            ASSERT(error.to > error.from);
//
//            logger->info(" {} [{}] [{}]", value.substr(from, to - from), value[loc],
//                value[error.to]);
//            logger->info(" {0:_^{1}}^{0:=^{2}}^{0:_^{1}}", "", context, error.to - error.from - 1);
//        } else {
//            size_t from = std::max(size_t(0), loc - context);
//            size_t to = std::min(value.size(), loc + context + 1);
//
//            logger->info(" {} [{}]", value.substr(from, to - from), value[loc]);
//            logger->info(" {0:_^{1}}^{0:_^{1}}", "", context);
//        }
    }

#ifndef NDEBUG
    std::cerr << "Got output:" << settings.value() << "\nFrom Input:" << value << '\n';
#endif

    if (result.generator != nullptr && settings.value() != value)
        std::cerr << "Input and output passed but not identical!\n";

    return result;
}

}
