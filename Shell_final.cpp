// ============================================================
//  MyShell — Cross-Platform C++ OOP Shell
//  Works on: Linux (g++) and Windows (MinGW g++)
//  Compile:  g++ Shell.cpp -o Shell
//
//  OOP CONCEPTS COVERED (13 total):
//  [1]  Abstract Base Class
//  [2]  Inheritance
//  [3]  Polymorphism
//  [4]  Encapsulation
//  [5]  Constructor & Destructor
//  [6]  Exception Handling
//  [7]  Static Members
//  [8]  Operator Overloading
//  [9]  Friend Function
//  [10] Template Function
//  [11] Copy Constructor (deleted — Rule of Three)
//  [12] namespace std
//  [13] Constructor Initialization List
// ============================================================

// ── Cross-platform headers ──────────────────────────────────
// #ifdef lets the compiler pick the right header automatically.
// On Windows: _WIN32 is defined by the compiler.
// On Linux:   _WIN32 is NOT defined, so the else-branch runs.

#ifdef _WIN32
    #include <direct.h>      // _getcwd, _chdir, _mkdir, _rmdir
    #include <windows.h>     // CopyFile
    // Map Linux names to Windows names so the rest of code is uniform
    #define GET_CWD(buf, sz)   _getcwd(buf, sz)
    #define CHANGE_DIR(path)   _chdir(path)
    #define MAKE_DIR(path)     _mkdir(path)
    #define REMOVE_DIR(path)   _rmdir(path)
    #define CLEAR_CMD          "cls"
#else
    #include <unistd.h>      // getcwd, chdir, rmdir
    #include <sys/stat.h>    // mkdir
    #define GET_CWD(buf, sz)   getcwd(buf, sz)
    #define CHANGE_DIR(path)   chdir(path)
    #define MAKE_DIR(path)     mkdir(path, 0755)
    #define REMOVE_DIR(path)   rmdir(path)
    #define CLEAR_CMD          "clear"
#endif

#include <dirent.h>          // opendir, readdir — works on both platforms
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <stdexcept>
#include <cstdio>            // remove(), rename()
#include <ctime>
#include <cstring>           // strerror()

// ── OOP CONCEPT [12] — namespace std ──────────────────────
// Brings all std:: names into scope so we write cout, string,
// vector instead of std::cout, std::string, std::vector.
using namespace std;


// ===========================================================
//  OOP CONCEPTS [7][8][9]
//  CommandStats — tracks how many commands passed or failed
//
//  [7] Static Members:  totalExecuted / totalFailed belong to
//      the CLASS, not any object. One shared copy for the whole
//      program. Accessed as CommandStats::recordSuccess().
//
//  [8] Operator Overloading: += lets us merge two stat objects
//      with natural syntax:  sessionA += sessionB;
//
//  [9] Friend Function: printStats() is NOT a member of the
//      class but is granted access to its private data via
//      the 'friend' keyword.
// ===========================================================
class CommandStats {
private:
    static int totalExecuted;   // [7] static — shared across all instances
    static int totalFailed;

public:
    // [7] Static member functions — no object needed to call them
    static void recordSuccess() { totalExecuted++; }
    static void recordFailure() { totalFailed++;   }
    static int  getExecuted()   { return totalExecuted; }
    static int  getFailed()     { return totalFailed;   }

    // [8] Operator Overloading — += merges two CommandStats objects
    CommandStats& operator+=(const CommandStats& other) {
        totalExecuted += other.totalExecuted;
        totalFailed   += other.totalFailed;
        return *this;   // return reference so chaining works: a += b += c
    }

    // [9] Friend declaration — printStats can read our private members
    friend void printStats(const CommandStats& cs);
};

// Static members must be defined outside the class body (C++ rule)
int CommandStats::totalExecuted = 0;
int CommandStats::totalFailed   = 0;

// [9] Friend function definition — directly reads private static members
void printStats(const CommandStats& cs) {
    cout << "\n ===========================\n";
    cout << "  Session Stats\n";
    cout << "  Succeeded : " << CommandStats::totalExecuted << "\n";
    cout << "  Failed    : " << CommandStats::totalFailed   << "\n";
    cout << " ===========================\n\n";
}

CommandStats gStats;   // one global stats object for the whole session


