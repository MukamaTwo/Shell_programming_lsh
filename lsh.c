/* 
 * GROUP 10: LAB 1: OPERATING SYSTEMS 
 *Shell scripting: Implementation of lsh shell
 *
 * Main source code file for lsh shell program
 *
 * You are free to add functions to this file.
 * If you want to add functions in a separate file 
 * you will need to modify Makefile to compile
 * your additional functions.
 *
 * Add appropriate comments in your code to make it
 * easier for us while grading your assignment.
 *
 * Submit the entire lab1 folder as a tar archive (.tgz).
 * Command to create submission archive: 
      $> tar cvf lab1.tgz lab1/
 *
 * All the best 
 */


#include <stdio.h>
#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "parse.h"
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>  
#include <signal.h>   
         
#define READ_END 0
#define WRITE_END 1
/*
 * Signal handler Function declarations
 */
void sig_handler( int  );


/*
 * Function declarations
 */

void PrintCommand(int, Command *);
void PrintPgm(Pgm *);
void stripwhite(char *);
void PipePgm(Pgm *);
void RedirectIn(char *);
void RedirectOut(char *);
int builtincmd(Command *cmd);
static char * dirr;
/* When non-zero, this global means the user is done using this program. */
int done = 0;

/*
 * Name: main
 *
 * Description: Gets the ball rolling...
 *
 */

int main(void)
{
Command cmd;
int n;
pid_t child_pid, pid;     /*declaration of child process*/
signal(SIGINT, sig_handler);   /*declaring/ registering signals*/
signal(SIGCHLD, sig_handler);

   
while (!done) 
{
    char *line;
    line = readline("> ");

    if (!line) 
      {
        /* Encountered EOF at top level */
        done = 1;
      }
    else 
      {
      /*
      * Remove leading and trailing whitespace from the line
      * Then, if there is anything left, add it to the history list
      * and execute it.
      */
      stripwhite(line);

      if(*line) 
        {
          add_history(line);
          /* execute it */

          n = parse(line, &cmd);

          //PrintCommand(n, &cmd);

          /*run built in commands*/
        if(builtincmd(&cmd) == 1) 
            {     }

        else 
         {

           /*create a child process by fork()*/
          if ((child_pid= fork())<0)
              {
              perror("Fork failed");
              return 1;
              }

          else if (child_pid==0)
              {
              /*execute child process*/
              /*BACKGOUND PROCESS:- */
              if (cmd.bakground==1)  
                {
                  signal(SIGINT,SIG_IGN);
                }

              /*REDIRECTION: - stdin to input_file and stdout to output_file*/
              if (cmd.rstdin != NULL)
                 RedirectIn(cmd.rstdin);

              if (cmd.rstdout != NULL)
                 RedirectOut(cmd.rstdout);

              /*PIPING PROCESS:-*/
              if(cmd.pgm->next != NULL)
                 PipePgm (cmd.pgm);
     
              /*execute simple commands from in /usr/bin, /usr/local/bin*/
               if(execvp(*cmd.pgm->pgmlist,cmd.pgm->pgmlist)== -1)
               {
                fprintf(stderr, "%s\n","Command not found." );  
                // return;
                exit(1);
               }
              }

          else
            /*execute parent process*/
              {
                
                if (cmd.bakground) 
                  { /*do nothing */ }
              
                else      
                /*if no background process, wait for child*/
                 wait(NULL);

              }

          }// end of else builtin
          
        } /*end of if(*line) */
      } /*end of first else*/
    
    if(line) 
    {
      free(line);
    }
  } /*end of while loop*/

  return 0;
}  /*end of main function*/

/*
 * Name: PrintCommand
 *
 * Description: Prints a Command structure as returned by parse on stdout.
 *
 */
void PrintCommand (int n, Command *cmd)
  {
    printf("\nParse returned %d:\n", n);
    printf("   stdin : %s\n", cmd->rstdin  ? cmd->rstdin  : "<none>" );
    printf("   stdout: %s\n", cmd->rstdout ? cmd->rstdout : "<none>" );
    printf("   bg    : %s\n", cmd->bakground ? "yes" : "no");
    PrintPgm(cmd->pgm);
  }

/*
 * Name: PrintPgm
 *
 * Description: Prints a list of Pgm:s
 *
 */
