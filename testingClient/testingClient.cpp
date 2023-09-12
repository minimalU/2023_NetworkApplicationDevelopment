// FILE          : testingClient.c
// PROJECT       : testingClient
// programmer    : Yujung Park
// FIRST VERSION : 2023-02-20
// DESCRIPTION   : This is a program that test a logging service.
// REFERENCE     : https://www.youtube.com/watch?v=vy1NIpA64pQ&list=PL7mmuO705dG265DxvC-oZgKViaRGt2-eS&index=15

#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#define BUFFER_SIZE 1024
#define INPUT_SIZE 64

const char kMachineID[INPUT_SIZE] = { 0 };

void printUI(char* mid);
void autoTestMenu();
void manualTestMenu();
void abuseTestMenu();
void logBuilder(char* message, char* device, char facility, int level, char* lmessage, int mnumb);
void* readMessage(void* socket);


int main(int argc, char* argv[])
{
    int my_sock = 0;
    struct sockaddr_in serv_addr;

    char message[BUFFER_SIZE] = { 0 };
    char logMessage[BUFFER_SIZE] = { 0 };
    char machineID[INPUT_SIZE] = { 0 };
    char userInput[BUFFER_SIZE] = { 0 };
    char testOptionInput[BUFFER_SIZE] = { 0 };
    int level = 1;
    int levelManual = 1;
    int requests = 1;
    int counter = 0;
    int msgNo = 1000;

    // !! input validation
    if(argc != 4)
    {
        printf("Usage: %s <IP> <PORT> <MACHINE ID>\n", argv[0]);
        exit(1);
    }

    // create socket
    my_sock = socket(PF_INET, SOCK_STREAM, 0);
    if(my_sock == -1)
    {
      printf("Error: socket error\n");
      return -1;
    }

    // set server address, port info & machineID
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr=inet_addr(argv[1]);
    serv_addr.sin_port=htons(atoi(argv[2]));
    sprintf(machineID, "%s", argv[3]);

    // connect socket
    if(connect(my_sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1)
    {
      printf("Error: logging service connection error\n");
      return -2;
    }
    else
    {
      // send an initial message of machine ID to server
      sprintf(message, "%s", machineID);
      write(my_sock, message, strlen(message));
      readMessage((void*)&my_sock); // connection confirmation

      // print menu and get user input for operation
      while(1)
      {
        printUI(machineID);
        printf("Please enter menu #>> ");
        if (fgets(userInput, BUFFER_SIZE, stdin) != NULL)
        {
          if (strcmp(userInput,"1\n") == 0)
          {
            autoTestMenu();
            // default value option input
            level = 1;
            while(1)
            {
              printf(">>Auto Logging>> ");
              if (fgets(testOptionInput, BUFFER_SIZE, stdin) != NULL)
              {
                // menu 1 auto logging
                if (strcmp(testOptionInput, "1\n") == 0)
                {
                  // build log and send it to the server
                  for (int i = level; i <= 5; i++)
                  {
                    logBuilder(message, machineID, 'a', i, logMessage, msgNo);
                    msgNo += 1;
                    printf("%s\n", message);

                    write(my_sock, message, strlen(message));

                    readMessage((void*)&my_sock); // server log confirmation
                  }
                  memset(message, '\0', strlen(message));
                  memset(logMessage, '\0', strlen(logMessage));
                }
                // menu 2 set logging level
                else if (testOptionInput[0] == '2')
                {
                  if (strcmp(testOptionInput, "2 debug\n") == 0) { level = 1; }
                  else if (strcmp(testOptionInput, "2 info\n") == 0) { level = 2; }
                  else if (strcmp(testOptionInput, "2 warning\n") == 0) { level = 3; }
                  else if (strcmp(testOptionInput, "2 error\n") == 0) { level = 4; }
                  else if (strcmp(testOptionInput, "2 critical\n") == 0) { level = 5; }
                  else { printf("Error: Invalid input\n"); }
                }
                // menu 3 manually configured log message
                else if (testOptionInput[0] == '3')
                {
                  strncpy(logMessage, testOptionInput + 2, strlen(testOptionInput) - 3);
                  logMessage[strlen(testOptionInput) - 3]='\0';
                }
                else
                {
                  // go back to main menu if user enter any key except 1-3
                  break;
                }
              }

              else
              {
                //fgets error
                exit(1);
              }
            }
          } // end main menu 1

          else if (strcmp(userInput,"2\n") == 0)
          {
            manualTestMenu();
            while(1)
            {
              printf(">>Manual Logging>> ");
              if (fgets(testOptionInput, BUFFER_SIZE, stdin) != NULL)
              {
                // menu 2 manual logging test
                if (strcmp(testOptionInput, "1\n") == 0)
                {
                  // build log and send it to the server
                  logBuilder(message, machineID, 'm', levelManual, logMessage, msgNo);
                  msgNo += 1;
                  printf("%s\n", message);

                  // send log message to server, clean buffer
                  write(my_sock, message, strlen(message));
                  memset(message, '\0', strlen(message));
                  memset(logMessage, '\0', strlen(logMessage));
                  // get server confirmation,
                  // if the server has no response? go back to menu
                  readMessage((void*)&my_sock);
                }
                // menu 2 set logging level
                else if (testOptionInput[0] == '2')
                {
                  if (strcmp(testOptionInput, "2 debug\n") == 0) { levelManual = 1; }
                  else if (strcmp(testOptionInput, "2 info\n") == 0) { levelManual = 2; }
                  else if (strcmp(testOptionInput, "2 warning\n") == 0) { levelManual = 3; }
                  else if (strcmp(testOptionInput, "2 error\n") == 0) { levelManual = 4; }
                  else if (strcmp(testOptionInput, "2 critical\n") == 0) { levelManual = 5; }
                  else { printf("Error: Invalid input\n"); }
                }
                // menu 3 manually configured log message
                else if (testOptionInput[0] == '3')
                {
                  size_t in_size = strlen(testOptionInput) - 3;
                  memcpy(logMessage, testOptionInput + 2, in_size);
                  logMessage[in_size] = '\0';
                }
                // go back to main menu if user enter any key except 1-3
                else
                {
                  break;
                }
              }
              else
              {
                exit(1);
              }
            }
          }// end main menu 2

          // Abuse prevention test
          else if (strcmp(userInput,"3\n") == 0)
          {
            // send x number of the same messages to server
            // rate limiter will filter this
            abuseTestMenu();
            while(1)
            {
              printf(">>Abuse Test>> ");
              if (fgets(testOptionInput, BUFFER_SIZE, stdin) != NULL)
              {
                // menu 2 manual logging test
                if (strcmp(testOptionInput, "1\n") == 0)
                {
                  // build log and send it to the server
                  logBuilder(message, machineID, 'b', levelManual, logMessage, msgNo);
                  msgNo += 1;
                  for(counter = 1; counter <= requests; counter++)
                  {
                    printf("Request# %d is sent: %s\n", counter, message);
                    // send log message to server, clean buffer
                    write(my_sock, message, strlen(message));
                    // get server confirmation,
                    // if the server has no response? go back to menu
                    readMessage((void*)&my_sock);
                  }
                  memset(message, '\0', strlen(message));
                  memset(logMessage, '\0', strlen(logMessage));
                }
                // menu 2 set logging level
                else if (testOptionInput[0] == '2')
                {
                  if (strcmp(testOptionInput, "2 debug\n") == 0) { levelManual = 1; }
                  else if (strcmp(testOptionInput, "2 info\n") == 0) { levelManual = 2; }
                  else if (strcmp(testOptionInput, "2 warning\n") == 0) { levelManual = 3; }
                  else if (strcmp(testOptionInput, "2 error\n") == 0) { levelManual = 4; }
                  else if (strcmp(testOptionInput, "2 critical\n") == 0) { levelManual = 5; }
                  else { printf("Error: Invalid input\n"); }
                }
                // menu 3 manually configured log message
                else if (testOptionInput[0] == '3')
                {
                  size_t in_size = strlen(testOptionInput) - 3;
                  memcpy(logMessage, testOptionInput + 2, in_size);
                  logMessage[in_size] = '\0';
                }
                else if (testOptionInput[0] == '4')
                {
                  // get the number from input_m
                  char num[INPUT_SIZE]= { 0 };
                  int retValue = 0;

                  if (strcmp(testOptionInput, "4\n") == 0)
                  {
                    printf("Error: invalid input\n");
                  }
                  else
                  {
                    size_t num_size = strlen(testOptionInput) - 3;
                    memcpy(num, testOptionInput + 2, num_size);
                    num[num_size] = '\0';
                    if ((retValue = atoi(num)) > 0)
                    {
                      requests = retValue;
                    }
                    else
                    {
                      printf("Error: invalid input\n");
                    }
                  }
                }
                // go back to main menu if user enter any key except 1-4
                else if (strcmp(userInput,"5\n") == 0 ||
                         strcmp(userInput,"b\n") == 0 ||
                         strcmp(userInput,"back\n") == 0)
                {
                  break;
                }
                else
                {
                  break;
                }
              }
              else
              {
                //fgets error
                exit(1);
              }
            }
          }// end main menu 3

          else if (strcmp(userInput,"4\n") == 0 ||
                   strcmp(userInput,"q\n") == 0 ||
                   strcmp(userInput,"quit\n") == 0)
          {
            exit(1);
          }// end main menu 4

          else
          {
            printf("Error: invalid input\n");
          }
        }
        else
        {
          //ferror
          return -3;
        }
      }
    }

    // close socket
    close(my_sock);
    return 0;
}

void* readMessage(void* socket)
{
  int sckt = *((int*)socket);
  char input_message[BUFFER_SIZE] = { 0 };
  int str_len = 0;

  str_len = read(sckt, input_message, sizeof(input_message));
  if (str_len == -1)
  {
    printf("ERROR: data read error\n");
    return (void*) -3;
  }

  printf("Logging service: %s\n", input_message);
  return NULL;
}


void logBuilder(char* message, char* device, char facility, int level, char* lmessage, int mnumb)
{
  // message = <date> <time> #<device#> #<Facilitycode(e.g.ATT)>-level-#<messageNumber>: messageText
  time_t rawtime;
	struct tm* timeinfo;
	time(&rawtime);
	timeinfo = localtime(&rawtime);


  memset(message, 0, BUFFER_SIZE);

  if(strcmp(lmessage,"\0") == 0)
  {
    strcpy(lmessage, "This is a default logging test message from testingClient.");
  }

  if(facility == 'a')
  {
    sprintf(message, "%04d%02d%02d%02d:%02d:%02d %s %s %d %d %s",
    timeinfo->tm_year+1900, timeinfo->tm_mon, timeinfo->tm_mday, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec,
    device, "auto", level, mnumb, lmessage);
  }
  if(facility == 'm')
  {
    sprintf(message, "%04d%02d%02d%02d:%02d:%02d %s %s %d %d %s",
    timeinfo->tm_year+1900, timeinfo->tm_mon, timeinfo->tm_mday, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec,
    device, "manual", level, mnumb, lmessage);
  }
  if(facility == 'b')
  {
    sprintf(message, "%04d%02d%02d%02d:%02d:%02d %s %s %d %d %s",
    timeinfo->tm_year+1900, timeinfo->tm_mon, timeinfo->tm_mday, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec,
    device, "abuse", level, mnumb, lmessage);
  }
}

void printUI(char* mid)
{
  printf("\n\n*A3 TEST CLIENT ***************************************************\n");
  printf(" Machine ID: %s\n\n", mid);
  printf("*MENU *************************************************************\n");
  printf(" 1\tAuto Logging Test\n");
  printf(" 2\tManual Logging Test\n");
  printf(" 3\tAbuse prevention Test\n");
  printf(" 4\tQuit\n");
  printf("*******************************************************************\n");
}

void autoTestMenu()
{
  printf("\n\n*A3 TEST CLIENT > Auto Logging Test *******************************\n");
  printf("*MENU *************************************************************\n");
  printf(" 1\tPerform auto logging test\t(usage: 1)\n\n");
  printf(" 2\tSet logging level\t\t(usage: 2 <level>)\n");
  printf(" <level>\t<Logging severity level>\n");
  printf(" debug\t\tDebugging messages\t(severity=1)\n");
  printf(" info\t\tInformational messages\t(severity=2)\n");
  printf(" warning\tWarning conditions\t(severity=3)\n");
  printf(" error\t\tError conditions\t(severity=4)\n");
  printf(" critical\tCritical conditions\t(severity=5)\n\n");
  printf(" 3\tSet logging message\t\t(usage: 3 <message>)\n");
  printf(" 4\tBack [go to main menu]\n");
  printf("*******************************************************************\n");
}

void manualTestMenu()
{
  printf("\n\n*A3 TEST CLIENT > Manual Logging Test *****************************\n");
  printf("*MENU *************************************************************\n");
  printf(" 1\tSend manual test log to the server\t(usage: 1)\n\n");
  printf(" 2\tChoose logging level\t\t(usage: 2 <level>)\n");
  printf(" <level>\t<Logging severity level>\n");
  printf(" debug\t\tDebugging messages\t(severity=1)\n");
  printf(" info\t\tInformational messages\t(severity=2)\n");
  printf(" warning\tWarning conditions\t(severity=3)\n");
  printf(" error\t\tError conditions\t(severity=4)\n");
  printf(" critical\tCritical conditions\t(severity=5)\n\n");
  printf(" 3\tSet logging message\t\t(usage: 3 <message>)\n");
  printf(" 4\tBack [go to main menu]\n");
  printf("*******************************************************************\n");
}

void abuseTestMenu()
{
  printf("\n*A3 TEST CLIENT > Abuse Prevention Test *************************\n\n");
  printf("*MENU *************************************************************\n");
  printf(" 1\tSend manual test log to the server\t(usage: 1)\n\n");
  printf(" 2\tChoose logging level\t\t(usage: 2 <level>)\n");
  printf(" <level>\t<Logging severity level>\n");
  printf(" debug\t\tDebugging messages\t(severity=1)\n");
  printf(" info\t\tInformational messages\t(severity=2)\n");
  printf(" warning\tWarning conditions\t(severity=3)\n");
  printf(" error\t\tError conditions\t(severity=4)\n");
  printf(" critical\tCritical conditions\t(severity=5)\n\n");
  printf(" 3\tSet logging message\t\t(usage: 3 <message>)\n");
  printf(" 4\tSet number of request \t\t(usage: 4 <# of requests>)\n");
  printf(" 5\tBack [go to main menu]\n");
  printf("*******************************************************************\n");
}
