NAME = webserv
DEBUG_NAME = debug_parser
SRC = parsing/Request.cpp parsing/requestParser.cpp debug_parser.cpp
DEBUG_SRC = parsing/Request.cpp parsing/requestParser.cpp debug_parser.cpp
OBJS = $(SRC:.cpp=.o)
DEBUG_OBJS = $(DEBUG_SRC:.cpp=.o)
CC = c++
FLAGS = -Wall -Wextra -g -fno-limit-debug-info
DEBUG_FLAGS = -Wall -Wextra -g -O0 -DDEBUG -fno-limit-debug-info
CXXFLAGS += -D_GLIBCXX_DEBUG -g -O0

all: $(NAME)

debug: $(DEBUG_NAME)

$(NAME): $(OBJS)
	$(CC) $(FLAGS) $(OBJS) -o $(NAME)

$(DEBUG_NAME): $(DEBUG_OBJS)
	$(CC) $(DEBUG_FLAGS) $(DEBUG_OBJS) -o $(DEBUG_NAME)

%.o: %.cpp
	$(CC) $(FLAGS) -c $< -o $@

clean:
	$(RM) $(OBJS) $(DEBUG_OBJS)

fclean: clean
	$(RM) $(NAME) $(DEBUG_NAME)

re: fclean all

re_debug: fclean debug