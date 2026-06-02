#ifdef _WIN32
    #include <direct.h>
    #include <windows.h>
    #define GET_CWD(b,s)  _getcwd(b,s)
    #define CHANGE_DIR(p) _chdir(p)
    #define MAKE_DIR(p)   _mkdir(p)
    #define REMOVE_DIR(p) _rmdir(p)
    #define CLEAR_CMD     "cls"
#else
    #include <unistd.h>
    #include <sys/stat.h>
    #define GET_CWD(b,s)  getcwd(b,s)
    #define CHANGE_DIR(p) chdir(p)
    #define MAKE_DIR(p)   mkdir(p,0755)
    #define REMOVE_DIR(p) rmdir(p)
    #define CLEAR_CMD     "clear"
#endif

#include <dirent.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <stdexcept>
#include <cstdio>
#include <ctime>

using namespace std;   

class CommandStats {
private:
    static int totalExecuted;   
    static int totalFailed;

public:
    static void recordSuccess() { totalExecuted++; }
    static void recordFailure() { totalFailed++;   }
    static int  getExecuted()   { return totalExecuted; }
    static int  getFailed()     { return totalFailed;   }

    CommandStats& operator+=(const CommandStats& o) {
        totalExecuted += o.totalExecuted;
        totalFailed   += o.totalFailed;
        return *this;
    }

    friend void printStats(const CommandStats& cs);
};

int CommandStats::totalExecuted = 0;
int CommandStats::totalFailed   = 0;

void printStats(const CommandStats& cs) {
    cout << "\n Session Stats | Succeeded: " << CommandStats::totalExecuted
         << "  Failed: "                     << CommandStats::totalFailed << "\n\n";
}

CommandStats gStats;   


// ============================================================
//  [10] Template Function
//  safeGet<T> — retrieves arg at index 'idx' as type T.
//  Throws out_of_range if index missing, invalid_argument if
//  conversion fails. One function works for string, int, etc.
// ============================================================
template<typename T>
T safeGet(const vector<string>& args, size_t idx, const string& name) {
    if (idx >= args.size())
        throw out_of_range("Missing argument: <" + name + ">");
    istringstream ss(args[idx]); T val;
    if (!(ss >> val))
        throw invalid_argument("Bad value for <" + name + ">: " + args[idx]);
    return val;
}

// Template specialization for string — no conversion needed
template<>
string safeGet<string>(const vector<string>& args, size_t idx, const string& name) {
    if (idx >= args.size())
        throw out_of_range("Missing argument: <" + name + ">");
    return args[idx];
}


// ============================================================
//  [5][11] Constructor, Destructor, Deleted Copy Constructor
//  CommandLogger — RAII file logger. Owns an open ofstream.
//  Copying is DELETED (Rule of Three) to prevent two objects
//  sharing the same stream (double-close bug).
// ============================================================
class CommandLogger {
private:
    ofstream logFile;
    int      entryCount;

public:
    // Constructor — opens log file; throws if it can't open
    explicit CommandLogger(const string& filename) : entryCount(0) {  // [13] init list
        logFile.open(filename.c_str(), ios::app);
        if (!logFile.is_open())
            throw runtime_error("Cannot open log: " + filename);
    }

    // [11] Deleted copy constructor — prevents unsafe copying
    CommandLogger(const CommandLogger&)            = delete;
    CommandLogger& operator=(const CommandLogger&) = delete;

    // Destructor — RAII: file closes automatically when object dies
    ~CommandLogger() {
        if (logFile.is_open()) {
            logFile << "[Session ended. Entries: " << entryCount << "]\n";
            logFile.close();
        }
    }

