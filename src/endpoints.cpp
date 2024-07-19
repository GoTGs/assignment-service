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

returnType CreateClassroom(CppHttp::Net::Request req) {
	soci::session* sql = Database::GetInstance()->GetSession();
	std::string token = req.m_info.headers["Authorization"];

	auto tokenJson = ValidateToken(token);

	if (std::holds_alternative<TokenError>(tokenJson)) {
		auto error = std::get<TokenError>(tokenJson);
		return { error.type, error.message, {} };
	}

	auto tokenPayload = std::get<json>(tokenJson);
	std::string id = tokenPayload["id"];

	User user;
	*sql << "SELECT * FROM users WHERE id = :id" , soci::use(id), soci::into(user);

	if (user.email.empty()) {
		return { CppHttp::Net::ResponseType::NOT_FOUND, "User not found", {} };
	}

	std::transform(user.role.begin(), user.role.end(), user.role.begin(), ::toupper);

	if (user.role != "ADMIN") {
		return { CppHttp::Net::ResponseType::FORBIDDEN, "You do not have permission to access this resource", {} };
	}

	json body;

	try {
		body = json::parse(req.m_info.body);
	}
	catch (json::parse_error& e) {
		return { CppHttp::Net::ResponseType::BAD_REQUEST, e.what(), {} };
	}

	std::string name = body["name"];

	if (name.length() > 120) {
		return { CppHttp::Net::ResponseType::BAD_REQUEST, "Name too long", {} };
	}

	Classroom classroom;
	*sql << "INSERT INTO classrooms (name, owner_id) VALUES (:name, :owner_id) RETURNING *", soci::use(name), soci::use(user.id), soci::into(classroom);
	*sql << "INSERT INTO classroom_users (classroom_id, user_id) VALUES (:classroom_id, :user_id)", soci::use(classroom.id), soci::use(user.id);

	json response = {
		{ "id", classroom.id },
		{ "name", classroom.name },
		{ "owner_id", classroom.ownerId }
	};

	return { CppHttp::Net::ResponseType::JSON, response.dump(4), {} };
}

returnType AddUserToClassroom(CppHttp::Net::Request req) {
	if (req.m_info.parameters["id"].empty()) {
		return { CppHttp::Net::ResponseType::BAD_REQUEST, "Missing classroom id in path parameters", {} };
	}
	
	soci::session* sql = Database::GetInstance()->GetSession();
	std::string token = req.m_info.headers["Authorization"];

	auto tokenJson = ValidateToken(token);

	if (std::holds_alternative<TokenError>(tokenJson)) {
		auto error = std::get<TokenError>(tokenJson);
		return { error.type, error.message, {} };
	}

	auto tokenPayload = std::get<json>(tokenJson);
	std::string id = tokenPayload["id"];

	User user;

	*sql << "SELECT * FROM users WHERE id = :id", soci::use(id), soci::into(user);

	Classroom classroom;

	*sql << "SELECT * FROM classrooms WHERE id = :id", soci::use(req.m_info.parameters["id"]), soci::into(classroom);

	
	if (user.email.empty()) {
		return { CppHttp::Net::ResponseType::NOT_AUTHORIZED, "User not found", {} };
	}

	std::transform(user.role.begin(), user.role.end(), user.role.begin(), ::toupper);

	if (user.role != "ADMIN" || classroom.ownerId != std::stoi(id)) {
		return { CppHttp::Net::ResponseType::FORBIDDEN, "You do not have permission to access this resource", {} };
	}
	
	if (classroom.name.empty()) {
		return { CppHttp::Net::ResponseType::NOT_FOUND, "Classroom not found", {} };
	}

	json body;
	try {
		body = json::parse(req.m_info.body);
	}
	catch (json::parse_error& e) {
		return { CppHttp::Net::ResponseType::BAD_REQUEST, e.what(), {} };
	}

	std::string email = body["email"];

	user = std::move(User());

	*sql << "SELECT * FROM users WHERE email = :email", soci::use(email), soci::into(user);

	if (user.email.empty()) {
		return { CppHttp::Net::ResponseType::NOT_FOUND, "User not found", {} };
	}

	*sql << "SELECT * FROM classroom_users WHERE classroom_id = :classroom_id AND user_id = :user_id", soci::use(classroom.id), soci::use(user.id);

	if (sql->got_data()) {
		return { CppHttp::Net::ResponseType::BAD_REQUEST, "User already in classroom", {} };
	}

	*sql << "INSERT INTO classroom_users (classroom_id, user_id) VALUES (:classroom_id, :user_id)", soci::use(classroom.id), soci::use(user.id);

	return { CppHttp::Net::ResponseType::OK, "User added to classroom", {} };
}

