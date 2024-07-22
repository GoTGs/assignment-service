#pragma once

#include <soci/soci.h>
#include <soci/postgresql/soci-postgresql.h>
#include <cstdlib>
#include <string>
#include <mutex>
#include <iostream>

using namespace soci;

class Database {
private:
	Database() {
		dbMutex.lock();
		sql.open(postgresql, "dbname=" + (std::string)std::getenv("PG_DB") + " user=" + (std::string)std::getenv("PG_USER") + " password=" + (std::string)std::getenv("PG_PASS") + " host=" + (std::string)std::getenv("PG_HOST") + " port=" + (std::string)std::getenv("PG_PORT") + " sslmode=require");	
		dbMutex.unlock();
	}

	static Database* databaseInstance;
	soci::session sql;

public:
	Database(const Database&) = delete;

	static Database* GetInstance() {
		if (databaseInstance == nullptr) {
			databaseInstance = new Database();
		}
		return databaseInstance;
	}

	soci::session* GetSession() {
		return &sql;
	}

	void Close() {
		sql.close();
		delete databaseInstance;
		databaseInstance = nullptr;
	}

	static std::mutex dbMutex;
};