// ===========================================================
//  OOP CONCEPT [10] — Template Function
//
//  safeGet<T> pulls argument at position 'idx' from the args
//  vector and converts it to type T.
//  If the index is out of bounds → throws out_of_range.
//  Works for string, int, double — one function for all types.
// ===========================================================
template<typename T>
T safeGet(const vector<string>& args, size_t idx, const string& name) {
    if (idx >= args.size())
        throw out_of_range("Missing argument: <" + name + ">");
    istringstream ss(args[idx]);
    T val;
    if (!(ss >> val))
        throw invalid_argument("Bad value for <" + name + ">: " + args[idx]);
    return val;
}

// Template specialization for string — skip conversion, return directly
template<>
string safeGet<string>(const vector<string>& args, size_t idx, const string& name) {
    if (idx >= args.size())
        throw out_of_range("Missing argument: <" + name + ">");
    return args[idx];
}


// ===========================================================
//  OOP CONCEPT [11] — Copy Constructor (deleted)
//
//  CommandLogger writes every typed command to a log file.
//  It OWNS an open file stream (a resource).
//  If we allowed copying, two objects would share the same
//  stream → double-close bug. So we explicitly DELETE the
//  copy constructor. This is called the Rule of Three.
//  RAII: file opens in constructor, closes in destructor.
// ===========================================================
class CommandLogger {
private:
    ofstream logFile;
    int      entryCount;

public:
    // Constructor opens the log file; throws if it can't
    explicit CommandLogger(const string& filename) : entryCount(0) {
        logFile.open(filename.c_str(), ios::app);
        if (!logFile.is_open())
            throw runtime_error("Cannot open log file: " + filename);
    }

    // [11] Explicitly deleted — copying a file-owning object is dangerous
    CommandLogger(const CommandLogger&)            = delete;
    CommandLogger& operator=(const CommandLogger&) = delete;

    // RAII destructor — file closes automatically, no manual cleanup needed
    ~CommandLogger() {
        if (logFile.is_open()) {
            logFile << "[Session ended. Total entries: " << entryCount << "]\n";
            logFile.close();
        }
    }

    void log(const string& line) {
        if (!logFile.is_open()) return;
        time_t now = time(nullptr);
        string ts  = ctime(&now);
        if (!ts.empty() && ts.back() == '\n') ts.pop_back();
        logFile << "[" << ts << "] " << line << "\n";
        logFile.flush();
        entryCount++;
    }
};


// ===========================================================
//  OOP CONCEPTS [1][2][3] — Abstract Base Class, Inheritance,
//                            Polymorphism
//
//  [1] Command is abstract: execute() = 0 means it CANNOT be
//      instantiated. It is a pure interface.
//
//  [2] Every command class (LsCommand, CdCommand, …) inherits
//      from Command and provides its own execute(). IS-A relation.
//
//  [3] MyShell stores Command* pointers and calls execute()
//      without knowing the actual type. The vtable dispatches
//      to the correct subclass at runtime — that is polymorphism.
// ===========================================================
class Command {
public:
    virtual ~Command() {}                                    // virtual destructor [2]
    virtual void   execute(const vector<string>& args) = 0; // pure virtual [1]
    virtual string description() const = 0;                 // pure virtual [1]
};


// ── Uniform error wrapper ───────────────────────────────────
// Every execute() body is wrapped in SAFE_EXECUTE_BEGIN / END.
// Any throw inside lands here, prints a clear message, and
// records a failure. The shell never crashes on bad input.
#define SAFE_EXECUTE_BEGIN  try {
#define SAFE_EXECUTE_END                                         \
    } catch (const out_of_range& e) {                           \
        cerr << " [Missing arg] " << e.what() << "\n";          \
        CommandStats::recordFailure(); return;                   \
    } catch (const exception& e) {                              \
        cerr << " [Error] " << e.what() << "\n";                \
        CommandStats::recordFailure(); return;                   \
    }                                                           \
    CommandStats::recordSuccess();


// ===========================================================
//  COMMAND CLASSES
//  Each class: inherits Command [2], overrides execute() [3],
//  uses safeGet [10], throws on error [6].
// ===========================================================

// ── ls ──────────────────────────────────────────────────────
class LsCommand : public Command {
public:
    void execute(const vector<string>& args) override {
        SAFE_EXECUTE_BEGIN
        string path = args.empty() ? "." : safeGet<string>(args, 0, "path");
        DIR* dir = opendir(path.c_str());
        if (!dir) throw runtime_error("Cannot open: " + path);
        cout << "\n Listing: " << path << "\n";
        cout << " --------------------\n";
        struct dirent* ent;
        int n = 0;
        while ((ent = readdir(dir)) != nullptr) {
            string nm(ent->d_name);
            if (nm == "." || nm == "..") continue;
            cout << (ent->d_type == DT_DIR ? " [DIR]  " : " [FILE] ") << nm << "\n";
            n++;
        }
        closedir(dir);
        cout << " --------------------\n Total: " << n << " item(s)\n\n";
        SAFE_EXECUTE_END
    }
    string description() const override {
        return "ls [path]          — List directory contents";
    }
};

