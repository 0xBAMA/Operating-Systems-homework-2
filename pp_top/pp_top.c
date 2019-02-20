#include <stdio.h>  /*standard io, file io */
#include <dirent.h> /*directory functions */
#include <stdlib.h> /*atol, atoi, qsort */
#include <string.h> /*string handling, strtok */
#include <unistd.h> /*sysconf */
#include <time.h>   /*for timing, sleep */
#include <sys/ioctl.h> /*for the ioctl function */

#define gotoxy(x,y) printf("\033[%d;%dH", (x), (y))
#define clear() printf("\033[H\033[J")

/*terminal colors - https://en.wikipedia.org/wiki/ANSI_escape_code
also https://stackoverflow.com/questions/3219393/stdlib-and-colored-output-in-c */

#define ANSI_COLOR_RED          "\x1b[31m"
#define ANSI_COLOR_RED_BG       "\x1b[41m"


#define ANSI_COLOR_GREEN        "\x1b[32m"
#define ANSI_COLOR_YELLOW       "\x1b[33m"

#define ANSI_COLOR_BLUE         "\x1b[34m"
#define ANSI_COLOR_BLUE_BG      "\x1b[44m"


#define ANSI_COLOR_MAGENTA      "\x1b[35m"
#define ANSI_COLOR_MAGENTA_BG   "\x1b[45m"

#define ANSI_COLOR_CYAN         "\x1b[36m"
#define ANSI_COLOR_RESET        "\x1b[0m"

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

  struct winsize wsize;

  int term_rows;
  int term_cols;

  int total_running;

  double us_taken;
  double cpu_perc_total;
  double mem_perc_total;

  long total_system_memory = sysconf(_SC_PAGE_SIZE)*sysconf(_SC_PHYS_PAGES)/1000000;


/*validate the input argument, then use it to decide which comparison function will be used */
  if(argc < 2)
  {
    printf("make sure to format the call to pp_top with either -cpu, -mem, -pid or -com\n");
    exit(-1);
  }

