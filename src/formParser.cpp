#include "../include/formParser.hpp"

FormParser::FormParser(CppHttp::Net::Request req) {
	this->body = req.m_info.ubody;

	if (req.m_info.headers["Content-Type"].find("boundary=") != std::string::npos) {
		std::string boundary = req.m_info.headers["Content-Type"].substr(req.m_info.headers["Content-Type"].find("boundary=") + 9);
		this->delimiter = std::u8string(boundary.begin(), boundary.end());
	}
	else {
		throw std::runtime_error("No delimiter found");
	}
}

std::unordered_map<std::u8string, std::u8string> FormParser::Parse() {
	std::unordered_map<std::u8string, std::u8string> fields;

	std::u8string delimiter = u8"--" + this->delimiter;
	std::u8string endDelimiter = u8"--" + this->delimiter + u8"--";

	size_t pos = 0;
	size_t endPos = 0;

	while ((pos = this->body.find(delimiter, pos)) != std::string::npos) {
		pos += delimiter.length();
		endPos = this->body.find(delimiter, pos);

		if (endPos == std::string::npos) {
			break;
		}

		std::u8string field = this->body.substr(pos, endPos - pos);

		size_t namePos = field.find(u8"name=\"") + 6;
		size_t nameEndPos = field.find(u8"\"", namePos);

		std::u8string name = field.substr(namePos, nameEndPos - namePos);

		size_t valuePos = field.find(u8"\r\n\r\n") + 4;

		std::u8string value = field.substr(valuePos, field.length() - valuePos - 2);

		fields[name] = value;
	}

	return fields;
}