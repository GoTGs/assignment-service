#pragma once

#include "CppHttp.hpp"
#include "database.hpp"
#include "jwt-cpp/traits/nlohmann-json/traits.h"
#include <iostream>
#include <iomanip>
#include <string>
#include <tuple>
#include <optional>
#include <vector>
#include <fstream>
#include <unordered_map>
#include <cstdlib>
#include <variant>
#include <cuchar>
#include "formParser.hpp"
#include "../include/azure.hpp"

using returnType = std::tuple<CppHttp::Net::ResponseType, std::string, std::optional<std::vector<std::string>>>;
using json = nlohmann::json;

struct User {
	int id;
	std::string email;
	std::string password;
	std::string salt;
	std::string firstName;
	std::string lastName;
    std::string role;
};

struct Classroom {
    int id;
    std::string name;
    int ownerId;
};

struct Assignment {
    int id;
    std::string title;
    std::string description;
    std::tm dueDate;
    int classroomId;
};

struct TokenError {
    CppHttp::Net::ResponseType type;
    std::string message;
};

namespace soci
{
    template<>
    struct type_conversion<User>
    {
        typedef values base_type;

        static void from_base(values const& v, indicator /* ind */, User& u)
        {
            u.id = v.get<int>("id", 0);
            u.email = v.get<std::string>("email");
            u.password = v.get<std::string>("password");
            u.salt = v.get<std::string>("salt");
            u.firstName = v.get<std::string>("first_name");
            u.lastName = v.get<std::string>("last_name");
            u.role = v.get<std::string>("role");
        }

        static void to_base(const User& u, values& v, indicator& ind)
        {
            v.set("id", u.id);
            v.set("email", u.email);
            v.set("password", u.password);
            v.set("salt", u.salt);
            v.set("first_name", u.firstName);
            v.set("last_name", u.lastName);
            v.set("role", u.role);
            ind = i_ok;
        }
    };

    template<>
    struct type_conversion<Classroom>
    {
		typedef values base_type;

        static void from_base(values const& v, indicator /* ind */, Classroom& c)
        {
			c.id = v.get<int>("id", 0);
			c.name = v.get<std::string>("name");
			c.ownerId = v.get<int>("owner_id");
		}

        static void to_base(const Classroom& c, values& v, indicator& ind)
        {
			v.set("id", c.id);
			v.set("name", c.name);
			v.set("owner_id", c.ownerId);
			ind = i_ok;
		}
	};

    template<>
    struct type_conversion<Assignment>
    {
        typedef values base_type;

        static void from_base(values const& v, indicator /* ind */, Assignment& a)
        {
            a.id = v.get<int>("id", 0);
            a.title = v.get<std::string>("title");
            a.description = v.get<std::string>("description");
            a.dueDate = v.get<std::tm>("due_date");
            a.classroomId = v.get<int>("classroom_id");
        }

        static void to_base(const Assignment& a, values& v, indicator& ind)
        {
            v.set("id", a.id);
            v.set("title", a.title);
            v.set("description", a.description);
            v.set("due_date", a.dueDate);
            v.set("classroom_id", a.classroomId);
            ind = i_ok;
        }
    };
}

std::variant<TokenError, json> ValidateToken(std::string& token);

returnType CreateAssignment(CppHttp::Net::Request req);

returnType GetAllAssignments(CppHttp::Net::Request req);

returnType GetAssignment(CppHttp::Net::Request req);