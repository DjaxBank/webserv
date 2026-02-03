NAME = webserv
DEBUG_NAME = debug_parser
SRC = parsing/Request.cpp \
	parsing/requestParser.cpp \
	parsing/requestParserRequestLine.cpp \
	parsing/requestParserHeaders.cpp \
	parsing/requestParserBody.cpp \
	parsing/requestParserUtils.cpp \
	src/main.cpp src/Config.cpp src/Socket.cpp src/handle_client.cpp src/methods/get.cpp src/Response.cpp
DEBUG_SRC = parsing/Request.cpp \
		parsing/requestParser.cpp \
		parsing/requestParserRequestLine.cpp \
		parsing/requestParserHeaders.cpp \
		parsing/requestParserBody.cpp \
		parsing/requestParserUtils.cpp \
		debug_parser.cpp
OBJS = $(SRC:.cpp=.o)
CC = c++
INC_DIR = inc
CPPFLAGS = -I$(INC_DIR)
FLAGS = -Wall -Wextra -g -fno-limit-debug-info -fsanitize=address,undefined -std=c++17
DEBUG_FLAGS = -Wall -Wextra -g -O0 -DDEBUG -std=c++17
CXXFLAGS += -D_GLIBCXX_DEBUG -g -O0

all: $(NAME)

$(NAME): $(OBJS)
	$(CC) $(FLAGS) $(CPPFLAGS) $(OBJS) -o $(NAME)

%.o: %.cpp
	$(CC) $(FLAGS) $(CPPFLAGS) -c $< -o $@

clean:
	$(RM) $(OBJS)

fclean: clean
	$(RM) $(NAME)

re: fclean all