// ── cd ──────────────────────────────────────────────────────
class CdCommand : public Command {
public:
    void execute(const vector<string>& args) override {
        SAFE_EXECUTE_BEGIN
        string dir = safeGet<string>(args, 0, "directory");
        if (CHANGE_DIR(dir.c_str()) != 0)
            throw runtime_error("Not found: " + dir);
        cout << " Changed to: " << dir << "\n";
        SAFE_EXECUTE_END
    }
    string description() const override {
        return "cd <dir>           — Change directory";
    }
};

// ── mkdir ────────────────────────────────────────────────────
class MkdirCommand : public Command {
public:
    void execute(const vector<string>& args) override {
        SAFE_EXECUTE_BEGIN
        string name = safeGet<string>(args, 0, "dirname");
        if (MAKE_DIR(name.c_str()) != 0)
            throw runtime_error("Cannot create: " + name + " (already exists?)");
        cout << " Directory created: " << name << "\n";
        SAFE_EXECUTE_END
    }
    string description() const override {
        return "mkdir <dir>        — Create a new directory";
    }
};

// ── pwd ──────────────────────────────────────────────────────
class PwdCommand : public Command {
public:
    void execute(const vector<string>& args) override {
        SAFE_EXECUTE_BEGIN
        char buf[1024];
        if (!GET_CWD(buf, sizeof(buf)))
            throw runtime_error("Cannot get current directory.");
        cout << " Current: " << buf << "\n";
        SAFE_EXECUTE_END
    }
    string description() const override {
        return "pwd                — Print working directory";
    }
};

// ── cat ──────────────────────────────────────────────────────
class CatCommand : public Command {
public:
    void execute(const vector<string>& args) override {
        SAFE_EXECUTE_BEGIN
        string filename = safeGet<string>(args, 0, "filename");
        ifstream f(filename.c_str());
        if (!f.is_open()) throw runtime_error("Cannot open: " + filename);
        cout << "\n --- " << filename << " ---\n";
        string line; int n = 1;
        while (getline(f, line)) cout << " " << n++ << "\t" << line << "\n";
        cout << " --- " << (n-1) << " line(s) ---\n\n";
        SAFE_EXECUTE_END
    }
    string description() const override {
        return "cat <file>         — Show file contents";
    }
};

// ── touch ────────────────────────────────────────────────────
class TouchCommand : public Command {
public:
    void execute(const vector<string>& args) override {
        SAFE_EXECUTE_BEGIN
        string filename = safeGet<string>(args, 0, "filename");
        ofstream f(filename.c_str());
        if (!f.is_open()) throw runtime_error("Cannot create: " + filename);
        cout << " File created: " << filename << "\n";
        SAFE_EXECUTE_END
    }
    string description() const override {
        return "touch <file>       — Create an empty file";
    }
};

// ── rm ───────────────────────────────────────────────────────
class RmCommand : public Command {
public:
    void execute(const vector<string>& args) override {
        SAFE_EXECUTE_BEGIN
        string filename = safeGet<string>(args, 0, "filename");
        if (remove(filename.c_str()) != 0)
            throw runtime_error("Cannot delete: " + filename);
        cout << " Deleted: " << filename << "\n";
        SAFE_EXECUTE_END
    }
    string description() const override {
        return "rm <file>          — Delete a file";
    }
};

// ── rmdir ────────────────────────────────────────────────────
class RmdirCommand : public Command {
public:
    void execute(const vector<string>& args) override {
        SAFE_EXECUTE_BEGIN
        string name = safeGet<string>(args, 0, "dirname");
        if (REMOVE_DIR(name.c_str()) != 0)
            throw runtime_error("Cannot remove: " + name + " (not empty or missing)");
        cout << " Removed: " << name << "\n";
        SAFE_EXECUTE_END
    }
    string description() const override {
        return "rmdir <dir>        — Remove an empty directory";
    }
};

