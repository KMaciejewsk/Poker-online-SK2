#include <iostream>
#include <vector>
#include <algorithm>
#include <random>
#include <map>
#include <ctime>
#include <cstring>
#include <thread>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

class PokerClient {
  private:
    int sockfd;
    int balance;

    void recieveCard() {
      char buffer[1024];
      memset(buffer, 0, sizeof(buffer));
      read(sockfd, buffer, 1024);
      std::cout << buffer;
      send(sockfd, "OK\n", 3, 0);
    }

    int extractValueFromMessage(const std::string& message) {
      size_t startPos = message.find(": ");  // Find the start position of the number
      if (startPos != std::string::npos) {
        startPos += 2; // Skip past ": " to reach the number
        size_t endPos = message.find("\n", startPos);  // Find the newline or end of the number
        std::string numberStr = message.substr(startPos, endPos - startPos);
        try {
          return std::stoi(numberStr);  // Convert the number string to an integer
        }
        catch (const std::invalid_argument& e) {
          std::cerr << "Invalid number in message: " << e.what() << std::endl;
        }
      }
      return 0;  // Default return value if no number is found
  }

  public:
    PokerClient(const std::string& server_ip, int server_port) : balance(1000) {
      sockfd = socket(AF_INET, SOCK_STREAM, 0);
      if (sockfd < 0) {
        std::cerr << "Error creating socket\n";
        exit(EXIT_FAILURE);
      }

      struct sockaddr_in server_addr;
      server_addr.sin_family = AF_INET;
      server_addr.sin_port = htons(server_port);
      server_addr.sin_addr.s_addr = inet_addr(server_ip.c_str());

      if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Error connecting to server\n";
        exit(EXIT_FAILURE);
      }
      std::cout << "Connected to server at " << server_ip << " on port " << server_port << "\n";
    }

    ~PokerClient() {
      close(sockfd);
    }

    void play() {
      char buffer[1024];
      
      while (1) {
        std::cout << "Your balance is: " << balance << "\n";

        memset(buffer, 0, sizeof(buffer));
        read(sockfd, buffer, 1024);
        std::cout << buffer;
        send(sockfd, "OK\n", 3, 0);

        for(int i = 0; i < 7; i++) {
          recieveCard();
        }

        int bet;
        memset(buffer, 0, sizeof(buffer));
        read(sockfd, buffer, 1024);
        std::cout << buffer;
        std::cin >> bet;
        balance -= bet;
        std::string betStr = std::to_string(bet) + "\n";
        send(sockfd, betStr.c_str(), betStr.size(), 0);

        memset(buffer, 0, sizeof(buffer));
        read(sockfd, buffer, 1024);
        std::cout << buffer;

        int value = extractValueFromMessage(buffer);
        balance += value;

        send(sockfd, "OK\n", 3, 0);

        memset(buffer, 0, sizeof(buffer));
        read(sockfd, buffer, 1024);
        std::cout << buffer;
        char response[1];
        std::cin >> response[0];
        send(sockfd, response, 1, 0);
      }
    }
};

int main(int argc, char* argv[]) {
  if (argc != 3) {
    std::cerr << "Usage: " << argv[0] << " <server_ip> <server_port>\n";
    return 1;
  }

  std::string server_ip = argv[1];
  int server_port = std::stoi(argv[2]);

  PokerClient client(server_ip, server_port);

  client.play();

  return 0;
}
