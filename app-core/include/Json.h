#pragma once

// ──────────────────────────────────────────────────────────────────────────────
// Minimal, dependency-free JSON helpers for the native ⇆ web bridge.
//
// The bridge protocol is a small, well-defined set of flat messages (see
// ARCHITECTURE.md §3), so a full JSON library is unnecessary. SimpleJSON builds
// outgoing messages; SimpleJSONParser reads the limited set of incoming ones.
//
// Scope/limitations (by design — keep messages flat):
//   * Parser handles string, number/bool and one level of nested object values.
//   * String values may contain escaped quotes; escape sequences are decoded.
//   * Not a general JSON library: it does not handle arrays of objects inbound.
// ──────────────────────────────────────────────────────────────────────────────

#include <string>
#include <map>
#include <vector>
#include <sstream>
#include <cctype>

// ──────────────────────────────────────────────────────────────────────────────
// JSON builder for outgoing messages (C++ → JS)
// ──────────────────────────────────────────────────────────────────────────────
class SimpleJSON {
public:
    SimpleJSON() = default;

    SimpleJSON& set(const std::string& key, const std::string& value) {
        stringValues[key] = escapeString(value);
        return *this;
    }

    // Explicit const char* overload: without it, string literals bind to the
    // bool overload (const char* → bool is a standard conversion, but
    // const char* → std::string is user-defined), silently turning
    // set("event", "FOO") into a boolean. This overload routes literals to the
    // string version.
    SimpleJSON& set(const std::string& key, const char* value) {
        return set(key, std::string(value));
    }

    SimpleJSON& set(const std::string& key, int value) {
        intValues[key] = value;
        return *this;
    }

    SimpleJSON& set(const std::string& key, double value) {
        doubleValues[key] = value;
        return *this;
    }

    SimpleJSON& set(const std::string& key, long long value) {
        longValues[key] = value;
        return *this;
    }

    SimpleJSON& set(const std::string& key, bool value) {
        boolValues[key] = value;
        return *this;
    }

    SimpleJSON& setArray(const std::string& key, const std::vector<std::string>& arr) {
        arrayValues[key] = arr;
        return *this;
    }

    SimpleJSON& setObject(const std::string& key, const SimpleJSON& obj) {
        objectValues[key] = obj;
        return *this;
    }

    std::string toString() const {
        std::ostringstream oss;
        oss << "{";
        bool first = true;

        for (const auto& [key, value] : stringValues) {
            if (!first) oss << ",";
            oss << "\"" << key << "\":\"" << value << "\"";
            first = false;
        }
        for (const auto& [key, value] : intValues) {
            if (!first) oss << ",";
            oss << "\"" << key << "\":" << value;
            first = false;
        }
        for (const auto& [key, value] : doubleValues) {
            if (!first) oss << ",";
            oss << "\"" << key << "\":" << value;
            first = false;
        }
        for (const auto& [key, value] : longValues) {
            if (!first) oss << ",";
            oss << "\"" << key << "\":" << value;
            first = false;
        }
        for (const auto& [key, value] : boolValues) {
            if (!first) oss << ",";
            oss << "\"" << key << "\":" << (value ? "true" : "false");
            first = false;
        }
        for (const auto& [key, arr] : arrayValues) {
            if (!first) oss << ",";
            oss << "\"" << key << "\":[";
            for (size_t i = 0; i < arr.size(); ++i) {
                if (i > 0) oss << ",";
                oss << "\"" << escapeString(arr[i]) << "\"";
            }
            oss << "]";
            first = false;
        }
        for (const auto& [key, obj] : objectValues) {
            if (!first) oss << ",";
            oss << "\"" << key << "\":" << obj.toString();
            first = false;
        }

        oss << "}";
        return oss.str();
    }

private:
    static std::string escapeString(const std::string& str) {
        std::string result;
        result.reserve(str.size());
        for (char c : str) {
            switch (c) {
                case '"':  result += "\\\""; break;
                case '\\': result += "\\\\"; break;
                case '\b': result += "\\b"; break;
                case '\f': result += "\\f"; break;
                case '\n': result += "\\n"; break;
                case '\r': result += "\\r"; break;
                case '\t': result += "\\t"; break;
                default:   result += c; break;
            }
        }
        return result;
    }

