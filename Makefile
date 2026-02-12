NAME = webserv
DEBUG_NAME = debug_parser
SRC = parsing/Request.cpp \
	parsing/requestParser.cpp \
	parsing/requestParserRequestLine.cpp \
	parsing/requestParserHeaders.cpp \
	parsing/requestParserBody.cpp \
	parsing/requestParserUtils.cpp \
<<<<<<< HEAD
	src/main.cpp src/Server.cpp src/Socket.cpp src/handle_client.cpp src/Response.cpp
=======
	parsing/requestURIParsing.cpp \
	src/main.cpp src/Config.cpp src/Socket.cpp src/handle_client.cpp src/Response.cpp

>>>>>>> 3338d82705114c85f1666ff2b8d10a938ac83104
OBJS = $(SRC:.cpp=.o)
DEPENDS = ${OBJS:.o=.d}
CC = c++
INC_DIR = inc
CPPFLAGS = -I$(INC_DIR)
FLAGS = -Wall -Wextra -MMD -g -fno-limit-debug-info -fsanitize=address,undefined -std=c++20

all: $(NAME)

$(NAME): $(OBJS)
	$(CC) $(FLAGS) $(CPPFLAGS) $(OBJS) -o $(NAME)

%.o: %.cpp Makefile
	$(CC) $(FLAGS) $(CPPFLAGS) -c $< -o $@

clean:
	$(RM) $(OBJS) $(DEPENDS)

fclean: clean
	$(RM) $(NAME)

re: fclean all

debug: build-debug-parser

build-debug-parser: debug_parser

debug_parser: parser_debug.o parsing/requestParser.o parsing/Request.o parsing/requestParserRequestLine.o parsing/requestParserHeaders.o parsing/requestParserBody.o parsing/requestParserUtils.o parsing/requestURIParsing.o
	$(CC) $(FLAGS) $(CPPFLAGS) $^ -o debug_parser

parser_debug.o: parser_debug.cpp Makefile
	$(CC) $(FLAGS) $(CPPFLAGS) -c parser_debug.cpp -o parser_debug.o

.PHONY: build-debug-parser debug_parser

-include ${DEPENDS}  