    // log() — writes timestamped entry to file
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


// ============================================================
//  [1] Abstract Base Class  [2] Inheritance  [3] Polymorphism
//  Command — pure interface. execute()=0 makes it abstract.
//  Every command class IS-A Command (inheritance).
//  MyShell calls execute() via Command* — runtime dispatch
//  picks the correct subclass method (polymorphism).
// ============================================================
class Command {
public:
    virtual ~Command() {}                                    // virtual destructor [2]
    virtual void   execute(const vector<string>& args) = 0; // pure virtual [1]
    virtual string description() const = 0;                 // pure virtual [1]
};

// ── Error-handling macros [6] ─────────────────────────────
// Wrap every execute() body so exceptions are caught uniformly.
#define SAFE_EXECUTE_BEGIN  try {
#define SAFE_EXECUTE_END                                            \
    } catch (const out_of_range& e) {                              \
        cerr << " [Missing arg] " << e.what() << "\n";             \
        CommandStats::recordFailure(); return;                      \
    } catch (const exception& e) {                                 \
        cerr << " [Error] " << e.what() << "\n";                   \
        CommandStats::recordFailure(); return;                      \
    }                                                              \
    CommandStats::recordSuccess();


// ============================================================
//  COMMAND CLASSES  [2] Inherit Command  [3] Override execute()
// ============================================================

