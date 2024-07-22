#include "../include/endpoints.hpp"
#include <thread>

int main() {
	Database::GetInstance();
	std::cout << "Starting server on port 8003\n";
	CppHttp::Net::Router router;
	CppHttp::Net::TcpListener server;
	server.CreateSocket();

	int requestCount = 0;

	auto onReceive = [&](CppHttp::Net::Request req) {
		router.Handle(req);
	};

	server.SetOnReceive(onReceive);

	router.AddRoute("GET", "/assignment/classroom/{assignment_id}/get/all", GetAllAssignments);
	router.AddRoute("GET", "/assignment/{assignment_id}/get", GetAssignment);


	server.Listen("0.0.0.0", 8003, std::thread::hardware_concurrency());

	Database::GetInstance()->Close();
}