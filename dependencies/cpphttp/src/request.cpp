#include "../include/request.hpp"

std::vector<std::string> CppHttp::Utils::Split(std::string const str, char delimiter) {
    std::vector<std::string> split;
    std::stringstream ss(str);
    std::string word;
    while (std::getline(ss, word, delimiter)) {
        if (!word.empty()) {
            split.push_back(word);
        }
    }
    return split;
}

std::string CppHttp::Utils::GetRoute(std::string const req)  {
    std::vector<std::string> split = CppHttp::Utils::Split(req, ' ');
    std::vector<std::string> split2 = CppHttp::Utils::Split(split[1], '?');

    return split2[0];
}

std::string CppHttp::Utils::GetMethod(const std::string req)  {
    std::vector<std::string> split = CppHttp::Utils::Split(req, ' ');
    return split[0];
}

std::unordered_map<std::string, std::string> CppHttp::Utils::GetParameters(const std::string req){
    std::vector<std::string> split = CppHttp::Utils::Split(req, ' ');
    std::string route = split[1];
    std::regex argsRegex("\\?(.*)");
    std::smatch argsMatch;
    std::regex_search(route, argsMatch, argsRegex);
    std::string parametersString = argsMatch[1];
    std::vector<std::string> parameters = CppHttp::Utils::Split(parametersString, '&');

    std::unordered_map<std::string, std::string> parametersMap;
    for (std::string parameter : parameters) {
        std::stringstream parameterStream(parameter);
        std::string key;
        std::string value;
        std::getline(parameterStream, key, '=');
        std::getline(parameterStream, value, '=');
        parametersMap[key] = value;
    }

    return parametersMap;
}

std::unordered_map<std::string, std::string> CppHttp::Utils::GetHeaders(std::string req){
    int bodyStartChar = req.size();

    if (req.find("\r\n\r\n") != std::string::npos) {
        bodyStartChar = req.find("\r\n\r\n");
    }
    else if (req.find("\n\n") != std::string::npos) {
        bodyStartChar = req.find("\n\n");
    }

    // remove body
    req = req.substr(0, bodyStartChar);

    auto split = CppHttp::Utils::Split(req, '\n');
    std::unordered_map<std::string, std::string> headers;

    for (int i = 0; i < split.size(); ++i) {
        auto headerMatch = ctre::match<"(.*?): (.*)">(split[i]);
        auto matched = headerMatch.matched();
        if (matched.to_string() != "") {
            std::string header = matched.get<1>().to_string();
            std::string value = matched.get<2>().to_string();
            value.pop_back();

            headers[header] = value;
        }
    }

    return headers;
}

std::string CppHttp::Utils::GetBody(const std::string req)  {
    std::string body = "";

    int bodyStartChar = -1;

    if (req.find("\r\n\r\n") != std::string::npos) {
        bodyStartChar = req.find("\r\n\r\n");
    }
    else if (req.find("\n\n") != std::string::npos) {
        bodyStartChar = req.find("\n\n");
    }

    if (bodyStartChar == -1) {
        return "";
    }

    return req.substr(bodyStartChar + 4);
}

std::string CppHttp::Utils::GetHeader(std::string const& req, std::string const& header)  {
    std::vector<std::string> split = CppHttp::Utils::Split(req, '\n');
    std::string headerLine = "";

    for (int i = 0; i < split.size(); ++i) {
        if (split[i].find(header) != std::string::npos) {
            headerLine = split[i];
            break;
        }
    }

    if (headerLine == "") {
        return "";
    }

    std::regex headerRegex(header + ": (.*)");
    std::smatch headerMatch;
    std::regex_search(headerLine, headerMatch, headerRegex);
    std::string headerStr = headerMatch[1];

    return headerStr;
}

std::string CppHttp::Utils::GetProtocol(std::string const& req)  {
    std::vector<std::string> split = CppHttp::Utils::Split(req, ' ');
    return split[2];
}

std::string CppHttp::Utils::GetProtocolVersion(std::string const& req)  {
    std::vector<std::string> split = CppHttp::Utils::Split(req, ' ');
    return split[3];
}