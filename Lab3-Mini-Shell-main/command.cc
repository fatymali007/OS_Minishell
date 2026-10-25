
/*
 *
 *
 * Template file.
 * You will need to add more code here to execute the command table.
 *
 * NOTE: You are responsible for fixing any bugs this code may have!
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <time.h>
#include <glob.h>
#include <wordexp.h>
#include <vector>
#include <string>
#include <cctype>
#include <iostream>
#include <sstream>
#include "command.h"
#include "tokenizer.h"

void parse(std::vector<Token> &tokens)
{
    Command currentCommand;

    if (tokens.empty())
    {
        return;
    }
    SimpleCommand *currentSimpleCommand = new SimpleCommand();

    int i = 0;
    bool newCommand = false;
    bool eoa = false;
    bool failed = false;
    bool pipe = false;
    SimpleCommand **_pipeCommands;

    while (i < tokens.size())
    {
        if (i + 1 == tokens.size())
        {
            eoa = true;
        }
        if (tokens[i].type == TOKEN_COMMAND && !newCommand)
        {
            newCommand = true;
            currentSimpleCommand->insertArgument(const_cast<char*>(tokens[i].value.c_str()));
            i++;
            continue;
        }
        if (tokens[i].type == TOKEN_PIPE)
        {
            if (eoa)
            {   
                printf("'%s' : MISSING FOLLOWING COMMAND!\n",const_cast<char*>(tokens[i].value.c_str()));
                failed = true;
                break;
            }
            else if (tokens[i+1].type==TOKEN_PIPE)
            {
                printf("'%s' :TOKEN UNSUPPORTED!\n",const_cast<char*>(tokens[i].value.c_str()));
                failed=true;
                break;
            }
            else if (i==0)
            {
                printf("'%s' : MISSING PRECEEDING COMMAND!\n",const_cast<char*>(tokens[i].value.c_str()));
                failed=true;
                break;
            }
            else
            {
                pipe = true;
                currentCommand.insertSimpleCommand(currentSimpleCommand);
                currentSimpleCommand = new SimpleCommand();
                currentSimpleCommand->insertArgument(const_cast<char*>(tokens[i++].value.c_str()));
                newCommand=false;
                i++;
                continue;
            }
        }
        if (tokens[i].type == TOKEN_ARGUMENT && newCommand == true)
        {
            currentSimpleCommand->insertArgument(const_cast<char*>(tokens[i].value.c_str()));
        }
        if (tokens[i].type == TOKEN_BACKGROUND)
        {
            if (!eoa)
            {
                printf("'%s' MUST BE AT THE END!\n",const_cast<char*>(tokens[i].value.c_str()));
                failed = true;
                break;
            }
            currentCommand._background = 1;
        }

        if (tokens[i].type == TOKEN_REDIRECT)
        {
            if (eoa)
            {   printf("'%s' : MISSING ARGUMENT!\n",const_cast<char*>(tokens[i].value.c_str()));
                failed = true;
                break;
            }
            if (tokens[i + 1].type == TOKEN_ARGUMENT)
            {
                currentCommand._outFile = strdup(tokens[i + 1].value.c_str());
            }
        }
        if (tokens[i].type == TOKEN_APPEND)
        {
            if (eoa)
            {   
                printf("'%s' : MISSING ARGUMENT!\n",const_cast<char*>(tokens[i].value.c_str()));
                failed = true;
                break;
            }
            else if (tokens[i + 1].type == TOKEN_ARGUMENT)
            {
                currentCommand._outFile = strdup(tokens[i + 1].value.c_str());
                currentCommand._append = 1;
            }
        }
        if (tokens[i].type == TOKEN_INPUT)
        {
            if (eoa)
            {
                printf("'%s' : MISSING ARGUMENT!\n",const_cast<char*>(tokens[i].value.c_str()));
                failed = true;
                break;
            }
            else if (tokens[i + 1].type == TOKEN_ARGUMENT)
            {
                currentCommand._inputFile = strdup(tokens[i + 1].value.c_str());
            }
        }
        if (tokens[i].type == TOKEN_ERROR)
        {   if (!eoa)
            {
                printf("'%s' : MUST BE AT THE END!\n",const_cast<char*>(tokens[i].value.c_str()));
            }
            currentCommand._out_error = 1;
        }
        if (tokens[i].type == TOKEN_REDIRECT_AND_ERROR)
        {
            if (eoa)
            {
                printf("'%s' : MISSING ARGUMENT!\n",const_cast<char*>(tokens[i].value.c_str()));
                failed = true;
                break;
            }
            else if (tokens[i++].type == TOKEN_ARGUMENT)
            {
                currentCommand._errFile = strdup(tokens[i + 1].value.c_str());
            }
        }
        if (tokens[i].type == TOKEN_EOF)
        {
            break;
        }
        i++;
    };
    currentCommand.insertSimpleCommand(currentSimpleCommand);
    //-----------------------------------------------------------------------------
    if (failed)
    {   fprintf(stderr,"ERROR: '%s' PARSING ERROR\n",const_cast<char*>(tokens[i].value.c_str()));
        currentCommand.clear();
    }
    //-----------------------------------------------------------------------------
    currentCommand.execute();
}

SimpleCommand::SimpleCommand()
{
    _numberOfAvailableArguments = 5;
    _numberOfArguments = 0;
    _arguments = (char **)malloc(_numberOfAvailableArguments * sizeof(char *));
}

void SimpleCommand::insertArgument(char *argument)
{
    if (_numberOfAvailableArguments == _numberOfArguments + 1)
    {
        _numberOfAvailableArguments *= 2;
        _arguments = (char **)realloc(_arguments,
                                      _numberOfAvailableArguments * sizeof(char *));
    }

    _arguments[_numberOfArguments] = argument;
    _arguments[_numberOfArguments + 1] = NULL;

    _numberOfArguments++;
}

Command::Command()
{
    _numberOfAvailableSimpleCommands = 1;
    _simpleCommands = (SimpleCommand **)
        malloc(_numberOfSimpleCommands * sizeof(SimpleCommand *));

    _numberOfSimpleCommands = 0;
    _outFile = 0;
    _inputFile = 0;
    _errFile = 0;
    _background = 0;
}

void Command::insertSimpleCommand(SimpleCommand *simpleCommand)
{
    if (_numberOfAvailableSimpleCommands == _numberOfSimpleCommands)
    {
        _numberOfAvailableSimpleCommands *= 2;
        _simpleCommands = (SimpleCommand **)realloc(_simpleCommands,
                                                    _numberOfAvailableSimpleCommands * sizeof(SimpleCommand *));
    }

    _simpleCommands[_numberOfSimpleCommands] = simpleCommand;
    _numberOfSimpleCommands++;
}

void Command::clear()
{
    for (int i = 0; i < _numberOfSimpleCommands; i++)
    {
        for (int j = 0; j < _simpleCommands[i]->_numberOfArguments; j++)
        {
            free(_simpleCommands[i]->_arguments[j]);
        }

        free(_simpleCommands[i]->_arguments);
        free(_simpleCommands[i]);
    }

    if (_outFile)
    {
        free(_outFile);
    }

    if (_inputFile)
    {
        free(_inputFile);
    }

    if (_errFile)
    {
        free(_errFile);
    }

    _numberOfSimpleCommands = 0;
    _outFile = 0;
    _inputFile = 0;
    _errFile = 0;
    _background = 0;
}

void Command::print()
{
    printf("\n\n");
    printf("              COMMAND TABLE                \n");
    printf("\n");
    printf("  #   Simple Commands\n");
    printf("  --- ----------------------------------------------------------\n");

    for (int i = 0; i < _numberOfSimpleCommands; i++)
    {
        printf("  %-3d ", i);
        for (int j = 0; j < _simpleCommands[i]->_numberOfArguments; j++)
        {
            printf("\"%s\" \t", _simpleCommands[i]->_arguments[j]);
        }
    }

    printf("\n\n");
    printf("  Output       Input        Error        Err&Out       Background\n");
    printf("  ------------ ------------ ------------ ------------ ------------\n");
    printf("  %-12s %-12s %-12s %-12s %-12s\n", _outFile ? _outFile : "default",
           _inputFile ? _inputFile : "default", _errFile ? _errFile : "default", _out_error == 1 ? _errFile : "default", _background ? "YES" : "NO");
    printf("\n\n");
}

void Command::execute()
{
    print();
    //
    int i = 0;
    bool simple = (_numberOfSimpleCommands == 1);
    bool last;
    bool execerr;
    int prevr=-1;
    int exc;
    int children_pids[_numberOfSimpleCommands];
    FILE * logfile =fopen("logfile.txt","a");
    if(logfile==nullptr)
    {
        fprintf(stderr,"ERROR: LOGFILE NOT FOUND!");
        exit(0);
    }

    if (strcmp(_simpleCommands[0]->_arguments[0],"exit")==0)
    {
        printf("GOODBYE! :) \n");
        exit(0);
    }
    if (strcmp(_simpleCommands[0]->_arguments[0],"cd")==0)
    {
        char* dir;
        if(_simpleCommands[0]->_arguments[2] == nullptr)
        {
            dir=getenv("HOME");
        }
        else
        {
            dir=_simpleCommands[0]->_arguments[2];
        }
        if (chdir(dir) == -1)
        {
            printf("CANNOT CHANGE DIRECTORY TO '%s' \n", dir);
            fprintf(stderr, "ERROR:CANNOT CHANGE DIRECTORY TO '%s' \n", dir);
        }
    }
    while (i < _numberOfSimpleCommands)
    {
        last = (i + 1 == _numberOfSimpleCommands);
        int pipfd[2];
        if (!last)
        {
            if (pipe(pipfd) == -1)
            {
                fprintf(stderr, "ERROR: PIPE CREATION FAILED\n");
                execerr=true;
                break;
            }
        }
        pid_t pid = fork();
        if (pid == -1)
        {
            fprintf(stderr,"ERROR: FORK CREATION FAILED");
            execerr=true;
            break;
        }
        else if (pid == 0)
        {
            // child process
            if (i == 0) // first command
            {
                if (_inputFile != nullptr)
                {
                    int ipfile = open(_inputFile, O_RDONLY);
                    if (ipfile == -1)
                    {
                        fprintf(stderr, "ERROR: INPUT FILE NOT FOUND!");
                        execerr = true;
                        break;
                    }
                    if (dup2(ipfile, STDIN_FILENO) == -1)
                    {
                        fprintf(stderr, "ERROR: INVALID FILE DESCRIPTOR!");
                        execerr = true;
                        break;
                    }
                    close(ipfile);
                }
                else if (!last)
                {
                    dup2(pipfd[1], STDOUT_FILENO);
                }
                else if (_outFile != nullptr)
                {
                    int opfile = open(_outFile, O_WRONLY | O_CREAT ,0666);
                    if (opfile == -1)
                    { 
                        fprintf(stderr,"ERROR: OUTPUT FILE NOT FOUND!");
                        execerr=true;
                        break;
                    }
                    if (dup2(opfile, STDOUT_FILENO) == -1)
                    {
                        fprintf(stderr,"ERROR: INVALID FILE DESCRIPTOR!");
                        execerr=true;
                        break;
                    }
                    close(opfile);
                }
            }
            else 
            {
                dup2(prevr, STDIN_FILENO);

                if (!last)
                {
                    dup2(pipfd[1], STDOUT_FILENO);
                    
                    close(pipfd[0]);
                    close(pipfd[1]);
                }
                else
                {
                    if (_outFile != nullptr)
                    {   int opfile;
                        if (_append)
                        {
                            opfile = open(_outFile, O_APPEND);
                        }
                        else
                        {
                            opfile = open(_outFile, O_WRONLY | O_CREAT ,0666);
                        }
                        if (opfile == -1)
                        { 
                            fprintf(stderr,"ERROR: OUTPUT FILE NOT FOUND!");
                            execerr=true;
                            break;
                        }
                        if (dup2(opfile, STDOUT_FILENO) == -1)
                        {
                            fprintf(stderr, "ERROR: INVALID FILE DESCRIPTOR!");
                            execerr = true;
                            break;
                        }
                        close(opfile);
                    }
                    if (_errFile != nullptr)
                        {   int erfile;
                            if (_append)
                            {
                                erfile = open(_errFile, O_APPEND);
                            }
                            else
                            {   
                                erfile = open(_errFile, O_WRONLY | O_CREAT ,0666);
                            }
                            if (erfile == -1)
                            {
                                fprintf(stderr, "ERROR: ERROR FILE NOT FOUND!");
                                execerr = true;
                                break;
                            }
                            if (dup2(erfile, STDOUT_FILENO) == -1)
                            {
                                fprintf(stderr,"ERROR: INVALID FILE DESCRIPTOR!");
                                execerr=true;
                                break;
                            }
                            close(erfile);
                        }
                        if (_out_error)
                        {
                            dup2 (STDERR_FILENO, STDOUT_FILENO);
                        }
                    }
                }
                    
                    exc = execvp(_simpleCommands[i]->_arguments[0], _simpleCommands[i]->_arguments);
                    if (exc==-1)
                    {
                       fprintf(stderr,"ERROR 'execvp' : COMMAND NOT FOUND!");
                       execerr=true;
                       break;
                    }
        }
        if (pid>0)
        {//parent process
            
            if(!last){
                close(pipfd[1]);
                prevr = pipfd[0];
            }
            else 
            {
                close(prevr);
            }
            children_pids[i]=pid;
        }
                
    }
    int pid_ter;
    if (!_background && !execerr)
    {   
        for (int j=0 ; j<_numberOfSimpleCommands ; j++)
        { // here we cannot use while(waitpid(-1,nullptr,0)!=-1){}; since we have no control on the pids passed to waitpid
            //it can cause waiting of this parent on processes other than its children.
            pid_ter=waitpid(children_pids[j],nullptr,0);
            fprintf(logfile, "Child process %d terminated\n", pid_ter);
            fflush(logfile);
        }
    } 
    else if(!execerr)
    {
        while((pid_ter=waitpid(-1,nullptr,WNOHANG))>0)
        {
            fprintf(logfile, "Child process %d terminated\n", pid_ter);
            fflush(logfile);
        }
    }
    else
    {
        fprintf(stderr,"ERROR: EXECUTION ERROR\n");
        clear();
    }
    fclose(logfile);
    clear();
    prompt();
}

    void Command::prompt()
    {
        printf("myshell>");
        fflush(stdout);
        std::string input;
        while (true)
        {
            std::getline(std::cin, input);
            std::vector<Token> tokens = tokenize(input);
            parse(tokens);
        }
    }

    Command Command::_currentCommand;
    SimpleCommand *Command::_currentSimpleCommand;

    int main()
    {   signal(SIGINT, SIG_IGN);
        
        int logg=open("logfile.txt",O_CREAT |O_WRONLY |O_APPEND , 0666);
        if(logg==-1)
        {
            fprintf(stderr,"ERROR: UNABLE TO CREATE LOGFILE");
        }

        Command::_currentCommand.prompt();
        return 0;
    }
