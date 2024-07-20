#include "../include/endpoints.hpp"

std::variant<TokenError, json> ValidateToken(std::string& token) {
	// remove "Bearer "
	token.erase(0, 7);

	if (token.empty()) {
		return TokenError{ CppHttp::Net::ResponseType::NOT_AUTHORIZED, "Missing token" };
	}

	jwt::verifier<jwt::default_clock, jwt::traits::nlohmann_json> verifier = jwt::verify<jwt::traits::nlohmann_json>().allow_algorithm(jwt::algorithm::rs512{ "", std::getenv("RSASECRET"), "", ""}).with_issuer("auth0");
	auto decodedToken = jwt::decode<jwt::traits::nlohmann_json>(token);

	std::error_code ec;
	verifier.verify(decodedToken, ec);

	if (ec) {
		std::osyncstream(std::cout) << "\033[1;31m[-] Error: " << ec.message() << "\033[0m\n";
		return TokenError{ CppHttp::Net::ResponseType::INTERNAL_ERROR, ec.message() };
	}

	auto tokenJson = decodedToken.get_payload_json();

	return tokenJson;
}

returnType GetAllAssignments(CppHttp::Net::Request req) {
	std::unordered_map<std::u8string, std::u8string> formData;
	formData = FormParser(req).Parse();

	Azure::Storage::Blobs::BlobServiceClient* blobServiceClient = Blobs::GetInstance()->GetClient();
	Azure::Storage::Blobs::BlobContainerClient containerClient = blobServiceClient->GetBlobContainerClient("assignments");
	Azure::Storage::Blobs::BlockBlobClient blockBlobClient = containerClient.GetBlockBlobClient("assignment.pdf");

	uint8_t* data = new uint8_t[formData[u8"file"].size()];

	std::memcpy(data, formData[u8"file"].data(), formData[u8"file"].size());

	blockBlobClient.UploadFrom(data, formData[u8"file"].size());

	return { CppHttp::Net::ResponseType::OK, "done", {}};
}