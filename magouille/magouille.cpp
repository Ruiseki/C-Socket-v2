#include <iostream>
#include <thread>
#include <vector>
#include <string>
#include <chrono>
#include <algorithm>
#include <windows.h>


// Function to get the console height
int getConsoleHeight() {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    return csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
}

// Function to go the the line y at the character x
void gotoxy(int x, int y) {
    COORD coord;
    coord.X = x;
    coord.Y = y;
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
}

// Function for the reader thread
void readerThread(std::vector<std::string>& messages, int maxLines) {
    std::vector<std::string> allMessages;

    while (true) {
        if(messages.empty()) continue;

        for(std::string m : messages) {
            allMessages.push_back(m);
        }

        messages.clear();

        int allowedLines = maxLines - 4;
        int startIndex = allowedLines < allMessages.size() ? allMessages.size() - allowedLines : 0;

        for(int lineIndex = 0; (lineIndex + startIndex) < allMessages.size(); lineIndex++) {
            gotoxy(0, lineIndex);
            std::cout << "\033[2K\r" << allMessages[lineIndex + startIndex];
        }

        gotoxy(0,maxLines - 2);

        std::this_thread::sleep_for(std::chrono::milliseconds(100));  // Adjust the delay as needed
    }
}


int main() {
    const int maxLines = getConsoleHeight();  // Automatically determine max from console height
    std::vector<std::string> messages;

    system("cls");
    // Start the reader thread
    std::thread reader(readerThread, std::ref(messages), maxLines);

    while (true) {
        // Get user input
        std::string userInput;
        gotoxy(0,maxLines - 2);
        std::getline(std::cin, userInput);

        // Clear the line
        gotoxy(0,maxLines - 2);
        std::cout << "\033[2K\r";

        // Push user input to messages
        messages.push_back(userInput);

        // You can add additional logic to break the loop or perform other actions based on user input
    }

    // Join the reader thread
    reader.join();

    return 0;
}
