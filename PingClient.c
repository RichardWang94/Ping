#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <time.h>

int main( int argc, char **argv ) {
  int sockfd, portno, n;
  struct sockaddr_in serveraddr;
  struct hostent *server;
  int serverlen;
  char *hostname;
  char buf[1024];
  int seqNumber;
  time_t ltime;
  struct timeval tv;
  struct timeval tvBegin, tvEnd;
  int diff;
  int receivedpackets;
  int min, max, sum;
  double avg, packetloss;
  tv.tv_sec = 1;
  tv.tv_usec = 0;

  /* check command line arguments */
  if (argc != 3) {
    printf("Format is PingClient host port\n");
    exit(0);
  }
  hostname = argv[1];
  portno = atoi(argv[2]);

  /* socket: create the socket */
  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0) {
    printf("ERROR opening socket\n");
    exit(0);
  }

  /* gethostbyname: get the server's DNS entry */
  server = gethostbyname(hostname);
  if (server == NULL) {
    printf("ERROR, no such host as %s\n", hostname);
    exit(0);
  }

  /* build the server's Internet address */
  bzero((char *) &serveraddr, sizeof(serveraddr));
  serveraddr.sin_family = AF_INET;
  bcopy((char *)server->h_addr, (char *)&serveraddr.sin_addr.s_addr, server->h_length);
  serveraddr.sin_port = htons(portno);

  /* build and send ping requests, wait for timeouts and repies */
  serverlen = sizeof(serveraddr);
  receivedpackets = 0;
  min = -1;
  max = -1;
  sum = 0;
  for (seqNumber = 0; seqNumber < 10; seqNumber++) {
    ltime = time(NULL);
    snprintf(buf, sizeof(buf), "PING #%c %s", seqNumber, asctime(localtime(&ltime)));
    tvBegin.tv_sec = 0;
    tvBegin.tv_usec = 0;
    tvEnd.tv_sec = 0;
    tvBegin.tv_usec = 0;
    gettimeofday(&tvBegin, NULL);
    n = sendto(sockfd, buf, strlen(buf), 0, (struct sockaddr* )&serveraddr, serverlen);
    if (n < 0) {
      printf("ERROR in sendto\n");
      exit(0);
    }
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
      printf("ERROR in setting timeout value\n");
      exit(0);
    }
    n = recvfrom(sockfd, buf, strlen(buf), 0, (struct sockaddr* )&serveraddr, &serverlen);
    if (n < 0) {
      printf("Timeout reached\n");
    }
    else {
      gettimeofday(&tvEnd, NULL);
      diff = ((((unsigned long long)tvEnd.tv_sec) * 1000) + (((unsigned long long)tvEnd.tv_usec) / 1000)) - ((((unsigned long long)tvBegin.tv_sec) * 1000) + (((unsigned long long)tvBegin.tv_usec) / 1000)); 
      printf("PING received from %s: seq#=%d time=%d ms\n", hostname, seqNumber, diff);
      receivedpackets++;
      if (min == -1 || diff < min) min = diff;
      if (max == -1 || diff > max) max = diff;
      sum += diff;
    }
  }
  packetloss = ((double)(seqNumber - receivedpackets))/seqNumber * 100;
  avg = ((double)sum) / receivedpackets;
  printf("--- ping statistics --- %d packets transmitted, %d received, %f%% packet loss rtt min/avg/max = %d %f %d ms\n",
	 seqNumber, receivedpackets, packetloss, min, avg, max);
  close(sockfd);
  return 0;
}