void PrintPgm (Pgm *p)
  {
    if (p == NULL) 
      {
      return;
      }
    else 
      {
        char **pl = p->pgmlist;

        /* The list is in reversed order so print
        * it reversed to get right
        */
        PrintPgm(p->next);
        printf("    [");

        while (*pl) 
          {
          printf("%s ", *pl++);
          }

        printf("]\n");
      }
  }

/*
 * Name: stripwhite
 *
 * Description: Strip whitespace from the start and end of STRING.
 */
void stripwhite (char *string)
  {
    register int i = 0;

    while (whitespace( string[i] )) 
      {
        i++;
      }

    if (i) 
      {
        strcpy (string, string + i);
      }

    i = strlen( string ) - 1;
    while (i> 0 && whitespace (string[i])) 
      {
        i--;
      }

    string [++i] = '\0';
  }

/*
 * Name: RedirectIn
 *
 * Description: handles redirection in
 * Translates input_file descriptor to STDIN_FILENO
 */
void RedirectIn(char *p)
{
 
    int input_file;

    input_file = open( p, O_RDONLY);              
    if (dup2(input_file,STDIN_FILENO ) == -1)
    {
      printf("Redirection failed!!!\n");
      exit (0);
    }
    close(input_file);

}
/*
 * Name: RedirectOut
 *
 * Description: handles redirection out
 * Translates output_file descriptor to STDOUT_FILENO
 */
void RedirectOut(char *p)
{
    int output_file; 

    output_file = open( p, O_CREAT |O_RDWR , S_IRWXU );
    dup2(output_file,STDOUT_FILENO );                  
    close(output_file);

}
/*
 * Name: PipePgm
 *
 * Description: handles the piping process
 *
 */
void PipePgm (Pgm *p)
{
  if (p == NULL) 
    {
    return ;
    }
    
pid_t pid;
int pipefd[2];
pipe(pipefd);

pid= fork();

if (pid <0)
  perror("Fork failed in pipe");

else if (pid==0)
  {

   /*child writes to parent and closes read end*/
   close(pipefd[READ_END]);
   dup2(pipefd[WRITE_END],STDOUT_FILENO);
   close(pipefd[WRITE_END]);

   /*if p->next) !=NULL execute the last command of the structure 
   *and let parent execute the one before*/
   if((p->next->next) !=NULL)
     {      
      PipePgm(p->next);
     }
   else 
     {


      if (execvp(*p->next->pgmlist,p->next->pgmlist)==-1)
   {
                fprintf(stderr, "%s: %s\n",*(p->next->pgmlist), "Command not found." );  
                exit(1);
               }
     }
  }

else 
  {
   /*close write end and read from child*/  
   close(pipefd[WRITE_END]);
   dup2(pipefd[READ_END],STDIN_FILENO);
   close(pipefd[READ_END]); 

   if (execvp(*(p->pgmlist),p->pgmlist)==-1)
   {
                fprintf(stderr, "%s: %s\n", *(p->pgmlist),"Command not found." );  
               exit(1);
               }

  // wait(NULL);

  }
}

/*
 * Name: builtincmd
 *
 * Description: handles the built in commands exit and cd
 *
 */

int builtincmd(Command *cmd) 
{
  int r = 0;
  if (!strcmp(cmd->pgm->pgmlist[0],"exit")) 
    {
    r = 1;
    //printf("exited!\n");
    exit(0);
    } 

  else if (!strcmp(cmd->pgm->pgmlist[0],"cd")) 
    {
      if (cmd->pgm->pgmlist[1]== NULL)
      {        
        dirr = getenv("HOME");
      }
      else
      {
        dirr = cmd->pgm->pgmlist[1];
      }

      if(chdir(dirr) == 0) 
        {           
           printf("%s\n", dirr);
        }
      else 
        printf("Directory doesnt exist\n");

      r = 1;
    }
  
  return r;
}
/*
 * Name: sig_handler
 *                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                
 * Description: handles the signals
 *
 */
void sig_handler( int sig )
{  
  if (sig == SIGCHLD)
      {
        while((waitpid(-1, 0, WNOHANG))>0)
        {}

      }

    if (sig == SIGINT)
      {      }
}