/*set the compare function based upon the input argument */
  set_compare_function(argv[1]);

  clear();
  gotoxy(0,0);

  while(1)
  {
    /*reset */
      total_running = 0;
      cpu_perc_total = 0.0;
      mem_perc_total = 0.0;
      us_taken = 0.0;
      num_entries = 0;



    /*get information about the terminal window */
      ioctl(STDOUT_FILENO,TIOCGWINSZ,&wsize);
      term_rows = wsize.ws_row;
      term_cols = wsize.ws_col;


    if(term_rows < 24 || term_cols < 80)
    {/*terminal is not of sufficient size */
      printf("please resize your terminal\n");
      printf("minimum terminal size is set to 24 rows and 80 columns\n");
      printf("your current configuration is %d rows and %d columns\n", term_rows, term_cols);
    }
    else
    {/*terminal is at least 24 rows, and 80 columns - I saw this listed as a 'small' preset on a terminal emulator
      space is allocated for the different sections as follows

      PID| |COMMAND| |STATE| |%CPU| |%MEM| |VSZ(k)| |RSS(k)| |LAST
#chars 5  1   33    1   5   1  6   1  6   1   7    1   7    1  4 */

    /*get the system uptime, to be used for the CPU usage calculation */
      FILE *fp = fopen("/proc/uptime","r");
      fscanf(fp,"%f",&uptime);
      fclose(fp);

    /*get all the information from all running programs, i.e. /proc/<pid>/stat for all pids */
      readdirs();

    /*sort the entries, with the relevant ordering */
      qsort(entries,num_entries,sizeof(entry),compare);

    /*print out all the entries */

      for (size_t i = 0; i < num_entries; i++)
      {
        /*printf("%6d %-30.30s %5c    %8.5f    %8.5f %10ld   %8d        %d\n", entries[i].pid, entries[i].command, entries[i].state, entries[i].cpu_perc, entries[i].mem_perc, entries[i].vsz/1000, entries[i].rss/1000, entries[i].last_cpu);*/
        cpu_perc_total += entries[i].cpu_perc;
        mem_perc_total += entries[i].mem_perc;

        if(entries[i].state == 'R')
        {
          total_running++;
        }
      }


      /*output the system information - two lines (system information, then a row of padding)
 Running: <number of running processes> of <total system processes> total     CPU: <percent>%      MEM: <percent>% of <total_system_memory> */

      printf(ANSI_COLOR_BLUE_BG ANSI_COLOR_YELLOW);
      /*for some weird reason, under load, this first row disappears sometimes*/
      printf("Running processes: %4d of %4d total   CPU: %6.3f %%   MEM: %6.3f %% of %5ld M \n" ANSI_COLOR_RESET, total_running, num_entries, cpu_perc_total, mem_perc_total, total_system_memory);
      printf(ANSI_COLOR_BLUE_BG ANSI_COLOR_YELLOW);
      printf("═════════════════════════════════════════════════════════════════════════════════\n"ANSI_COLOR_RESET);
      printf(ANSI_COLOR_BLUE_BG ANSI_COLOR_YELLOW);


      /*output the headings - one line - using the terminal color codes, show which section is being used to sort */
      if(!strcmp(argv[1], "-cpu"))
      {
        printf(" PID | COMMAND                         |STATE|" ANSI_COLOR_MAGENTA_BG ANSI_COLOR_BLUE " %%CPU " ANSI_COLOR_RESET ANSI_COLOR_BLUE_BG ANSI_COLOR_YELLOW "| %%MEM |VSZ(mb)|RSS(mb)|LAST \n" ANSI_COLOR_RESET);
      }
      else if(!strcmp(argv[1], "-mem"))
      {
        printf(" PID | COMMAND                         |STATE| %%CPU |" ANSI_COLOR_MAGENTA_BG ANSI_COLOR_BLUE " %%MEM " ANSI_COLOR_RESET ANSI_COLOR_BLUE_BG ANSI_COLOR_YELLOW "|VSZ(mb)|RSS(mb)|LAST \n" ANSI_COLOR_RESET);
      }
      else if(!strcmp(argv[1], "-pid"))
      {
        printf(ANSI_COLOR_MAGENTA_BG ANSI_COLOR_BLUE " PID " ANSI_COLOR_RESET ANSI_COLOR_BLUE_BG ANSI_COLOR_YELLOW "| COMMAND                         |STATE| %%CPU | %%MEM |VSZ(mb)|RSS(mb)|LAST \n" ANSI_COLOR_RESET);
      }
      else if(!strcmp(argv[1], "-com"))
      {
        printf(" PID |" ANSI_COLOR_MAGENTA_BG ANSI_COLOR_BLUE " COMMAND                         " ANSI_COLOR_RESET ANSI_COLOR_BLUE_BG ANSI_COLOR_YELLOW "|STATE| %%CPU | %%MEM |VSZ(mb)|RSS(mb)|LAST \n" ANSI_COLOR_RESET);
      }

      /*output the info - at least 21 lines*/
      for (size_t j = 0; j < (term_rows-3); j++) {

        printf("%5d  %-30.30s     %c  ", entries[j].pid, entries[j].command, entries[j].state);


        /*coloration based upon cpu percentage - cyan(0-25%) -> green(25-50%) -> yellow(50-75%) -> red(75%+)*/
        if (entries[j].cpu_perc < 25.0)
        {
          printf(ANSI_COLOR_CYAN);
        }
        else if(entries[j].cpu_perc < 50.0)
        {
          printf(ANSI_COLOR_GREEN);
        }
        else if(entries[j].cpu_perc < 75.0)
        {
          printf(ANSI_COLOR_YELLOW);
        }
        else
        {
          printf(ANSI_COLOR_RED);
        }

        printf("%6.2f%%" ANSI_COLOR_RESET, entries[j].cpu_perc);

        printf("%6.2f%% ", entries[j].mem_perc);

        printf("%7ld ", entries[j].vsz/(1024*1024)); /*expressed in mb to save space*/

        printf("%7ld ", entries[j].rss/(1024*1024));

        printf("  %d", entries[j].last_cpu);





        if(j < (term_rows-4))
        {/*don't want to overrun, it bumps the top off*/
          printf("\n");
        }
      }
    }

    /*printf("waiting\n");*/
    usleep(1000000);

  /*reset the terminal */
    clear();
    gotoxy(0,0);

  }

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

        entries[num_entries].vsz = atol(strtok(NULL," "))*sysconf(_SC_PAGE_SIZE);

        /*get the program's resident set size - twenty-fourth value */
        /*also, compute the percent memory usage */

        entries[num_entries].rss = atol(strtok(NULL," "))*sysconf(_SC_PAGE_SIZE);
        entries[num_entries].mem_perc = ((double)entries[num_entries].rss * 100.0 / (double)(sysconf(_SC_PAGE_SIZE)*sysconf(_SC_PHYS_PAGES)));

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
