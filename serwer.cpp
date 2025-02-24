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
#include <unordered_map>

struct Card {
  std::string suit;
  std::string rank;
};

class DealerServer {
  private:
    int server_fd;
    struct sockaddr_in address;
    int addrlen;
    std::vector<Card> deck;
    std::map<int, int> playerBets;
    std::map<int, Card> playerCards;
    std::vector<Card> tableCards;

    void initializeDeck() {
      const std::vector<std::string> suits = {"Hearts", "Diamonds", "Clubs", "Spades"};
      const std::vector<std::string> ranks = {"2", "3", "4", "5", "6", "7", "8", "9", "10", "Jack", "Queen", "King", "Ace"};
        
      deck.clear();
      for (const auto& suit : suits) {
        for (const auto& rank : ranks) {
          deck.push_back({suit, rank});
        }
      }
    }

    void shuffleDeck() {
      std::random_device rd;
      std::mt19937 g(rd());
      std::shuffle(deck.begin(), deck.end(), g);
    }

    Card dealCard() {
      if (deck.empty()) {
          throw std::runtime_error("Deck is empty!");
      }
      Card card = deck.back();
      deck.pop_back();
      return card;
    }

    int getCardValue(const std::string& rank) {
      if (rank == "2") return 2;
      if (rank == "3") return 3;
      if (rank == "4") return 4;
      if (rank == "5") return 5;
      if (rank == "6") return 6;
      if (rank == "7") return 7;
      if (rank == "8") return 8;
      if (rank == "9") return 9;
      if (rank == "10") return 10;
      if (rank == "Jack") return 11;
      if (rank == "Queen") return 12;
      if (rank == "King") return 13;
      if (rank == "Ace") return 14;
      return 0;
    }

    void sendCard(int client_socket, const Card& card) {
      std::string cardData = card.rank + " of " + card.suit + "\n";
      send(client_socket, cardData.c_str(), cardData.size(), 0);
      playerCards[client_socket] = card;
      char buffer[1024];
      memset(buffer, 0, sizeof(buffer));
      recv(client_socket, buffer, sizeof(buffer), 0);
      std::cout << "Received card confirmation from " << client_socket << "\n";
    }

    void collectBet(int client_socket) {
      char buffer[1024];
      memset(buffer, 0, sizeof(buffer));
      
      std::cout << "Waiting for bet from ", client_socket, "...\n";
      send(client_socket, "Enter your bet: ", 16, 0);
  
      int bytesReceived = recv(client_socket, buffer, sizeof(buffer), 0);
      if (bytesReceived <= 0) {
        std::cerr << "Error receiving bet from " << client_socket << "\n";
      }
      int bet = std::stoi(buffer);
      playerBets[client_socket] = bet;
      std::cout << "Received bet: " << bet << " from " << client_socket << "\n";
    }

    std::string getCardSuit(const Card& card) {
      return card.suit;
    }

    bool isStraight(std::vector<int>& values) {
      std::sort(values.begin(), values.end());
      for (size_t i = 1; i < values.size(); ++i) {
        if (values[i] != values[i-1] + 1) {
          return false;
        }
      }
      return true;
    }

    bool isFlush(const std::vector<Card>& hand) {
      std::string suit = getCardSuit(hand[0]);
      for (const auto& card : hand) {
        if (getCardSuit(card) != suit) {
          return false;
        }
      }
      return true;
    }

    std::unordered_map<int, int> countValues(const std::vector<Card>& hand) {
      std::unordered_map<int, int> valueCount;
      for (const auto& card : hand) {
        int value = getCardValue(card.rank);
        valueCount[value]++;
      }
      return valueCount;
    }

