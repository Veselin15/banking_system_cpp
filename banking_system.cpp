#include <cstdlib>
#include <thread>
#include <chrono>
#include <iostream>
#include <vector>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <random>
#include "json.hpp"
#include <openssl/sha.h>
#include <openssl/rand.h>
using namespace std;
using json = nlohmann::json;

const string FILENAME = "profiles.json";
const size_t SALT_LENGTH = 16; // 16 bytes = 128 bits

void waitForUserInput() {
    cout << "\nPress Enter to continue...";
    cin.ignore(numeric_limits<streamsize>::max(), '\n'); // Clear the input buffer
    cin.get(); // Wait for user to press Enter
}

void clearConsole() {
    #ifdef _WIN32
        system("cls");
    #else
        system("clear");
    #endif
    cout << flush;
}

// Helper: Generate a random salt (hex string)
string generateSalt(size_t length = SALT_LENGTH) {
    unsigned char salt_bytes[SALT_LENGTH];
    if (RAND_bytes(salt_bytes, length) != 1) {
        throw runtime_error("Failed to generate random salt");
    }
    stringstream ss;
    for (size_t i = 0; i < length; ++i) {
        ss << hex << setw(2) << setfill('0') << (int)salt_bytes[i];
    }
    return ss.str();
}

// Helper: Hash password+salt using SHA-256, return hex string
string hashPassword(const string& password, const string& salt) {
    string input = password + salt;
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256((const unsigned char*)input.c_str(), input.size(), hash);
    stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        ss << hex << setw(2) << setfill('0') << (int)hash[i];
    }
    return ss.str();
}

class Profile{
    private:
        double balance;
        string password_hash;
        string salt;
    public:
        string username;
        
        // Constructor for new user (hashes password)
        Profile(const string& uname, const string& pwd, double initial_balance = 10)
            : username(uname), balance(initial_balance) {
            salt = generateSalt();
            password_hash = hashPassword(pwd, salt);
        }

        // Constructor for loading from JSON
        Profile(const string& uname, const string& hash, const string& salt_val, double bal)
            : username(uname), password_hash(hash), salt(salt_val), balance(bal) {}

        // Default constructor
        Profile() : username(""), password_hash(""), salt(""), balance() {}

        // Getters and Setters
        double getBalance() const {
            return balance;
        }

        void setBalance(const double new_balance){
            balance = new_balance;
        }

        string getPasswordHash() const {
            return password_hash;
        }

        string getSalt() const {
            return salt;
        }

        void setPassword(const string& new_password){
            salt = generateSalt();
            password_hash = hashPassword(new_password, salt);
        }

        // Serialize this Profile to JSON
        json serialize_to_json() const {
            return json{
                {"username", username},
                {"password_hash", password_hash},
                {"salt", salt},
                {"balance", balance}
            };
        }

        // Create a Profile from a JSON object
        static Profile deserialize_from_json(const json& j) {
            Profile p;
            p.username = j.at("username").get<string>();
            p.password_hash = j.at("password_hash").get<string>();
            p.salt = j.at("salt").get<string>();
            p.balance = j.at("balance").get<double>();
            return p;
        }
};

class BankSystem{
    public:
        vector <Profile> profiles;
        int current_user_index; // -1 means no user logged in
        
        BankSystem() : current_user_index(-1) {}

        void addProfile(const Profile& profile) {
            profiles.push_back(profile);
        }

        void RegisterUser(const string& username, const string& password) {
            if (usernameExists(username)) {
                cout << "Username already exists! Please choose another." << endl;
                waitForUserInput();
                return;
            }
            Profile new_profile(username, password);
            addProfile(new_profile);
            cout << "Registration successful!" << endl;
            saveProfiles(FILENAME); // Save after registration
            waitForUserInput();
        }

