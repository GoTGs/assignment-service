#include <unordered_map>
#include <syncstream>
#include <string>
#include <iostream>
#include "../dependencies/cpphttp/include/request.hpp"

class FormParser {
private:
	std::u8string delimiter;
	std::u8string body;

	std::unordered_map<std::u8string, std::u8string> fields;
public:
	FormParser(CppHttp::Net::Request req);

	std::vector<std::unordered_map<std::u8string, std::u8string>> Parse();
};