    int evaluateHand(const std::vector<Card>& hand) {
      std::vector<int> values;
      for (const auto& card : hand) {
        values.push_back(getCardValue(card.rank));
      }
  
      std::unordered_map<int, int> valueCount = countValues(hand);
      bool flush = isFlush(hand);
      bool straight = isStraight(values);
  
      if (flush && straight) {
        if (std::count(values.begin(), values.end(), 10) && std::count(values.begin(), values.end(), 14)) {
          return 10;  // Royal Flush
        }
        return 9;  // Straight Flush
      }
  
      for (const auto& [value, count] : valueCount) {
        if (count == 4) {
          return 8;  // Four of a Kind
        }
      }
  
      bool threeOfAKind = false;
      bool pair = false;
      for (const auto& [value, count] : valueCount) {
        if (count == 3) {
          threeOfAKind = true;
        }
        else if (count == 2) {
          pair = true;
        }
      }
      if (threeOfAKind && pair) {
        return 7;  // Full House
      }
  
      if (flush) {
        return 6;  // Flush
      }
  
      if (straight) {
        return 5;  // Straight
      }
  
      if (threeOfAKind == true) {
        return 4;  // Three of a Kind
      }
  
      if (std::count(values.begin(), values.end(), 2) == 2) {
        return 3;  // Two Pair
      }
  
      if (pair == true) {
        return 2;  // Pair
      }
  
      return 1;  // High Card
    }

    std::vector<int> determineWinner(const std::vector<int>& client_sockets, const std::map<int, Card>& playerCards, const std::vector<Card>& tableCards) {
      int highestValue = 0;
      std::vector<int> winners;
  
      for (int socket : client_sockets) {
        std::vector<Card> hand = { playerCards.at(socket) };
        hand.insert(hand.end(), tableCards.begin(), tableCards.end());
          
        int handValue = evaluateHand(hand);
          
        if (handValue > highestValue) {
          highestValue = handValue;
          winners = { socket };
        }
        else if (handValue == highestValue) {
          winners.push_back(socket);
        }
      }
      
      if (winners.size() == 1) {
        std::cout << "The winner is player: " << winners[0] << "\n";
      }
      else {
        std::cout << "It's a draw between players: ";
        for (int winner : winners) {
          std::cout << winner << " ";
        }
        std::cout << "\n";
      }
      
      return winners;
    }

    void distributeWinnings(const std::vector<int>& winners, const std::vector<int>& client_sockets) {
      int totalPot = 0;
      for (const auto& [player, bet] : playerBets) {
        totalPot += bet;
      }
    
      if (totalPot == 0) {
        std::string noWinnings = "No bets were placed, so no winnings to distribute.\n";
        for (int client : client_sockets) {
          send(client, noWinnings.c_str(), noWinnings.size(), 0);
        }
        return;
      }
  
      if (!winners.empty()) {
        if (winners.size() == 1) {
          std::string winnings = "You won: " + std::to_string(totalPot) + "\n";
          send(winners[0], winnings.c_str(), winnings.size(), 0);
        }
        else {
          int splitPot = totalPot / winners.size();
          for (int winner : winners) {
            std::string winnings = "It's a tie! You receive: " + std::to_string(splitPot) + "\n";
            send(winner, winnings.c_str(), winnings.size(), 0);
          }
        }

        for (int client : client_sockets) {
          if (std::find(winners.begin(), winners.end(), client) == winners.end()) {
            std::string lostMessage = "You lost. Better luck next time!\n";
            send(client, lostMessage.c_str(), lostMessage.size(), 0);
          }
        }

      }
      else {
        std::string noWinner = "No winner determined.\n";
        for (int client : client_sockets) {
          send(client, noWinner.c_str(), noWinner.size(), 0);
        }
      }
      char buffer[1024];
      for (int client : client_sockets) {
        memset(buffer, 0, sizeof(buffer));
        recv(client, buffer, sizeof(buffer), 0);
        std::cout << "Received winnings confirmation from " << client << "\n";
      }
    }
  
  public:
    DealerServer(int port) {
      server_fd = socket(AF_INET, SOCK_STREAM, 0);
      if (server_fd == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
      }

      address.sin_family = AF_INET;
      address.sin_addr.s_addr = INADDR_ANY;
      address.sin_port = htons(port);
      addrlen = sizeof(address);

      if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
      }

      if (listen(server_fd, 3) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
      }

