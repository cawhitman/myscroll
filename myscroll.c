#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>   
#include <sys/ioctl.h>
#include <termios.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>


//Prototypes
char * loadFile(int fd, int filesize);
int getFileSize(int fd);
void parseLines(char *filebuf, int filesize);
int printPage(int start, int t_rows);
void clearTerminal();
void run(int t_rows);
int scroll(int current_line_index);
//Globals
char **lineBuffer;
int line_buf_index;
int terminal;
struct termios orig_settings;


int main(int argc, char **argv) {

  //make sure valid file name is entered as a command line argument
  int fd;
  if ((fd = open(argv[1], O_RDWR))<0) 
  {
    printf("Missing valid textfile name in commandline argument.\n");
    exit(0);
  }

  fd = open(argv[1], O_RDWR );


  //get terminal window size
  int numCols;
  int numRows; 
  struct winsize winsizestruct;
  ioctl(1, TIOCGWINSZ, &winsizestruct);
  numCols = winsizestruct.ws_col;
  numRows = winsizestruct.ws_row - 1;
  printf("Columns: %d \n", numCols);
  printf("Rows: %d \n", numRows);

  //open terminal get/update settings
  terminal = open("/dev/tty", O_RDWR);

  tcgetattr(terminal, &orig_settings); 
  
  struct termios new_settings = orig_settings;

  /*Disable canonical mode */
  new_settings.c_lflag &= ~ICANON;     //disable canonical mode 
  new_settings.c_lflag &= ~ECHO;       //turn off echo
  new_settings.c_cc[VMIN] = 0;         // wait until at least one keystroke available
  new_settings.c_cc[VTIME] = 0;        // no timeout

  if(tcsetattr(terminal, TCSANOW, &new_settings) != 0){
    printf("Error applying terminal settings \n");
  }

   //create buffer for file to be read into
  int sizeoffile = getFileSize(fd);
  char *loadedFileBuffer = loadFile(fd, sizeoffile);  
  parseLines(loadedFileBuffer, sizeoffile); 
  close(fd);

  run(numRows);
 
  tcsetattr(terminal, TCSANOW, &orig_settings);
  exit(0);
}

void run(int t_rows)
{
  char command;
  int curr_line_index;
  curr_line_index = printPage(0, t_rows);
  
  while(curr_line_index != -1)
    {
      int percent = (curr_line_index *100)/line_buf_index;
      printf("\033[1G\033[7m--%s--(%d%%)","myScroll", percent);
      
      read(terminal,&command,sizeof(command));

      //if command is a space 
      if(command == 32)
      {
        printf("\033[0m");
        curr_line_index = printPage(curr_line_index, t_rows);
      }
      //if command is enter 
      if(command == 10)
      {
        printf("\033[0m");
        curr_line_index = scroll(curr_line_index);
      }
      command = 0;
    }
}


//Scroll function, initiated on press of return character
//f speeds up scrolling, s slows scrolling down
int scroll(int current_line_index)
{ 
  clearTerminal();
  char command = 0;
  char line[1000];
  int index = current_line_index;
  int scroll_speed = 1000000;//scroll speed microseconds
  while(1)
  {
    if(index >= line_buf_index)
    {
      return -1;
    }
    read(terminal,&command,sizeof(command));

    switch(command)
    {
      case 10:  //turn off scrolling
        return index; 

      case 102://f, scroll 20% faster
        scroll_speed = (scroll_speed * .80);
	      strcpy(line,lineBuffer[index]);
        printf("%s\n",line);
        index++;
        usleep(scroll_speed);
        command = 0;
	      continue;


      case 115://s, scroll 20% slower
        scroll_speed = (scroll_speed * .20) + scroll_speed;
	      strcpy(line,lineBuffer[index]);
        printf("%s\n",line);
        index++;
        usleep(scroll_speed);
        command = 0;
	      continue;

      case 113: //q, quit
	       tcsetattr(terminal, TCSANOW, &orig_settings);
	       exit(1);

      default:  //normal scroll
        strcpy(line,lineBuffer[index]);
        printf("%s\n",line);
        index++;  
        usleep(scroll_speed);
        command = 0;
    }
  }
}

char * loadFile(int fd, int filesize)
{
  char *loadedFileBuffer;
 
  loadedFileBuffer = malloc(filesize);

   //return to beginning and read in file
  read(fd, loadedFileBuffer, filesize);
 
  return loadedFileBuffer; 
}

int getFileSize(int fd)
{
  int buffLen;

  int flength = lseek(fd, 0, SEEK_END);
  buffLen = sizeof(char) * flength;
  lseek(fd, 0, SEEK_SET);

  return buffLen;
}


void parseLines(char *filebuf, int filesize)
{
  char *token;
  lineBuffer = malloc(1000 * sizeof(char *));

  line_buf_index = 0;
  token = strtok(filebuf, "\n");

  while(token != NULL)
  {
    lineBuffer[line_buf_index] = malloc(1000 * sizeof(char *));
    strcpy(lineBuffer[line_buf_index], token);
    token = strtok(NULL, "\n"); //get next token
    line_buf_index ++;

  }
}

int printPage(int start, int t_rows)
{
  clearTerminal();
  char line[1000];
  int i = start;
  int end = start + t_rows;
  while(i < end)
  { 
    if(i >= line_buf_index)
    {
      return -1;
    }
    strcpy(line,lineBuffer[i]);
    printf("%s\n",line);
    i++;
  }
  return i;
}

/* Clears the terminal and moves cursor to the start of the line. */
void clearTerminal()
{
    printf("\033[1G\033[0K");
}

