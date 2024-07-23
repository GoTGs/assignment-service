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

#pragma region Assignment Functions

returnType CreateAssignment(CppHttp::Net::Request req) {
	if (req.m_info.parameters["classroom_id"].empty()) {
		return { CppHttp::Net::ResponseType::BAD_REQUEST, "Missing classroom_id in path parameters", {} };
	}

	soci::session* sql = Database::GetInstance()->GetSession();
	std::string token = req.m_info.headers["Authorization"];

	auto tokenResult = ValidateToken(token);

	if (std::holds_alternative<TokenError>(tokenResult)) {
		auto error = std::get<TokenError>(tokenResult);
		return { error.type, error.message, {} };
	}

	auto tokenJson = std::get<json>(tokenResult);
	std::string id = tokenJson["id"];

	User user;
	{
		std::lock_guard<std::mutex> lock(Database::dbMutex);
		*sql << "SELECT users.* FROM users LEFT JOIN classroom_users ON classroom_users.user_id=users.id WHERE classroom_users.classroom_id=:classroom_id AND users.id=:user_id", soci::use(req.m_info.parameters["classroom_id"]), soci::use(id), soci::into(user);
	}

	if (user.email.empty()) {
		return { CppHttp::Net::ResponseType::FORBIDDEN, "User is not a member of this classroom", {} };
	}

	std::transform(user.role.begin(), user.role.end(), user.role.begin(), ::toupper);

	if (user.role != "TEACHER" && user.role != "ADMIN") {
		return { CppHttp::Net::ResponseType::FORBIDDEN, "User is not a teacher", {} };
	}

	json body;
	try {
		body = json::parse(req.m_info.body);
	}
	catch (json::parse_error& e) {
		return { CppHttp::Net::ResponseType::BAD_REQUEST, "Invalid JSON", {} };
	}

	std::string title = body["title"];

	if (title.empty()) {
		return { CppHttp::Net::ResponseType::BAD_REQUEST, "Missing title in request body", {} };
	}

	std::string description = body["description"];
	std::string dueDate = body["dueDate"];

	if (dueDate.empty()) {
		return { CppHttp::Net::ResponseType::BAD_REQUEST, "Missing due date in request body", {} };
	}

	std::tm dueDateTm = {};

	std::istringstream iss(dueDate);
	iss >> std::get_time(&dueDateTm, "%d-%m-%Y %H:%M:%S");

	if (iss.fail()) {
		return { CppHttp::Net::ResponseType::BAD_REQUEST, "Invalid due date format", {} };
	}

	Assignment assignment;
	assignment.title = std::move(title);
	assignment.description = std::move(description);
	assignment.dueDate = std::move(dueDateTm);
	assignment.classroomId = std::stoi(req.m_info.parameters["classroom_id"]);

	{
		std::lock_guard<std::mutex> lock(Database::dbMutex);
		*sql << "INSERT INTO assignments (title, description, due_date, classroom_id) VALUES (:title, :description, :due_date, :classroom_id) RETURNING *", soci::use(assignment.title), soci::use(assignment.description), soci::use(assignment.dueDate), soci::use(assignment.classroomId), soci::into(assignment);
	}

	json response = {
		{ "id", assignment.id },
		{ "title", assignment.title },
		{ "description", assignment.description },
		{ "dueDate", dueDate },
		{ "classroomId", assignment.classroomId }
	};


	return { CppHttp::Net::ResponseType::JSON, response.dump(4), {} };
}

