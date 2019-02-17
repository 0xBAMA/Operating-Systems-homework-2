#include <stdio.h>  /*standard io*/
#include <dirent.h> /*directory functions*/
#include <stdlib.h> /*strtol, */

struct entry{

  u_int pid;

  string command;

  char state;

  float cpu_perc;

  float mem_perc;

  u_long vsz;

  u_long rss;

  u_int last_cpu;
  
}

void readdirs();


int main(int argc, char const *argv[]) {

  readdirs();

  return 0;
}


void readdirs()
{
  DIR *dp;

  struct dirent *ep;

  long int pid;

  dp = opendir("/proc");

  if(dp != NULL)
  {
    while(ep = readdir(dp))
    {
      pid = strtol(ep->d_name, NULL, 10);

      if((ep->d_type == DT_DIR) && (pid > 0))
      {
        printf("directory name: %s \n", ep->d_name);

        /*do stuff with the directory*/
      }
    }
    closedir(dp);
  }
  else
  {
    perror("couldn't open directory");
    exit(-1);
  }
}
