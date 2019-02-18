#include <stdio.h>  /*standard io, file io*/
#include <dirent.h> /*directory functions*/
#include <stdlib.h> /*atol, atoi, qsort*/
#include <string.h> /*string handling, strtok*/
#include <unistd.h> /*sysconf*/
#include <time.h>   /*for timing, just out of curiousity - then for the pp_top program, it'll be relevant*/

typedef struct{
/*all the information for the entry */
  u_int pid;

  char command[50];

  char state;

  float cpu_perc;

  float mem_perc;

  u_long vsz;

  u_long rss;

  u_int last_cpu;

}entry;


/*the list of entries, and an index of where the last entry was put - I figure this is easier than trying to do it dynamically */
entry entries[1000];
size_t num_entries = 0;

float uptime; /*in global scope, this gets used in readdirs() */


/*functions related to the compare sent to qsort */
void set_compare_function(const char* a);

int (*compare)(const void* a, const void* b);

int cpu_compare(const void* a, const void* b);

int mem_compare(const void* a, const void* b);

int pid_compare(const void* a, const void* b);

int com_compare(const void* a, const void* b);

void readdirs();


int main(int argc, const char **argv) {

  clock_t t = clock();


/*validate the input argument, then use it to decide which comparison function will be used */
  if(argc < 2)
  {
    printf("make sure to format the call to pp_ps with either -cpu, -mem, -pid or -com\n");
    exit(-1);
  }

/*set the compare function based upon the input argument */
  set_compare_function(argv[1]);


/*get the system uptime, to be used for the CPU usage calculation */
  FILE *fp = fopen("/proc/uptime","r");
  fscanf(fp,"%f",&uptime);
  fclose(fp);

/*get all the information from all running programs, i.e. /proc/<pid>/stat for all pids */
  readdirs();

/*output the headings
the headings are as follows -
PID      COMMAND       STATE      %CPU      %MEM       VSZ      RSS       LAST CPU*/
printf("  PID             COMMAND               STATE     %%CPU        %%MEM       VSZ(k)     RSS(k)     LAST_CPU\n");

/*sort the entries, with the relevant ordering */
  qsort(entries,num_entries,sizeof(entry),compare);

/*print out all the entries */

  for (size_t i = 0; i < num_entries; i++)
  {
    printf("%6d %-30.30s %5c    %8.5f    %8.5f %10ld   %8d        %d\n", entries[i].pid, entries[i].command, entries[i].state, entries[i].cpu_perc, entries[i].mem_perc, entries[i].vsz/1000, entries[i].rss/1000, entries[i].last_cpu);
  }

  printf("\n\nthe pp_ps program ran in %f ms\n", (((double)(clock()-t))/CLOCKS_PER_SEC)*1000);

  return 0;
}

void set_compare_function(const char* a)
{
  /*function pointers are neat */
  if(!strcmp(a, "-cpu"))
  {
      compare = cpu_compare;
  }
  else if(!strcmp(a, "-mem"))
  {
      compare = mem_compare;
  }
  else if(!strcmp(a, "-pid"))
  {
      compare = pid_compare;
  }
  else if(!strcmp(a, "-com"))
  {
      compare = com_compare;
  }
  else
  {
      printf("Unknown argument passed to pp_ps\n");
      exit(-1);
  }
}


