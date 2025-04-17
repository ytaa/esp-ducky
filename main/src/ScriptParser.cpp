#include "ScriptParser.hpp"

std::array<ScriptParser::ExpressionHandler, ScriptParser::EXPRESSIONS_NUM> ScriptParser::expressionHandlers = {{
    {
        R"([\S]*REM ([\s\S]*?)\n)", 
        [](std::smatch &match, std::vector<std::tuple<ScriptParser::Command, std::any>> &actionList) {
            // Do nothing - comment is ignored
            return ErrorCode::Success;
        }
    },
    {
        R"([\s]*REM_BLOCK([\s\S]*?)END_REM( |\t)*\n)", 
        [](std::smatch &match, std::vector<std::tuple<ScriptParser::Command, std::any>> &actionList) {
            // Do nothing - comment is ignored
            return ErrorCode::Success;
        }
    },
    {
        R"(( |\t)*?STRING( |\t)+?([\S]|([\S]([\S]| |\t)*[\S]))[\s]*?\n)", 
        [](std::smatch &match, std::vector<std::tuple<ScriptParser::Command, std::any>> &actionList) {
            // The regex should match the string command and parameter ignoring leading and trailing spaces
            // The parameter should be available in the match group with index 3
            if (match.size() < 4u) {
                return ErrorCode::GeneralError;
            }
            actionList.emplace_back(ScriptParser::Command::WriteString, std::string(match[3u].str()));
            return ErrorCode::Success;
        }
    },
}};

ErrorCode ScriptParser::parse(const std::string &script, std::vector<std::tuple<Command, std::any>> &actionList){
    return ErrorCode::GeneralError;
}