      std::cout << "Dealer server running on port " << port << "...\n";

      initializeDeck();
      shuffleDeck();
    }
    
    void acceptConnections() {
      std::vector<int> client_sockets;
      std::vector<int> nextRoundQueue;

      char buffer[1024];

      fd_set readfds;
      int max_sd = server_fd;

      while(true) {
        // int client_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
        // if (client_socket < 0) {
        //   perror("Accept failed");
        //   exit(EXIT_FAILURE);
        // }

        struct timeval timeout;
        timeout.tv_sec = 5;  // 5 seconds
        timeout.tv_usec = 0; // 0 microseconds

        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(server_fd, &readfds);

        int max_sd = server_fd;

        // Wait for an event on the server_fd with a 5-second timeout
        int activity = select(max_sd + 1, &readfds, NULL, NULL, &timeout);

        if (activity < 0 && errno != EINTR) {
          perror("Select error");
          exit(EXIT_FAILURE);
        } else if (activity == 0) {
          std::cout << "No incoming connections after 5 seconds. Continuing...\n";
          //return;  // No incoming connection, just return
        }

        if (FD_ISSET(server_fd, &readfds)) {
          // socklen_t addrlen = sizeof(address);
          int client_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
          if (client_socket < 0) {
            perror("Accept failed");
            return;
          }
          std::cout << "Player " << client_socket << " connected.\n";
          client_sockets.push_back(client_socket);
        }

        // std::cout << "Player " << client_socket << " connected.\n";
        // client_sockets.push_back(client_socket);
      
        
        if (client_sockets.size() >= 2) {
          std::cout << "Players connected, starting the game...\n";
          for(int client_socket : client_sockets) {
            send(client_socket, "Round starting...\n", 18, 0);
            memset(buffer, 0, sizeof(buffer));
            recv(client_socket, buffer, sizeof(buffer), 0);
            std::cout << "Received round start confirmation from " << client_socket << "\n";
          }
          
          for (int client_socket : client_sockets) {
            memset(buffer, 0, sizeof(buffer));
            send(client_socket, "Table cards: \n", 14, 0);
            read(client_socket, buffer, sizeof(buffer));
          }
          for (int i = 0; i < 3; i++) {
            Card card = dealCard();
            tableCards.push_back(card);
            std::cout << "Dealt table card: " << card.rank << " of " << card.suit << "\n";
            for (int client_socket : client_sockets) {
              sendCard(client_socket, card);
            }
          }

          for (int client_socket : client_sockets) {
            memset(buffer, 0, sizeof(buffer));
            send(client_socket, "Your cards: \n", 13, 0);
            read(client_socket, buffer, sizeof(buffer));
          }
          for (int i = 0; i < 2; i++) {
            for (int client_socket : client_sockets) {
              Card card = dealCard();
              std::cout << "Dealt card to player " << client_socket << ": " << card.rank << " of " << card.suit << "\n";
              sendCard(client_socket, card);
            }
          }

          for(int client_socket : client_sockets) {
            collectBet(client_socket);
          }

          std::vector<int> winners = determineWinner(client_sockets, playerCards, tableCards);

          distributeWinnings(winners, client_sockets);

          for (int client_socket : client_sockets) {
            std::cout << "Asking player " << client_socket << " if they want to continue...\n";
            send(client_socket, "Continue? (y/n): ", 17, 0);
            char buffer[1];
            memset(buffer, 0, sizeof(buffer));
            recv(client_socket, buffer, sizeof(buffer), 0);
            if (buffer[0] == 'y') {
              nextRoundQueue.push_back(client_socket);
              std::cout << "Player " << client_socket << " wants to continue.\n";
            }
            else {
              close(client_socket);
            }
          }

          client_sockets.clear();
          client_sockets.insert(client_sockets.end(), nextRoundQueue.begin(), nextRoundQueue.end());
          nextRoundQueue.clear();
        }
      }
    }
};

int main() {
  DealerServer server(8080);
  server.acceptConnections();
  return 0;
}