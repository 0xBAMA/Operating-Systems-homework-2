#include <stdio.h>  /*standard io, file io*/
#include <dirent.h> /*directory functions*/
#include <stdlib.h> /*atol, atoi*/
#include <string.h> /*string handling, strtok*/

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

float uptime;

void readdirs();


int main(int argc, char const *argv[]) {

  FILE *fp = fopen("/proc/uptime","r");

  fscanf(fp,"%f",&uptime);

  readdirs();

  return 0;
}


void readdirs()
{
  DIR *dp;
  FILE *fp;

  char data_string[1000];
  char *token;

  long utime;
  long stime;

  void *junk;


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

        /*assemble a string to represent the filename of this pid's stat file*/

        sprintf(p,"%d",pid);

        strcpy(beg,"/proc/");

        strcpy(end,"/stat");

        strcat(beg,p);

        strcat(beg,end);

        /*beg holds the filename of /proc/<pid>/stat */

        fp = fopen(beg,"r");

        /*get the data from the file*/

        fgets(data_string,1000,fp);

        /*get the pid - first entry*/

        token = strtok(data_string, " ");

        entries[num_entries].pid = atoi(token);

        /*get the command - second entry*/

        token = strtok(NULL,")");

        sprintf(token,"%s", token+1); /*dump the leading parenthesis*/

        entries[num_entries].command = token;

        /*get the state - third entry*/

        token = strtok(NULL," ");

        entries[num_entries].state = token;

        /*get rid of junk values*/

        token = strtok(NULL," ");/* fourth entry     - ppid*/
        token = strtok(NULL," ");/* fifth entry      - pgrp*/
        token = strtok(NULL," ");/* sixth entry      - session*/
        token = strtok(NULL," ");/* seventh entry    - tty_nr*/
        token = strtok(NULL," ");/* eigth entry      - tpgid*/
        token = strtok(NULL," ");/* ninth entry      - flags*/
        token = strtok(NULL," ");/* tenth entry      - minflt*/
        token = strtok(NULL," ");/* eleventh entry   - cminflt*/
        token = strtok(NULL," ");/* twelfth entry    - majflt*/
        token = strtok(NULL," ");/* thirteenth entry - cmajflt*/

        /*get the program's utime - fourteenth entry*/

        token = strtok(NULL," ");
        utime = atol(token);

        /*get the program's stime - fifteenth entry*/

        token = strtok(NULL," ");
        stime = atol(token);

        /*****compute the time, for %CPU******/



        token = strtok(NULL," ");


        num_entries++;
        fclose(fp);
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
