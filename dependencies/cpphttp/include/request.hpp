#pragma once

#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <iostream>
#include <sstream>
#include <regex>
#include "ctre.hpp"

#ifdef _WIN32 || _WIN64 || _MSC_VER
	#define WIN32_LEAN_AND_MEAN
	#define _WINSOCK_DEPRECATED_NO_WARNINGS
	#include <Windows.h>
	#include <WinSock2.h>
	#include <WS2tcpip.h>

#elif defined(__linux__) || defined(__APPLE__)
	#include <sys/socket.h>
	#define SOCKET int
	#define INVALID_SOCKET -1
#endif

namespace CppHttp {
	#ifndef CPPHTTPUTILS
		namespace Utils {
			std::vector<std::string> Split(std::string const str, char delimiter);

			// Extract route from request
			std::string GetRoute(std::string const req);

			// Extract method from request
			std::string GetMethod(const std::string req);

			// Extract parameters from request
			// Returns a map of parameters
			std::unordered_map<std::string, std::string> GetParameters(const std::string req);

			// Extract headers from request
			// Returns a map of headers
			std::unordered_map<std::string, std::string> GetHeaders(std::string req);

			// Extract body from request
			std::string GetBody(const std::string req);

			// Extract header from request
			std::string GetHeader(std::string const& req, std::string const& header);

			// Extract protocol from request
			std::string GetProtocol(std::string const& req);

			// Extract protocol version from request
			std::string GetProtocolVersion(std::string const& req);
		}
		#define CPPHTTPUTILS
	#endif

	namespace Net {

		struct RequestInformation {
			RequestInformation() = default;
			
			RequestInformation(std::string req, SOCKET sender) :
				sender(sender),
				original(req),
				route(CppHttp::Utils::GetRoute(original)),
				method(CppHttp::Utils::GetMethod(original)),
				parameters(CppHttp::Utils::GetParameters(original)),
				headers(CppHttp::Utils::GetHeaders(original)),
				body(CppHttp::Utils::GetBody(original))
			{}
			
			SOCKET sender = INVALID_SOCKET;
			std::string original;
			std::string route;
			std::string method;
			std::unordered_map<std::string, std::string> parameters;
			std::unordered_map<std::string, std::string> headers;
			std::string body;
		};
		
		class Request {
		public:
			Request() {
				this->m_info = {};
			}

			Request(std::string req, SOCKET sender) {
				this->m_info = {
					req,
					sender
				};
			}
			RequestInformation m_info;
		};

		#ifndef REQOPOVERLOAD
			#define REQOPOVERLOAD
			inline std::ostream& operator<<(std::ostream& os, const CppHttp::Net::Request& req) {
				return os << req.m_info.original;
			}
		#endif
	}
}