#include <iostream>
#include <thread>
#include <mutex>

std::mutex cinMutex; // Mutex to synchronize access to std::cin

void threadedFunction() {
    std::string userInput;

    // Acquire the lock before using std::cin in the thread
    {
        std::lock_guard<std::mutex> lock(cinMutex);
        std::cout << "Enter input in the threaded function: ";
        std::getline(std::cin, userInput);
    }
    
    // Process the input obtained in the thread
    std::cout << "Threaded function received: " << userInput << std::endl;
}

int main() {
    std::string userInput;

    std::thread threadObj(threadedFunction);

    // Acquire the lock before using std::cin in the main function
    {
        std::lock_guard<std::mutex> lock(cinMutex);
        std::cout << "Enter input in the main function: ";
        std::getline(std::cin, userInput);
    }

    // Process the input obtained in the main function
    std::cout << "Main function received: " << userInput << std::endl;

    threadObj.join(); // Wait for the thread to finish

    return 0;
}
