/*
    Bank Management Application
    -----------------------------
    A console-based banking system in C++ demonstrating object-oriented
    programming (classes, encapsulation) and persistent file handling
    (binary file: accounts.dat).

    Features:
      - Create Account
      - Deposit
      - Withdraw
      - Balance Inquiry
      - Display All Accounts
      - Modify Account
      - Close (Delete) Account
      - Simple PIN-based authentication for sensitive operations

    Compile:  g++ -o bank bank_management_application.cpp
    Run:      ./bank
*/

#include <iostream>
#include <fstream>
#include <iomanip>
#include <cstring>
#include <limits>
#include <ctime>

using namespace std;

const char* FILENAME = "accounts.dat";

// =====================================================
//                  Account Class
// =====================================================
class Account {
private:
    int    accNo;
    char   name[50];
    char   pin[5];      // 4-digit PIN + null terminator
    char   accType[15]; // "Savings" or "Current"
    double balance;
    char   createdOn[20];

public:
    // ---------- Setup ----------
    void setData(int no, const char* nm, const char* p,
                 const char* type, double bal) {
        accNo = no;
        strncpy(name, nm, sizeof(name) - 1);
        name[sizeof(name) - 1] = '\0';
        strncpy(pin, p, sizeof(pin) - 1);
        pin[sizeof(pin) - 1] = '\0';
        strncpy(accType, type, sizeof(accType) - 1);
        accType[sizeof(accType) - 1] = '\0';
        balance = bal;

        time_t now = time(0);
        tm* ltm = localtime(&now);
        snprintf(createdOn, sizeof(createdOn), "%02d-%02d-%04d",
                 ltm->tm_mday, 1 + ltm->tm_mon, 1900 + ltm->tm_year);
    }

    // ---------- Getters ----------
    int    getAccNo()   const { return accNo; }
    string getName()    const { return name; }
    string getPin()     const { return pin; }
    string getType()    const { return accType; }
    double getBalance() const { return balance; }
    string getDate()    const { return createdOn; }

    // ---------- Behavior ----------
    bool verifyPin(const string& enteredPin) const {
        return enteredPin == string(pin);
    }

    void deposit(double amount) {
        balance += amount;
    }

    bool withdraw(double amount) {
        if (amount > balance) return false; // insufficient funds
        balance -= amount;
        return true;
    }

    void display() const {
        cout << left
             << setw(8)  << accNo
             << setw(20) << name
             << setw(12) << accType
             << setw(12) << fixed << setprecision(2) << balance
             << setw(12) << createdOn
             << "\n";
    }
};

// =====================================================
//                  Utility Functions
// =====================================================
void clearInputBuffer() {
    cin.clear();
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
}

int readInt(const string& prompt) {
    int value;
    while (true) {
        cout << prompt;
        cin >> value;
        if (cin.fail()) {
            clearInputBuffer();
            cout << "Invalid input. Please enter a number.\n";
        } else {
            clearInputBuffer();
            return value;
        }
    }
}

double readDouble(const string& prompt) {
    double value;
    while (true) {
        cout << prompt;
        cin >> value;
        if (cin.fail() || value < 0) {
            clearInputBuffer();
            cout << "Invalid input. Please enter a non-negative number.\n";
        } else {
            clearInputBuffer();
            return value;
        }
    }
}

void readLine(const string& prompt, char* buffer, size_t size) {
    cout << prompt;
    cin.getline(buffer, size);
}

string readPin(const string& prompt) {
    char buf[10];
    while (true) {
        readLine(prompt, buf, sizeof(buf));
        string p(buf);
        bool valid = (p.length() == 4);
        for (char c : p) if (!isdigit((unsigned char)c)) valid = false;
        if (valid) return p;
        cout << "PIN must be exactly 4 digits.\n";
    }
}

// =====================================================
//               Core Banking Operations
// =====================================================