returnType GetAllAssignments(CppHttp::Net::Request req) {
	if (req.m_info.parameters["classroom_id"].empty()) {
		return { CppHttp::Net::ResponseType::BAD_REQUEST, "Missing classroom_id in path parameters", {} };
	}

	soci::session* sql = Database::GetInstance()->GetSession();
	std::string token = req.m_info.headers["Authorization"];

	auto tokenResult = ValidateToken(token);

	if (std::holds_alternative<TokenError>(tokenResult)) {
		auto error = std::get<TokenError>(tokenResult);
		return { error.type, error.message, {} };
	}

	auto tokenJson = std::get<json>(tokenResult);
	std::string id = tokenJson["id"];

	User user;
	{
		std::lock_guard<std::mutex> lock(Database::dbMutex);
		*sql << "SELECT users.* FROM users LEFT JOIN classroom_users ON classroom_users.user_id=users.id WHERE classroom_users.classroom_id=:classroom_id AND users.id=:user_id", soci::use(req.m_info.parameters["classroom_id"]), soci::use(id), soci::into(user);
	}

	if (user.email.empty()) {
		return { CppHttp::Net::ResponseType::FORBIDDEN, "User is not a member of this classroom", {} };
	}

	json response = json::array();
	{
		std::lock_guard<std::mutex> lock(Database::dbMutex);
		soci::rowset<Assignment> rs = (sql->prepare << "SELECT * FROM assignments WHERE classroom_id=:classroom_id", soci::use(req.m_info.parameters["classroom_id"]));
		soci::rowset<Submission> rs2 = (sql->prepare << "SELECT submissions.* FROM submissions LEFT JOIN assignments ON assignments.id = submissions.assignment_id LEFT JOIN classrooms ON classrooms.id=assignments.classroom_id WHERE submissions.user_id=:user_id AND classrooms.id=:classroom_id", soci::use(id), soci::use(req.m_info.parameters["classroom_id"]));

		for (auto assignment : rs) {
			auto it = std::find_if(rs2.begin(), rs2.end(), [&assignment](Submission& submission) {
				return submission.assignmentId == assignment.id;
			});

			json assignmentJson;
			assignmentJson["id"] = assignment.id;
			assignmentJson["title"] = assignment.title;
			assignmentJson["description"] = assignment.description;
			std::ostringstream oss;
			oss << std::put_time(&assignment.dueDate, "%d-%m-%Y %H:%M:%S");
			assignmentJson["dueDate"] = std::move(oss.str());
			assignmentJson["completed"] = (it != rs2.end());

			response.push_back(assignmentJson);
		}
	}

	return { CppHttp::Net::ResponseType::JSON, response.dump(4), {}};
}

returnType GetAssignment(CppHttp::Net::Request req) {
	if (req.m_info.parameters["assignment_id"].empty()) {
		return { CppHttp::Net::ResponseType::BAD_REQUEST, "Missing classroom_id in path parameters", {} };
	}

	soci::session* sql = Database::GetInstance()->GetSession();
	std::string token = req.m_info.headers["Authorization"];

	auto tokenResult = ValidateToken(token);

	if (std::holds_alternative<TokenError>(tokenResult)) {
		auto error = std::get<TokenError>(tokenResult);
		return { error.type, error.message, {} };
	}

	auto tokenJson = std::get<json>(tokenResult);
	std::string id = tokenJson["id"];

	User user;
	{
		std::lock_guard<std::mutex> lock(Database::dbMutex);
		*sql << "SELECT * FROM users WHERE id=:user_id", soci::use(id), soci::into(user);
	}

	if (user.email.empty()) {
		return { CppHttp::Net::ResponseType::FORBIDDEN, "User is not a member of this classroom", {} };
	}

	Assignment assignment;
	{
		std::lock_guard<std::mutex> lock(Database::dbMutex);
		*sql << "SELECT * FROM assignments WHERE id=:id", soci::use(req.m_info.parameters["assignment_id"]), soci::into(assignment);
		*sql << "SELECT * FROM classroom_users WHERE classroom_id=:classroom_id AND user_id=:user_id", soci::use(assignment.classroomId), soci::use(id);

		if (!sql->got_data()) {
			return { CppHttp::Net::ResponseType::FORBIDDEN, "User is not a member of this classroom", {} };
		}
	}

	std::vector<Submission> submissions;
	std::vector<FileSubmission> fileSubmissions;
	{
		std::lock_guard<std::mutex> lock(Database::dbMutex);
		soci::rowset<Submission> rs = (sql->prepare << "SELECT * FROM submissions WHERE assignment_id=:assignment_id AND user_id=:user_id", soci::use(assignment.id), soci::use(id));
		std::move(rs.begin(), rs.end(), std::back_inserter(submissions));

		soci::rowset<FileSubmission> rs2 = (sql->prepare << "SELECT file_submissions.* FROM file_submissions LEFT JOIN submissions ON file_submissions.submission_id=submissions.id WHERE submissions.assignment_id=:assignment_id AND submissions.user_id=:user_id", soci::use(assignment.id), soci::use(id));
		std::copy(rs2.begin(), rs2.end(), std::back_inserter(fileSubmissions));
	}

	std::ostringstream oss;
	oss << std::put_time(&assignment.dueDate, "%d-%m-%Y %H:%M:%S");
	
	json response = {
		{ "id", assignment.id },
		{ "title", assignment.title },
		{ "description", assignment.description },
		{ "dueDate", std::move(oss.str()) },
		{ "classroomId", assignment.classroomId },
		{ "submissions", json::array() }
	};

	for (auto& submission : submissions) {
		json submissionJson = {
			{ "id", submission.id },
			{ "text", submission.text },
			{ "file_links", json::array() }
		};

		for (auto& fileSubmission : fileSubmissions) {
			if (fileSubmission.submissionId == submission.id) {
				submissionJson["file_links"].push_back(fileSubmission.link);
			}
		}

		response["submissions"].push_back(submissionJson);
	}

	return { CppHttp::Net::ResponseType::JSON, response.dump(4), {} };
}