        bool LoginUser(const string& username, const string& password) {
            for (size_t i = 0; i < profiles.size(); ++i) {
                if (profiles[i].username == username) {
                    string hash_attempt = hashPassword(password, profiles[i].getSalt());
                    if (profiles[i].getPasswordHash() == hash_attempt) {
                        current_user_index = static_cast<int>(i);
                        cout << "Login successful! Welcome, " << getCurrentUsername() << endl;
                        cout << "Your balance is: $" << getCurrentUserBalance() << endl;
                        waitForUserInput();
                        return true;
                    }
                }
            }
            current_user_index = -1;
            cout << "Invalid username or password!" << endl;
            waitForUserInput();
            return false;
        }

        void LogoutUser() {
            current_user_index = -1;
            cout << "Logged out successfully!" << endl;
            waitForUserInput();
        }

        void Withdraw(double amount){
            if (!isLoggedIn()) {
                cout << "No user logged in!" << endl;
                return;
            }
            if (amount <= 0){
                cout << "Invalid amount! Please enter a positive value." << endl;
                return;
            }
            if (profiles[current_user_index].getBalance() < amount) {
                cout << "Insufficient balance!" << endl;
                return;
            }
            profiles[current_user_index].setBalance(profiles[current_user_index].getBalance() - amount);
            cout << "Withdrawal successful! New balance: $" << profiles[current_user_index].getBalance() << endl;
            saveProfiles(FILENAME); // Save after withdrawal
            waitForUserInput();
        }

        void Deposit(double amount) {
            if (!isLoggedIn()) {
                cout << "No user logged in!" << endl;
                return;
            }
            if (amount <= 0) {
                cout << "Invalid amount! Please enter a positive value." << endl;
                return;
            }
            profiles[current_user_index].setBalance(profiles[current_user_index].getBalance() + amount);
            cout << "Deposit successful! New balance: $" << profiles[current_user_index].getBalance() << endl;
            saveProfiles(FILENAME); // Save after deposit
            waitForUserInput();
        }

        void Transaction(double amount, const string reciever_username){
            if (!isLoggedIn()) {
                cout << "No user logged in!" << endl;
                waitForUserInput();
                return;
            }
            if (reciever_username == getCurrentUsername()) {
                cout << "You cannot transfer to yourself!" << endl;
                waitForUserInput();
                return;
            }
            if (amount <= 0) {
                cout << "Invalid amount!" << endl;
                waitForUserInput();
                return;
            }
            Profile* receiver = nullptr;
            for (auto& profile : profiles) {
                if (profile.username == reciever_username) {
                    receiver = &profile;
                    break;
                }
            }
            if (receiver == nullptr) {
                cout << "Receiver not found!" << endl;
                waitForUserInput();
                return;
            }
            Profile* sender = &profiles[current_user_index];
            if (sender->getBalance() < amount) {
                cout << "Insufficient balance!" << endl;
                waitForUserInput();
                return;
            }
            sender->setBalance(sender->getBalance() - amount);
            receiver->setBalance(receiver->getBalance() + amount);
            cout << "Transaction successful! Your new balance: $" << sender->getBalance() << endl;
            saveProfiles(FILENAME);
            waitForUserInput();
        }

        bool usernameExists(const string& username) const {
            for (const auto& profile : profiles) {
                if (profile.username == username) {
                    return true;
                }
            }
            return false;
        }

        bool isLoggedIn() const {
            return current_user_index != -1;
        }

        string getCurrentUsername() const {
            if (isLoggedIn()) {
                return profiles[current_user_index].username;
            }
            return "";
        }

