// http://www.cs.cmu.edu/afs/cs/academic/class/15213-s00/www/class28/tiny.c
/* 
 * tiny.c - a minimal HTTP server that serves static and
 *          dynamic content with the GET method. Neither 
 *          robust, secure, nor modular. Use for instructional
 *          purposes only.
 *          Dave O'Hallaron, Carnegie Mellon
 *
 *          usage: tiny <port>
 */
#include "quickjs/quickjs.h"
#include "quickjs/quickjs-libc.h"
#include <stdint.h>
#include <stdio.h>
#include <stdio.h>
#include <strings.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <fcntl.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUFSIZE 1024
#define MAXERRS 16

extern char **environ; /* the environment */

/*
 * error - wrapper for perror used for bad syscalls
 */
void error(char *msg) {
  perror(msg);
  exit(1);
}

/*
 * cerror - returns an error message to the client
 */
void cerror(FILE *stream, char *cause, char *errno, 
	    char *shortmsg, char *longmsg) {
  fprintf(stream, "HTTP/1.1 %s %s\n", errno, shortmsg);
  fprintf(stream, "Content-type: text/html\n");
  fprintf(stream, "\n");
  fprintf(stream, "<html><title>Tiny Error</title>");
  fprintf(stream, "<body bgcolor=""ffffff"">\n");
  fprintf(stream, "%s: %s\n", errno, shortmsg);
  fprintf(stream, "<p>%s: %s\n", longmsg, cause);
  fprintf(stream, "<hr><em>The Tiny Web server</em>\n");
}

extern const uint8_t preact[];
extern const uint32_t preact_size;
extern const uint8_t preact_module[];
extern const uint32_t preact_module_size;
extern const uint8_t htm[];
extern const uint32_t htm_size;
typedef struct JSString JSString;

char *strrpc(char *str,char *oldstr,char *newstr){
    char bstr[strlen(str)+strlen(newstr)+1];
    memset(bstr,0,sizeof(bstr));
    int i;
    for(i = 0;i < strlen(str);i++){
        if(!strncmp(str+i,oldstr,strlen(oldstr))){
            strcat(bstr,newstr);
            i += strlen(oldstr) - 1;
        }else{
                strncat(bstr,str + i,1);
            }
    }

    strcpy(str,bstr);
    return str;
}

const char* init(int argc,char* argv[]){
    int pfd[2];
    if(pipe(pfd)==-1){
        perror("pipe failed");
        exit(1);
    }
    int stdout_copy = dup(STDOUT_FILENO);
    dup2(pfd[1], STDOUT_FILENO);
    close(pfd[1]);

    JSRuntime *rt;
    JSContext *ctx;

    rt = JS_NewRuntime();
    JS_SetModuleLoaderFunc(rt, NULL, js_module_loader, NULL);
    js_std_init_handlers(rt);

    ctx = JS_NewContext(rt);
    js_std_add_helpers(ctx, argc, argv);
    js_init_module_std(ctx, "std");
    js_init_module_os(ctx, "os");

    js_std_eval_binary(ctx, preact, preact_size, 0);
    js_std_eval_binary(ctx, htm, htm_size,0);
    
    if (argc < 3) {
        printf("A file need to be specified");
        exit(1);
    }
    char* filename = argv[2];
    char *buf;
    size_t buf_len;
    buf = (char*)js_load_file(ctx, &buf_len, filename);
    if (!buf) {
        perror(filename);
        exit(1);
    }

    JSValue val = JS_Eval(ctx, buf, buf_len, filename, JS_EVAL_TYPE_MODULE);
    js_std_loop(ctx);


    if (JS_IsException(val)) {
        js_std_dump_error(ctx);
        exit(1);
    }

    // JSString* js_str = JS_VALUE_GET_STRING(val);
    // char* res = (char*)malloc(js_str->len+1);
    // memcpy(res, js_str->u.str8, js_str->len);
    fflush(stdout);
    dup2(stdout_copy, 1);
    char res[BUFSIZE];
    while(1){
        int n = read(pfd[0], res, BUFSIZE-1);
        if(n<=0){
            break;
        }
    }

    JS_FreeValue(ctx, val);
    js_free(ctx, buf);
    JS_FreeContext(ctx);
    JS_FreeRuntime(rt);

    buf = (char*)js_load_file(NULL, &buf_len, "index.html");
    
    strrpc(buf, "PLACE_HOLDER", res);

    return buf;
}

