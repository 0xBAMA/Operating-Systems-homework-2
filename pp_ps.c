#include <stdio.h>  /*standard io, file io*/
#include <dirent.h> /*directory functions*/
#include <stdlib.h> /*atol, atoi, qsort*/
#include <string.h> /*string handling, strtok*/
#include <unistd.h> /*sysconf*/

typedef struct{

  u_int pid;

  char command[50];

  char state;

  float cpu_perc;

  float mem_perc;

  u_long vsz;

  u_long rss;

  u_int last_cpu;

}entry;

entry entries[1000];
u_int num_entries = 0;

float uptime;

void readdirs();

int cpu_compare(entry a, entry b)
{
  return 0;
}

int mem_compare(entry a, entry b)
{
  return 0;
}

int pid_compare(entry a, entry b)
{
  return 0;
}

int com_compare(entry a, entry b)
{
  return 0;
}


int main(int argc, const char **argv) {

  const char * cpu_string = "-cpu";
  int (*compare)(entry a, entry b);

  if(argc != 2)
  {
    printf("make sure to format the call to pp_ps with either -cpu, -mem, -pid or -com\n");
  }

  FILE *fp = fopen("/proc/uptime","r");

  fscanf(fp,"%f",&uptime);

  readdirs();

  /*function pointers are neat*/
  if(!strcmp(argv[1], "-cpu")) {
      /*printf("ordering based upon cpu usage\n");*/
      compare = cpu_compare;
  }
  else if(!strcmp(argv[1], "-mem")) {
      /*printf("ordering based upon memory usage\n");*/
      compare = mem_compare;
  }
  else if(!strcmp(argv[1], "-pid")) {
      /*printf("ordering based upon pid\n");*/
      compare = pid_compare;
  }
  else if(!strcmp(argv[1], "-com")) {
      /*printf("ordering based upon command\n");*/
      compare = com_compare;
  }
  else {
      printf("Unknown argument passed to pp_ps\n");
      return 0;
  }



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

  double ptime;

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

        entries[num_entries].pid = atoi(strtok(data_string, " "));

        /*get the command - second entry*/

        token = strtok(NULL,")");

        sprintf(entries[num_entries].command,"%s", token+1); /*dump the leading parenthesis*/

        /*get the state - third entry*/

        token = strtok(NULL," ");

        entries[num_entries].state = token[0];

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

        utime = atol(strtok(NULL," "));

        /*get the program's stime - fifteenth entry*/

        stime = atol(strtok(NULL," "));

        /*more junk values*/

        token = strtok(NULL," ");/*sixteenth entry     - cutime*/
        token = strtok(NULL," ");/*seventeenth entry   - cstime*/
        token = strtok(NULL," ");/*eigteenth entry     - priority*/
        token = strtok(NULL," ");/*nineteenth entry    - nice*/
        token = strtok(NULL," ");/*twentieth entry     - num_threads*/
        token = strtok(NULL," ");/*twenty-first entry  - itrealvalue*/

        /*get the startime - twenty-second value*/
        /*also compute the %CPU usage*/

        ptime = utime/sysconf(_SC_CLK_TCK) + stime/sysconf(_SC_CLK_TCK);

        entries[num_entries].cpu_perc = (ptime * 100) / (uptime - atol(strtok(NULL," "))/sysconf(_SC_CLK_TCK));

        /*get the program's virtual memory size - twenty-third value*/

        entries[num_entries].vsz = atol(strtok(NULL," "));

        /*get the program's resident set size - twenty-fourth value*/
        /*also, compute the percent memory usage*/

        entries[num_entries].rss = atol(strtok(NULL," "));
        entries[num_entries].mem_perc = ((entries[num_entries].rss * 100 * sysconf(_SC_PAGE_SIZE)) / (sysconf(_SC_PAGE_SIZE)*sysconf(_SC_PHYS_PAGES)));

        /*even more junk values*/

        token = strtok(NULL," ");/*twenty-fifth    - rsslim*/
        token = strtok(NULL," ");/*twenty-sixth    - startcode*/
        token = strtok(NULL," ");/*twenty-seventh  - endcode*/
        token = strtok(NULL," ");/*twenty-eigth    - startstack*/
        token = strtok(NULL," ");/*twenty-ninth    - kstkesp*/
        token = strtok(NULL," ");/*thirtieth       - kstkeip*/
        token = strtok(NULL," ");/*thirty-first    - signal*/
        token = strtok(NULL," ");/*thirty-second   - blocked*/
        token = strtok(NULL," ");/*thirty-third    - sigignore*/
        token = strtok(NULL," ");/*thirty-fourth   - sigignore*/
        token = strtok(NULL," ");/*thirty-fifth    - wchan*/
        token = strtok(NULL," ");/*thirty-sixth    - nswap*/
        token = strtok(NULL," ");/*thirty-seventh  - cnswap*/
        token = strtok(NULL," ");/*thirty-eigth    - exit_signal*/

        /*get the last cpu that the program ran on - thirty-ninth value*/
        entries[num_entries].last_cpu = atoi(strtok(NULL," "));

        /*no more information to get from the file*/

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
