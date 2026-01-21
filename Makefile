NAME = webserv
SRC = parsing/Request.cpp parsing/requestParser.cpp
OBJS = $(SRC:.cpp=.o)
CC = c++
FLAGS = -Wall -Wextra -g

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