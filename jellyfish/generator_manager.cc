/*  This file is part of Jellyfish.

    Jellyfish is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Jellyfish is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Jellyfish.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <config.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>

#include <cstdlib>
#include <vector>
#include <string>
#include <map>
#include <memory>

#include <jellyfish/spawn_external.hpp>
#include <jellyfish/err.hpp>
#include <jellyfish/start_debugger.hpp>

namespace jellyfish {
std::string tmp_pipes::create_tmp_dir() {
  std::vector<const char*> prefixes;
  const char* tmpdir = getenv("TMPDIR");
  if(tmpdir)
    prefixes.push_back(tmpdir);
#ifdef P_tmpdir
  prefixes.push_back(P_tmpdir);
#endif
  prefixes.push_back(".");

  for(auto it = prefixes.begin(); it != prefixes.end(); ++it) {
    size_t len = strlen(*it) + 6 + 1;
    std::unique_ptr<char[]> tmppath(new char[len]);
    sprintf(tmppath.get(), "%sXXXXXX", *it);
    const char* res = mkdtemp(tmppath.get());
    if(res)
      return std::string(res);
  }
  eraise(std::runtime_error) << "Failed to create a temporary directory for the pipes. Set the variable TMPDIR properly";
  return "";
}

std::vector<std::string> tmp_pipes::create_pipes(const std::string& tmpdir, int nb_pipes)
{
  std::vector<std::string> pipes;
  std::string path;
  for(int i = 0; i < nb_pipes; ++i) {
    path = tmpdir + "/fifo" + std::to_string(i);
    if(mkfifo(path.c_str(), S_IRUSR|S_IWUSR) == -1)
      eraise(std::runtime_error) << "Failed to create named fifos";
    pipes.push_back(path);
  }
  return pipes;
}

void tmp_pipes::discard(int i) {
  if(pipes_[i].empty())
    return;
  // First rename the fifo so no new reader will open it, then open
  // the fifo (with its new name for writing, in non-blocking mode. If
  // we get a valid file descriptor, some readers are blocked reading
  // on the fifo: we close the fifo and free the readers. Otherwise,
  // no readers is blocked and no action is required. Finally we
  // unlink the fifo for good.
  std::string discarded_name(pipes_[i]);
  discarded_name += "_discarded";
  if(rename(pipes_[i].c_str(), discarded_name.c_str()) == -1)
    return;
  pipes_[i].clear();
  int fd = open(discarded_name.c_str(), O_WRONLY|O_NONBLOCK);
  if(fd != -1)
    close(fd);
  unlink(discarded_name.c_str());
}

void generator_manager::start()  {
  if(manager_pid_ != -1)
    return;
  manager_pid_ = fork();
  switch(manager_pid_) {
  case -1:
    eraise(std::runtime_error) << "Failed to start manager process";
    break;
  case 0:
    manager_pid_ = -1;
    start_commands(); // child start commands
    exit(EXIT_SUCCESS);
  default:
    cmds_.close();
    return;
  }
}

bool generator_manager::wait() {
  if(manager_pid_ == -1) return false;
  pid_t pid = manager_pid_;
  manager_pid_ = -1;
  int status;
  if(pid != waitpid(pid, &status, 0))
    return false;
  return WIFEXITED(status) && (WEXITSTATUS(status) == 0);
}

void generator_manager::start_one_command(const std::string& command, int pipe)
{
  cmd_info_type info = { command, pipe };
  pid_t child = fork();
  switch(child) {
  case -1:
    std::cerr << "Failed to fork. Command '" << command << "' not run" << std::endl;
    return;
  case 0:
    break;
  default:
    pid2pipe_[child] = info;
    return;
  }

  // In child
  int dev_null = open("/dev/null", O_RDONLY|O_CLOEXEC);
  if(dev_null != -1)
    dup2(dev_null, 0);
  int pipe_fd = open(pipes_[pipe], O_WRONLY|O_CLOEXEC);
  if(pipe_fd == -1) {
    std::cerr << "Failed to open output pipe. Command '" << command << "' not run" << std::endl;
    exit(EXIT_FAILURE);
  }
  if(dup2(pipe_fd, 1) == -1) {
    std::cerr << "Failed to dup pipe to stdout. Command '" << command << "' not run" << std::endl;
    exit(EXIT_FAILURE);
  }
  execl("/bin/sh", "/bin/sh", "-c", command.c_str(), (char*)0);
  std::cerr << "Failed to exec. Command '" << command << "' not run" << std::endl;
  exit(EXIT_FAILURE);
}

void generator_manager::start_commands()
{
  std::string command;
  int i;
  for(i = 0; i < pipes_.size(); ++i) {
    if(!std::getline(cmds_, command))
      break;
    start_one_command(command, i);
  }
  for( ; i < pipes_.size(); ++i)
    pipes_.discard(i);

  while(!pid2pipe_.empty()) {
    int status;
    int res = ::wait(&status);
    if(res == -1) {
      if(errno == EINTR) continue;
      break;
    }
    cmd_info_type info = pid2pipe_[res];
    pid2pipe_.erase(info.pipe);
    if(std::getline(cmds_, command)) {
      start_one_command(command, info.pipe);
    } else {
      pipes_.discard(info.pipe);
    }
    display_status(status, info.command);
  }
}

void generator_manager::display_status(int status, const std::string& command)
{
  if(WIFEXITED(status) && WEXITSTATUS(status) != 0) {
    std::cerr << "Command '" << command
              << "' exited with error status " << WEXITSTATUS(status) << std::endl;
  } else if(WIFSIGNALED(status)) {
    std::cerr << "Command '" << command
              << "' killed by signal " << WTERMSIG(status) << std::endl;
  }
}

} // namespace jellyfish
