#include "database.hpp"

Database* Database::databaseInstance = nullptr;
std::mutex Database::dbMutex;