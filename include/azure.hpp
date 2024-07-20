#pragma once

#include <azure/core.hpp>
#include <azure/identity/default_azure_credential.hpp>
#include <azure/storage/blobs.hpp>
#include <cstdlib>
#include <string>
#include <mutex>

class Blobs {
private:
	Blobs() = default;

	static Blobs* azureInstance;
	Azure::Storage::Blobs::BlobServiceClient serviceClient = Azure::Storage::Blobs::BlobServiceClient::CreateFromConnectionString(std::getenv("BLOBS_CONN"));

public:
	Blobs(const Blobs&) = delete;

	static Blobs* GetInstance() {
		if (azureInstance == nullptr) {
			azureInstance = new Blobs();
		}
		return azureInstance;
	}

	Azure::Storage::Blobs::BlobServiceClient* GetClient() {
		return &serviceClient;
	}

	void Close() {
		delete azureInstance;
		azureInstance = nullptr;
	}

	static std::mutex azureMutex;
};