int main(int argc, char **argv) {

  /* variables for connection management */
  int parentfd;          /* parent socket */
  int childfd;           /* child socket */
  int portno;            /* port to listen on */
  int clientlen;         /* byte size of client's address */
  struct hostent *hostp; /* client host info */
  char *hostaddrp;       /* dotted decimal host addr string */
  int optval;            /* flag value for setsockopt */
  struct sockaddr_in serveraddr; /* server's addr */
  struct sockaddr_in clientaddr; /* client addr */

  /* variables for connection I/O */
  FILE *stream;          /* stream version of childfd */
  char buf[BUFSIZE];     /* message buffer */
  char method[BUFSIZE];  /* request method */
  char uri[BUFSIZE];     /* request uri */
  char version[BUFSIZE]; /* request method */
  char filename[BUFSIZE];/* path derived from uri */
  char filetype[BUFSIZE];/* path derived from uri */
  char cgiargs[BUFSIZE]; /* cgi argument list */
  char *p;               /* temporary pointer */
  int is_static;         /* static request? */
  struct stat sbuf;      /* file status */
  int fd;                /* static content filedes */
  int pid;               /* process id from fork */
  int wait_status;       /* status from wait */

  /* check command line args */
  if (argc < 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }
  portno = atoi(argv[1]);

  /* open socket descriptor */
  parentfd = socket(AF_INET, SOCK_STREAM, 0);
  if (parentfd < 0) 
    error("ERROR opening socket");

  /* allows us to restart server immediately */
  optval = 1;
  setsockopt(parentfd, SOL_SOCKET, SO_REUSEADDR, 
	     (const void *)&optval , sizeof(int));

  /* bind port to socket */
  bzero((char *) &serveraddr, sizeof(serveraddr));
  serveraddr.sin_family = AF_INET;
  serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
  serveraddr.sin_port = htons((unsigned short)portno);
  if (bind(parentfd, (struct sockaddr *) &serveraddr, 
	   sizeof(serveraddr)) < 0) 
    error("ERROR on binding");

  /* get us ready to accept connection requests */
  if (listen(parentfd, 5) < 0) /* allow 5 requests to queue up */ 
    error("ERROR on listen");

  /* 
   * main loop: wait for a connection request, parse HTTP,
   * serve requested content, close connection.
   */

  const char* res =  init(argc, argv);
  printf("GENERAYE:\n %s\n",res);
  fflush(stdout);

  clientlen = sizeof(clientaddr);
  while (1) {

    /* wait for a connection request */
    childfd = accept(parentfd, (struct sockaddr *) &clientaddr, (socklen_t*)&clientlen);
    if (childfd < 0) 
      error("ERROR on accept");
    hostaddrp = inet_ntoa(clientaddr.sin_addr);
    if (hostaddrp == NULL)
      error("ERROR on inet_ntoa\n");
    
    /* open the child socket descriptor as a stream */
    if ((stream = fdopen(childfd, "r+")) == NULL)
      error("ERROR on fdopen");

    /* get the HTTP request line */
    bzero(buf, BUFSIZE);
    char* _ = fgets(buf, BUFSIZE, stream);
    printf("%s", buf);
    sscanf(buf, "%s %s %s\n", method, uri, version);

    /* tiny only supports the GET method */
    if (strcasecmp(method, "GET")) {
      cerror(stream, method, "501", "Not Implemented", 
	     "Tiny does not implement this method");
      fclose(stream);
      close(childfd);
      continue;
    }

    /* read (and ignore) the HTTP headers */
    _ = fgets(buf, BUFSIZE, stream);
    printf("%s", buf);
    while(strcmp(buf, "\r\n")) {
      char* _ = fgets(buf, BUFSIZE, stream);
      printf("%s", buf);
    }

    bzero(filename, sizeof(filename));
    strcpy(filename, ".");
    strcat(filename, uri);
    if (uri[strlen(uri)-1] == '/') 
        strcat(filename, "index.html");

    /* make sure the file exists */
    if (stat(filename, &sbuf) < 0) {
      cerror(stream, filename, "404", "Not found", 
	     "Tiny couldn't find this file");
      fclose(stream);
      close(childfd);
      continue;
    }

    /* serve static content */
      if (strstr(filename, ".html"))
	strcpy(filetype, "text/html");
      else if (strstr(filename, ".gif"))
	strcpy(filetype, "image/gif");
      else if (strstr(filename, ".jpg"))
	strcpy(filetype, "image/jpg");
      else if (strstr(filename, ".js"))
	    strcpy(filetype, "application/javascript");
      else 
	strcpy(filetype, "text/plain");

      /* print response header */
      fprintf(stream, "HTTP/1.1 200 OK\n");
      fprintf(stream, "Server: Tiny Web Server\n");
      fprintf(stream, "Content-type: %s\n", filetype);

      /* Use mmap to return arbitrary-sized response body */
      if(!strcmp("./index.html", filename)){
        fprintf(stream, "Content-length: %d\n", (int)strlen(res));
        fprintf(stream, "\r\n"); 
        fprintf(stream, "%s",res);
        fflush(stream);
      } else {
        fprintf(stream, "Content-length: %d\n", (int)sbuf.st_size);
        fprintf(stream, "\r\n"); 
        fd = open(filename, O_RDONLY);
        p = mmap(0, sbuf.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
        fwrite(p, 1, sbuf.st_size, stream);
        munmap(p, sbuf.st_size);
        fflush(stream);
      }
    /* clean up */
    fclose(stream);
    close(childfd);

  }
}