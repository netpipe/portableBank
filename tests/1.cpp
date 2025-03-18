#include <iostream>
#include <sqlite3.h>
#include <openssl/sha.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <vector>

class CryptoBank {
public:
    CryptoBank() {
        initDatabase();
    }

    void createAccount(const std::string& username, const std::string& publicKey) {
        std::string sql = "INSERT INTO accounts (username, public_key, balance) VALUES ('" + username + "', '" + publicKey + "', 0);";
        executeSQL(sql);
    }

    void deposit(const std::string& username, double amount) {
        std::string sql = "UPDATE accounts SET balance = balance + " + std::to_string(amount) + " WHERE username = '" + username + "';";
        executeSQL(sql);
    }

    void transfer(const std::string& fromUser, const std::string& toUser, double amount) {
        std::string sql = "UPDATE accounts SET balance = balance - " + std::to_string(amount) + " WHERE username = '" + fromUser + "';";
        executeSQL(sql);
        sql = "UPDATE accounts SET balance = balance + " + std::to_string(amount) + " WHERE username = '" + toUser + "';";
        executeSQL(sql);
    }

    void printBalance(const std::string& username) {
        std::string sql = "SELECT balance FROM accounts WHERE username = '" + username + "';";
        executeSQL(sql);
    }

private:
    sqlite3* db;

    void initDatabase() {
        if (sqlite3_open("bank.db", &db)) {
            std::cerr << "Error opening database" << std::endl;
            return;
        }
        std::string sql = "CREATE TABLE IF NOT EXISTS accounts (id INTEGER PRIMARY KEY, username TEXT UNIQUE, public_key TEXT, balance REAL);";
        executeSQL(sql);
    }

    void executeSQL(const std::string& sql) {
        char* errMsg = nullptr;
        if (sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errMsg) != SQLITE_OK) {
            std::cerr << "SQL Error: " << errMsg << std::endl;
            sqlite3_free(errMsg);
        }
    }
};

int main() {
    CryptoBank bank;
    bank.createAccount("Alice", "AlicePublicKey");
    bank.createAccount("Bob", "BobPublicKey");
    bank.deposit("Alice", 100);
    bank.transfer("Alice", "Bob", 50);
    bank.printBalance("Alice");
    bank.printBalance("Bob");
    return 0;
}