// Generates the next available account number
int getNextAccountNumber() {
    ifstream file(FILENAME, ios::binary);
    int maxNo = 1000; // starting account number base
    if (!file) return maxNo + 1;

    Account acc;
    while (file.read(reinterpret_cast<char*>(&acc), sizeof(Account))) {
        if (acc.getAccNo() > maxNo) maxNo = acc.getAccNo();
    }
    file.close();
    return maxNo + 1;
}

void createAccount() {
    char name[50], type[15];
    int accNo = getNextAccountNumber();

    cout << "\n--- Create New Account ---\n";
    readLine("Enter Full Name: ", name, sizeof(name));

    cout << "Account Type (1-Savings, 2-Current): ";
    int choice = readInt("");
    strcpy(type, (choice == 2) ? "Current" : "Savings");

    string pin = readPin("Set a 4-digit PIN: ");
    double initialDeposit = readDouble("Enter Initial Deposit Amount: ");

    Account acc;
    acc.setData(accNo, name, pin.c_str(), type, initialDeposit);

    ofstream file(FILENAME, ios::binary | ios::app);
    if (!file) {
        cout << "Error: Could not open file for writing.\n";
        return;
    }
    file.write(reinterpret_cast<char*>(&acc), sizeof(Account));
    file.close();

    cout << "Account created successfully! Your Account Number is: "
         << accNo << "\n";
}

// Finds account by number; returns true if found
bool findAccount(int accNo, Account& result, streampos& position) {
    ifstream file(FILENAME, ios::binary);
    if (!file) return false;

    Account acc;
    while (file.read(reinterpret_cast<char*>(&acc), sizeof(Account))) {
        if (acc.getAccNo() == accNo) {
            result = acc;
            position = file.tellg();
            position -= static_cast<streamoff>(sizeof(Account));
            file.close();
            return true;
        }
    }
    file.close();
    return false;
}

// Authenticates a user by Account Number + PIN
bool authenticate(Account& acc, streampos& pos) {
    int accNo = readInt("Enter Account Number: ");
    if (!findAccount(accNo, acc, pos)) {
        cout << "Account not found.\n";
        return false;
    }
    string pin = readPin("Enter PIN: ");
    if (!acc.verifyPin(pin)) {
        cout << "Incorrect PIN. Access denied.\n";
        return false;
    }
    return true;
}

void rewriteAccount(const Account& acc, streampos pos) {
    fstream file(FILENAME, ios::binary | ios::in | ios::out);
    file.seekp(pos);
    file.write(reinterpret_cast<const char*>(&acc), sizeof(Account));
    file.close();
}

void depositMoney() {
    Account acc;
    streampos pos;
    cout << "\n--- Deposit Money ---\n";
    if (!authenticate(acc, pos)) return;

    double amount = readDouble("Enter Deposit Amount: ");
    if (amount <= 0) {
        cout << "Deposit amount must be positive.\n";
        return;
    }

    acc.deposit(amount);
    rewriteAccount(acc, pos);

    cout << fixed << setprecision(2);
    cout << "Deposit successful. New Balance: " << acc.getBalance() << "\n";
}

void withdrawMoney() {
    Account acc;
    streampos pos;
    cout << "\n--- Withdraw Money ---\n";
    if (!authenticate(acc, pos)) return;

    double amount = readDouble("Enter Withdrawal Amount: ");
    if (amount <= 0) {
        cout << "Withdrawal amount must be positive.\n";
        return;
    }

    if (acc.withdraw(amount)) {
        rewriteAccount(acc, pos);
        cout << fixed << setprecision(2);
        cout << "Withdrawal successful. New Balance: " << acc.getBalance() << "\n";
    } else {
        cout << "Insufficient balance. Current Balance: "
             << fixed << setprecision(2) << acc.getBalance() << "\n";
    }
}

