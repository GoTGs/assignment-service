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

	router.AddRoute("GET", "/assignment/classroom/{classroom_id}/get/all", GetAllAssignments);
	router.AddRoute("GET", "/assignment/{assignment_id}/get", GetAssignment);
	router.AddRoute("POST", "/assignment/classroom/{classroom_id}/create", CreateAssignment);
	router.AddRoute("PUT", "/assignment/{assignment_id}/edit", EditAssignment);
	router.AddRoute("DELETE", "/assignment/{assignment_id}/delete", DeleteAssignment);
	router.AddRoute("POST", "/assignment/{assignment_id}/submit", SubmitAssignment);

	server.Listen("0.0.0.0", 8003, std::thread::hardware_concurrency());

	Database::GetInstance()->Close();
}