        double getCurrentUserBalance() const {
            if (isLoggedIn()) {
                return profiles[current_user_index].getBalance();
            }
            return 0.0;
        }
        void saveProfiles(const string& filename) const {
            json j_profiles = json::array();
            for (const auto& profile : profiles) {
                j_profiles.push_back(profile.serialize_to_json());
            }
            ofstream ofs(filename);
            if (ofs) {
                ofs << j_profiles.dump(4); // Pretty print with 4 spaces
                ofs.close();
            } else {
                cerr << "Failed to open file for saving: " << filename << endl;
            } 
        }
        void loadProfiles(const string& filename) {
            ifstream ifs(filename);
            if (!ifs) {
                cout << "No profile data found. Starting fresh." << endl;
                return;
            }
            // Check if file is empty
            if (ifs.peek() == ifstream::traits_type::eof()) {
                cout << "Profile data file is empty. Starting fresh." << endl;
                return;
            }
            json j_profiles;
            ifs >> j_profiles;
            profiles.clear();
            for (const auto& j_profile : j_profiles) {
                profiles.push_back(Profile::deserialize_from_json(j_profile));
            }
            cout << "Profiles loaded from " << filename << endl;
        }
};

int main() {    
    BankSystem bank_system;
    bank_system.loadProfiles(FILENAME); // Load profiles at startup
    if (bank_system.profiles.empty()) {
        bank_system.addProfile(Profile("admin", "admin123")); // Add default admin if no profiles
        bank_system.saveProfiles(FILENAME); // Save the default admin
    }
    
    // Main loop for the banking system
    while (true) {
        clearConsole();

        string username, password;
        int choice;

        if (bank_system.isLoggedIn()){
            cout << "You are logged in as: " << bank_system.getCurrentUsername()<< endl; 
            cout << "Your balance is: " << bank_system.getCurrentUserBalance()<< endl; 
            cout << "1. Withdraw\n2. Deposit\n3. Transaction\n4. Log out\n\nChoose an option: ";
            cin  >> choice;
            if (cin.fail()) {
                clearConsole();
                cin.clear(); // Clear the error flag
                cin.ignore(numeric_limits<streamsize>::max(), '\n'); // Discard invalid input
                cout << "Invalid input! Please enter a number." << endl;
                waitForUserInput();
                continue; // Restart the loop for valid input
            }
            switch (choice) {
                case 1: {
                    cout << "Enter amount to withdraw: ";
                    double withdraw_amount;
                    cin >> withdraw_amount;
                    bank_system.Withdraw(withdraw_amount);
                    break;
                }
                case 2: {
                    cout << "Enter amount to deposit: ";
                    double deposit_amount;
                    cin >> deposit_amount;
                    bank_system.Deposit(deposit_amount);
                    break;
                }
                case 3: {
                    cout << "Enter the username of the user you want to transfer to: ";
                    string receiver_username;
                    cin >> receiver_username;
                    cout << "Enter amount to transfer: ";
                    double transfer_amount;
                    cin >> transfer_amount;
                    bank_system.Transaction(transfer_amount, receiver_username);
                    break;
                }
                case 4:
                    bank_system.LogoutUser();
                    break;
                default:
                    cout << "Invalid choice!" << endl;
            }
        }
        else{
            cout << "Welcome to the Banking System!" << endl;
            cout << "1. Register\n2. Login\n\nChoose an option: ";
            cin >> choice;
            
            if (cin.fail()) {
                clearConsole();
                cin.clear(); // Clear the error flag
                cin.ignore(numeric_limits<streamsize>::max(), '\n'); // Discard invalid input
                cout << "Invalid input! Please enter a number." << endl;
                waitForUserInput();
                continue; // Restart the loop for valid input
            }
        
            switch (choice) {
                case 1:
                    cout << "Enter username: ";
                    cin >> username;
                    cout << "Enter password: ";
                    cin >> password;
                    bank_system.RegisterUser(username, password);
                    break;
                case 2:
                    cout << "Enter username: ";
                    cin >> username;
                    cout << "Enter password: ";
                    cin >> password;
                    bank_system.LoginUser(username, password);
                    break;
                default:
                    cout << "Invalid choice!" << endl;
                    waitForUserInput();
                    continue; // Restart the loop for valid input
            }
        }
    }
}