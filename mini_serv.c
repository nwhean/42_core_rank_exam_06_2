#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
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
void		*ft_memmove(void *dst, const void *src, size_t len);
char		*ft_strchr(const char *s, int c);

t_client	*client_new(int fd);
void		client_add(t_client *new);
void		client_remove(t_client **client);
void		client_clear(void);

int			receive(t_client *client);
int			extract(t_client *client, int is_open);
size_t		extract_one(int id, const char *buf, char delimiter);
void		broadcast(int source, const char *str);

int			setup_listener(int port);
void		handle_connection(int listener);
int			get_max_fd(int listener);
void		wait_events(fd_set *rfds, fd_set *wfds, int listener);
void		manage_events(fd_set *rfds, fd_set *wfds, int listener);

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

/* Copies len byets from string src to string dst. */
void	*ft_memmove(void *dst, const void *src, size_t len)
{
	char		*d;
	const char	*s;

	d = dst + len;
	s = src + len;
	while (len-- > 0)
		*--d = *--s;
	return (dst);
}

/* Locate first ocurrence of 'c' in string 's' */
char	*ft_strchr(const char *s, int c)
{
	char ch;

	ch = (char) c;
	while (*s != ch && *s != '\0')
		++s;
	if (*s == ch)
		return ((char *)s);
	else
		return (NULL);
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

/* Call recv to store message into buf_in. */
int	receive(t_client *client)
{
	ssize_t	byte;

	byte = recv(client->fd, client->buf_in + client->off_in,
			client->cap_in - client->off_in - 1, 0);
	if (byte <= 0)
		return (0);
	client->off_in += byte;
	client->buf_in[client->off_in] = '\0';
	if (client->off_in == client->cap_in - 1)
	{
		client->cap_in *= 2;
		client->buf_in = realloc(client->buf_in, client->cap_in);
		if (client->buf_in == NULL)
			ft_fatal();
	}
	return (1);
}

/* Extract messages from buf_in and broadcast to other clients */
int	extract(t_client *client, int is_open)
{
	size_t	processed;
	size_t	len;

	processed = 0;
	len = 1;
	while (len != 0)
	{
		len = extract_one(client->id, client->buf_in + processed, '\n');
		processed += len;
	}
	if (!is_open)
	{
		len = extract_one(client->id, client->buf_in + processed, '\0');
		processed += len;
	}
	client->off_in -= processed;
	ft_memmove(client->buf_in, client->buf_in + processed,
		client->off_in + 1);
	return (is_open);
}

/* Extract one message from buffer and broadcast to other clients */
size_t	extract_one(int id, const char *buf, char delimiter)
{
	char	*str;
	char	*end;
	size_t	len;

	end = ft_strchr(buf, delimiter);
	if (end == NULL)
		return (0);
	len = end - buf + (delimiter != '\0');
	str = malloc(sizeof(char) * (len + 1));
	if (str == NULL)
		ft_fatal();
	ft_memmove(str, buf, len);
	str[len] = '\0';
	broadcast(id, str);
	free(str);
	return (len);
}

/* Add str into buf_out of other clients */
void	broadcast(int source, const char *str)
{
	char		str_source[100];
	size_t		len;
	t_client	*this;

	if (source < 0)
		sprintf(str_source, "server: ");
	else
		sprintf(str_source, "client %d: ", source);
	ft_putstr_fd(str_source, 1);
	ft_putstr_fd(str, 1);
	len = strlen(str_source) + strlen(str);
	this = g_clients;
	while (this != NULL)
	{
		if (this->id != source)
		{
			this->off_out += len;
			while (this->off_out + 1 > this->cap_out)
			{
				this->cap_out *= 2;
				this->buf_out = realloc(this->buf_out, this->cap_out);
				if (this->buf_out == NULL)
					ft_fatal();
			}
			strcat(this->buf_out, str_source);
			strcat(this->buf_out, str);
		}
		this = this->next;
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

/* Return the maximum fd used in this program. */
int	get_max_fd(int listener)
{
	int			retval;
	t_client	*this;

	retval = listener;
	this = g_clients;
	while (this != NULL)
	{
		if (this->fd > retval)
			retval = this->fd;
		this = this->next;
	}
	return (retval);
}

/* Call select to wait for events */
void	wait_events(fd_set *rfds, fd_set *wfds, int listener)
{
	t_client	*this;

	FD_ZERO(rfds);
	FD_ZERO(wfds);
	FD_SET(listener, rfds);
	this = g_clients;
	while (this != NULL)
	{
		FD_SET(this->fd, rfds);
		if (this->off_out > 0)
			FD_SET(this->fd, wfds);
		this = this->next;
	}
	select(get_max_fd(listener) + 1, rfds, wfds, NULL, NULL);
}

/* Manage connection, read or write events */
void	manage_events(fd_set *rfds, fd_set *wfds, int listener)
{
	t_client	*this;
	t_client	*next;

	if (FD_ISSET(listener, rfds))
		handle_connection(listener);
	this = g_clients;
	while (this != NULL)
	{
		next = this->next;
		if (FD_ISSET(this->fd, rfds))
		{
			if (!extract(this, receive(this)))
				client_remove(&this);
		}
		if (FD_ISSET(this->fd, wfds))
		{
			// transmit
		}
		this = next;
	}
}

int	main(int argc, char **argv)
{
	int		listener;
	fd_set	rfds;
	fd_set	wfds;

	if (argc != 2)
		ft_error("Wrong number of arguments\n");
	listener = setup_listener(atoi(argv[1]));
	while (1)
	{
		wait_events(&rfds, &wfds, listener);
		manage_events(&rfds, &wfds, listener);
	}
	close(listener);
	client_clear();
	return (0);
}
