#include <stdio.h>  /*standard io*/
#include <dirent.h> /*directory functions*/
#include <stdlib.h> /*strtol, */
#include <string.h> /*string handling*/

struct entry{

  u_int pid;

  char command[50];

  char state;

  float cpu_perc;

  float mem_perc;

  u_long vsz;

  u_long rss;

  u_int last_cpu;

};

struct entry entries[500];
u_int num_entries = 0;

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

  char beg[50],p[50],end[50];

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

        sprintf(p,"%d",pid);

        strcpy(beg,"/proc/");

        strcpy(end,"/stat");

        strcat(beg,p);

        strcat(beg,end);

        printf("out: %s\n",beg);
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
