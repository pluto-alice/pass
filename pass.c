#include <stdio.h>
#include <stdlib.h>

#include <netdb.h>
#include <netinet/in.h>

#include <string.h>

#include <unistd.h> /* for close() */

/* find file name from path */
/* file name have to be shorter than 512 characters*/
/* remember to deallocate memory */
char *findFilename(char *filepath)
{
  size_t pt;
  size_t filenamePt = 0;
  char ch;
  char *filename = (char*) malloc(512);

  for (pt = 0; pt < strlen(filepath); pt++)
  {
    ch = filepath[pt];
    filename[filenamePt] = ch;
    filenamePt++;

    if(ch == '/')
    {
      bzero(filename, 512);
      filenamePt = 0;
    }
  }

  filename[filenamePt + 1] = '\0';

  return filename;
}

int serveFile(char* filepath, int port)
{
  FILE *file;
  char *buffer;
  long int fileLength;
  size_t bufferSize;

  /* get file name */
  char* filename = findFilename(filepath);
  if (!filename)
  {
    fprintf(stderr, "file path error\n");
    return -1;
  }



  /* open file */
  file = fopen(filepath, "rb");
  if (!file)
  {
    fprintf(stderr, "file open error\n");
    return -1;
  }

  /* get file length */
  fseek(file, 0, SEEK_END);
  fileLength=ftell(file);
  fseek(file, 0, SEEK_SET);
  bufferSize = fileLength + 1;

  /* allocate memory */
  buffer=(char *)malloc(bufferSize);
  if (!buffer)
  {
      fprintf(stderr, "allocate memory error\n");
      fclose(file);
      return -1;
  }

  /* read file */
  fread(buffer, fileLength, 1, file);
  fclose(file);

  /* compose header */
  char header[1024];

  sprintf(header,
          "HTTP/1.1 200 OK\n"
          "Content-Length: %li\n"
          "Accept-Ranges: bytes\n"
          "Content-Disposition: attachment; filename=\"%s\"\n"
          "\n", fileLength, filename);

  /* do not need filename anymore */
  free(filename);

  /* create socket */
  int socketfd = socket(PF_INET, SOCK_STREAM, 0);

  /* configure socket */
  struct sockaddr_in address;

  bzero(&address, sizeof(address));
  address.sin_family = AF_INET;
  address.sin_port = htons(port); /* host to network short */
  address.sin_addr.s_addr = INADDR_ANY;

  /* bind & listen */
  if(bind(socketfd, (struct sockaddr*) &address, sizeof(address)) != 0)
  {
    fprintf(stderr, "bind error\n");
    return -1;
  }

  if (listen(socketfd, 16)!=0)
  {
    fprintf(stderr, "listen error\n");
    return -1;
  }

  /* accept client */
  while (1)
  {
    socklen_t size = sizeof(address);
    int clientSocket = accept(socketfd, (struct sockaddr*) &address, &size);

    puts("client connected");

    if (fork() == 0)
    {
      write(clientSocket, header, strlen(header));
      write(clientSocket, buffer, bufferSize);
      exit(0);
    }
    else
    {
      close(clientSocket);
    }
  }

}


int main(int argc, char *argv[])
{
  if (argc < 3 || argc > 3)
  {
    fprintf(stderr, "needs 2 arguments: file path & port\n");
    return -1;
  }
  char *filepath = argv[1];
  int port = atoi(argv[2]);

  printf("serving %s on port %d\n"
       "file name(not path) cannot exceed 512 characters\n"
         "keyboard interrupt to kill\n", filepath, port);
  serveFile(filepath, port);
}
