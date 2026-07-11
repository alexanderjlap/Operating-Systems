#include <iostream>
#include <vector>
#include <string>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>

#define MAX_ARGUMENTS 32

struct ChildProcess {
    int processID;
    std::string commandString;
    long startTime;
};

// Global variable for the shell prompt
std::string shellPrompt = "CustomShell"; // Default shell prompt

void printProcessInfo(const char* info, long value) {
    std::cout << "INFO: " << info << " = " << value << std::endl;
}

void printChildProcessInfo(long startMillis) {
    struct rusage childResourceUsage;
    struct timeval endTime;
    gettimeofday(&endTime, nullptr);
    long endMillis = endTime.tv_sec * 1000 + endTime.tv_usec / 1000;
    getrusage(RUSAGE_CHILDREN, &childResourceUsage);
    printProcessInfo("User CPU Time (ms)", childResourceUsage.ru_utime.tv_sec * 1000 + childResourceUsage.ru_utime.tv_usec / 1000);
    printProcessInfo("System CPU Time (ms)", childResourceUsage.ru_stime.tv_sec * 1000 + childResourceUsage.ru_stime.tv_usec / 1000);
    printProcessInfo("Wall Clock Time (ms)", endMillis - startMillis);
    printProcessInfo("Max Resident Set Size (bytes)", childResourceUsage.ru_maxrss);
    printProcessInfo("Page Faults", childResourceUsage.ru_majflt);
    printProcessInfo("Page Reclaims", childResourceUsage.ru_minflt);
    printProcessInfo("Voluntary Context Switches", childResourceUsage.ru_nvcsw);
    printProcessInfo("Involuntary Context Switches", childResourceUsage.ru_nivcsw);
}

bool executeBuiltInCommand(const char* command, std::vector<ChildProcess>& childProcesses) {
    if (strcmp(command, "exit") == 0) {
        std::cout << "Exiting the shell..." << std::endl;
        for (size_t i = 0; i < childProcesses.size(); i++) {
            int processStatus;
            pid_t result = waitpid(childProcesses[i].processID, &processStatus, WNOHANG);
            if (result > 0) {
                std::cout << "[Process] " << childProcesses[i].processID << ": " << childProcesses[i].commandString << " [Finished]" << std::endl;
                printChildProcessInfo(childProcesses[i].startTime);
            }
        }
        exit(0);
        return true;
    } else if (strncmp(command, "set prompt =", 12) == 0) {
        const char* newPrompt = command + 13;
        shellPrompt = newPrompt;
        std::cout << "Updated shell prompt to: " << shellPrompt << std::endl;
        return true;
    }
    return false;
}

void executeCommand(const char* command, std::vector<ChildProcess>& childProcesses) {
    int processID;
    struct timeval start;
    gettimeofday(&start, nullptr);
    long startMillis = start.tv_sec * 1000 + start.tv_usec / 1000;
    char* arguments[MAX_ARGUMENTS];
    char* commandStringCopy = strdup(command);
    if (commandStringCopy == nullptr) {
        perror("strdup");
        exit(1);
    }
    int argCount = 0;
    int backgroundFlag = 0;
    char* token = strtok(commandStringCopy, " ");
    while (token != nullptr && argCount < MAX_ARGUMENTS - 1) {
        if (strncmp(token, "&", 1) == 0) {
            std::cout << "Running in the background." << std::endl;
            backgroundFlag = 1;
        } else {
            arguments[argCount++] = token;
        }
        token = strtok(nullptr, " ");
    }
    arguments[argCount] = nullptr;
    if (backgroundFlag != 1) {
        if ((processID = fork()) < 0) {
            perror("fork");
            exit(1);
        } else if (processID == 0) {
            if (execvp(arguments[0], arguments) < 0) {
                perror("execvp");
                exit(1);
            }
        } else {
            wait(nullptr);
            printChildProcessInfo(startMillis);
        }
    } else {
        if ((processID = fork()) < 0) {
            perror("fork");
            exit(1);
        } else if (processID == 0) {
            if (execvp(arguments[0], arguments) < 0) {
                perror("execvp");
                exit(1);
            }
        } else {
            ChildProcess processInfo = {processID, command, startMillis};
            childProcesses.push_back(processInfo);
            std::cout << "[Background Process] " << childProcesses.size() << ": " << childProcesses.back().processID << " " << childProcesses.back().commandString << std::endl;
        }
    }
    free(commandStringCopy);
}

int main(int argc, char **argv) {
    if (argc == 1) {
        std::cout << shellPrompt << std::endl;
        std::vector<ChildProcess> childProcesses;

        while (true) {
            for (size_t i = 0; i < childProcesses.size(); i++) {
                int processStatus;
                pid_t result = waitpid(childProcesses[i].processID, &processStatus, WNOHANG);
                if (result == 0) {
                    // Handle case where process is still running
                } else if (result < 0) {
                    perror("waitpid");
                } else {
                    std::cout << "[Process] " << i + 1 << ": " << childProcesses[i].processID << " " << childProcesses[i].commandString << " [Finished]" << std::endl;
                    printChildProcessInfo(childProcesses[i].startTime);
                    childProcesses.erase(childProcesses.begin() + i);
                }
            }
            char hostname[128];
            if (gethostname(hostname, sizeof(hostname)) != 0) {
                perror("gethostname");
                exit(1);
            }
            char currentpath[256];
            if (getcwd(currentpath, sizeof(currentpath)) == nullptr) {
                perror("getcwd");
                exit(1);
            }
            std::cout << shellPrompt << "@" << hostname << ":" << currentpath << "==> ";

            std::string command;
            getline(std::cin, command);
            if (!std::cin) {
                if (std::cin.eof()) {
                    std::cout << "Got EOF, exiting..." << std::endl;
                    for (size_t i = 0; i < childProcesses.size(); i++) {
                        int processStatus;
                        pid_t result = waitpid(childProcesses[i].processID, &processStatus, WNOHANG);
                        if (result > 0) {
                            std::cout << "[Process] " << childProcesses[i].processID << ": " << childProcesses[i].commandString << " [Finished]" << std::endl;
                            printChildProcessInfo(childProcesses[i].startTime);
                        }
                    }
                    exit(0);
                }
            }
            const char* commandCStr = command.c_str();
            if (executeBuiltInCommand(commandCStr, childProcesses)) {
                continue;
            }
            executeCommand(commandCStr, childProcesses);
        }
    }
    return 0;
}
