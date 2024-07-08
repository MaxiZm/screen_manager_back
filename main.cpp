#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <csignal>
#include <fstream>

// Signal handler to catch termination signals
void signalHandler(int signum) {
    std::cout << "Daemon exiting..." << std::endl;
    exit(signum);
}

pid_t sigma_fork(){
    pid_t pid = fork();

    // If we got a good PID, then we can exit the parent process.
    if (pid > 0) {
        exit(EXIT_SUCCESS);
    }

    // On error fork returns -1.
    if (pid < 0) {
        exit(EXIT_FAILURE);
    }

    return pid;
}

void daemonize() {
    pid_t pid;

    // Fork the process
    pid = sigma_fork();

    // Create a new session
    if (setsid() < 0) {
        exit(EXIT_FAILURE);
    }

    // Fork again to ensure the daemon cannot acquire a terminal again
    pid = sigma_fork();

    // Change the file mode mask
    umask(0);

    // Change the working directory to the root directory
    if (chdir("/") < 0) {
        exit(EXIT_FAILURE);
    }

    // Close standard file descriptors
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    // Open a log file in write mode.
    std::ofstream logFile("/tmp/daemon.log", std::ios_base::out | std::ios_base::app);
    if (!logFile.is_open()) {
        exit(EXIT_FAILURE);
    }

    logFile << "Daemon started successfully" << std::endl;
}

int main() {
    // Signal handling
    signal(SIGTERM, signalHandler);
    signal(SIGINT, signalHandler);

    daemonize();

    // Daemon main loop
    while (true) {
        // Daemon's tasks
        std::ofstream logFile("/tmp/daemon.log", std::ios_base::out | std::ios_base::app);
        if (logFile.is_open()) {
            logFile << "Daemon is running..." << std::endl;
            logFile.close();
        }
        sleep(30); // Sleep for 30 seconds
    }

    return 0;
}
