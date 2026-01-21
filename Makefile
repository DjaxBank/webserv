NAME = webserv
SRC = src/main.cpp src/Config.cpp src/Socket.cpp src/handle_client.cpp
OBJS = $(SRC:.cpp=.o)
CC = c++
FLAGS = -Wall -Wextra -I inc/ -g

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