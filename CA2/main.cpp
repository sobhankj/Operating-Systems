#include <bits/stdc++.h>
#include <fstream>
#include <sstream>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <filesystem>
#include <iostream>
#include <unistd.h>
#include <cstring>
#include <vector>
#include <sys/wait.h>

using namespace std;
using namespace std::filesystem;

#define endl "\n"
#define COMMA ','
#define SPACE " "
#define PARTS_PATH "/Parts.csv"
#define TAB "\t"
#define READ 0
#define WRITE 1
#define STORES_PATH "/stores/"

void SetColor(int textColor)
{
    cout << "\033[" << textColor << "m";
}

void ResetColor() { cout << "\033[0m"; }

vector <string> tokenize(string inp_str, char delimiter)
{
    stringstream ss(inp_str); 
    string s; 
    vector <string> str;
    while (getline(ss, s, delimiter)) {    
        str.push_back(s);
    }
    return str; 
}

vector<string> read_warehouses(string directorypath) {
    vector< string> warehouses, temp;
    if (exists(directorypath) && is_directory(directorypath)) { 
        for (const auto& entry: directory_iterator(directorypath)) { 
            temp = tokenize(entry.path().string(), '/');
            int ind = temp[1].find('.');
            if (temp[1].substr(0, ind) != "Parts")
                warehouses.push_back(temp[1].substr(0, ind));
        }
    }
    else
        cerr << "Directory not found." << endl; 
    return warehouses;
}

void show_warehouses(const vector<string>& warehouses) {
    cout << warehouses.size() << " warehouses found in Parts.csv: " << endl << TAB;
    for (const auto& warehouse: warehouses)
        cout << warehouse << SPACE;
    cout << endl;
    for (int i = 0; i < 50; i++)
        cout << '-';
    cout << endl;
}

vector<string> read_items(const string &filePath) {
    vector<string> items_in_parts_csv;
    ifstream file(filePath);
    if (file.is_open()) {
        string line;
        getline(file, line);
        stringstream ss(line);
        string item;
        while (getline(ss, item, COMMA))
            items_in_parts_csv.push_back(item);
        file.close();
    } else
        cerr << "Error: Unable to open file " << filePath << endl;

    return items_in_parts_csv;
}

void show_items(const vector <string> &items) {
    cout << items.size() << " items found in Parts.csv: " << endl << TAB;
    for (auto& item: items)
        cout << item << SPACE;
    cout << endl;
    for (int i = 0; i < 50; i++)
        cout << '-';
    cout << endl;
}

vector<string> make_csv_addresses(string path, const vector<string> &warehouses) {
    vector<string> addresses;
    for (auto warehouse: warehouses)
        addresses.push_back("./" + path + "/" + warehouse + ".csv");
    return addresses;
}

void show_addresses(vector<string> &addresses) {
    cout << "CSV addresses" << endl;
    for (auto i: addresses)
        cout << i << endl;
}

int create_process(int& write_pipe, int& read_pipe, string executable)
{
    int pipe_fd[2];
    int pipe_fd2[2];
    if (pipe(pipe_fd) == -1) {
        perror("pipe");
    }
    if (pipe(pipe_fd2) == -1) {
        perror("pipe");
    }
    int pid = fork();
    if (pid == 0) {
        // Child process
        close(pipe_fd[WRITE]);
        close(pipe_fd2[READ]);
        char read_fd[20];
        char write_fd[20];
        sprintf(read_fd , "%d" , pipe_fd[READ]);
        sprintf(write_fd , "%d" , pipe_fd2[WRITE]);
        execl(executable.c_str(), read_fd , write_fd, NULL);
        perror("execl");
    } else if (pid > 0) {
        // Parent process
        close(pipe_fd[READ]);
        close(pipe_fd2[WRITE]);
        read_pipe = pipe_fd2[READ] ;
        write_pipe = pipe_fd[WRITE];
    }else{
        perror("fork");
    }
    return pid;
}

void create_named_pipe(vector<string> item , vector<string> warehouses) {
    for (int i = 0; i < warehouses.size(); i++)
    {
        for (int j = 0; j < item.size(); j++)
        {
            string fifo_name = warehouses[i] + "_" + item[j];
            cout << "Create *" << fifo_name << "* FIFO named pipe" << endl;
            if (exists(fifo_name)) {
                cout << fifo_name << "EXISTS *" << fifo_name << "* before" << endl;
                unlink(fifo_name.c_str());
            }
            if(mkfifo(fifo_name.c_str(), 0666) == -1){
                perror("Making named pipe failed!");
            }
        }
    }
}

void remove_named_pipe(vector<string> item , vector<string> warehouses) {
    for (int i = 0; i < warehouses.size(); i++)
    {
        for (int j = 0; j < item.size(); j++)
        {
            string fifo_name = warehouses[i] + "_" + item[j];
            if (exists(fifo_name)) {
                cout << "Remove *" << fifo_name << "* FIFO named pipe" << endl;
                unlink(fifo_name.c_str());
            }
        }
    }
}