returnType EditAssignment(CppHttp::Net::Request req) {
	if (req.m_info.parameters["assignment_id"].empty()) {
		return { CppHttp::Net::ResponseType::BAD_REQUEST, "Missing classroom_id in path parameters", {} };
	}

	soci::session* sql = Database::GetInstance()->GetSession();
	std::string token = req.m_info.headers["Authorization"];

	auto tokenResult = ValidateToken(token);

	if (std::holds_alternative<TokenError>(tokenResult)) {
		auto error = std::get<TokenError>(tokenResult);
		return { error.type, error.message, {} };
	}

	auto tokenJson = std::get<json>(tokenResult);
	std::string id = tokenJson["id"];

	User user;
	{
		std::lock_guard<std::mutex> lock(Database::dbMutex);
		*sql << "SELECT * FROM users WHERE id=:user_id", soci::use(id), soci::into(user);
	}

	if (user.email.empty()) {
		return { CppHttp::Net::ResponseType::FORBIDDEN, "User is not a member of this classroom", {} };
	}

	std::transform(user.role.begin(), user.role.end(), user.role.begin(), ::toupper);

	if (user.role != "TEACHER" && user.role != "ADMIN") {
		return { CppHttp::Net::ResponseType::FORBIDDEN, "User is not a teacher", {} };
	}

	Assignment assignment;
	{
		std::lock_guard<std::mutex> lock(Database::dbMutex);
		*sql << "SELECT * FROM assignments WHERE id=:id", soci::use(req.m_info.parameters["assignment_id"]), soci::into(assignment);

		if (assignment.title.empty()) {
			return { CppHttp::Net::ResponseType::NOT_FOUND, "Assignment not found", {} };
		}

		*sql << "SELECT * FROM classroom_users WHERE classroom_id=:classroom_id AND user_id=:user_id", soci::use(assignment.classroomId), soci::use(id);

		if (!sql->got_data()) {
			return { CppHttp::Net::ResponseType::FORBIDDEN, "User is not a member of this classroom", {} };
		}
	}

	json body;
	try {
		body = json::parse(req.m_info.body);
	}
	catch (json::parse_error& e) {
		return { CppHttp::Net::ResponseType::BAD_REQUEST, "Invalid JSON", {} };
	}

	std::string title = body["title"];
	std::string description = body["description"];
	std::string dueDate = body["due_date"];

	if (!title.empty()) {
		if (title.size() > 120) {
			return { CppHttp::Net::ResponseType::BAD_REQUEST, "Title is too long", {} };
		}
		assignment.title = std::move(title);
	}
	if (!description.empty()) {
		if (description.size() > 2048) {
			return { CppHttp::Net::ResponseType::BAD_REQUEST, "Description is too long", {} };
		}
		assignment.description = std::move(description);
	}
	if (!dueDate.empty()) {
		std::tm dueDateTm = {};

		std::istringstream iss(dueDate);
		iss >> std::get_time(&dueDateTm, "%d-%m-%Y %H:%M:%S");

		if (iss.fail()) {
			return { CppHttp::Net::ResponseType::BAD_REQUEST, "Invalid due date format", {} };
		}

		assignment.dueDate = std::move(dueDateTm);
	}

	{
		std::lock_guard<std::mutex> lock(Database::dbMutex);
		*sql << "UPDATE assignments SET title=:title, description=:description, due_date=:due_date WHERE id=:id RETURNING *", soci::use(assignment.title), soci::use(assignment.description), soci::use(assignment.dueDate), soci::use(req.m_info.parameters["assignment_id"]), soci::into(assignment);
	}

	json response = {
		{ "id", assignment.id },
		{ "title", assignment.title },
		{ "description", assignment.description },
		{ "dueDate", dueDate },
		{ "classroomId", assignment.classroomId }
	};

	return { CppHttp::Net::ResponseType::JSON, response.dump(4), {} };
}