void readdirs()
{
  DIR *dp;
  FILE *fp;

  char data_string[1000]; /*should be sufficient - the longest file I saw in the /proc/ directory was a few hundred characters */
  char *token;/*used to hold strings in some cases and also to get rid of junk values */

  long utime; /*these three variables hold values for computing the CPU time */
  long stime;
  double ptime;
  double rtime;

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
        /*printf("directory name: %s \n", ep->d_name);*/

        /*assemble a string to represent the filename of this pid's stat file*/

        sprintf(p,"%d",pid);

        strcpy(beg,"/proc/");

        strcpy(end,"/stat");

        strcat(beg,p);

        strcat(beg,end);

        /*beg holds the filename of /proc/<pid>/stat */

        fp = fopen(beg,"r");

        /*get the data from the file - everything up to the newline character */

        fgets(data_string,1000,fp);

        /*get the pid - first entry */

        entries[num_entries].pid = atoi(strtok(data_string, " "));

        /*get the command - second entry */

        token = strtok(NULL,")");

        sprintf(entries[num_entries].command,"%s", token+1); /*dump the leading parenthesis */

        /*get the state - third entry */

        token = strtok(NULL," ");

        if(!strcmp(token,")")) /*in the case of a command contained in parenthesis, i.e. one I had called "(sd-pam)" that was grabbing ")" as the state */
        {
          token = strtok(NULL," "); /*get the real state */
          strcat(entries[num_entries].command,")"); /*put the ")" back */
        }

        entries[num_entries].state = token[0];

        /*get rid of junk values */

        token = strtok(NULL," ");/* fourth entry     - ppid */
        token = strtok(NULL," ");/* fifth entry      - pgrp */
        token = strtok(NULL," ");/* sixth entry      - session */
        token = strtok(NULL," ");/* seventh entry    - tty_nr */
        token = strtok(NULL," ");/* eigth entry      - tpgid */
        token = strtok(NULL," ");/* ninth entry      - flags */
        token = strtok(NULL," ");/* tenth entry      - minflt */
        token = strtok(NULL," ");/* eleventh entry   - cminflt */
        token = strtok(NULL," ");/* twelfth entry    - majflt */
        token = strtok(NULL," ");/* thirteenth entry - cmajflt */

        /*get the program's utime - fourteenth entry */

        utime = atol(strtok(NULL," "));

        /*get the program's stime - fifteenth entry */

        stime = atol(strtok(NULL," "));

        /*more junk values */

        token = strtok(NULL," ");/*sixteenth entry     - cutime */
        token = strtok(NULL," ");/*seventeenth entry   - cstime */
        token = strtok(NULL," ");/*eigteenth entry     - priority */
        token = strtok(NULL," ");/*nineteenth entry    - nice */
        token = strtok(NULL," ");/*twentieth entry     - num_threads */
        token = strtok(NULL," ");/*twenty-first entry  - itrealvalue */

        /*get the startime - twenty-second value */
        /*also compute the %CPU usage now that we have all the relevant information */

        ptime = utime/sysconf(_SC_CLK_TCK) + stime/sysconf(_SC_CLK_TCK);
        rtime = uptime - atol(strtok(NULL," "))/sysconf(_SC_CLK_TCK);

        if(rtime != 0.0)
        {
          entries[num_entries].cpu_perc = (ptime * 100) / rtime;
        }
        else
        {
          entries[num_entries].cpu_perc = 0.0;
        }

        /*get the program's virtual memory size - twenty-third value */

        entries[num_entries].vsz = atol(strtok(NULL," "));

        /*get the program's resident set size - twenty-fourth value */
        /*also, compute the percent memory usage */

        entries[num_entries].rss = atol(strtok(NULL," "));
        entries[num_entries].mem_perc = (((double)entries[num_entries].rss * 100.0 * (double)sysconf(_SC_PAGE_SIZE)) / (double)(sysconf(_SC_PAGE_SIZE)*sysconf(_SC_PHYS_PAGES)));

        /*even more junk values */

        token = strtok(NULL," ");/*twenty-fifth    - rsslim */
        token = strtok(NULL," ");/*twenty-sixth    - startcode */
        token = strtok(NULL," ");/*twenty-seventh  - endcode */
        token = strtok(NULL," ");/*twenty-eigth    - startstack */
        token = strtok(NULL," ");/*twenty-ninth    - kstkesp */
        token = strtok(NULL," ");/*thirtieth       - kstkeip */
        token = strtok(NULL," ");/*thirty-first    - signal */
        token = strtok(NULL," ");/*thirty-second   - blocked */
        token = strtok(NULL," ");/*thirty-third    - sigignore */
        token = strtok(NULL," ");/*thirty-fourth   - sigignore */
        token = strtok(NULL," ");/*thirty-fifth    - wchan */
        token = strtok(NULL," ");/*thirty-sixth    - nswap */
        token = strtok(NULL," ");/*thirty-seventh  - cnswap */
        token = strtok(NULL," ");/*thirty-eigth    - exit_signal */

        /*get the last cpu that the program ran on - thirty-ninth value */
        entries[num_entries].last_cpu = atoi(strtok(NULL," "));

        /*no more information to get from the file */

        num_entries++; /*increment when finished - for the last element, this is important, since num_entries is used as a size argument for qsort */
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

/*comparison functions */

int cpu_compare(const void* a, const void* b)
{/*cast the void pointer as an 'entry' type */
  entry *entryA = (entry *)a;
  entry *entryB = (entry *)b;

  /*compare the relevant values */
  return (entryA->cpu_perc < entryB->cpu_perc);
}

int mem_compare(const void* a, const void* b)
{
  entry *entryA = (entry *)a;
  entry *entryB = (entry *)b;

  return (entryA->mem_perc < entryB->mem_perc);
}

int pid_compare(const void* a, const void* b)
{
  entry *entryA = (entry *)a;
  entry *entryB = (entry *)b;

  return (entryA->pid > entryB->pid);
}

int com_compare(const void* a, const void* b)
{
  entry *entryA = (entry *)a;
  entry *entryB = (entry *)b;

  return (strcmp(entryA->command,entryB->command));
}