// ── ls — list directory contents ─────────────────────────────
class LsCommand : public Command {
public:
    // execute() — opens dir with opendir/readdir, prints each entry
    void execute(const vector<string>& args) override {
        SAFE_EXECUTE_BEGIN
        string path = args.empty() ? "." : safeGet<string>(args, 0, "path");
        DIR* dir = opendir(path.c_str());
        if (!dir) throw runtime_error("Cannot open: " + path);
        cout << "\n Listing: " << path << "\n --------------------\n";
        struct dirent* ent; int n = 0;
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
    string description() const override { return "ls [path]          — List directory"; }
};

// ── cd — change directory ────────────────────────────────────
class CdCommand : public Command {
public:
    // execute() — calls CHANGE_DIR macro (chdir/Linux, _chdir/Win)
    void execute(const vector<string>& args) override {
        SAFE_EXECUTE_BEGIN
        string dir = safeGet<string>(args, 0, "directory");
        if (CHANGE_DIR(dir.c_str()) != 0)
            throw runtime_error("Not found: " + dir);
        cout << " Changed to: " << dir << "\n";
        SAFE_EXECUTE_END
    }
    string description() const override { return "cd <dir>           — Change directory"; }
};

// ── mkdir — create directory ─────────────────────────────────
class MkdirCommand : public Command {
public:
    // execute() — calls MAKE_DIR macro; throws if already exists
    void execute(const vector<string>& args) override {
        SAFE_EXECUTE_BEGIN
        string name = safeGet<string>(args, 0, "dirname");
        if (MAKE_DIR(name.c_str()) != 0)
            throw runtime_error("Cannot create: " + name + " (exists?)");
        cout << " Directory created: " << name << "\n";
        SAFE_EXECUTE_END
    }
    string description() const override { return "mkdir <dir>        — Create directory"; }
};

// ── pwd — print working directory ───────────────────────────
class PwdCommand : public Command {
public:
    // execute() — fills buffer with cwd via GET_CWD macro
    void execute(const vector<string>& args) override {
        SAFE_EXECUTE_BEGIN
        char buf[1024];
        if (!GET_CWD(buf, sizeof(buf)))
            throw runtime_error("Cannot get current directory.");
        cout << " Current: " << buf << "\n";
        SAFE_EXECUTE_END
    }
    string description() const override { return "pwd                — Print working dir"; }
};

// ── cat — display file contents ──────────────────────────────
class CatCommand : public Command {
public:
    // execute() — opens file with ifstream, prints numbered lines
    void execute(const vector<string>& args) override {
        SAFE_EXECUTE_BEGIN
        string fn = safeGet<string>(args, 0, "filename");
        ifstream f(fn.c_str());
        if (!f.is_open()) throw runtime_error("Cannot open: " + fn);
        cout << "\n --- " << fn << " ---\n";
        string line; int n = 1;
        while (getline(f, line)) cout << " " << n++ << "\t" << line << "\n";
        cout << " --- " << (n-1) << " line(s) ---\n\n";
        SAFE_EXECUTE_END
    }
    string description() const override { return "cat <file>         — Show file contents"; }
};

// ── touch — create empty file ────────────────────────────────
class TouchCommand : public Command {
public:
    // execute() — opens ofstream to create an empty file
    void execute(const vector<string>& args) override {
        SAFE_EXECUTE_BEGIN
        string fn = safeGet<string>(args, 0, "filename");
        ofstream f(fn.c_str());
        if (!f.is_open()) throw runtime_error("Cannot create: " + fn);
        cout << " File created: " << fn << "\n";
        SAFE_EXECUTE_END
    }
    string description() const override { return "touch <file>       — Create empty file"; }
};

// ── rm — delete a file ───────────────────────────────────────
class RmCommand : public Command {
public:
    // execute() — calls C remove(); throws if file not found
    void execute(const vector<string>& args) override {
        SAFE_EXECUTE_BEGIN
        string fn = safeGet<string>(args, 0, "filename");
        if (remove(fn.c_str()) != 0)
            throw runtime_error("Cannot delete: " + fn);
        cout << " Deleted: " << fn << "\n";
        SAFE_EXECUTE_END
    }
    string description() const override { return "rm <file>          — Delete a file"; }
};

// ── rmdir — remove empty directory ──────────────────────────
class RmdirCommand : public Command {
public:
    // execute() — calls REMOVE_DIR macro; fails if dir is non-empty
    void execute(const vector<string>& args) override {
        SAFE_EXECUTE_BEGIN
        string name = safeGet<string>(args, 0, "dirname");
        if (REMOVE_DIR(name.c_str()) != 0)
            throw runtime_error("Cannot remove: " + name + " (not empty?)");
        cout << " Removed: " << name << "\n";
        SAFE_EXECUTE_END
    }
    string description() const override { return "rmdir <dir>        — Remove empty dir"; }
};

// ── rename — rename file or directory ───────────────────────
class RenameCommand : public Command {
public:
    // execute() — uses C rename(); works on files and directories
    void execute(const vector<string>& args) override {
        SAFE_EXECUTE_BEGIN
        string from = safeGet<string>(args, 0, "old_name");
        string to   = safeGet<string>(args, 1, "new_name");
        if (rename(from.c_str(), to.c_str()) != 0)
            throw runtime_error("Cannot rename: " + from);
        cout << " Renamed: " << from << " -> " << to << "\n";
        SAFE_EXECUTE_END
    }
    string description() const override { return "rename <old> <new> — Rename file/dir"; }
};

// ── copy — copy a file ───────────────────────────────────────
class CopyCommand : public Command {
public:
    // execute() — streams src to dst via rdbuf() (pure C++, cross-platform)
    void execute(const vector<string>& args) override {
        SAFE_EXECUTE_BEGIN
        string src = safeGet<string>(args, 0, "source");
        string dst = safeGet<string>(args, 1, "destination");
        ifstream in (src.c_str(), ios::binary);
        ofstream out(dst.c_str(), ios::binary);
        if (!in.is_open())  throw runtime_error("Cannot read: "  + src);
        if (!out.is_open()) throw runtime_error("Cannot write: " + dst);
        out << in.rdbuf();
        cout << " Copied: " << src << " -> " << dst << "\n";
        SAFE_EXECUTE_END
    }
    string description() const override { return "copy <src> <dst>   — Copy a file"; }
};

// ── echo — print text or append to file ─────────────────────
class EchoCommand : public Command {
public:
    // execute() — scans args for ">>" redirect; otherwise prints to stdout
    void execute(const vector<string>& args) override {
        SAFE_EXECUTE_BEGIN
        if (args.empty()) { cout << "\n"; CommandStats::recordSuccess(); return; }
        string text, file; bool redirect = false;
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
    string description() const override { return "echo <text> [>> f] — Print or append"; }
};

// ── clear — clear the screen ─────────────────────────────────
class ClearCommand : public Command {
public:
    // execute() — runs OS clear command ("cls"/"clear")
    void execute(const vector<string>& args) override {
        system(CLEAR_CMD);
        CommandStats::recordSuccess();
    }
    string description() const override { return "clear              — Clear the screen"; }
};

// ── history — show typed command history ─────────────────────
class HistoryCommand : public Command {
private:
    const vector<string>* history;   // [4] Encapsulation: private pointer
public:
    HistoryCommand() : history(nullptr) {}       // [13] init list
    void setHistory(const vector<string>* h) { history = h; }

