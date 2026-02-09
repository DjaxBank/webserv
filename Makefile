NAME = webserv
DEBUG_NAME = debug_parser
SRC = parsing/Request.cpp \
	parsing/requestParser.cpp \
	parsing/requestParserRequestLine.cpp \
	parsing/requestParserHeaders.cpp \
	parsing/requestParserBody.cpp \
	parsing/requestParserUtils.cpp \
	src/main.cpp src/Server.cpp src/Socket.cpp src/handle_client.cpp src/Response.cpp
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

-include ${DEPENDS}  