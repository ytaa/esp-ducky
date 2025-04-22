#include <bits/stdc++.h>

#include "Script.hpp"
#include "Logger.hpp"

const uint8_t Script::asciiToKeycodeConvTable[ASCII_CHAR_NUM][ASCII_CONV_TABLE_DIM_NUM] =  { HID_ASCII_TO_KEYCODE }; 

std::array<Script::SpecialKey, Script::SPECIAL_KEY_NUM> Script::specialKeys = {{
    {"UP", HID_KEY_ARROW_UP},//
    {"UPARROW", HID_KEY_ARROW_UP},
    {"DOWN", HID_KEY_ARROW_DOWN},
    {"DOWNARROW ", HID_KEY_ARROW_DOWN},
    {"LEFT", HID_KEY_ARROW_LEFT},
    {"LEFTARROW", HID_KEY_ARROW_LEFT},
    {"RIGHT", HID_KEY_ARROW_RIGHT},
    {"RIGHTARROW", HID_KEY_ARROW_RIGHT},
    {"ENTER", HID_KEY_ENTER},
    {"ESCAPE", HID_KEY_ESCAPE},
    {"BACKSPACE", HID_KEY_BACKSPACE},
    {"TAB", HID_KEY_TAB},
    {"SPACE", HID_KEY_SPACE},
    {"DELETE", HID_KEY_DELETE},
    {"DEL", HID_KEY_DELETE},
    {"INSERT", HID_KEY_INSERT},
    {"HOME", HID_KEY_HOME},
    {"END", HID_KEY_END},
    {"PAGEUP", HID_KEY_PAGE_UP},
    {"PAGEDOWN", HID_KEY_PAGE_DOWN},
    {"PAUSE", HID_KEY_PAUSE},
    {"PRINTSCREEN", HID_KEY_PRINT_SCREEN},
    {"MENU", HID_KEY_MENU},
    {"F1", HID_KEY_F1},
    {"F2", HID_KEY_F2},
    {"F3", HID_KEY_F3},
    {"F4", HID_KEY_F4},
    {"F5", HID_KEY_F5},
    {"F6", HID_KEY_F6},
    {"F7", HID_KEY_F7},
    {"F8", HID_KEY_F8},
    {"F9", HID_KEY_F9},
    {"F10", HID_KEY_F10},
    {"F11", HID_KEY_F11},
    {"F12", HID_KEY_F12},
    // LOCKING KEYS
    {"CAPSLOCK", HID_KEY_CAPS_LOCK},
    {"NUMLOCK", HID_KEY_NUM_LOCK},
    {"SCROLLLOCK", HID_KEY_SCROLL_LOCK},
    // MODIFIER KEYS
    {"CTRL", HID_KEY_CONTROL_LEFT},
    {"CONTROL", HID_KEY_CONTROL_LEFT},
    {"SHIFT", HID_KEY_SHIFT_LEFT},
    {"ALT", HID_KEY_ALT_LEFT},
    {"GUI", HID_KEY_GUI_LEFT},
    {"WINDOWS", HID_KEY_GUI_LEFT},
    {"COMMAND", HID_KEY_GUI_LEFT},
}};

