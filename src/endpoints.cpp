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

		for (auto assignment : rs) {
			json assignmentJson;
			assignmentJson["id"] = assignment.id;
			assignmentJson["title"] = assignment.title;
			assignmentJson["description"] = assignment.description;
			std::ostringstream oss;
			oss << std::put_time(&assignment.dueDate, "%d-%m-%Y %H:%M:%S");
			assignmentJson["dueDate"] = std::move(oss.str());

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

	std::ostringstream oss;
	oss << std::put_time(&assignment.dueDate, "%d-%m-%Y %H:%M:%S");
	
	json response = {
		{ "id", assignment.id },
		{ "title", assignment.title },
		{ "description", assignment.description },
		{ "dueDate", std::move(oss.str()) },
		{ "classroomId", assignment.classroomId }
	};

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