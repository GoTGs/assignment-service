#include "../include/endpoints.hpp"
#include <thread>
#include <azure/core.hpp>
#include <azure/identity/default_azure_credential.hpp>
#include <azure/storage/blobs.hpp>

using namespace Azure::Identity;
using namespace Azure::Storage::Blobs;

int main() {
	BlobServiceClient blobServiceClient = BlobServiceClient::CreateFromConnectionString(std::getenv("BLOBS_CONN"));
	BlobContainerClient containerClient = blobServiceClient.GetBlobContainerClient("assignments");

	ListBlobsPagedResponse blobs = containerClient.ListBlobs();

	for (auto blob : blobs.Blobs) {
		std::cout << "Blob Name: " << blob.Name << "\n";
	}

	CppHttp::Net::Router router;
	CppHttp::Net::TcpListener server;
	server.CreateSocket();

	int requestCount = 0;

	auto onReceive = [&](CppHttp::Net::Request req) {
		router.Handle(req);
	};

	server.SetOnReceive(onReceive);

	//router.AddRoute("GET", "/assignment/get/all", GetAllAssignments);

	server.Listen("0.0.0.0", 8003, std::thread::hardware_concurrency());

	Database::GetInstance()->Close();
}