std::array<Script::ExpressionHandler, Script::EXPRESSIONS_NUM> Script::expressionHandlers = {{
    { // EMPTY LINE
        R"(^( |\t)*?\n)", 
        [](std::smatch &match, Script::CommandVector &commands) {
            // Do nothing - empty line is ignored
            LOGD("=== Processing empty line ===");
            return ErrorCode::Success;
        }
    },
    { // REM
        R"(^( |\t)*?REM ([\s\S]*?)\n)", 
        [](std::smatch &match, Script::CommandVector &commands) {
            // Do nothing - comment is ignored
            LOGD("=== Processing REM expression ===");
            return ErrorCode::Success;
        }
    },
    { // REM_BLOCK
        R"(^( |\t)*?REM_BLOCK ([\s\S]*?)END_REM( |\t)*\n)", 
        [](std::smatch &match, Script::CommandVector &commands) {
            // Do nothing - comment is ignored
            LOGD("=== Processing REM_BLOCK expression ===");
            return ErrorCode::Success;
        }
    },
    { // STRING
        R"(^( |\t)*?STRING (([\S]| |\t)+?)\n)", 
        [](std::smatch &match, Script::CommandVector &commands) {
            // The regex should match the string command and parameter without ignoring leading and trailing spaces
            // The parameter should be available in the match group with index 2
            LOGD("=== Processing STRING expression ===");
            if (match.size() < 3u) {
                LOGE("Invalid number of match groups for STRING expression: %zu", match.size());
                return ErrorCode::GeneralError;
            }

            LOGD("STRING parameter: '%s'", match[2u].str().c_str());
            commands.emplace_back(Script::Command::StringWrite, std::string(match[2u].str()));
            return ErrorCode::Success;
        }
    },
    { // STRINGLN
        R"(^( |\t)*?STRINGLN (([\S]| |\t)+?)\n)", 
        [](std::smatch &match, Script::CommandVector &commands) {
            // The regex should match the stringln command and parameter without ignoring leading and trailing spaces
            // The parameter should be available in the match group with index 2
            LOGD("=== Processing STRINGLN expression ===");
            if (match.size() < 3u) {
                LOGE("Invalid number of match groups for STRINGLN expression: %zu", match.size());
                return ErrorCode::GeneralError;
            }

            LOGD("STRINGLN parameter: '%s'", match[2u].str().c_str());
            commands.emplace_back(Script::Command::StringWrite, std::string(match[2u].str()));
            commands.emplace_back(Script::Command::KeyStroke, std::vector<uint8_t>{HID_KEY_ENTER});
            return ErrorCode::Success;
        }
    },
    { // DELAY
        R"(^( |\t)*?DELAY (([1-9]+[0-9]*)|0)( |\t)*?\n)", 
        [](std::smatch &match, Script::CommandVector &commands) {
            // The regex should match the delay command and parameter and ignore leading and trailing spaces
            // The parameter should be available in the match group with index 2
            LOGD("=== Processing DELAY expression ===");
            if (match.size() < 3u) {
                LOGE("Invalid number of match groups for DELAY expression: %zu", match.size());
                return ErrorCode::GeneralError;
            }

            LOGD("DELAY parameter: '%s'", match[2u].str().c_str());
            commands.emplace_back(Script::Command::Delay, static_cast<uint32_t>(std::stoul(match[2u].str())));
            return ErrorCode::Success;
        }
    },
    { // KEYSTROKE - SINGLE KEY
        R"(^( |\t)*?([\S]+)( |\t)*?\n)", 
        [](std::smatch &match, Script::CommandVector &commands) {
            // The regex should match a single key and ignore leading and trailing spaces
            // The key should be available in the match group with index 2
            LOGD("=== Processing single key expression ===");
            if (match.size() < 3u) {
                LOGE("Invalid number of match groups for single key expression: %zu", match.size());
                return ErrorCode::GeneralError;
            }

            LOGD("Single key parameter: '%s'", match[2u].str().c_str());
            std::vector<uint8_t> keyCodes{};
            auto errorCode = Script::parseKeyStroke(match[2u].str(), keyCodes);

            if (errorCode != ErrorCode::Success) {
                LOGE("Failed to parse key stroke: '%s'", match[2u].str().c_str());
                return errorCode;
            }
            if (keyCodes.empty()) {
                // Should never happen...
                LOGE("No key codes found for key: '%s'", match[2u].str().c_str());
                return ErrorCode::GeneralError;
            }

            // Add the key code to the command vector
            commands.emplace_back(Script::Command::KeyStroke, std::move(keyCodes));
            return ErrorCode::Success;
        }
    },
    { // KEYSTROKE - MULTIPLE KEYS
        R"(^( |\t)*?(([\S]+ )+[\S]+)( |\t)*?\n)", 
        [](std::smatch &match, Script::CommandVector &commands) {
            // The regex should match a line containing multiple keys and ignore leading and trailing spaces
            // The keys should be available in the match group with index 2
            LOGD("=== Processing multiple keys expression ===");
            if (match.size() < 3u) {
                LOGE("Invalid number of match groups for multiple keys expression: %zu", match.size());
                return ErrorCode::GeneralError;
            }

            LOGD("Multiple keys parameter: '%s'", match[2u].str().c_str());
            std::vector<uint8_t> keyCodes{};

            // Create a string stream from the matched string for parsing
            std::stringstream keyStream(match[2u].str());
            std::string keyName;

            while(keyCodes.size() < 6u)
            {

                // Split the matched line by spaces
                if(!getline(keyStream, keyName, ' '))
                {
                    // No more keys to process
                    break;
                }

                LOGD("Parsing key: '%s'", keyName.c_str());
                auto errorCode = Script::parseKeyStroke(keyName, keyCodes);
                
                if (errorCode != ErrorCode::Success) {
                    LOGE("Failed to parse key stroke: '%s'", keyName.c_str());
                    return errorCode;
                }
                if (keyCodes.empty()) {
                    // Should never happen...
                    LOGE("No key codes found for key: '%s'", keyName.c_str());
                    return ErrorCode::GeneralError;
                }
            }

            // Add the key code to the command vector
            commands.emplace_back(Script::Command::KeyStroke, std::move(keyCodes));
            return ErrorCode::Success;
        }
    },
}};

