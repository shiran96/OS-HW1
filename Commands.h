#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <vector>
#include <string.h>
#include <fstream>

#define COMMAND_ARGS_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)

class Command {
// TODO: Add your data members
 public:
 
  std::vector<std::string> words;
  Command(const char* cmd_line);
  virtual ~Command() {}
  virtual void execute(std::ostream& out) = 0;
  //virtual void prepare();
  //virtual void cleanup();
  // TODO: Add your extra methods if needed
};

class BuiltInCommand : public Command {
 public:
  BuiltInCommand(const char* cmd_line) : Command(cmd_line) {}
  virtual ~BuiltInCommand() {}
};

class ExternalCommand : public Command {
 public:
  ExternalCommand(const char* cmd_line) : Command(cmd_line) {}
  virtual ~ExternalCommand() {}
  void execute(std::ostream& out) override;
};

class RedirectionCommand : public Command {
 public:
 bool valid;
 const char* dest_file;
 const char* wtype;
 std::ostream* outputStream = nullptr;
  RedirectionCommand(const char* cmd_line);
  ~RedirectionCommand();
  void execute(std::ostream& out) override {};
  std::ostream* prepare(); 
};

class PipeCommand : public Command {
  // TODO: Add your data members
 public:
  PipeCommand(const char* cmd_line);
  virtual ~PipeCommand() {}
  void execute(std::ostream& out) override;
};

class ChangeDirCommand : public BuiltInCommand {
  public: 
    char ** last;
    std::string dest;
// TODO: Add your data members public:
    ChangeDirCommand(const char* cmd_line, char** plastPwd);
    virtual ~ChangeDirCommand() {}
    void execute(std::ostream& out) override;
};

class GetCurrDirCommand : public BuiltInCommand {
 public:
  GetCurrDirCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {}
  virtual ~GetCurrDirCommand() {}
  void execute(std::ostream& out) override;
};

class ShowPidCommand : public BuiltInCommand {
 public:
  ShowPidCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {}
  virtual ~ShowPidCommand() {}
  void execute(std::ostream& out) override;
};

class JobsList;
class QuitCommand : public BuiltInCommand {
  public:
// TODO: Add your data members public:
    QuitCommand(const char* cmd_line, JobsList* jobs) : BuiltInCommand(cmd_line) {}
    virtual ~QuitCommand() {}
    void execute(std::ostream& out) override;
};




class JobsList {
 public:
  class JobEntry {
   // TODO: Add your data members
  };
 // TODO: Add your data members
 public:
  JobsList();
  ~JobsList();
  void addJob(Command* cmd, bool isStopped = false);
  void printJobsList();
  void killAllJobs();
  void removeFinishedJobs();
  JobEntry * getJobById(int jobId);
  void removeJobById(int jobId);
  JobEntry * getLastJob(int* lastJobId);
  JobEntry *getLastStoppedJob(int *jobId);
  // TODO: Add extra methods or modify exisitng ones as needed
};

class JobsCommand : public BuiltInCommand {
 // TODO: Add your data members
 public:
  JobsCommand(const char* cmd_line, JobsList* jobs);
  virtual ~JobsCommand() {}
  void execute(std::ostream& out) override;
};

class KillCommand : public BuiltInCommand {
 // TODO: Add your data members
 public:
  KillCommand(const char* cmd_line, JobsList* jobs);
  virtual ~KillCommand() {}
  void execute(std::ostream& out) override;
};

class ForegroundCommand : public BuiltInCommand {
 // TODO: Add your data members
 public:
  ForegroundCommand(const char* cmd_line, JobsList* jobs);
  virtual ~ForegroundCommand() {}
  void execute(std::ostream& out) override;
};

class ChmodCommand : public BuiltInCommand {
 public:
  int perm;
  bool valid;
  const char* dest_file;
  ChmodCommand(const char* cmd_line);
  virtual ~ChmodCommand() {}
  void execute(std::ostream& out) override;
};


class SmallShell {
 private:
  // TODO: Add your data members
  SmallShell();
 public:
  char* last_pwd;
  Command *CreateCommand(const char* cmd_line);
  SmallShell(SmallShell const&)      = delete; // disable copy ctor
  void operator=(SmallShell const&)  = delete; // disable = operator
  static SmallShell& getInstance() // make SmallShell singleton
  {
    static SmallShell instance; // Guaranteed to be destroyed.
    // Instantiated on first use.
    return instance;
  }
  ~SmallShell();
  void executeCommand(const char* cmd_line);
  // TODO: add extra methods as needed
};

#endif //SMASH_COMMAND_H_
