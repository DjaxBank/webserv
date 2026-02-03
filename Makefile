NAME = webserv
DEBUG_NAME = debug_parser
SRC = parsing/Request.cpp \
      parsing/requestParser.cpp \
      parsing/requestParserRequestLine.cpp \
      parsing/requestParserHeaders.cpp \
      parsing/requestParserBody.cpp \
      parsing/requestParserUtils.cpp \
      debug_parser.cpp
DEBUG_SRC = parsing/Request.cpp \
            parsing/requestParser.cpp \
            parsing/requestParserRequestLine.cpp \
            parsing/requestParserHeaders.cpp \
            parsing/requestParserBody.cpp \
            parsing/requestParserUtils.cpp \
            debug_parser.cpp
OBJS = $(SRC:.cpp=.o)
CC = c++
FLAGS = -Wall -Wextra -g 
DEBUG_FLAGS = -Wall -Wextra -g -O0 -DDEBUG
CXXFLAGS += -D_GLIBCXX_DEBUG -g -O0
FLAGS = -Wall -Wextra -I inc/ -g -fno-limit-debug-info -fsanitize=address,undefined

all: $(NAME)

$(NAME): $(OBJS)
	$(CC) $(FLAGS) $(OBJS) -o $(NAME)

%.o: %.cpp
	$(CC) $(FLAGS) -c $< -o $@

clean:
	$(RM) $(OBJS)

fclean: clean
	$(RM) $(NAME)

re: fclean all