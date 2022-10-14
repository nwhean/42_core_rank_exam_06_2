#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define BUFFER_SIZE 131072

typedef struct s_client
{
	int				id;
	int				fd;
	int				off_in;
	int 			off_out;
	int 			cap_in;
	int 			cap_out;
	char			*buf_in;
	char			*buf_out;
	struct s_client	*next;
}	t_client;

t_client	*g_clients = NULL;
int			g_id = 0;

void		ft_putstr_fd(const char *s, int fd);
void		ft_error(const char *s);
void		ft_fatal(void);

t_client	*client_new(int fd);
void		client_add(t_client *new);
void		client_remove(t_client **client);
void		client_clear(void);

int			setup_listener(int port);
void		handle_connection(int listener);

/* print string s to file descriptor fd. */
void	ft_putstr_fd(const char *s, int fd)
{
	write(fd, s, strlen(s));
}

/* print error message s to stderr and exit */
void	ft_error(const char *s)
{
	ft_putstr_fd(s, 2);
	exit(1);
}

/* print "Fatal error\n" to stderr and exit */
void	ft_fatal(void)
{
	ft_error("Fatal error\n");
}

int	extract_message(char **buf, char **msg)
{
	char	*newbuf;
	int		i;

	*msg = 0;
	if (*buf == 0)
		return (0);
	i = 0;
	while ((*buf)[i])
	{
		if ((*buf)[i] == '\n')
		{
			newbuf = calloc(1, sizeof(*newbuf) * (strlen(*buf + i + 1) + 1));
			if (newbuf == 0)
				return (-1);
			strcpy(newbuf, *buf + i + 1);
			*msg = *buf;
			(*msg)[i + 1] = 0;
			*buf = newbuf;
			return (1);
		}
		i++;
	}
	return (0);
}

char	*str_join(char *buf, char *add)
{
	char	*newbuf;
	int		len;

	if (buf == 0)
		len = 0;
	else
		len = strlen(buf);
	newbuf = malloc(sizeof(*newbuf) * (len + strlen(add) + 1));
	if (newbuf == 0)
		return (0);
	newbuf[0] = 0;
	if (buf != 0)
		strcat(newbuf, buf);
	free(buf);
	strcat(newbuf, add);
	return (newbuf);
}

/* Allocated memory for a new t_client struct */
t_client	*client_new(int fd)
{
	t_client	*new;

	new = malloc(sizeof(t_client));
	if (new == NULL)
		ft_fatal();
	new->id = g_id++;
	new->fd = fd;
	new->off_in = 0;
	new->off_out = 0;
	new->cap_in = BUFFER_SIZE;
	new->cap_out = BUFFER_SIZE;
	new->buf_in = malloc(sizeof(char) * BUFFER_SIZE);
	new->buf_out = malloc(sizeof(char) * BUFFER_SIZE);
	if (new->buf_in == NULL || new->buf_out == NULL)
		ft_fatal();
	new->buf_in[0] = '\0';
	new->buf_out[0] = '\0';
	new->next = NULL;
	return (new);
}

/* Add a new client to the g_clients linked list */
void	client_add(t_client *new)
{
	t_client	*this;

	if (g_clients == NULL)
		g_clients = new;
	else
	{
		this = g_clients;
		while (this->next != NULL)
			this = this->next;
		this->next = new;
	}
}

/* Remove client from g_clients linked list */
void	client_remove(t_client **client)
{
	t_client	*this;

	if (g_clients == *client)
		g_clients = g_clients->next;
	else
	{
		this = g_clients;
		while (this->next != *client)
			this = this->next;
		this->next = (*client)->next;
	}
	close((*client)->fd);
	free((*client)->buf_in);
	free((*client)->buf_out);
	free(*client);
	*client = NULL;
}

/* Clear the g_clients linked list */
void	client_clear(void)
{
	t_client	*this;

	while (g_clients != NULL)
	{
		this = g_clients;
		client_remove(&this);
	}
}

/* Setup and return the listener fd. */
int	setup_listener(int port)
{
	int					sockfd;
	struct sockaddr_in	servaddr;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1)
		ft_fatal();
	ft_putstr_fd("Socket successfully created..\n", 1);
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(2130706433);
	servaddr.sin_port = htons(port);
	if ((bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)))
		!= 0)
		ft_fatal();
	ft_putstr_fd("Socket successfully binded..\n", 1);
	if (listen(sockfd, 10) != 0)
		ft_fatal();
	return (sockfd);
}

/* Handle a new connection from the listener fd */
void	handle_connection(int listener)
{
	int					connfd;
	socklen_t			len;
	struct sockaddr_in	cli;
	t_client			*new;

	len = sizeof(cli);
	connfd = accept(listener, (struct sockaddr *)&cli, &len);
	if (connfd < 0)
		ft_fatal();
	else
		ft_putstr_fd("server acccept the client...\n", 1);
	new = client_new(connfd);
	client_add(new);
}

int	main(int argc, char **argv)
{
	int					listener;

	if (argc != 2)
		ft_error("Wrong number of arguments\n");
	listener = setup_listener(atoi(argv[1]));
	handle_connection(listener);
	close(listener);
	client_clear();
	return (0);
}
