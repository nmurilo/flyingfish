// Flynig Fish - Flipper ESP32 console 
// Version 0.1 - Initial public release 
// Version 0.2 - Add sniff command support - 2023-08-03
//               
// (c)  Nelson Murilo - nmurilo@gmail.com 
// License: AMS 

#include <termios.h>
#include <string.h>
#include <stdio.h> 
#include <unistd.h> 
#include <stdlib.h> 
#include <fcntl.h>
#include <errno.h>

#define DEV "/dev/tty.usbmodemflip_Myself1"
#define ENTER "\n\r"
#define MAXBUF 1024
#define TRUE 1
#define FALSE 0

int getln(int, char *, size_t, int) ; 
int scanap(int); 
int sniff(int, char *); 

int getln(int fd, char *ln, size_t len, int prompt)  
{
   char c;  
   size_t l = 0; 
   if (prompt) fprintf(stderr, "ff> ");    
   while((read (fd, &c, 1)) > 0) 
   {
      if (!l && c == '\n') 
      {
         if (prompt)
            fprintf(stderr, "ff> ");    
         continue; 
      }
      
      if (c == '\n' || l >= len)
      { 
         *ln='\0'; 
         return l; 
      } 
      l++; 
      *ln++=c; 
   }
   return l; 
}

int main(int argc, char *argv[])
{
   int fd; 
   int r;
   char c; 
   size_t len = 0; 
   char line[MAXBUF];
   struct termios new_kbd_mode;
   struct termios g_old_kbd_mode;
   char tty[128] = DEV; 

   if (argc == 2) 
   { 
      len = (size_t) strlen(argv[1]); 
      memset(&tty[0], '\0', 127); 
      strncpy(tty, argv[1], (len < 127) ? len : 127); 
   } 

   if ((fd = open(tty, O_SYNC|O_RDWR|O_NOCTTY|O_NDELAY)) < 0) 
   {
     fprintf(stderr, "Error opening %s\n", tty); 
     exit(1); 
   } 

   tcgetattr (fd, &g_old_kbd_mode);
   memcpy (&new_kbd_mode, &g_old_kbd_mode, sizeof (struct termios));

   new_kbd_mode.c_cc[VTIME] = 10;
   new_kbd_mode.c_cc[VMIN] = 0;
   new_kbd_mode.c_cflag |= CS8;
   new_kbd_mode.c_cflag &= ~PARENB;
   new_kbd_mode.c_cflag &= ~CSTOPB; 
   new_kbd_mode.c_cflag &= ~CRTSCTS; 
   new_kbd_mode.c_cflag |= CREAD | CLOCAL; 
//   new_kbd_mode.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
   new_kbd_mode.c_lflag &=  ~(IXON | IXOFF | IXANY);
   cfsetispeed(&new_kbd_mode, B115200); 
   cfsetospeed(&new_kbd_mode, B115200); 
   tcsetattr (fd, TCSANOW, &new_kbd_mode);
   tcflush(fd, TCIFLUSH);
   while ((len = getln(0, line, 64, TRUE)) > 0)
   {
      if (!strncmp("exit", line, 4))
         break; 
      if (!strncmp("cls", line, 3))
      {
         system("clear"); 
         continue;
      } 
      if (!strncmp("scanap", line, 6))
      { 
         len = scanap(fd); 
         strcpy(line, "stopscan"); 
         len = 8; 
      } 
      if (!strncmp("sniff", line, 5))
      { 
         len = sniff(fd, line); 
         strcpy(line, "stopscan"); 
         len = 8; 
      } 
      if ((r = write(fd, line, len)) < 0)
      {
        fprintf(stderr, "Error writing %s errno=(%d) r=%d \n", 
                         tty, errno, r ); 
        exit(1); 
      }
      sleep(2); 
      while ((read (fd, &c, 1)) > 0) 
         printf("%c", c); 
   }
   tcsetattr (fd, TCSANOW, &g_old_kbd_mode);
}

int scanap(int fd)
{ 
   char c; 
   char ln[16] = "scanap"; 
   int len; 

   fcntl(0, F_SETFL, fcntl(0, F_GETFL) | O_NONBLOCK); 
   tcflush(0, TCIFLUSH);

   write(fd, ln, strlen(ln)); 

   while (strncmp("stopscan", ln, 8))
   { 
      sleep(1); 
      while ((read (fd, &c, 1)) > 0)
         printf("%c", c); 
      ln[0] = '\0'; 
      len = getln(0, ln, 15, FALSE); 
      if  ( (ln[0] != '\0') && strncmp("stopscan", ln, 8)) 
          write(fd, ln, (strlen(ln))); 
   } 
   fcntl(0, F_SETFL, fcntl(0, F_GETFL) & (~O_NONBLOCK)); 
   return len; 
}

int sniff(int fd, char *line)
{ 
   char c; 
   char ln[16]; 
   int len; 
   
   strncpy(ln, line, 15); 
   fcntl(0, F_SETFL, fcntl(0, F_GETFL) | O_NONBLOCK); 
   tcflush(0, TCIFLUSH);

   write(fd, ln, strlen(ln)); 

   while (strncmp("stopscan", ln, 8))
   { 
      sleep(1); 
      while ((read (fd, &c, 1)) > 0)
         printf("%c", c); 
      ln[0] = '\0'; 
      len = getln(0, ln, 15, FALSE); 
      if  ( (ln[0] != '\0') && strncmp("stopscan", ln, 8)) 
          write(fd, ln, (strlen(ln))); 
   } 
   fcntl(0, F_SETFL, fcntl(0, F_GETFL) & (~O_NONBLOCK)); 
   return len; 
}
