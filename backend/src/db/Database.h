#pragma once

#include <sqlite3.h>

#include <string>
#include <vector>

namespace blog {

class Database {
 public:
  explicit Database(std::string dbPath);

  const std::string& path() const;

  bool ensureParentDir(std::string& error) const;
  sqlite3* open(std::string& error) const;
  bool exec(sqlite3* db, const std::string& sql, std::string& error) const;

 private:
  std::string dbPath_;
};

bool runMigrations(const Database& db,
                   const std::string& migrationsDir,
                   std::vector<std::string>& applied,
                   std::string& error);

}  // namespace blog