    std::map<std::string, std::string>              stringValues;
    std::map<std::string, int>                      intValues;
    std::map<std::string, double>                   doubleValues;
    std::map<std::string, long long>                longValues;
    std::map<std::string, bool>                     boolValues;
    std::map<std::string, std::vector<std::string>> arrayValues;
    std::map<std::string, SimpleJSON>               objectValues;
};

// ──────────────────────────────────────────────────────────────────────────────
// JSON parser for incoming messages (JS → C++)
// ──────────────────────────────────────────────────────────────────────────────
class SimpleJSONParser {
public:
    static std::map<std::string, std::string> parse(const std::string& json) {
        std::map<std::string, std::string> result;

        size_t pos = 0;
        while (pos < json.size()) {
            // Find key
            size_t keyStart = json.find('"', pos);
            if (keyStart == std::string::npos) break;
            keyStart++;

            size_t keyEnd = json.find('"', keyStart);
            if (keyEnd == std::string::npos) break;

            std::string key = json.substr(keyStart, keyEnd - keyStart);

            // Find colon
            size_t colon = json.find(':', keyEnd);
            if (colon == std::string::npos) break;

            // Find value
            size_t valueStart = colon + 1;
            while (valueStart < json.size() && std::isspace(json[valueStart])) valueStart++;

            std::string value;
            if (json[valueStart] == '"') {
                // String value: find the closing quote, skipping escaped quotes
                // (an odd number of preceding backslashes means it is escaped).
                valueStart++;
                size_t valueEnd = valueStart;
                while (valueEnd < json.size()) {
                    if (json[valueEnd] == '"') {
                        size_t backslashes = 0;
                        size_t k = valueEnd;
                        while (k > valueStart && json[k - 1] == '\\') {
                            backslashes++;
                            k--;
                        }
                        if (backslashes % 2 == 0) break;
                    }
                    valueEnd++;
                }
                if (valueEnd < json.size()) {
                    value = unescapeString(json.substr(valueStart, valueEnd - valueStart));
                    pos = valueEnd + 1;
                } else {
                    break;
                }
            } else if (json[valueStart] == '{') {
                // Object value - find matching }
                int braceCount = 1;
                size_t i = valueStart + 1;
                while (i < json.size() && braceCount > 0) {
                    if (json[i] == '{') braceCount++;
                    else if (json[i] == '}') braceCount--;
                    i++;
                }
                value = json.substr(valueStart, i - valueStart);
                pos = i;
            } else {
                // Number or boolean
                size_t valueEnd = json.find_first_of(",}", valueStart);
                if (valueEnd != std::string::npos) {
                    value = json.substr(valueStart, valueEnd - valueStart);
                    // Trim
                    value.erase(0, value.find_first_not_of(" \t\n\r"));
                    value.erase(value.find_last_not_of(" \t\n\r") + 1);
                    pos = valueEnd;
                }
            }

            result[key] = value;
        }

        return result;
    }

    static std::string getValue(const std::map<std::string, std::string>& data,
                                const std::string& key,
                                const std::string& defaultValue = "") {
        auto it = data.find(key);
        return (it != data.end()) ? it->second : defaultValue;
    }

private:
    // Turn JSON escape sequences back into their literal characters so values
    // such as Windows paths ("C:\\Games") or quoted text arrive intact.
    static std::string unescapeString(const std::string& raw) {
        std::string out;
        out.reserve(raw.size());
        for (size_t i = 0; i < raw.size(); ++i) {
            if (raw[i] == '\\' && i + 1 < raw.size()) {
                switch (raw[i + 1]) {
                    case '"':  out += '"';  ++i; break;
                    case '\\': out += '\\'; ++i; break;
                    case '/':  out += '/';  ++i; break;
                    case 'b':  out += '\b'; ++i; break;
                    case 'f':  out += '\f'; ++i; break;
                    case 'n':  out += '\n'; ++i; break;
                    case 'r':  out += '\r'; ++i; break;
                    case 't':  out += '\t'; ++i; break;
                    default:   out += raw[i]; break;
                }
            } else {
                out += raw[i];
            }
        }
        return out;
    }
};
