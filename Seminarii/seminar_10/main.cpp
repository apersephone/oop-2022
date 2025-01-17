#include <iostream>
#include <stdio.h>
#include <map>
#include <string>
#include <functional>
#include <vector>
#include <string.h>
#include <sstream>
#include <fstream>

using namespace std;

#define SIZE 4096

class MyException : public exception {
    string message;
public:
    MyException(string str) : message(str) {};
    const char* what() const noexcept override {
        return message.c_str();
    }
};

class OpenFileException : public MyException {
public:
    OpenFileException(string file_name) : MyException("can't open file " + file_name) {};
};


class ReadFileException : public MyException {
public:
    ReadFileException(string file_name) : MyException("can't read file " + file_name) {};
};

class WriteFileException : public MyException {
public:
    WriteFileException(string file_name) : MyException("can't write to file " + file_name) {};
};

class MyFile {
public:
    FILE* file;
    MyFile(FILE* _file) : file(_file) {};
    ~MyFile() {
        fclose(file);
    }
};

// the CommandManager class should implement a mini terminal that receive commands and dispatches them.
// example: copy a.jpg b.jpg
// this will search the map for a "copy" command, and if it finds it, it will invoke its callback with a vector made of {"a.jpg", "b.jpg"}
// no global(or statics!) variables allowed

class CommandManager {
  private:
    map<string, function<void(vector<string>)>> commands;

  public:
    void add(string name, function<void(vector<string>)> fn);

    // run shall read a line from stdin in a loop, split it by whitespaces.
    // the first word will be the command name. if no word has been found, it will do nothing
    // then it will search the map for the name, and it will invoke the callback if it founds it
    // or it will show a message if it can't find it
    // the args for the callback will not contain the command name
    // if the `stop` command is found, the function will return
    // try doing this yourself; if you spent too much time on this, look at https://gist.github.com/xTachyon/9e698a20ce6dfcae9a483b28778af9fb
    void run();
};

void CommandManager::add(string name, function<void(vector<string>)> fn) {
    commands[name] = fn;
}

void CommandManager::run() {
    while(true) {
        string command, word, commandName;
        getline(cin, command, '\n');
        istringstream iss(command);
        getline(iss, commandName, ' ');

        vector<string> args;

        if(commandName == "stop")
            break;

        while(getline(iss, word, ' ')) {
            if(word != commandName)
                args.push_back(word);
        }

        try {
            if (commands.find(commandName) != commands.cend())
                commands.at(commandName)(args);
            else {
                printf("No command with that name\n");
                continue;
            }
        } catch (exception& e) {
            fprintf(stderr, "%s\n", e.what());
        }
    }
}

int main() {
    CommandManager manager;
    // number_of_errors represents the number of IO errors (eg. file errors) that happened in the commands
    // this is only relevant for the append and copy commands
    unsigned number_of_errors = 0;

    // prints pong!
    auto ping = [&](vector<string> args) { printf("pong!\n"); };
    manager.add("ping", ping);

    // prints the number of arguments it received
    // `count a b c` -> will print `counted 3 args`
    auto count = [&](vector<string> args) {
        printf("Function has %u args\n", static_cast<unsigned>(args.size()));
    };
    manager.add("count", count);

    // the first argument will be treated as a file name
    // the rest of the arguments will be appended to that file with a space as delimiter
    // remember to count the errors, if any
    auto append = [&](vector<string> args) {
        const char* fileName = args.at(0).c_str();
        FILE* file_out;
        file_out = fopen(fileName, "a");
        MyFile _file_out(file_out);
        if(file_out == nullptr) {
            //number_of_errors++;
            throw OpenFileException(args.at(0));
        }
        for(auto word = args.begin() + 1; word != args.end(); word++) {
            auto written_ch = fprintf(file_out, "%s ", (*word).c_str());
            if(written_ch < 0) {
                //number_of_errors++;
                throw WriteFileException(args.at(0));
            }
        }
        printf("Successfully written in the file\n");

    };
    manager.add("append", append);

    // will print the number of times the command was called
    // do not capture any reference for this lambda
    auto times = [counter = 0](vector<string> args) mutable {
        printf("Function called %d times\n", counter);
        counter++;
    };
    manager.add("times", times);

    // copy a file; args[0] will provide the source, and args[1] the destination
    // make sure it works with binary files (test it with an image)
    // remember to count the errors, if any

    auto copy = [&] (vector<string> args) {
        FILE* file_in, *file_out;
        file_in = fopen(args.at(0).c_str(), "rb");
        if(file_in == nullptr) {
            //number_of_errors++;
            throw OpenFileException(args.at(0));
        }
        file_out = fopen(args.at(1).c_str(), "wb");
        if(file_out == nullptr) {
            throw OpenFileException(args.at(1));
            //number_of_errors++;
        }
        MyFile _file_in(file_in);
        MyFile _file_out(file_out);
        while(true) {
            char buffer[SIZE];
            int read = fread(buffer, sizeof(char), SIZE, file_in);
            if(read < 0) {
                //number_of_errors++;
                throw ReadFileException(args.at(0));
            }
            else if(read == 0)
                break; //end of file
            int written = fwrite(buffer, sizeof(char), read, file_out); // se scriu read bytes
            if(written != read) {
                //number_of_errors++;
                throw WriteFileException(args.at(1));
            }
        }
        printf("Successfully written in the file\n");

    };
    manager.add("copy", copy);

    // will sort the arguments by size
    // `sort_size abc a 1234` -> will print `a abc 1234`
    // use std::sort with a lambda comparator to sort
    // and std::for_each with a lambda to print afterwards
     auto sort_size = [](vector<string> args) {
         sort(args.begin(), args.end(), [](const string& s1, const string& s2) { return s1.size() < s2.size(); });
         for_each(args.begin(), args.end(), [](const string& s) { printf("%s ", s.c_str()); });
         printf("\n");
     };
     manager.add("sort_size", sort_size);


    manager.run();

    printf("number of errors: %u\ndone!\n", number_of_errors);
}