// ── rename ───────────────────────────────────────────────────
class RenameCommand : public Command {
public:
    void execute(const vector<string>& args) override {
        SAFE_EXECUTE_BEGIN
        string from = safeGet<string>(args, 0, "old_name");
        string to   = safeGet<string>(args, 1, "new_name");
        if (rename(from.c_str(), to.c_str()) != 0)
            throw runtime_error("Cannot rename: " + from);
        cout << " Renamed: " << from << " → " << to << "\n";
        SAFE_EXECUTE_END
    }
    string description() const override {
        return "rename <old> <new> — Rename a file or folder";
    }
};

// ── copy ─────────────────────────────────────────────────────
// Uses ifstream/ofstream (pure C++ — works on both platforms)
// rdbuf() streams the entire file content in one operation
class CopyCommand : public Command {
public:
    void execute(const vector<string>& args) override {
        SAFE_EXECUTE_BEGIN
        string src = safeGet<string>(args, 0, "source");
        string dst = safeGet<string>(args, 1, "destination");
        ifstream in (src.c_str(), ios::binary);
        ofstream out(dst.c_str(), ios::binary);
        if (!in.is_open())  throw runtime_error("Cannot read: "  + src);
        if (!out.is_open()) throw runtime_error("Cannot write: " + dst);
        out << in.rdbuf();
        cout << " Copied: " << src << " → " << dst << "\n";
        SAFE_EXECUTE_END
    }
    string description() const override {
        return "copy <src> <dst>   — Copy a file";
    }
};

// ── echo ─────────────────────────────────────────────────────
class EchoCommand : public Command {
public:
    void execute(const vector<string>& args) override {
        SAFE_EXECUTE_BEGIN
        if (args.empty()) { cout << "\n"; CommandStats::recordSuccess(); return; }
        string text, file;
        bool redirect = false;
        for (int i = 0; i < (int)args.size(); i++) {
            if (args[i] == ">>" && i + 1 < (int)args.size()) {
                redirect = true; file = args[i+1]; break;
            }
            if (!text.empty()) text += " ";
            text += args[i];
        }
        if (redirect) {
            ofstream f(file.c_str(), ios::app);
            if (!f.is_open()) throw runtime_error("Cannot open: " + file);
            f << text << "\n";
            cout << " Written to: " << file << "\n";
        } else {
            cout << " " << text << "\n";
        }
        SAFE_EXECUTE_END
    }
    string description() const override {
        return "echo <text> [>> f] — Print or append text to file";
    }
};

// ── clear ────────────────────────────────────────────────────
class ClearCommand : public Command {
public:
    void execute(const vector<string>& args) override {
        system(CLEAR_CMD);  // "cls" on Windows, "clear" on Linux
        CommandStats::recordSuccess();
    }
    string description() const override {
        return "clear              — Clear the screen";
    }
};

// ── history ──────────────────────────────────────────────────
// [13] Constructor initialization list sets history to nullptr
class HistoryCommand : public Command {
private:
    const vector<string>* history;   // [4] Encapsulation: private pointer
public:
    HistoryCommand() : history(nullptr) {}    // [13] init list
    void setHistory(const vector<string>* h) { history = h; }
    void execute(const vector<string>& args) override {
        SAFE_EXECUTE_BEGIN
        if (!history || history->empty()) {
            cout << " No history yet.\n";
            CommandStats::recordSuccess(); return;
        }
        cout << "\n --- History ---\n";
        for (int i = 0; i < (int)history->size(); i++)
            cout << "  " << (i+1) << ". " << (*history)[i] << "\n";
        cout << " ---------------\n\n";
        SAFE_EXECUTE_END
    }
    string description() const override {
        return "history            — Show command history";
    }
};

// ── help ─────────────────────────────────────────────────────
// [3] Polymorphism in action: it->second->description() calls
//     the correct virtual method for each Command subclass
class HelpCommand : public Command {
private:
    const map<string, Command*>* commands;   // [4] Encapsulation
public:
    HelpCommand() : commands(nullptr) {}     // [13] init list
    void setCommands(const map<string, Command*>* c) { commands = c; }
    void execute(const vector<string>& args) override {
        SAFE_EXECUTE_BEGIN
        cout << "\n ==============================\n";
        cout << "   MyShell — Commands\n";
        cout << " ==============================\n";
        if (commands)
            for (auto it = commands->begin(); it != commands->end(); ++it)
                cout << "  " << it->second->description() << "\n";  // virtual dispatch
        cout << "  stats              — Session statistics\n";
        cout << "  exit               — Quit\n";
        cout << " ==============================\n\n";
        SAFE_EXECUTE_END
    }
    string description() const override {
        return "help               — Show all commands";
    }
};