returnType DeleteAssignment(CppHttp::Net::Request req) {
	if (req.m_info.parameters["assignment_id"].empty()) {
		return { CppHttp::Net::ResponseType::BAD_REQUEST, "Missing classroom_id in path parameters", {} };
	}

	soci::session* sql = Database::GetInstance()->GetSession();
	std::string token = req.m_info.headers["Authorization"];

	auto tokenResult = ValidateToken(token);

	if (std::holds_alternative<TokenError>(tokenResult)) {
		auto error = std::get<TokenError>(tokenResult);
		return { error.type, error.message, {} };
	}

	auto tokenJson = std::get<json>(tokenResult);
	std::string id = tokenJson["id"];

	User user;
	{
		std::lock_guard<std::mutex> lock(Database::dbMutex);
		*sql << "SELECT * FROM users WHERE id=:user_id", soci::use(id), soci::into(user);
	}

	if (user.email.empty()) {
		return { CppHttp::Net::ResponseType::FORBIDDEN, "User is not a member of this classroom", {} };
	}

	std::transform(user.role.begin(), user.role.end(), user.role.begin(), ::toupper);

	if (user.role != "TEACHER" && user.role != "ADMIN") {
		return { CppHttp::Net::ResponseType::FORBIDDEN, "User is not a teacher", {} };
	}

	Assignment assignment;
	{
		std::lock_guard<std::mutex> lock(Database::dbMutex);
		*sql << "SELECT * FROM assignments WHERE id=:id", soci::use(req.m_info.parameters["assignment_id"]), soci::into(assignment);

		if (assignment.title.empty()) {
			return { CppHttp::Net::ResponseType::NOT_FOUND, "Assignment not found", {} };
		}

		*sql << "SELECT * FROM classroom_users WHERE classroom_id=:classroom_id AND user_id=:user_id", soci::use(assignment.classroomId), soci::use(id);

		if (!sql->got_data()) {
			return { CppHttp::Net::ResponseType::FORBIDDEN, "User is not a member of this classroom", {} };
		}
	}

	{
		std::lock_guard<std::mutex> lock(Database::dbMutex);
		*sql << "DELETE FROM assignments WHERE id=:id", soci::use(req.m_info.parameters["assignment_id"]);
	}

	return { CppHttp::Net::ResponseType::OK, "Assignment deleted", {} };
}

#pragma endregion

#pragma region Submission Functions