ErrorCode Script::parseAscii(const char chr, std::vector<uint8_t> &keyCodes) {
    uint8_t chrIdx = static_cast<uint8_t>(chr);
    // Check if the key is mappable to a ASCII character
    if (chrIdx >= ASCII_CHAR_NUM) {
        // Character is not mappable to a key code
        LOGE("Character '%c' is not mappable to a key code", chr);
        return ErrorCode::GeneralError;
    }
    // Check if it is an uppercase letter
    if(asciiToKeycodeConvTable[chrIdx][0u]) {
        // Check if the shift key is already in the vector
        if (std::find(keyCodes.begin(), keyCodes.end(), HID_KEY_SHIFT_LEFT) == keyCodes.end()) {
            // Add shift key code for uppercase
            keyCodes.push_back(HID_KEY_SHIFT_LEFT);
        }
    }

    // Add the mapped key code
    keyCodes.push_back(asciiToKeycodeConvTable[chrIdx][1u]);
    return ErrorCode::Success;

}

ErrorCode Script::parseKeyStroke(const std::string &keyName, std::vector<uint8_t> &keyCodes) {
    // Check if the key is a single character
    if (keyName.length() == 1u) {
        return parseAscii(keyName[0u], keyCodes);
    }
    else
    {
        // Key is not a single ASCII character, check if it is a special key
        auto it = std::find_if(specialKeys.begin(), specialKeys.end(), [&keyName](const SpecialKey &key) {
            return key.name == keyName;
        });

        if (it == specialKeys.end()) {
            // Special key not found - parsing failed
            return ErrorCode::GeneralError;          
        }

        // Add the corresponding key code
        keyCodes.push_back(it->keyCode);
    }

    return ErrorCode::Success;
}

std::optional<Script> Script::parse(std::string input){
    Script::CommandVector commands{};

    // Ensure there is a newline at the end of the input string
    if (input.back() != '\n') {
        input += '\n';
    }

    while(input.size() > 0){
        std::smatch match;
        bool found = false;

        // Iterate through the expression handlers
        for (const auto &handler : expressionHandlers) {
            if (std::regex_search(input, match, std::regex(handler.regexStr))) {
                // Call the process function of the matched handler
                auto errorCode = handler.process(match, commands);
                if (errorCode != ErrorCode::Success) {
                    return std::nullopt; // Parsing failed
                }
                found = true;
                break;
            }
        }

        if (!found) {
            // No matching expression found - parsing failed
            return std::nullopt;
        }

        // Remove the matched part from the input string
        input = match.suffix().str();
    }

    return Script(std::move(commands));
}

Script::Script(Script::CommandVector &commands)
:commands(commands)
{}


Script::Script(Script::CommandVector &&commands)
:commands(std::move(commands))
{}

ErrorCode Script::run(UsbDevice &usbDevice) {
    for (const auto &command : commands) {
        switch (std::get<0>(command)) {
            case Command::StringWrite: {
                const std::string &str = std::any_cast<std::string>(std::get<1>(command));
                for(const char &chr : str) {
                    std::vector<uint8_t> keyCodes{};
                    auto errorCode = Script::parseAscii(chr, keyCodes);
                    if (errorCode != ErrorCode::Success) {
                        LOGE("Failed to parse ASCII character: '%c'", chr);
                        return errorCode;
                    }
                    if (keyCodes.empty()) {
                        // Should never happen...
                        LOGE("No key codes found for character: '%c'", chr);
                        return ErrorCode::GeneralError;
                    }
                    usbDevice.hidKeyStroke(keyCodes);
                }
                break;
            }
            case Command::KeyStroke: {
                const std::vector<uint8_t> &keyCodes = std::any_cast<std::vector<uint8_t>>(std::get<1>(command));
                usbDevice.hidKeyStroke(keyCodes);
                break;
            }
            case Command::Delay: {
                uint32_t delay = std::any_cast<uint32_t>(std::get<1>(command));
                vTaskDelay(delay / portTICK_PERIOD_MS);
                break;
            }
            default:
                LOGE("Unknown command in command vector");
                return ErrorCode::GeneralError;
        }
    }

    return ErrorCode::Success;
}