returnType GetUserClassrooms(CppHttp::Net::Request req) {
	soci::session* sql = Database::GetInstance()->GetSession();
	std::string token = req.m_info.headers["Authorization"];

	auto tokenJson = ValidateToken(token);

	if (std::holds_alternative<TokenError>(tokenJson)) {
		auto error = std::get<TokenError>(tokenJson);
		return { error.type, error.message, {} };
	}

	auto tokenPayload = std::get<json>(tokenJson);
	std::string id = tokenPayload["id"];

	User user;
	*sql << "SELECT * FROM users WHERE id = :id", soci::use(id), soci::into(user);

	if (user.email.empty()) {
		return { CppHttp::Net::ResponseType::NOT_FOUND, "User not found", {} };
	}

	std::vector<Classroom> classrooms;
	soci::rowset<Classroom> rs = (sql->prepare << "SELECT DISTINCT classrooms.* FROM classrooms LEFT JOIN classroom_users ON classrooms.id=classroom_users.classroom_id WHERE classroom_users.user_id=:user_id", soci::use(user.id));

	std::move(rs.begin(), rs.end(), std::back_inserter(classrooms));

	json response = json::array();

	for (auto& classroom : classrooms) {
		json classroomJson = {
			{ "id", classroom.id },
			{ "name", classroom.name },
			{ "owner_id", classroom.ownerId }
		};

		response.push_back(classroomJson);
	}

	return { CppHttp::Net::ResponseType::JSON, response.dump(4), {} };
}

returnType GetClassroom(CppHttp::Net::Request req) {
	if (req.m_info.parameters["id"].empty()) {
		return { CppHttp::Net::ResponseType::BAD_REQUEST, "Missing classroom id in path parameters", {} };
	}

	soci::session* sql = Database::GetInstance()->GetSession();
	std::string token = req.m_info.headers["Authorization"];

	auto tokenJson = ValidateToken(token);

	if (std::holds_alternative<TokenError>(tokenJson)) {
		auto error = std::get<TokenError>(tokenJson);
		return { error.type, error.message, {} };
	}

	auto tokenPayload = std::get<json>(tokenJson);
	std::string id = tokenPayload["id"];

	User user;
	*sql << "SELECT * FROM users WHERE id = :id", soci::use(id), soci::into(user);

	if (user.email.empty()) {
		return { CppHttp::Net::ResponseType::NOT_FOUND, "User not found", {} };
	}

	Classroom classroom;
	*sql << "SELECT DISTINCT classrooms.* FROM classrooms LEFT JOIN classroom_users ON classrooms.id=classroom_users.classroom_id WHERE classroom_users.user_id=:user_id AND classroom_users.classroom_id=:classroom_id LIMIT 1", soci::use(id), soci::use(req.m_info.parameters["id"]), soci::into(classroom);

	if (classroom.name.empty()) {
		return { CppHttp::Net::ResponseType::NOT_FOUND, "Classroom not found", {} };
	}

	json response = {
		{ "id", classroom.id },
		{ "name", classroom.name },
		{ "owner_id", classroom.ownerId }
	};

	return { CppHttp::Net::ResponseType::JSON, response.dump(4), {} };
}