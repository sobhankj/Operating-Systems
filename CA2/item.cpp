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
    float quantity;
    float unitPrice;
    string type;
};

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


string recive_messege(string FIFO_name){
    string result;
    SetColor(94);
    int fd = open(FIFO_name.c_str(), O_RDONLY);
    cout << "OPEN *" << FIFO_name << "* Complete for read" << endl;
    if(fd < 0)
        perror("file descriptor wasn't acquired!");
    char buffer[256];
    ssize_t bytes_red = read(fd, buffer, sizeof(buffer) - 1);
    SetColor(94);
    cout << "Messege recive to *" << FIFO_name << "* seccusfully" << endl;

    if (bytes_red > 0) {
        buffer[bytes_red] = '\0';  // Null-terminate the buffer
    } else {
        perror("Read failed");
    }
    close(fd);
    result = buffer;
    return result;
}


void convert_mes_to_items(vector<Item*>& items , string& mes) {
    vector<string> result;
    stringstream ss(mes);
    string token;

    while (getline(ss , token , '$'))
    {
        result.push_back(token);
    }

    for (int i = 0; i < result.size(); i++)
    {
        vector<string> item;
        stringstream ss2(result[i]);
        string token2;

        while (getline(ss2 , token2 , '%'))
        {
            item.push_back(token2);
        }
        Item* temp = new Item({stof(item[0]) , stof(item[1]) , item[2]});

        items.push_back(temp);
    }
}


void calculate_availability(vector<Item*> item , int& num_available , double& price_avaiable) {    
    for (int i = 0; i < item.size(); i++)
    {
        if (item[i]->type == "input")
        {
            num_available += item[i]->quantity;
            // price_avaiable += item[i]->quantity * item[i]->unitPrice;
        } else {
            num_available -= item[i]->quantity;
            for (int j = 0; j < i; j++)
            {
                if (item[j]->type == "input")
                {
                    if(item[j]->quantity >= item[i]->quantity) {
                        item[j]->quantity = item[j]->quantity - item[i]->quantity;
                        break;
                    }
                    else {
                        item[i]->quantity -= item[j]->quantity;
                        item[j]->quantity = 0;
                    }
                }
                
            }
            
            // price_avaiable -= item[i]->quantity * item[i]->unitPrice;
        }
    }
    
    for (int i = 0; i < item.size(); i++)
    {
        if (item[i]->type == "input")
        {
            price_avaiable += item[i]->quantity * item[i]->unitPrice;
        }
        
    }
       
}


int main(int argc, char *argv[]) {
    SetColor(94);
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
        string message(buffer);
        // parse the messege
        SetColor(94);
        new_buffer = create_filepath_items(message);
        cout << "Recive messege from parent in *" << new_buffer[0] << "*" << endl;
        
        int num = 0;
        double price = 0;

        for (int i = 1; i < new_buffer.size(); i++)
        {
            string fifo_name = new_buffer[i] + "_" + new_buffer[0];
            string result = recive_messege(fifo_name);
            convert_mes_to_items(items , result);
            SetColor(94);
            cout << "Convert messege to item are somplete for *" << new_buffer[0] << "*" << endl;
        }
        calculate_availability(items , num , price);
        SetColor(94);
        cout << "Calculateion of availability are complete for *" << new_buffer[0] << "*" << endl;
        string send_to_parent = "this item : " + new_buffer[0] + " avi num = " + to_string(num) + " avi price = " + to_string(price) + "\n";
        SetColor(94);
        cout << "Messege succesfully send to parent from *" << new_buffer[0] << "*" << endl;
        ResetColor();
        if (write(pipe_fd_child_to_parent, send_to_parent.c_str(), send_to_parent.size()) == -1) {
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
