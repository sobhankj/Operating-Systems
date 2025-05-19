#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <algorithm>
#include <cctype>
#include <string>
#include <iostream>
#include <chrono>
#include <thread>

using namespace std;

void SetColor(int textColor)
{
    cout << "\033[" << textColor << "m";
}

void ResetColor() { cout << "\033[0m"; }

struct Item {
    string name;
    float unitPrice;
    float quantity;
    string type;
};

template<typename T, typename P>
T remove_if(T beg, T end, P pred)
{
    T dest = beg;
    for (T itr = beg;itr != end; ++itr)
        if (!pred(*itr))
            *(dest++) = *itr;
    return dest;
}

void parseCSV(const string& filePath, vector<Item*> &items) {
    ifstream file(filePath);
    if (file.is_open()) {
        string line;
        while (getline(file, line)) {
            stringstream ss(line);
            string name, price, quantity, type;
            getline(ss, name, ',');
            getline(ss, price, ',');
            getline(ss, quantity, ',');
            getline(ss, type);
            type.erase(remove(type.begin(), type.end(), '\r'), type.end());

            float unitPrice = stof(price);
            float qty = stof(quantity);
            Item* temp = new Item({name, unitPrice, qty, type});
            items.push_back(temp);
        }
        file.close();
    } else {
        cerr << "Error: Unable to open file " << filePath << endl;
    }
}

float calculate(vector<Item*> items) {
    float profit = 0;
    for (int i = 0; i < items.size(); i++) {
        if (items[i]->type == "output") {
            for (int j = 0; j < i; j++) {
                if ((items[j]->name == items[i]->name) && (items[j]->type == "input")) {
                    if (items[j]->quantity >= items[i]->quantity) {
                        float diff = items[i]->unitPrice - items[j]->unitPrice;
                        profit += diff * items[i]->quantity;
                        items[j]->quantity = items[j]->quantity - items[i]->quantity;
                        break;
                    }
                    else {
                        float diff = items[i]->unitPrice - items[j]->unitPrice;
                        profit += diff * items[j]->quantity;
                        items[i]->quantity -= items[j]->quantity; 
                        items[j]->quantity = 0;
                    }
                }
                else {
                    continue;
                }
            }
        }
    }
    return profit;
}

vector<string> create_filepath_items(string& buffer) {
    vector<string> result;
    stringstream ss(buffer);
    string token;

    while (getline(ss , token , '$'))
    {
        result.push_back(token);
    }
    return result;
}

string message_send_item(vector<Item*> items , string name_item) {
    string res = "";
    for (int i = 0; i < items.size(); i++)
    {
        if (items[i]->name == name_item)
        {
            res = res + to_string(items[i]->quantity) + "%" + to_string(items[i]->unitPrice) + "%" + items[i]->type + "$";
        }
    }
    return res;
}


void send_data_to_product(string fifo_name , string msg){
    SetColor(35);
    int fd = open(fifo_name.c_str(), O_WRONLY);
    cout << "OPEN *" << fifo_name << "* Complete for write" << endl;
    write(fd, msg.c_str(), msg.size()+1);
    SetColor(35);
    cout << "Messege send to *" << fifo_name << "* seccusfully" << endl;
    close(fd);
    return;
}


string extractFileName(const string& filePath) {
    size_t lastSlash = filePath.find_last_of('/');
    size_t lastDot = filePath.find_last_of('.');
    if (lastSlash != std::string::npos && lastDot != std::string::npos && lastDot > lastSlash) {
        return filePath.substr(lastSlash + 1, lastDot - lastSlash - 1);
    }
    return "";
}


int main(int argc, char *argv[]) {
    SetColor(35);
    if (argc != 3) {
        cerr << "Usage: ./child1 <pipe_fd>" << endl;
        return 1;
    }

    vector<Item*> items;
    
    int pipe_fd_parent_to_child = atoi(argv[1]);
    int pipe_fd_child_to_parent = atoi(argv[2]);
    char buffer[256];

    vector<string> new_buffer;
    ssize_t bytesRead = read(pipe_fd_parent_to_child, buffer, sizeof(buffer) - 1);

    if (bytesRead > 0) {
        buffer[bytesRead] = '\0';
        string filePath(buffer);
        new_buffer = create_filepath_items(filePath);  // parse messege from main

        string name_warehouse = extractFileName(new_buffer[0]); // parse name from address csv
        SetColor(35);
        cout << "Recive messege from main to " << name_warehouse << "are complete succesfully" << endl;
        SetColor(35);
        cout << "Parseing messege complete succesfully in " << name_warehouse << endl;

        parseCSV(new_buffer[0], items); // parse CSV in this function
        SetColor(35);
        cout << "Parseing *" << new_buffer[0] << "* complete succesfully in " << name_warehouse << endl;

        for (int i = 1; i < new_buffer.size(); i++)
        {
            string name_fifo = name_warehouse + "_" + new_buffer[i];
            string mes = message_send_item(items , new_buffer[i]);
            SetColor(35);
            cout << "Create messege for *" << new_buffer[i] << "* complete succesfully in " << name_warehouse << endl;
            send_data_to_product(name_fifo , mes);
        }

        SetColor(35);
        float profit = calculate(items);
        cout << "Caculate Profit in *" << name_warehouse << "* complete" << endl;
        
        // Write the profit back to the parent
        SetColor(35);
        stringstream ss;
        ss << "Profit for " << new_buffer[0] << ": " << profit << endl;
        string message = ss.str();
        SetColor(35);
        cout << "Messege succesfully send to parent from *" << name_warehouse << "*" << endl;
        if (write(pipe_fd_child_to_parent, message.c_str(), message.size()) == -1) {
            perror("write");
            return EXIT_FAILURE;
        }

    } else {
        perror("read");
    }
    close(pipe_fd_parent_to_child);
    close(pipe_fd_child_to_parent);
    return 0;
}