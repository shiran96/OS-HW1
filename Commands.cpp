#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include <fstream>
#include "Commands.h"
#include <sys/stat.h>

using namespace std;

#if 0
#define FUNC_ENTRY()  \
  cout << __PRETTY_FUNCTION__ << " --> " << endl;

#define FUNC_EXIT()  \
  cout << __PRETTY_FUNCTION__ << " <-- " << endl;
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
  string temp = string(cmd_line);
  stringstream ss(temp);
  string buffer;
  while (ss >> buffer) {
    this->words.push_back(buffer);
  }
}
RedirectionCommand::RedirectionCommand(const char* cmd_line) : Command(cmd_line) {
  this->valid = this->words.size() > 2;
  if (this->valid) {
    this->dest_file = this->words[this->words.size()-1].c_str();
    if (this->words[this->words.size()-2].compare(">") == 0) {
      this->wtype = "w";
    }
    else if (this->words[this->words.size()-2].compare(">>") == 0) {
      this->wtype = "a";
    }
    else {
      this->valid = false;
    }
  }
}
std::ostream* RedirectionCommand::prepare() {
  if (this->valid) {
    if (*this->wtype=='a')
    {
      this->outputStream = new std::ofstream(this->dest_file, std::ios::app);
    }
    else {
      this->outputStream = new std::ofstream(this->dest_file);
    }
    return outputStream;
  }
  else {
    return &std::cout;
  }
}
RedirectionCommand::~RedirectionCommand() {
  if (this->valid) {
    if (this->outputStream != nullptr) {
      delete outputStream;
    }
  }
}
ChangeDirCommand::ChangeDirCommand(const char* cmd_line, char** plastPwd) : BuiltInCommand(cmd_line) {
  this->dest = this->words[1];
  this->last = plastPwd;
}

void ChangeDirCommand::execute(std::ostream& out) {
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

void ChmodCommand::execute(std::ostream& out) {
  if (!this->valid) {
    std::cerr << "smash error: chmod: invalid arguments\n";
  }
  else if (chmod(this->dest_file, this->perm) == -1) {
    perror("smash error: chmod: failed");
  }
}

void QuitCommand::execute(std::ostream& out) {
  exit(0);
}

void ShowPidCommand::execute(std::ostream& out) {
  int p=getpid();
  if (p==-1) {
    perror("smash error: getpid: failed");
  }
  else {
    out << "smash pid is " << getpid() << "\n";
  }
}

void GetCurrDirCommand::execute(std::ostream& out) {
  const char* cwd = getcwd(NULL, 0);
  if (cwd == NULL) {
    perror("smash error: getcwd: failed");
  }
  else {
    out << cwd << "\n";
    delete cwd;
  }
}

void ExternalCommand::execute(std::ostream& out) {
  /*int p=fork();
  // Assumes not in background, make when a child is done send a signal to main process?
  if (p == 0) {
    if (this->words.size() > 1)
    {
      char** args = new char*[this->words.size()-1];
      for (string arg : std::vector<string>(this->words.begin()+1, this->words.end()))
      {
        dup2()
      }
      delete args;
    }
    else {
    }
    exit(0);
  }
  else {
    int* ret_val;
    waitpid(p, ret_val, 0);
  }*/
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

  if (firstWord.compare("showpid") == 0) {
    return new ShowPidCommand(cmd_line);
  }
  else if (firstWord.compare("pwd") == 0) {
    return new GetCurrDirCommand(cmd_line);
  }
  else if (firstWord.compare("cd") == 0) {
      return new ChangeDirCommand(cmd_line, &(this->last_pwd));
  }
  else if (firstWord.compare("quit") == 0) {
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
  RedirectionCommand rd = RedirectionCommand(_trim(string(cmd_line)).c_str());
  std::ostream* out = rd.prepare();
  Command* cmd = CreateCommand(cmd_line);
  cmd->execute(*out);
  //delete rd;
  // Please note that you must fork smash process for some commands (e.g., external commands....)
}