returnType SubmitAssignment(CppHttp::Net::Request req) {
	if (req.m_info.parameters["assignment_id"].empty()) {
		return { CppHttp::Net::ResponseType::BAD_REQUEST, "Missing assignment_id in path parameters", {} };
	}

	soci::session* sql = Database::GetInstance()->GetSession();
	std::string token = req.m_info.headers["Authorization"];

	auto tokenResult = ValidateToken(token);

	if (std::holds_alternative<TokenError>(tokenResult)) {
		auto error = std::get<TokenError>(tokenResult);
		return { error.type, error.message, {} };
	}

	auto tokenJson = std::get<json>(tokenResult);
	std::string id = tokenJson["id"];

	User user;
	{
		std::lock_guard<std::mutex> lock(Database::dbMutex);
		*sql << "SELECT * FROM users WHERE id=:user_id", soci::use(id), soci::into(user);
	}

	if (user.email.empty()) {
		return { CppHttp::Net::ResponseType::NOT_AUTHORIZED, "Not authorized", {} };
	}

	std::string assignmentId = req.m_info.parameters["assignment_id"];

	Assignment assignment;
	{
		std::lock_guard<std::mutex> lock(Database::dbMutex);

		*sql << "SELECT * FROM assignments WHERE id=:id", soci::use(assignmentId), soci::into(assignment);
		if (assignment.title.empty()) {
			return { CppHttp::Net::ResponseType::NOT_FOUND, "Assignment not found", {} };
		}
	
		*sql << "SELECT * FROM classroom_users WHERE classroom_id=:classroom_id AND user_id=:user_id", soci::use(assignment.classroomId), soci::use(id);
		if (!sql->got_data()) {
			return { CppHttp::Net::ResponseType::FORBIDDEN, "User is not a member of this classroom", {} };
		}
	}

	std::vector<std::unordered_map<std::u8string, std::u8string>> formData;
	formData = FormParser(req).Parse();

	Azure::Storage::Blobs::BlobServiceClient* blobServiceClient = Blobs::GetInstance()->GetClient();
	Azure::Storage::Blobs::BlobContainerClient containerClient = blobServiceClient->GetBlobContainerClient("assignments");

	std::vector<std::string> fileUrls;
	std::string text;
	for (auto& entry : formData) {
		if (entry[u8"filename"].empty()) {
			if (entry[u8"name"] == u8"text") {
				text = std::string(entry[u8"value"].begin(), entry[u8"value"].end());
			}

			continue;
		}

		Azure::Storage::Blobs::BlockBlobClient blockBlobClient = containerClient.GetBlockBlobClient(std::string(entry[u8"filename"].begin(), entry[u8"filename"].end()));
		uint8_t* data = new uint8_t[entry[u8"value"].size()];

		std::memcpy(data, entry[u8"value"].data(), entry[u8"value"].size());

		blockBlobClient.UploadFrom(data, entry[u8"value"].size());

		fileUrls.push_back(blockBlobClient.GetUrl());

		delete[] data;
	}

	if (text.empty() && fileUrls.empty()) {
		return { CppHttp::Net::ResponseType::BAD_REQUEST, "Submission is empty", {} };
	}

	Submission submission;
	submission.assignmentId = std::stoi(assignmentId);
	submission.userId = std::stoi(id);
	submission.text = std::move(text);
	{
		std::lock_guard<std::mutex> lock(Database::dbMutex);
		*sql << "INSERT INTO submissions (assignment_id, user_id, text) VALUES (:assignment_id, :user_id, :text) RETURNING *", soci::use(submission.assignmentId), soci::use(submission.userId), soci::use(submission.text), soci::into(submission);
		for (auto& url : fileUrls) {
			*sql << "INSERT INTO file_submissions (submission_id, link) VALUES (:submission_id, :link)", soci::use(submission.id), soci::use(url);
		}
	}

	json response = {
		{ "id", submission.id },
		{ "assignmentId", submission.assignmentId },
		{ "userId", submission.userId },
		{ "text", submission.text },
		{ "files", json::array() }
	};

	for (auto& url : fileUrls) {
		response["files"].push_back(std::move(url));
	}

	return { CppHttp::Net::ResponseType::JSON, response.dump(4), {} };
}

#pragma endregion