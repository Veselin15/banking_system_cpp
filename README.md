# 💰 Secure Banking System (C++)

A robust and secure **console-based banking system** built with C++.  
Supports user registration, login, deposit, withdrawal, and transactions, with secure password hashing (SHA-256 + salt), persistent user data storage in JSON, and automatic transaction recovery via a journal log.

---

## 🚀 Features

- ✅ **User Registration & Login**
- 🔐 **Password Security** with SHA-256 + Salt (via OpenSSL)
- 💾 **Persistent Profiles** stored in `profiles.json`
- 🧾 **Transaction Journal** (`journal.log`) for deposit, withdrawal & transfer recovery
- 🔁 **Crash Recovery** via journal replay on startup
- 🧮 **Deposit, Withdraw & Transfer Funds**
- 🧑‍💻 **Admin Account Auto-Creation** if no profiles exist
- 🧵 **Thread-Safe Transactions** using `std::mutex`
- 🧼 **Cross-Platform Console Clear**

---