void checkBalance() {
    Account acc;
    streampos pos;
    cout << "\n--- Balance Inquiry ---\n";
    if (!authenticate(acc, pos)) return;

    cout << fixed << setprecision(2);
    cout << "Account Holder: " << acc.getName() << "\n";
    cout << "Account Type  : " << acc.getType() << "\n";
    cout << "Current Balance: " << acc.getBalance() << "\n";
}

void displayAllAccounts() {
    ifstream file(FILENAME, ios::binary);
    if (!file) {
        cout << "No accounts found.\n";
        return;
    }

    Account acc;
    bool found = false;

    cout << "\n" << left
         << setw(8)  << "AccNo"
         << setw(20) << "Name"
         << setw(12) << "Type"
         << setw(12) << "Balance"
         << setw(12) << "Opened On" << "\n";
    cout << string(64, '-') << "\n";

    while (file.read(reinterpret_cast<char*>(&acc), sizeof(Account))) {
        acc.display();
        found = true;
    }
    file.close();

    if (!found) cout << "No accounts to display.\n";
}

void modifyAccount() {
    Account acc;
    streampos pos;
    cout << "\n--- Modify Account ---\n";
    if (!authenticate(acc, pos)) return;

    char name[50];
    readLine("Enter New Name: ", name, sizeof(name));
    string newPin = readPin("Enter New 4-digit PIN: ");

    acc.setData(acc.getAccNo(), name, newPin.c_str(),
                acc.getType().c_str(), acc.getBalance());
    rewriteAccount(acc, pos);

    cout << "Account details updated successfully.\n";
}

void closeAccount() {
    Account acc;
    streampos pos;
    cout << "\n--- Close Account ---\n";
    if (!authenticate(acc, pos)) return;

    if (acc.getBalance() > 0) {
        cout << "Warning: Account has a remaining balance of "
             << fixed << setprecision(2) << acc.getBalance() << ".\n";
    }

    cout << "Are you sure you want to close this account? (y/n): ";
    char confirm;
    cin >> confirm;
    clearInputBuffer();
    if (tolower(confirm) != 'y') {
        cout << "Account closure cancelled.\n";
        return;
    }

    int targetAccNo = acc.getAccNo();
    ifstream inFile(FILENAME, ios::binary);
    ofstream tempFile("temp.dat", ios::binary);

    Account temp;
    while (inFile.read(reinterpret_cast<char*>(&temp), sizeof(Account))) {
        if (temp.getAccNo() != targetAccNo) {
            tempFile.write(reinterpret_cast<char*>(&temp), sizeof(Account));
        }
    }
    inFile.close();
    tempFile.close();

    remove(FILENAME);
    rename("temp.dat", FILENAME);

    cout << "Account closed successfully.\n";
}

// =====================================================
//                       Menu
// =====================================================
void showMenu() {
    cout << "\n=========================================\n";
    cout << "          BANK MANAGEMENT SYSTEM\n";
    cout << "=========================================\n";
    cout << "1. Create Account\n";
    cout << "2. Deposit Money\n";
    cout << "3. Withdraw Money\n";
    cout << "4. Balance Inquiry\n";
    cout << "5. Display All Accounts\n";
    cout << "6. Modify Account\n";
    cout << "7. Close Account\n";
    cout << "8. Exit\n";
    cout << "=========================================\n";
}

int main() {
    int choice;

    do {
        showMenu();
        choice = readInt("Enter your choice (1-8): ");

        switch (choice) {
            case 1: createAccount();       break;
            case 2: depositMoney();        break;
            case 3: withdrawMoney();       break;
            case 4: checkBalance();        break;
            case 5: displayAllAccounts();  break;
            case 6: modifyAccount();       break;
            case 7: closeAccount();        break;
            case 8: cout << "Thank you for banking with us. Goodbye!\n"; break;
            default: cout << "Invalid choice. Please select between 1 and 8.\n";
        }

    } while (choice != 8);

    return 0;
}