vector<string> create_warehouses_messege(vector<string> old_mes , vector<string> item) {
    string add_mes = "$";
    for (int i = 0; i < item.size(); i++)
    {
        add_mes = add_mes + item[i] + "$";
    }
    for (int i = 0; i < old_mes.size(); i++)
    {
        old_mes[i] = old_mes[i] + add_mes;
    }
    return old_mes;
}


vector<string> create_item_messege(vector<string> item , vector<string> warehouse) {
    vector<string> res;
    for (int i = 0; i < item.size(); i++)
    {
        string mes = item[i] + "$";
        for (int j = 0; j < warehouse.size(); j++)
        {
            mes = mes + warehouse[j] + "$";
        }
        res.push_back(mes);
    }
    return res;
}



vector<string> choose_and_drop (vector<string> item) {
    cout << "we have these items.\nwhat do you want to show it's datails?\n";
    for (int i = 0; i < item.size(); i++)
    {
        cout << i+1 << " " << item[i] << endl;
    }
    string input;
    vector<int>n;
    vector<string>temp;

    // get numbers
    getline(cin , input);
    stringstream ss(input);
    int number;
    while (ss >> number)
    {
        n.push_back(number);
    }

    // correct item when we return temp
    for (int i = 0; i < n.size(); i++)
    {
        temp.push_back(item[n[i] - 1]);
    }
    
    return temp;
}


