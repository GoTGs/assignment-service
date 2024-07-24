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
#include "../include/hash.hpp"
#include "../include/azure.hpp"

using returnType = std::tuple<CppHttp::Net::ResponseType, std::string, std::optional<std::vector<std::string>>>;
using json = nlohmann::json;

#pragma region Data Structures
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

struct Submission {
    int id;
    int assignmentId;
    int userId;
    std::string text;
};

struct FileSubmission {
    int id;
    int submissionId;
    std::string link;
};

struct FileAssignment {
    int id;
    int assignmentId;
    std::string link;
};

struct UserSubmissionJoin {
    int id;
    std::string firstName;
    std::string lastName;
    std::string email;
    int userId;
};

struct Grade {
    int id;
    int assignmentId;
    int userId;
    double grade;
    std::string feedback;
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

    template<>
    struct type_conversion<FileAssignment>
    {
        typedef values base_type;

        static void from_base(values const& v, indicator /* ind */, FileAssignment& fa)
        {
            fa.id = v.get<int>("id", 0);
            fa.assignmentId = v.get<int>("assignment_id");
            fa.link = v.get<std::string>("link");
        }

        static void to_base(const FileAssignment& fa, values& v, indicator& ind)
        {
            v.set("id", fa.id);
            v.set("assignment_id", fa.assignmentId);
            v.set("link", fa.link);
            ind = i_ok;
        }
    };

    template<>
    struct type_conversion<Submission>
    {
        typedef values base_type;

        static void from_base(values const& v, indicator /* ind */, Submission& s)
        {
            s.id = v.get<int>("id", 0);
            s.assignmentId = v.get<int>("assignment_id");
            s.userId = v.get<int>("user_id");
            s.text = v.get<std::string>("text");
        }

        static void to_base(const Submission& s, values& v, indicator& ind)
        {
            v.set("id", s.id);
            v.set("assignment_id", s.assignmentId);
            v.set("user_id", s.userId);
            v.set("text", s.text);
            ind = i_ok;
        }
    };

    template<>
    struct type_conversion<FileSubmission>
    {
        typedef values base_type;

        static void from_base(values const& v, indicator /* ind */, FileSubmission& fs)
        {
            fs.id = v.get<int>("id", 0);
            fs.submissionId = v.get<int>("submission_id");
            fs.link = v.get<std::string>("link");
        }

        static void to_base(const FileSubmission& fs, values& v, indicator& ind)
        {
            v.set("id", fs.id);
            v.set("submission_id", fs.submissionId);
            v.set("link", fs.link);
            ind = i_ok;
        }
    };

    template<>
    struct type_conversion<UserSubmissionJoin>
    {
        typedef values base_type;

        static void from_base(values const& v, indicator /* ind */, UserSubmissionJoin& usj)
        {
            usj.id = v.get<int>("id", 0);
            usj.firstName = v.get<std::string>("first_name");
            usj.lastName = v.get<std::string>("last_name");
            usj.email = v.get<std::string>("email");
            usj.userId = v.get<int>("user_id");
        }

        static void to_base(const UserSubmissionJoin& usj, values& v, indicator& ind)
        {
            v.set("id", usj.id);
            v.set("first_name", usj.firstName);
            v.set("last_name", usj.lastName);
            v.set("email", usj.email);
            v.set("user_id", usj.userId);
            ind = i_ok;
        }
    };

    template<>
    struct type_conversion<Grade>
    {
        typedef values base_type;

        static void from_base(values const& v, indicator /* ind */, Grade& g)
        {
            g.id = v.get<int>("id", 0);
            g.assignmentId = v.get<int>("assignment_id");
            g.userId = v.get<int>("user_id");
            g.grade = v.get<double>("grade");
            g.feedback = v.get<std::string>("feedback");
        }

        static void to_base(const Grade& g, values& v, indicator& ind)
        {
            v.set("id", g.id);
            v.set("assignment_id", g.assignmentId);
            v.set("user_id", g.userId);
            v.set("grade", g.grade);
            v.set("feedback", g.feedback);
            ind = i_ok;
        }
    };
}
#pragma endregion

std::variant<TokenError, json> ValidateToken(std::string& token);

#pragma region Assignment Functions

returnType CreateAssignment(CppHttp::Net::Request req);

returnType GetAllAssignments(CppHttp::Net::Request req);

returnType GetAssignment(CppHttp::Net::Request req);

returnType EditAssignment(CppHttp::Net::Request req);

returnType DeleteAssignment(CppHttp::Net::Request req);

#pragma endregion

#pragma region Submission Functions

returnType SubmitAssignment(CppHttp::Net::Request req);

returnType GetAllSubmissions(CppHttp::Net::Request req);

returnType DeleteSubmission(CppHttp::Net::Request req);

#pragma endregion

#pragma region Grade Functions

returnType GradeAssignment(CppHttp::Net::Request req);

returnType RemoveGrade(CppHttp::Net::Request req);

returnType EditGrade(CppHttp::Net::Request req);

#pragma endregion