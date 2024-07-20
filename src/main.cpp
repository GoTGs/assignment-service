#include "../include/endpoints.hpp"
#include <thread>

int main() {
	CppHttp::Net::Router router;
	CppHttp::Net::TcpListener server;
	server.CreateSocket();

	int requestCount = 0;

	auto onReceive = [&](CppHttp::Net::Request req) {
		router.Handle(req);
	};

	server.SetOnReceive(onReceive);

	router.AddRoute("GET", "/assignment/get/all", GetAllAssignments);

	server.Listen("0.0.0.0", 8003, std::thread::hardware_concurrency());

	Database::GetInstance()->Close();
}