    // execute() — iterates history vector and numbers each entry
    void execute(const vector<string>& args) override {
        SAFE_EXECUTE_BEGIN
        if (!history || history->empty()) { cout << " No history yet.\n"; CommandStats::recordSuccess(); return; }
        cout << "\n --- History ---\n";
        for (int i = 0; i < (int)history->size(); i++)
            cout << "  " << (i+1) << ". " << (*history)[i] << "\n";
        cout << " ---------------\n\n";
        SAFE_EXECUTE_END
    }
    string description() const override { return "history            — Show command history"; }
};

// ── help — list all available commands ───────────────────────
class HelpCommand : public Command {
private:
    const map<string, Command*>* commands;   // [4] Encapsulation
public:
    HelpCommand() : commands(nullptr) {}     // [13] init list
    void setCommands(const map<string, Command*>* c) { commands = c; }

    // execute() — [3] Polymorphism: calls virtual description() on each Command*
    void execute(const vector<string>& args) override {
        SAFE_EXECUTE_BEGIN
        cout << "\n ===== MyShell Commands =====\n";
        if (commands)
            for (auto it = commands->begin(); it != commands->end(); ++it)
                cout << "  " << it->second->description() << "\n";  // virtual dispatch
        cout << "  exit               — Quit\n";
        cout << " ============================\n\n";
        SAFE_EXECUTE_END
    }
    string description() const override { return "help               — Show all commands"; }
};

// ── stats — show session statistics ─────────────────────────
class StatsCommand : public Command {
public:
    // execute() — [9] calls friend function printStats()
    void execute(const vector<string>& args) override {
        printStats(gStats);
        CommandStats::recordSuccess();
    }
    string description() const override { return "stats              — Show session stats"; }
};


// ============================================================
//  MyShell — The Shell Engine
//  [4]  All state is private (Encapsulation)
//  [5]  Constructor registers commands; Destructor frees memory
//  [13] Init list: running(true), logger(nullptr)
// ============================================================
class MyShell {
private:
    map<string, Command*> commands;  // command registry
    vector<string>        history;   // typed-command history
    bool                  running;   // main-loop flag
    CommandLogger*        logger;    // RAII heap logger

public:
    // Constructor — [5][13] registers all commands, starts logger
    MyShell() : running(true), logger(nullptr) {
        try { logger = new CommandLogger("myshell_log.txt"); }
        catch (...) { cerr << " [Note] Logger unavailable.\n"; }

        // Register built-in commands
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

    // Destructor — [5] frees every Command object and the logger
    ~MyShell() {
        for (auto it = commands.begin(); it != commands.end(); ++it)
            delete it->second;  // virtual destructor of each subclass runs
        delete logger;          // ~CommandLogger() closes the log file
    }

    // showBanner() — prints welcome message at startup
    void showBanner() {
        cout << "   MyShell — C++ OOP Shell  \n";
        cout << "   Type 'help' or 'exit'    \n";
    }

    // run() — REPL: Read → Evaluate → Print → Loop
    void run() {
        showBanner();
        string line;
        while (running) {
            char buf[1024];
            GET_CWD(buf, sizeof(buf));
            cout << "myshell:" << buf << "> ";

            if (!getline(cin, line)) break;   // EOF → quit
            if (line == "exit") { cout << "\n Goodbye!\n"; printStats(gStats); break; }
            if (line.empty()) continue;

            // Log and save to history (skip duplicates)
            if (logger) try { logger->log(line); } catch (...) {}
            if (history.empty() || history.back() != line)
                history.push_back(line);

            // Parse: first token = command name, rest = arguments
            istringstream ss(line);
            string cmd; ss >> cmd;
            vector<string> args; string tok;
            while (ss >> tok) args.push_back(tok);

            // [3] POLYMORPHISM — Command* dispatches to correct execute()
            if (commands.count(cmd))
                commands[cmd]->execute(args);
            else {
                cout << " [os] " << line << "\n";
                system(line.c_str());           // fall back to OS shell
                CommandStats::recordSuccess();
            }
        }
    }
};

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