int main(int argc, char *argv[]) {
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " <path_to_stores>" << endl;
        return 1;
    }
    string stores_path = argv[1];
    string parts_file_path = stores_path + PARTS_PATH;


    // Read name of items
    vector<string> items = read_items(parts_file_path);
    if (items.empty()) {
        cerr << "Error: No items found in " << parts_file_path << endl;
        return 1;
    }
    // show_items(items);  // This is for test item


    // Read name of warehouses
    vector<string> warehouses = read_warehouses(stores_path);
    if (warehouses.empty()) {
        cerr << "Error: No warehouses found in " << stores_path << endl;
        return 1;
    }
    // show_warehouses(warehouses);  // This is for test warehouses


    // we get input from user and replace correct for items
    items = choose_and_drop(items);
    SetColor(32);

    // Create csv_addresses
    vector<string> csv_addresses = make_csv_addresses(stores_path, warehouses);
    // show_addresses(csv_addresses);  this is for test


    // create named pipes
    create_named_pipe(items , warehouses);

    // These ordinary pipes for warehouses
    const int NUM_WAREHOUSES = warehouses.size(); // Number of warehouses pipes
    std::vector<int[2]> PtoCPipes_warehouses(NUM_WAREHOUSES); // Pipes for parent-to-child warehouses
    std::vector<int[2]> CtoPPipes_warehouses(NUM_WAREHOUSES); // Pipes for child-to-parent warehouses

    // These ordinary pipes for items
    const int NUM_ITEMS = items.size(); // Number of warehouses pipes
    std::vector<int[2]> PtoCPipes_items(NUM_ITEMS); // Pipes for parent-to-child items
    std::vector<int[2]> CtoPPipes_items(NUM_ITEMS); // Pipes for child-to-parent items

    // Create warehouses pipes
    for (int i = 0; i < NUM_WAREHOUSES; i++) {
        if (pipe(PtoCPipes_warehouses[i]) == -1 || pipe(CtoPPipes_warehouses[i]) == -1) {
            perror("pipe");
            return 1;
        }
    }
    SetColor(32);
    cout << "Ordinary(unnamed) Pipes for connection warehouses and main are Created!" << endl;


    // Create items pipes
    for (int i = 0; i < NUM_ITEMS; i++) {
        if (pipe(PtoCPipes_items[i]) == -1 || pipe(CtoPPipes_items[i]) == -1) {
            perror("pipe");
            return 1;
        }
    }
    SetColor(32);
    cout << "Ordinary(unnamed) Pipes for connection item and main are Created!" << endl;

    ResetColor();
    // Fork and exec each child warehouses
    for (int i = 0; i < warehouses.size(); ++i) {
        SetColor(33);
        pid_t pid = fork();
        if (pid == -1) {
            perror("fork");
            return 1;
        }

        if (pid == 0) {
            // Child process
            SetColor(33);
            cout << "Fork complete successfully for warehouse -> *" << warehouses[i] << "*" << endl;
            for (int j = 0; j < NUM_WAREHOUSES; ++j) {
                if (j != i) {
                    close(PtoCPipes_warehouses[j][0]); // Close unused read ends
                    close(PtoCPipes_warehouses[j][1]); // Close unused write ends
                    close(CtoPPipes_warehouses[j][0]); // Close unused read ends
                    close(CtoPPipes_warehouses[j][1]); // Close unused write ends
                }
            }

            close(PtoCPipes_warehouses[i][1]); // Close write end (child only reads)
            close(CtoPPipes_warehouses[i][0]); // Close read end (child only writes)

            char pipeFD_PtoC_warehouses[10];
            char pipeFD_CtoP_warehouses[10];
            snprintf(pipeFD_PtoC_warehouses, sizeof(pipeFD_PtoC_warehouses), "%d", PtoCPipes_warehouses[i][0]);
            snprintf(pipeFD_CtoP_warehouses, sizeof(pipeFD_CtoP_warehouses), "%d", CtoPPipes_warehouses[i][1]);

            ResetColor();
            execl("./warehouse", "./warehouse", pipeFD_PtoC_warehouses, pipeFD_CtoP_warehouses, nullptr);
            
            perror("execl");
            return 1;
        }
    }

    // fork and exec each child items
    for (int i = 0; i < items.size(); ++i) {
        SetColor(33);
        pid_t pid = fork();
        if (pid == -1) {
            perror("fork");
            return 1;
        }

        if (pid == 0) {
            // Child process
            SetColor(33);
            cout << "Fork complete successfully for item -> *" << items[i] << "*" << endl;
            for (int j = 0; j < NUM_ITEMS; ++j) {
                if (j != i) {
                    close(PtoCPipes_items[j][0]); // Close unused read ends
                    close(PtoCPipes_items[j][1]); // Close unused write ends
                    close(CtoPPipes_items[j][0]); // Close unused read ends
                    close(CtoPPipes_items[j][1]); // Close unused write ends
                }
            }

            close(PtoCPipes_items[i][1]); // Close write end (child only reads)
            close(CtoPPipes_items[i][0]); // Close read end (child only writes)

            char pipeFD_PtoC_items[10];
            char pipeFD_CtoP_items[10];
            snprintf(pipeFD_PtoC_items, sizeof(pipeFD_PtoC_items), "%d", PtoCPipes_items[i][0]);
            snprintf(pipeFD_CtoP_items, sizeof(pipeFD_CtoP_items), "%d", CtoPPipes_items[i][1]);
            
            ResetColor();
            execl("./item", "./item", pipeFD_PtoC_items, pipeFD_CtoP_items, nullptr);
            
            perror("execl");
            return 1;
        }
    }

    // Parent process
    SetColor(32);
    for (int i = 0; i < NUM_WAREHOUSES; ++i) {
        close(PtoCPipes_warehouses[i][0]); // Close read ends in parent warehouses(only used by children)
        close(CtoPPipes_warehouses[i][1]); // Close write ends in parent warehouses(only used by children)
    }

    for (int i = 0; i < NUM_ITEMS; ++i) {
        close(PtoCPipes_items[i][0]); // Close read ends in parent items(only used by children)
        close(CtoPPipes_items[i][1]); // Close write ends in parent items(only used by children)
    }


    // add name of items to csv_addresses to send them to warehouses
    csv_addresses = create_warehouses_messege(csv_addresses , items);
    cout << "Create messages for warehouses complete successfully" << endl;

    // create messages for items i send them name of warehouses and item
    vector<string> item_messages = create_item_messege(items , warehouses);
    cout << "Create messages for items complete successfully" << endl;
    
    // write to warehouses
    for (int i = 0; i < NUM_WAREHOUSES; ++i) {
        write(PtoCPipes_warehouses[i][1], csv_addresses[i].c_str(), csv_addresses[i].size());
        close(PtoCPipes_warehouses[i][1]); // Close write end after sending warehouses
    }

    // write to items
    for (int i = 0; i < NUM_ITEMS; ++i) {
        write(PtoCPipes_items[i][1], item_messages[i].c_str(), item_messages[i].size());
        close(PtoCPipes_items[i][1]); // Close write end after sending itmes
    }

    SetColor(96);
    // read from warehouses
    vector<string> buffer_WA;
    for (int i = 0; i < NUM_WAREHOUSES; ++i) {
        char buffer[256];
        ssize_t bytesRead = read(CtoPPipes_warehouses[i][0], buffer, sizeof(buffer) - 1);
        if (bytesRead > 0) {
            buffer[bytesRead] = '\0'; // Null-terminate the received data
            buffer_WA.push_back(buffer);
        } else {
            perror("read");
        }
        close(CtoPPipes_warehouses[i][0]); // Close read end after reading
    }


    SetColor(96);
    // read from items
    vector<string> buffer_IA;
    for (int i = 0; i < NUM_ITEMS; ++i) {
        char buffer[256];
        ssize_t bytesRead = read(CtoPPipes_items[i][0], buffer, sizeof(buffer) - 1);
        if (bytesRead > 0) {
            buffer[bytesRead] = '\0'; // Null-terminate the received data
            buffer_IA.push_back(buffer);
        } else {
            perror("read");
        }
        close(CtoPPipes_items[i][0]); // Close read end after reading
    }



    // Wait for all children to finish
    for (int i = 0; i < NUM_WAREHOUSES + NUM_ITEMS; ++i) {
        wait(nullptr);
    }
    SetColor(96);
    // write warehouses messeges
    for (int i = 0; i < buffer_WA.size(); i++)
    {
        cout << buffer_WA[i];
    }

    SetColor(96);
    // write items messeges
    for (int i = 0; i < buffer_IA.size(); i++)
    {
        cout << buffer_IA[i];
    }
    
    // remove named pipe
    remove_named_pipe(items , warehouses);
    ResetColor();
    return 0;
}