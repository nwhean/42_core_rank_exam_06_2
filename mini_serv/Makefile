CC		=	gcc
CFLAGS	=	-Wall -Wextra -Werror -Wfatal-errors -g -fsanitize=address
NAME	=	mini_serv

all: $(NAME)

$(NAME): mini_serv.c
	$(CC) $(CFLAGS) -o $(NAME) $?

clean:

fclean: clean
	$(RM) $(NAME)

re: fclean all

.PHONY: clean
