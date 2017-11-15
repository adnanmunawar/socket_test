#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <unistd.h>

#define SERVER_PORT  80

#define TRUE             1
#define FALSE            0

int main (int argc, char *argv[])
{
  int    len, rc, on = 1;
  int    listen_sd = -1, new_sd = -1;
  int    desc_ready, end_server = FALSE, compress_array = FALSE;
  int    close_conn;
  char   buffer[512];
  char   file_buffer[512], response_buffer[512];
  struct sockaddr_in   addr, hints;
  int    timeout;
  struct pollfd fds[200];
  int    nfds = 1, current_size = 0, i, j;
  FILE *file_ptr;
  bool _file_set = false, _response_set = false;

  file_ptr = fopen("first.html","r");
  if(!file_ptr){
      printf("Error opening file \n");
      return -1;
  }
  else{
      _file_set = true;
      memset(file_buffer, 0, sizeof(file_buffer));
      size_t newLen = fread(file_buffer, sizeof(char), sizeof(file_buffer), file_ptr);
        if ( ferror( file_ptr ) != 0 ) {
            fputs("Error reading file", stderr);
        } else {
            file_buffer[newLen++] = '\n'; /* Just to be safe. */
        }

        fclose(file_ptr);
        printf("File Contents are : \n %s", file_buffer);
  }


  listen_sd = socket(AF_INET, SOCK_STREAM, 0);
  if (listen_sd < 0)
  {
    perror("socket() failed");
    exit(-1);
  }

  rc = setsockopt(listen_sd, SOL_SOCKET,  SO_REUSEADDR,
                  (char *)&on, sizeof(on));
  if (rc < 0)
  {
    perror("setsockopt() failed");
    close(listen_sd);
    exit(-1);
  }

  rc = fcntl(listen_sd, F_SETFL, O_NONBLOCK);
  if (rc < 0)
  {
    perror("FNCTL() failed");
    close(listen_sd);
    exit(-1);
  }

  memset(&addr, 0, sizeof(addr));
  addr.sin_family      = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port        = htons(SERVER_PORT);
  rc = bind(listen_sd,
            (struct sockaddr *)&addr, sizeof(addr));
  if (rc < 0)
  {
    perror("bind() failed");
    close(listen_sd);
    exit(-1);
  }

  rc = listen(listen_sd, 32);
  if (rc < 0)
  {
    perror("listen() failed");
    close(listen_sd);
    exit(-1);
  }

  memset(fds, 0 , sizeof(fds));

  fds[0].fd = listen_sd;
  fds[0].events = POLLIN;

  timeout = (3 * 60 * 1000);

  do
  {
    printf("Waiting on poll()...\n");
    rc = poll(fds, nfds, timeout);

    if (rc < 0)
    {
      perror("  poll() failed");
      break;
    }
    if (rc == 0)
    {
      printf("  poll() timed out.  End program.\n");
      break;
    }
    current_size = nfds;
    for (i = 0; i < current_size; i++)
    {
      if(fds[i].revents == 0)
        continue;
      if(fds[i].revents != POLLIN)
      {
        printf("  Error! revents = %d\n", fds[i].revents);
        end_server = TRUE;
        break;

      }
      if (fds[i].fd == listen_sd)
      {
        printf("  Listening socket is readable\n");
        do
        {
          new_sd = accept(listen_sd, NULL, NULL);
          if (new_sd < 0)
          {
            if (errno != EWOULDBLOCK)
            {
              perror("  accept() failed");
              end_server = TRUE;
            }
            break;
          }
          printf("  New incoming connection - %d\n", new_sd);
          fds[nfds].fd = new_sd;
          fds[nfds].events = POLLIN;
          nfds++;
        } while (new_sd != -1);
      }

      else
      {
        printf("  Descriptor %d is readable\n", fds[i].fd);
        close_conn = FALSE;

        do
        {
          rc = recv(fds[i].fd, buffer, sizeof(buffer), 0);
          if (rc < 0)
          {
            if (errno != EWOULDBLOCK)
            {
              perror("  recv() failed");
              close_conn = TRUE;
            }
            break;
          }
          if (rc == 0)
          {
            printf("  Connection closed\n");
            close_conn = TRUE;
            break;
          }

          len = rc;
          printf("  %d bytes received\n", len);
          printf("Message: %s", buffer);
          char resp[] = "HTTP/1.1 200 OK\r\n"
                        "Content-Length: 100\r\n"
                        "\r\n";
          rc = send(fds[i].fd, resp, strlen(resp), 0);
          //usleep(10000);
          rc = send(fds[i].fd, file_buffer, strlen(file_buffer), 0);
          if (rc < 0)
          {
            perror("  send() failed");
            close_conn = TRUE;
            break;
          }

        } while(TRUE);

        if (close_conn)
        {
          close(fds[i].fd);
          fds[i].fd = -1;
          compress_array = TRUE;
        }


      }  /* End of existing connection is readable             */
    } /* End of loop through pollable descriptors              */
    if (compress_array)
    {
      compress_array = FALSE;
      for (i = 0; i < nfds; i++)
      {
        if (fds[i].fd == -1)
        {
          for(j = i; j < nfds; j++)
          {
            fds[j].fd = fds[j+1].fd;
          }
          nfds--;
        }
      }
    }

  } while (end_server == FALSE); /* End of serving running.    */

  for (i = 0; i < nfds; i++)
  {
    if(fds[i].fd >= 0)
      close(fds[i].fd);
  }
}
