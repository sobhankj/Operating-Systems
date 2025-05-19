#include <iostream>
#include <cstring>
#include <string>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <algorithm>
#include <fcntl.h>
#include <map>
#include <vector>
#include <csignal>

#define MAIN_PORT 8080
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 2


// this for handel timer in our room
std::map<int,timer_t>active_timers;
std::map<int,int>room_timer_map;
int current_timer_id = 0;


// this is structur of our subserver(room) it include socket number of port
// number of client client's information client's choices and flag that game start or no
struct SubServer
{
    int socket;
    int port;
    int clients_count;
    std::vector<int> clients;
    std::map<int, std::string> choices;
    bool game_started = false;
};



// this is client info that save name and his or her score
struct ClientInfo
{
    std::string name;
    int score;
};


// global varible that give us computation timer
std::vector<SubServer> subservers;
std::map<int, ClientInfo> clients;
std::map<int, ClientInfo> client_history;

// this function do something that the code be none blocking
void setNonBlocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}


// in this scope i create the server socket
int createServerSocket(int port)
{
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == 0)
    {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_socket, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("Bind failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }
    if (listen(server_socket, MAX_CLIENTS) < 0)
    {
        perror("Listen failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    setNonBlocking(server_socket);
    std::string message = "SubServer created on port: " + std::to_string(port) + "\n";
    write(STDOUT_FILENO, message.c_str(), message.size());
    return server_socket;
}


// when we want to show the rooms to client we go here
std::string getAvailableSubservers(const std::vector<SubServer> &subservers)
{
    std::string subserver_list = "Available Rooms:\n";
    for (const auto &sub : subservers)
    {
        if (sub.clients_count < MAX_CLIENTS)
        {
            subserver_list += "Port: " + std::to_string(sub.port) + "\n";
        }
    }
    subserver_list += "Enter the Room's port to connect to it:\n";
    return subserver_list;
}


// after all action that client do i show him or her score
void sendScoreToClient(int client_socket, const ClientInfo &client_info)
{
    std::string score_message = "Your current score: " + std::to_string(client_info.score) + "\n";
    send(client_socket, score_message.c_str(), score_message.size(), 0);
}


// in this scope i handel when a client go out to server
void handleClientDisconnection(int client_socket, std::map<int, ClientInfo> &clients, std::vector<SubServer> &subservers)
{
    auto client = clients.find(client_socket);
    if (client != clients.end())
    {
        std::string message = "Client disconnected, fd: " + std::to_string(client_socket) + ", name: " + client->second.name + "\n";
        client_history[client_socket] = {clients[client_socket].name , clients[client_socket].score};
        write(STDOUT_FILENO, message.c_str(), message.size());
    }

    for (auto &sub : subservers)
    {
        auto it = std::find(sub.clients.begin(), sub.clients.end(), client_socket);
        if (it != sub.clients.end())
        {
            sub.clients.erase(it);
            sub.clients_count--;
            sub.choices.erase(client_socket);
            close(client_socket);
            break;
        }
    }
}


// in this function we turn of timer
void remove_room_timer(int room_id) {
    for (const auto& entry : room_timer_map) {
        if (entry.second == room_id) {
            int timer_id = entry.first;  
            timer_t timer_to_delete = active_timers[timer_id];
            if (timer_delete(timer_to_delete) == -1) {
                perror("timer_delete");
            } else {
                std::string message = "Timer canceled for Room: " + std::to_string(room_id) + "\n";
                write(1, message.c_str(), std::strlen(message.c_str()));
            }
            room_timer_map.erase(timer_id);
            active_timers.erase(timer_id);  
            return;
        }
    }
    std::cout << "No active timer for Room: " << room_id << std::endl;
}


// i get decide about the game finish in typically way
void handleGame(SubServer &subserver, std::map<int, ClientInfo> &clients , std::vector<SubServer> &subservers)
{
    if (subserver.clients.size() != 2 || !subserver.game_started)
        return;

    auto it = subserver.clients.begin();
    int client1 = *it++;
    int client2 = *it;
    std::string choice1 = subserver.choices[client1];
    std::string choice2 = subserver.choices[client2];
    std::string message = "";

    if (choice1 != "" && choice2 != "")
    {
        
        std::cout << choice1 << "\n" << choice2 << "\n";
        if (choice1 == choice2)
        {
            send(client1, "Draw! No points awarded.\n", strlen("Draw! No points awarded.\n"), 0);
            send(client2, "Draw! No points awarded.\n", strlen("Draw! No points awarded.\n"), 0);
            message = "Choices: " + clients[client1].name + " chose " + choice1 + ", " + clients[client2].name +
                      " chose " + choice2 + "\n" + "the game between " + clients[client1].name + " and " +
                      clients[client2].name + " is Draw\n";
        }
        else if ((choice1 == "rock" && (choice2 == "scissors" || choice2 == "")) ||
                 (choice1 == "scissors" && (choice2 == "paper" || choice2 == "")) ||
                 (choice1 == "paper" && (choice2 == "rock" || choice2 == "")))
        {
            clients[client1].score++;
            send(client1, "You won! +1 point.\n", strlen("You won! +1 point.\n"), 0);
            send(client2, "You lost.\n", strlen("You lost.\n"), 0);
            message = "Choices: " + clients[client1].name + " chose " + choice1 + ", " + clients[client2].name +
                      " chose " + choice2 + "\n" + clients[client1].name + " won the game between " + clients[client1].name + " and " + clients[client2].name + '\n';
        }
        else
        {
            clients[client2].score++;
            send(client2, "You won! +1 point.\n", strlen("You won! +1 point.\n"), 0);
            send(client1, "You lost.\n", strlen("You lost.\n"), 0);
            message = "Choices: " + clients[client1].name + " chose " + choice1 + ", " + clients[client2].name +
                      " chose " + choice2 + "\n" + clients[client2].name + " won the game between " + clients[client2].name + " and " + clients[client1].name + '\n';
        }
        write(STDOUT_FILENO, message.c_str(), message.size());
        sendScoreToClient(client1, clients[client1]);
        sendScoreToClient(client2, clients[client2]);

        subserver.clients.clear();
        subserver.choices.clear();
        subserver.clients_count = 0;
        subserver.game_started = false;

        std::string subserver_list = getAvailableSubservers(subservers);
        send(client1, subserver_list.c_str(), subserver_list.size(), 0);
        send(client2, subserver_list.c_str(), subserver_list.size(), 0);
        remove_room_timer(subserver.port);
    }
}


// i cant to implement broadcast so i must to do this function
void broadcastMessageToAllClients(const std::string &message, const std::map<int, ClientInfo> &clients, const std::vector<SubServer> &subservers)
{
    for (const auto &client : clients)
    {
        send(client.first, message.c_str(), message.size(), 0);
    }
    // for (const auto &sub : subservers)
    // {
    //     for (const auto &client_sd : sub.clients)
    //     {
    //         send(client_sd, message.c_str(), message.size(), 0);
    //     }
    // }
}


// in this i handel the end game when server write end_game the games are finished
std::string end_game(const std::map<int, ClientInfo> &clients , const std::map<int,ClientInfo> &client_history) {
    std::string ret_massage = "\nRESUALTS:\n";
    for (auto const& x : client_history)
    {
        ret_massage += x.second.name + " : " + std::to_string(x.second.score) + "\n";
    }
    
    for (auto const& x : clients)
    {
        ret_massage += x.second.name + " : " + std::to_string(x.second.score) + "\n";
    }
    return ret_massage;
}


// when the game was finished with times up i handel the points in this scope
void time_gameup(SubServer &subserver, std::map<int, ClientInfo> &clients , std::vector<SubServer> subservers) {
    
    auto it = subserver.clients.begin();
    int client1 = *it++;
    int client2 = *it;

    std::string choice1 = subserver.choices[client1];
    std::string choice2 = subserver.choices[client2];
    std::string message = "";
    std::cout << choice1 << "\n" << choice2 << "\n";
    if (choice1 == "" && choice2 == "")
    {
        send(client1, "Draw NO BODY CHOOSE ANYTHING! No points awarded.\n", strlen("Draw NO BODY CHOOSE ANYTHING! No points awarded.\n"), 0);
        send(client2, "Draw NO BODY CHOOSE ANYTHING! No points awarded.\n", strlen("Draw NO BODY CHOOSE ANYTHING! No points awarded.\n"), 0);
        message = "Choices: " + clients[client1].name + " chose " + choice1 + ", " + clients[client2].name +
              " chose " + choice2 + "\n" + "the game between " + clients[client1].name + " and " +
               clients[client2].name + " is Draw NO BODY CHOOSE ANYTHING NO POINT AWARDED\n";
    }
    else if (((choice1 == "rock") && choice2 == "") ||
            ((choice1 == "scissors") && choice2 == "") ||
            ((choice1 == "paper") && choice2 == ""))
    {
        clients[client1].score++;
        send(client1, "You won! +1 point. YOUR OPONION DONT CHOOSE ANYTHING\n", strlen("You won! +1 point. YOUR OPONION DONT CHOOSE ANYTHING\n"), 0);
        send(client2, "You lost. YOU DONT CHOOSE ANYTHING IN TIME\n", strlen("You lost. YOU DONT CHOOSE ANYTHING IN TIME\n"), 0);
        message = "Choices: " + clients[client1].name + " chose " + choice1 + ", " + clients[client2].name +
                    " chose " + choice2 + "\n" + clients[client1].name + " won the game between " + clients[client1].name + " and " + clients[client2].name + "CLIENT 2 DONT CHOOSE ANYTHING\n";
    }
    else if(((choice2 == "rock") && choice1 == "") ||
            ((choice2 == "scissors") && choice1 == "") ||
            ((choice2 == "paper") && choice1 == ""))
    {
        clients[client2].score++;
        send(client2, "You won! +1 point. YOUR OPONION DONT CHOOSE ANYTHING\n", strlen("You won! +1 point. YOUR OPONION DONT CHOOSE ANYTHING\n"), 0);
        send(client1, "You lost. YOU DONT CHOOSE ANYTHING IN TIME\n", strlen("You lost. YOU DONT CHOOSE ANYTHING IN TIME\n"), 0);
        message = "Choices: " + clients[client1].name + " chose " + choice1 + ", " + clients[client2].name +
                    " chose " + choice2 + "\n" + clients[client2].name + " won the game between " + clients[client2].name + " and " + clients[client1].name + "CLIENT 1 DONT CHOOSE ANYTHING\n";
    }
    write(STDOUT_FILENO, message.c_str(), message.size());
    sendScoreToClient(client1, clients[client1]);
    sendScoreToClient(client2, clients[client2]);

    for (auto i : subservers)
    {
        if (i.port == subserver.port)
        {
            i.clients.clear();
            i.choices.clear();
            i.clients_count = 0;
            i.game_started = false;
            break;
        }
    }

    std::string subserver_list = getAvailableSubservers(subservers);
    send(client1, subserver_list.c_str(), subserver_list.size(), 0);
    send(client2, subserver_list.c_str(), subserver_list.size(), 0);
    remove_room_timer(subserver.port);
}


// i handle the timer in this when time up we go to time_gameup and decide the game
void handle_room_timer(int signum, siginfo_t *si, void *uc) {
    int timer_id = si->si_value.sival_int;  
    int room_id = room_timer_map[timer_id];
    std::string message = "Timer expired for Room: " + std::to_string(room_id) + "\n";
    write(1, message.c_str(), std::strlen(message.c_str()));
    SubServer temp;
    for (auto i : subservers)
    {
        if (i.port == room_id) {
            temp = i;
            break;
        }
    }
    time_gameup(temp, clients, subservers);
    room_timer_map.erase(timer_id);
    active_timers.erase(timer_id);
}


// i set a timer in this
void set_room_timer(int room_id, int seconds) {
    struct sigevent sev;
    struct itimerspec its;
    timer_t timer_id;

    current_timer_id++;
    room_timer_map[current_timer_id] = room_id;

    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_signo = SIGRTMIN;  
    sev.sigev_value.sival_int = current_timer_id;  
    timer_create(CLOCK_REALTIME, &sev, &timer_id);

    its.it_value.tv_sec = seconds;
    its.it_value.tv_nsec = 0;
    its.it_interval.tv_sec = 0;  
    its.it_interval.tv_nsec = 0;
    timer_settime(timer_id, 0, &its, NULL);

    active_timers[current_timer_id] = timer_id;
}


//when a signal has alarm we go to this part of code and after that we go to handle_room_timer function
void register_signal_handler() {
    struct sigaction sa;
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = handle_room_timer;  
    sigemptyset(&sa.sa_mask);
   
    if (sigaction(SIGRTMIN, &sa, NULL) == -1) {
        perror("sigaction");
    }
}



int main(int argc, char*argv[])
{
    register_signal_handler();
    int broadcast_socket, new_socket, max_sd;
    struct sockaddr_in address;
    fd_set readfds;
    char buffer[BUFFER_SIZE];
    char stdin_buffer[BUFFER_SIZE];
    broadcast_socket = createServerSocket(std::stoi(argv[2]));
    
    int num_subservers;
    num_subservers = std::stoi(argv[3]);

    for (int i = 0; i < num_subservers; ++i)
    {
        int port = MAIN_PORT + i + 1;
        SubServer subserver = {createServerSocket(port), port, 0, {}, {}, false};
        subservers.push_back(subserver);
    }

    setNonBlocking(STDIN_FILENO);

    while (true)
    {
        FD_ZERO(&readfds);
        FD_SET(broadcast_socket, &readfds);
        FD_SET(STDIN_FILENO, &readfds);
        max_sd = broadcast_socket;

        for (const auto &client : clients)
        {
            int sd = client.first;
            if (sd > 0)
                FD_SET(sd, &readfds);
            if (sd > max_sd)
                max_sd = sd;
        }

        for (auto &subserver : subservers)
        {
            FD_SET(subserver.socket, &readfds);
            if (subserver.socket > max_sd)
                max_sd = subserver.socket;
            for (const auto &client_sd : subserver.clients)
            {
                FD_SET(client_sd, &readfds);
                if (client_sd > max_sd)
                    max_sd = client_sd;
            }
        }

        int activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);

        if (FD_ISSET(broadcast_socket, &readfds))
        {
            socklen_t addrlen = sizeof(address);
            new_socket = accept(broadcast_socket, (struct sockaddr *)&address, &addrlen);
            if (new_socket >= 0)
            {
                setNonBlocking(new_socket);
                clients[new_socket] = {"", 0};
                std::string message = "New client connected, socket fd: " + std::to_string(new_socket) + "\n";
                write(STDOUT_FILENO, message.c_str(), message.size());
                send(new_socket, "Enter your name: ", strlen("Enter your name: "), 0);
                sendScoreToClient(new_socket, clients[new_socket]);
            }
        }

        if (FD_ISSET(STDIN_FILENO, &readfds))
        {
            int valread = read(STDIN_FILENO, stdin_buffer, BUFFER_SIZE);
            if (valread > 0)
            {
                stdin_buffer[valread] = '\0';
                std::string server_message(stdin_buffer);
                if (!server_message.empty())
                {
                    if (server_message == "end_game\n") {
                        std::string server_message_end = end_game(clients , client_history);
                        write(STDOUT_FILENO, server_message_end.c_str(), server_message_end.size());
                        broadcastMessageToAllClients(server_message_end, clients, subservers);
                        exit(EXIT_SUCCESS);
                    }
                    else {
                        server_message = "[Server]: " + server_message;
                        write(STDOUT_FILENO, server_message.c_str(), server_message.size());
                        broadcastMessageToAllClients(server_message, clients, subservers);
                    }
                }
            }
        }

        for (auto it = clients.begin(); it != clients.end();)
        {
            int sd = it->first;
            if (FD_ISSET(sd, &readfds))
            {
                int valread = read(sd, buffer, BUFFER_SIZE);
                if (valread == 0)
                {
                    handleClientDisconnection(sd, clients, subservers);
                    it = clients.erase(it);
                    continue;
                }
                else
                {
                    buffer[valread] = '\0';
                    if (it->second.name.empty())
                    {
                        it->second.name = buffer;
                        std::string message = "Client set name: " + it->second.name + "\n";
                        write(STDOUT_FILENO, message.c_str(), message.size());
                        std::string subserver_list = getAvailableSubservers(subservers);
                        send(sd, subserver_list.c_str(), subserver_list.size(), 0);
                    }
                    else
                    {
                        try
                        {
                            int subserver_port = std::stoi(buffer);
                            bool joined = false;
                            for (auto &sub : subservers)
                            {
                                if (sub.port == subserver_port && sub.clients_count < MAX_CLIENTS)
                                {
                                    sub.clients.push_back(sd);
                                    sub.clients_count++;
                                    std::string message = clients[sd].name + " connected to Subserver on port " + std::to_string(subserver_port) + "\n";
                                    write(STDOUT_FILENO, message.c_str(), message.size());
                                    send(sd, "Connected to Subserver.\n", strlen("Connected to Subserver.\n"), 0);

                                    if (sub.clients_count == MAX_CLIENTS)
                                    {
                                        set_room_timer(sub.port , 10);
                                        sub.game_started = true;
                                        std::string start_message = "Enter your element. rock, scissors, paper: ";
                                        send(sub.clients[0], start_message.c_str(), start_message.size(), 0);
                                        send(sub.clients[1], start_message.c_str(), start_message.size(), 0);
                                    }
                                    joined = true;
                                    break;
                                }
                            }
                            if (!joined)
                            {
                                send(sd, "Subserver is full or invalid port. Try again.\n", strlen("Subserver is full or invalid port. Try again.\n"), 0);
                            }
                        }
                        catch (const std::invalid_argument &e)
                        {
                            bool inGame = false;
                            for (const auto &sub : subservers)
                            {
                                if (std::find(sub.clients.begin(), sub.clients.end(), sd) != sub.clients.end() && sub.game_started)
                                {
                                    inGame = true;
                                    break;
                                }
                            }
                            if (!inGame)
                            {
                                send(sd, "Invalid port format. Enter a numeric port.\n", strlen("Invalid port format. Enter a numeric port.\n"), 0);
                            }
                        }
                    }
                }
            }
            ++it;
        }

        for (auto &sub : subservers)
        {
            if (FD_ISSET(sub.socket, &readfds))
            {
                socklen_t addrlen = sizeof(address);
                int sub_socket = accept(sub.socket, (struct sockaddr *)&address, &addrlen);
                if (sub_socket >= 0)
                {
                    setNonBlocking(sub_socket);
                    if (sub.clients_count < MAX_CLIENTS)
                    {
                        sub.clients.push_back(sub_socket);
                        sub.clients_count++;
                        std::string message = "Client connected to Subserver on port " + std::to_string(sub.port) + "\n";
                        write(STDOUT_FILENO, message.c_str(), message.size());
                    }
                    else
                    {
                        send(sub_socket, "Subserver is full.\n", strlen("Subserver is full.\n"), 0);
                        close(sub_socket);
                    }
                }
            }

            for (auto it = sub.clients.begin(); it != sub.clients.end();)
            {
                int client_sd = *it;
                if (FD_ISSET(client_sd, &readfds))
                {
                    int valread = read(client_sd, buffer, BUFFER_SIZE);
                    if (valread == 0)
                    {
                        std::string message = "Client disconnected from Subserver on port " + std::to_string(sub.port) + "\n";
                        write(STDOUT_FILENO, message.c_str(), message.size());
                        it = sub.clients.erase(it);
                        sub.clients_count--;
                        close(client_sd);
                        continue;
                    }
                    else if (sub.game_started)
                    {
                        buffer[valread] = '\0';
                        std::string choice(buffer);
                        choice.erase(std::remove(choice.begin(), choice.end(), '\n'), choice.end());

                        if (choice == "rock" || choice == "paper" || choice == "scissors")
                        {
                            sub.choices[client_sd] = choice;
                        }
                        else
                        {
                            // send(client_sd, "Invalid choice. Enter 'rock', 'scissors', or 'paper'.\n", strlen("Invalid choice. Enter 'rock', 'scissors', or 'paper'.\n"), 0);
                        }
                    }
                }
                ++it;
            }
            if (sub.game_started == true) {
                handleGame(sub, clients , subservers);
            }
        }
    }
    return 0;
}
