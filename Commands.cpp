#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include "Commands.h"
#include <sys/stat.h>
#include <fcntl.h>

using namespace std;

#if 0
#define FUNC_ENTRY()  \
  cstd::cout << __PRETTY_FUNCTION__ << " --> " << endl;

#define FUNC_EXIT()  \
  cstd::cout << __PRETTY_FUNCTION__ << " <-- " << endl;
#else
#define FUNC_ENTRY()
#define FUNC_EXIT()
#define WHITESPACE " \n\r\t\f\v"
#define CMD_MAX_LEN 200
#endif

string _ltrim(const std::string& s)
{
  size_t start = s.find_first_not_of(WHITESPACE);
  return (start == std::string::npos) ? "" : s.substr(start);
}

string _rtrim(const std::string& s)
{
  size_t end = s.find_last_not_of(WHITESPACE);
  return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

string _trim(const std::string& s)
{
  return _rtrim(_ltrim(s));
}

int _parseCommandLine(const char* cmd_line, char** args) {
  FUNC_ENTRY()
  int i = 0;
  std::istringstream iss(_trim(string(cmd_line)).c_str());
  for(std::string s; iss >> s; ) {
    args[i] = (char*)malloc(s.length()+1);
    memset(args[i], 0, s.length()+1);
    strcpy(args[i], s.c_str());
    args[++i] = NULL;
  }
  return i;

  FUNC_EXIT()
}

bool _isBackgroundComamnd(const char* cmd_line) {
  const string str(cmd_line);
  return str[str.find_last_not_of(WHITESPACE)] == '&';
}

void _removeBackgroundSign(char* cmd_line) {
  const string str(cmd_line);
  // find last character other than spaces
  unsigned int idx = str.find_last_not_of(WHITESPACE);
  // if all characters are spaces then return
  if (idx == string::npos) {
    return;
  }
  // if the command line does not end with & then return
  if (cmd_line[idx] != '&') {
    return;
  }
  // replace the & (background sign) with space and then remove all tailing spaces.
  cmd_line[idx] = ' ';
  // truncate the command line string up to the last non-space character
  cmd_line[str.find_last_not_of(WHITESPACE, idx) + 1] = 0;
}

// TODO: Add your implementation for classes in Commands.h 
Command::Command(const char* cmd_line) {
  this->std_fd = dup(1);
  string temp = string(cmd_line);
  stringstream ss(temp);
  string buffer;
  while (ss >> buffer) {
    this->words.push_back(buffer);
  }

  // Find if need to redirect output
  this->redirect = this->words.size() > 2;
  const char* dest_file;
  int wtype;
  if (this->redirect) {
    dest_file = this->words[this->words.size()-1].c_str();
    if (this->words[this->words.size()-2].compare(">") == 0) {
      wtype = O_WRONLY|O_CREAT|O_TRUNC;
    }
    else if (this->words[this->words.size()-2].compare(">>") == 0) {
      wtype = O_WRONLY|O_APPEND|O_CREAT;
    }
    else {
      this->redirect = false;
    }
  }

  // Redirect stdout
  if (this->redirect) {
    int perm = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
    this->fd = open(dest_file, wtype, perm);
    if (this->fd == -1) {
      perror("smash error: open: failed");
      this->redirect = false;
    }
    else {
      close(1);
      if (dup2(this->fd, 1)==-1) {
        perror("smash error: dup2: failed");
        close(this->fd);
        dup2(this->std_fd, 1);
        this->redirect = false;
      }
    }
  }
  
  // Background check
  std::string last = this->words[this->words.size() - 1];
  this->background = *(last.end()-1) == '&';
  if (this->background) {
    if (last.compare("&")==0) {
      this->words.pop_back();
    }
    else {
      this->words[this->words.size() - 1] = last.substr(0, last.size()-1); // remove it from command
    }
  }
}
Command::~Command() {
  if (this->redirect) {
    close(this->fd);
    dup2(this->std_fd, 1);
  }
}

ChangeDirCommand::ChangeDirCommand(const char* cmd_line, char** plastPwd) : BuiltInCommand(cmd_line) {
  this->dest = this->words[1];
  this->last = plastPwd;
}

void ChangeDirCommand::execute() {
  char* current = getcwd(NULL, 0);
  if (this->words.size() > 2)
    std::cerr << "smash error: cd: too many arguments\n";
  else if (this->dest.compare("-")==0) {
    if (string(*(this->last)).compare("-")==0) {
      std::cerr << "smash error: cd: OLDPWD not set\n";
    }
    else if (chdir(*(this->last)) == -1)
      perror("smash error: chdir: failed");
    else {
      delete *(this->last);
      *(this->last) = current;
    }
  }
  else if (chdir(this->dest.c_str()) == -1)
    perror("smash error: chdir: failed");
  else {
    delete *(this->last);
    *(this->last) = current;
  }
}

ChmodCommand::ChmodCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {
  try {
    if (this->words.size() == 3) {
      this->dest_file = this->words[2].c_str();
      this->perm = std::stoi(this->words[1], 0, 8);
      this->valid = true;
    }
    else {
      this->valid = false;
    }
  }
  catch(...) {
    this->valid = false;
  }
}

void ChmodCommand::execute() {
  if (!this->valid) {
    std::cerr << "smash error: chmod: invalid arguments\n";
  }
  else if (chmod(this->dest_file, this->perm) == -1) {
    perror("smash error: chmod: failed");
  }
}

void QuitCommand::execute() {
  exit(0);
}

void ShowPidCommand::execute() {
  int p=getpid();
  if (p==-1) {
    perror("smash error: getpid: failed");
  }
  else {
    std::cout << "smash pid is " << getpid() << "\n";
  }
}

void GetCurrDirCommand::execute() {
  const char* cwd = getcwd(NULL, 0);
  if (cwd == NULL) {
    perror("smash error: getcwd: failed");
  }
  else {
    std::cout << cwd << "\n";
    delete cwd;
  }
}

ExternalCommand::ExternalCommand(const char* cmd_line) : Command(cmd_line){
  // Get full command (no redirection)
  std::string full_cmd = string(cmd_line);
  if (this->redirect) 
    full_cmd = full_cmd.substr(0, full_cmd.find(" >"));

  // Find if need /bin/bash
  this->complex = false;
  this->complex |= full_cmd.find('?') != std::string::npos;
  this->complex |= full_cmd.find('*') != std::string::npos;
  if (this->complex) {
    this->command = string("/bin/bash");
  }
  else {
    this->command = this->words[0];
  }

  // Find the argvs
  int argc = !this->complex ? this->words.size() - 2 * this->redirect : 3;
  if (!this->complex) { // normal external
    int argc = this->words.size() - 2 * this->redirect;
    this->argv = new char*[argc+1];
    for (int i=0; i < argc; i++) {
      this->argv[i] = new char[this->words[i].size()+1];
      strcpy(this->argv[i], this->words[i].c_str());
    }
  }
  else {
    this->argv = new char*[4]; // bash -c "command"
    this->argv[0] = new char[10]; // "/bin/bash" + null terminator
    strcpy(this->argv[0], "/bin/bash");
    this->argv[1] = new char[3]; // "-c" + null terminator
    strcpy(this->argv[1], "-c");
    this->argv[2] = new char[full_cmd.size()+1];
    strcpy(this->argv[2], full_cmd.c_str());
  }
  this->argv[argc] = nullptr;
}

void ExternalCommand::execute() {
  int p=fork();
  if (p == -1) { // failed
    perror("smash error: fork: failed");
  } // parent
  else if (p==0) {
    execvp(this->command.c_str(), this->argv);
    perror("smash error: execvp: failed");
    exit(1);
  }
  else {
    if (!this->background) {
      waitpid(p, nullptr, 0);
    }
    else {
      // add to jobs
    }
  }
}

ExternalCommand::~ExternalCommand() {
  int argc = (!this->complex) ? (this->words.size() - 2 * this->redirect) : 3;
  for (int i=0; i<argc; i++) {
    delete[] this->argv[i];
  }
  delete[] this->argv;
}

SmallShell::SmallShell() {
// TODO: add your implementation
  this->last_pwd = new char('-');
}

SmallShell::~SmallShell() {
// TODO: add your implementation
  delete this->last_pwd;
}
/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command * SmallShell::CreateCommand(const char* cmd_line) {
  // Get each word individually
  string cmd_s = _trim(string(cmd_line));
  string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));

  if (firstWord.compare("showpid") == 0 || firstWord.compare("showpid&") == 0) {
    return new ShowPidCommand(cmd_line);
  }
  else if (firstWord.compare("pwd") == 0 || firstWord.compare("pwd&") == 0) {
    return new GetCurrDirCommand(cmd_line);
  }
  else if (firstWord.compare("cd") == 0) {
      return new ChangeDirCommand(cmd_line, &(this->last_pwd));
  }
  else if (firstWord.compare("quit") == 0 ||firstWord.compare("quit&") == 0) {
    return new QuitCommand(cmd_line, NULL);
  }
  else if (firstWord.compare("chmod") == 0) {
    return new ChmodCommand(cmd_line);
  }
  else {
    return new ExternalCommand(cmd_line);
  }
}

void SmallShell::executeCommand(const char *cmd_line) {
  // TODO: Add your implementation here
  // for example:
  /*RedirectionCommand rd = RedirectionCommand(_trim(string(cmd_line)).c_str());
  std::ostream* std::cout = rd.prepare();*/
  Command* cmd = CreateCommand(cmd_line);
  cmd->execute();
  delete cmd;
  //delete rd;
  // Please note that you must fork smash process for some commands (e.g., external commands....)
}