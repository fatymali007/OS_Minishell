## you have command.cc command.h tokenizer.cc tokenizer.h you need compile them and link
## them to one object file called  : "myshell" 
## so we should run make , that will create our object myshell ,
## ./myshell should run your application 

SRCS=command.cc command.h tokenizer.cc tokenizer.h
OBJF=$(SRCS:.cpp=.o)
all: myshell

myshell: command.cc command.h tokenizer.cc tokenizer.h
	g++ -o myshell $(OBJF)

command.o: command.cc command.h
	g++ -c command.cc -o command.o

tokenizer.o: tokenizer.cc tokenizer.h
	g++ -c tokenizer.cc -o tokenizer.o

reset:
	rm -f $(OBJF) myshell

.PHONY: all reset