// ── stats ─────────────────────────────────────────────────────
// [9] Calls the friend function printStats()
class StatsCommand : public Command {
public:
    void execute(const vector<string>& args) override {
        printStats(gStats);   // friend function — reads private members
        CommandStats::recordSuccess();
    }
    string description() const override {
        return "stats              — Show session statistics";
    }
};


// ===========================================================
//  MyShell — The Shell Engine
//
//  [4] Encapsulation:  all state is private
//  [5] Constructor:    registers all commands, starts logger
//      Destructor:     frees every heap-allocated command
//  [13] Init list:     running(true), logger(nullptr)
// ===========================================================
class MyShell {
private:
    map<string, Command*> commands;   // command registry
    vector<string>        history;    // typed command history
    bool                  running;    // loop flag
    CommandLogger*        logger;     // RAII logger (heap)

public:
    // [5][13] Constructor with initialization list
    MyShell() : running(true), logger(nullptr) {

        // Try to start logging; shell works fine even if it fails
        try {
            logger = new CommandLogger("myshell_log.txt");
        } catch (...) {
            cerr << " [Note] Could not start logger.\n";
        }

        // Register commands — each 'new' creates a heap object
        commands["ls"]      = new LsCommand();
        commands["cd"]      = new CdCommand();
        commands["mkdir"]   = new MkdirCommand();
        commands["pwd"]     = new PwdCommand();
        commands["cat"]     = new CatCommand();
        commands["touch"]   = new TouchCommand();
        commands["rm"]      = new RmCommand();
        commands["rmdir"]   = new RmdirCommand();
        commands["rename"]  = new RenameCommand();
        commands["copy"]    = new CopyCommand();
        commands["echo"]    = new EchoCommand();
        commands["clear"]   = new ClearCommand();
        commands["stats"]   = new StatsCommand();

        HistoryCommand* hcmd = new HistoryCommand();
        hcmd->setHistory(&history);
        commands["history"] = hcmd;

        HelpCommand* helpcmd = new HelpCommand();
        helpcmd->setCommands(&commands);
        commands["help"] = helpcmd;
    }

    // [5] Destructor — called automatically when shell exits scope
    // Deletes every Command object to prevent memory leaks
    ~MyShell() {
        for (auto it = commands.begin(); it != commands.end(); ++it)
            delete it->second;   // virtual destructor of each Command runs
        delete logger;           // ~CommandLogger() closes the log file
    }

    void showBanner() {
        cout << "\n ====================================\n";
        cout << "   MyShell — C++ OOP Shell Project\n";
        cout << "   OOP Concepts: 13  |  Commands: 15\n";
        cout << "   Type 'help'  to see all commands\n";
        cout << "   Type 'exit'  to quit\n";
        cout << " ====================================\n\n";
    }

    void run() {
        showBanner();
        string line;

        while (running) {
            // Show prompt with current directory
            char buf[1024];
            GET_CWD(buf, sizeof(buf));
            cout << "myshell:" << buf << "> ";

            if (!getline(cin, line)) break;     // EOF → quit
            if (line == "exit") {
                cout << "\n Goodbye!\n";
                printStats(gStats);             // show final stats [9]
                break;
            }
            if (line.empty()) continue;

            // Log and save to history
            if (logger) try { logger->log(line); } catch (...) {}
            if (history.empty() || history.back() != line)
                history.push_back(line);

            // Parse: first word = command, rest = args
            istringstream ss(line);
            string cmd; ss >> cmd;
            vector<string> args;
            string tok;
            while (ss >> tok) args.push_back(tok);

            // [3] POLYMORPHISM — Command* dispatches to the right execute()
            if (commands.count(cmd))
                commands[cmd]->execute(args);
            else {
                // Fall back to OS shell for unknown commands
                cout << " [os] " << line << "\n";
                system(line.c_str());
                CommandStats::recordSuccess();
            }
        }
    }
};


// ===========================================================
//  main — entry point
//  MyShell shell  → constructor runs
//  shell.run()    → REPL starts
//  return 0       → destructor runs, all cleanup automatic
// ===========================================================
int main() {
    try {
        MyShell shell;
        shell.run();
    } catch (const exception& e) {
        cerr << " [Fatal] " << e.what() << "\n";
        return 1;
    }
    return 0;
}
