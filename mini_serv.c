#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>

void	ft_putstr_fd(const char *s, int fd);
void	ft_error(const char *s);
void	ft_fatal(void);

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

int extract_message(char **buf, char **msg)
{
	char	*newbuf;
	int	i;

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

char *str_join(char *buf, char *add)
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


int main() {
	int sockfd, connfd;
	socklen_t len;
	struct sockaddr_in servaddr, cli; 

	// socket create and verification 
	sockfd = socket(AF_INET, SOCK_STREAM, 0); 
	if (sockfd == -1) { 
		ft_putstr_fd("socket creation failed...\n", 1); 
		exit(0); 
	} 
	else
		ft_putstr_fd("Socket successfully created..\n", 1); 
	bzero(&servaddr, sizeof(servaddr)); 

	// assign IP, PORT 
	servaddr.sin_family = AF_INET; 
	servaddr.sin_addr.s_addr = htonl(2130706433); //127.0.0.1
	servaddr.sin_port = htons(8081); 
  
	// Binding newly created socket to given IP and verification 
	if ((bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr))) != 0) { 
		ft_putstr_fd("socket bind failed...\n", 1); 
		exit(0); 
	} 
	else
		ft_putstr_fd("Socket successfully binded..\n", 1);
	if (listen(sockfd, 10) != 0) {
		ft_putstr_fd("cannot listen\n", 1); 
		exit(0); 
	}
	len = sizeof(cli);
	connfd = accept(sockfd, (struct sockaddr *)&cli, &len);
	if (connfd < 0) { 
        ft_putstr_fd("server acccept failed...\n", 1); 
        exit(0); 
    } 
    else
        ft_putstr_fd("server acccept the client...\n", 1);
}
