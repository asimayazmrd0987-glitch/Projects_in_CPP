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

template<typename T>
T safeGet(const vector<string>& args, size_t idx, const string& name) {
    if (idx >= args.size())
        throw out_of_range("Missing argument: <" + name + ">");
    istringstream ss(args[idx]); T val;
    if (!(ss >> val))
        throw invalid_argument("Bad value for <" + name + ">: " + args[idx]);
    return val;
}

template<>
string safeGet<string>(const vector<string>& args, size_t idx, const string& name) {
    if (idx >= args.size())
        throw out_of_range("Missing argument: <" + name + ">");
    return args[idx];
}

class CommandLogger {
private:
    ofstream logFile;
    int      entryCount;

public:
    explicit CommandLogger(const string& filename) : entryCount(0) {  
        logFile.open(filename.c_str(), ios::app);
        if (!logFile.is_open())
            throw runtime_error("Cannot open log: " + filename);
    }

    CommandLogger(const CommandLogger&)            = delete;
    CommandLogger& operator=(const CommandLogger&) = delete;

    ~CommandLogger() {
        if (logFile.is_open()) {
            logFile << "[Session ended. Entries: " << entryCount << "]\n";
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

class Command {
public:
    virtual ~Command() {}                                    
    virtual void   execute(const vector<string>& args) = 0; 
    virtual string description() const = 0;                 
};


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

class LsCommand : public Command {
public:
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
    string description() const override { return "cd <dir>           — Change directory"; }
};

class MkdirCommand : public Command {
public:
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
    string description() const override { return "pwd                — Print working dir"; }
};

class CatCommand : public Command {
public:
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

class TouchCommand : public Command {
public:
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

class RmCommand : public Command {
public:
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

class RmdirCommand : public Command {
public:
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

class RenameCommand : public Command {
public:
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
        cout << " Copied: " << src << " -> " << dst << "\n";
        SAFE_EXECUTE_END
    }
    string description() const override { return "copy <src> <dst>   — Copy a file"; }
};

class EchoCommand : public Command {
public:
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

class ClearCommand : public Command {
public:
    void execute(const vector<string>& args) override {
        system(CLEAR_CMD);
        CommandStats::recordSuccess();
    }
    string description() const override { return "clear              — Clear the screen"; }
};

class HistoryCommand : public Command {
private:
    const vector<string>* history;   
public:
    HistoryCommand() : history(nullptr) {}       
    void setHistory(const vector<string>* h) { history = h; }

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

class HelpCommand : public Command {
private:
    const map<string, Command*>* commands;   
public:
    HelpCommand() : commands(nullptr) {}     
    void setCommands(const map<string, Command*>* c) { commands = c; }

    void execute(const vector<string>& args) override {
        SAFE_EXECUTE_BEGIN
        cout << "\n ===== MyShell Commands =====\n";
        if (commands)
            for (auto it = commands->begin(); it != commands->end(); ++it)
                cout << "  " << it->second->description() << "\n"; 
        cout << "  exit               — Quit\n";
        cout << " ============================\n\n";
        SAFE_EXECUTE_END
    }
    string description() const override { return "help               — Show all commands"; }
};

class StatsCommand : public Command {
public:
    void execute(const vector<string>& args) override {
        printStats(gStats);
        CommandStats::recordSuccess();
    }
    string description() const override { return "stats              — Show session stats"; }
};

class MyShell {
private:
    map<string, Command*> commands; 
    vector<string>        history;   
    bool                  running;   
    CommandLogger*        logger;    

public:
    MyShell() : running(true), logger(nullptr) {
        try { logger = new CommandLogger("myshell_log.txt"); }
        catch (...) { cerr << " [Note] Logger unavailable.\n"; }

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

    ~MyShell() {
        for (auto it = commands.begin(); it != commands.end(); ++it)
            delete it->second; 
        delete logger;          
    }

    void showBanner() {
        cout << "   MyShell — C++ OOP Shell  \n";
        cout << "   Type 'help' or 'exit'    \n";
    }

    void run() {
        showBanner();
        string line;
        while (running) {
            char buf[1024];
            GET_CWD(buf, sizeof(buf));
            cout << "myshell:" << buf << "> ";

            if (!getline(cin, line)) break;   
            if (line == "exit") { cout << "\n Goodbye!\n"; printStats(gStats); break; }
            if (line.empty()) continue;

            // Log and save to history (skip duplicates)
            if (logger) try { logger->log(line); } catch (...) {}
            if (history.empty() || history.back() != line)
                history.push_back(line);

            istringstream ss(line);
            string cmd; ss >> cmd;
            vector<string> args; string tok;
            while (ss >> tok) args.push_back(tok);

            if (commands.count(cmd))
                commands[cmd]->execute(args);
            else {
                cout << " [os] " << line << "\n";
                system(line.c_str());           
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
