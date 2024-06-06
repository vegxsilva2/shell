/*
Trabajo realizado por: Luis Vega Silva
*/


/**
UNIX Shell Project

Sistemas Operativos
Grados I. Informatica, Computadores & Software
Dept. Arquitectura de Computadores - UMA

Some code adapted from "Fundamentos de Sistemas Operativos", Silberschatz et al.

To compile and run the program:
   $ gcc Shell_project.c job_control.c -o Shell
   $ ./Shell          
	(then type ^D to exit program)

**/

#include "job_control.h"   // remember to compile with module job_control.c 
#include <string.h>

#define MAX_LINE 256 /* 256 chars per line, per command, should be enough. */

job * miLista;

void parse_redirections(char **args, char **file_in, char **file_out)
{
	*file_in = NULL;
	*file_out = NULL;
	char **args_start = args;
	while (*args)
	{
		int is_in = !strcmp(*args, "<");
		int is_out = !strcmp(*args, ">");
		if (is_in || is_out)
		{
			args++;
			if (*args)
			{
				if (is_in)
					*file_in = *args;
				if (is_out)
					*file_out = *args;
				char **aux = args + 1;
				while (*aux)
				{
					*(aux - 2) = *aux;
					aux++;
				}
				*(aux - 2) = NULL;
				args--;
			}
			else
			{
				/* Syntax error */
				fprintf(stderr, "syntax error in redirection\n");
				args_start[0] = NULL; // Do nothing
			}
		}
		else
		{
			args++;
		}
	}
}

void handler2(){

	FILE *fp;
	fp = fopen("hup.txt","a"); // Abre el fichero hup.txt en el modo append
	fprintf(fp,"SIGHUP recibido.\n"); // escribe en el fichero
	fclose(fp);

}


void handler(){
	/*
	
	*/

	int nElementos = list_size(miLista);
	job * tarea;
	int estadoTarea;
	enum status estado;
	int info;


	for(int i=nElementos;i>=1;i--){
		tarea = get_item_bypos(miLista,i);
		if(tarea != NULL){
			if(tarea->pgid == waitpid(tarea->pgid,&estadoTarea,WUNTRACED | WNOHANG | WCONTINUED)){
				estado = analyze_status(estadoTarea, &info);
				if(estado == SIGNALED || estado == EXITED){
					printf("Background pid: %d, command: %s, %s, info: %d\n",tarea->pgid, tarea->command,status_strings[estado], info);
					delete_job(miLista,tarea);
				}
				if(estado == SUSPENDED){
					printf("Background pid: %d, command: %s, %s, info: %d\n", tarea->pgid, tarea->command,status_strings[estado], info);
					tarea->state = STOPPED;
				}
				if(estado == CONTINUED){
					printf("Background pid: %d, command: %s, %s, info: %d\n", tarea->pgid, tarea->command,status_strings[estado], info);
					tarea->state = BACKGROUND;
				}
			}
		}
	}
}

// -----------------------------------------------------------------------
//                            MAIN          
// -----------------------------------------------------------------------

int main(void)
{
	char inputBuffer[MAX_LINE]; /* buffer to hold the command entered */
	int background;             /* equals 1 if a command is followed by '&' */
	char *args[MAX_LINE/2];     /* command line (of 256) has max of 128 arguments */
	// probably useful variables:
	int pid_fork, pid_wait; /* pid for created and waited process */
	int status;             /* status returned by wait */
	enum status status_res; /* status processed by analyze_status() */
	int info;				/* info processed by analyze_status() */
	

	int foreground;
	job * tarea;
	char * command;

	
	ignore_terminal_signals(); // Funcion que hace ignorar las señales del terminal
	miLista = new_list("Tareas Shell"); //Inicializamos la lista de trabajos con el nombre Tareas Shell
	signal(SIGCHLD, handler); // Llamamos al manejador para que trate la señal SIGCHLD
	signal(SIGHUP,handler2); // Llamamos al manejador para que trate la señal SIGUP

	while (1)   /* Program terminates normally inside get_command() after ^D is typed*/
	{   	
	
		printf("COMMAND->");
		fflush(stdout);
		get_command(inputBuffer, MAX_LINE, args, &background);  /* get next command */
		command = args[0];

		char *file_in, *file_out;
		parse_redirections(args,&file_in,&file_out);

		if(args[0]==NULL) continue;   // if empty command

		/* the steps are:
			 (1) fork a child process using fork()
			 (2) the child process will invoke execvp()
			 (3) if background == 0, the parent will wait, otherwise continue 
			 (4) Shell shows a status message for processed command 
			 (5) loop returns to get_commnad() function
		*/

		if(strcmp(args[0],"cd") == 0){ // Comprobamos si el comando introducido es el comando interno cd
			if(chdir(args[1])==0){ // Ejecuta el proceso interno
				chdir(args[1]);
			}else{
				printf("No se puede cambiar al directorio %s\n",args[1]);
			}
			continue;
		}if(strcmp(args[0],"logout")==0){
			exit(0);
		}if(strcmp(args[0],"jobs")==0){ // Comprobamos si el comando introducido es el comando interno jobs
			if(list_size(miLista)== 0){
				printf("ERROR:La lista está vacia\n");
			}else{
				print_job_list(miLista);
			}
			continue;
		}if(strcmp(args[0],"currjob")==0){

			if(list_size(miLista) == 0){
				printf("No hay trabajo actual\n");
			}else{
				printf("Trabajo actual: PID=%d command=%s\n",tarea->pgid,tarea->command);
			}
			continue;
				
		}if(strcmp(args[0],"deljob")==0){

			tarea = get_item_bypos(miLista,1);

			if(tarea == NULL){
				printf("No hay trabajo actual");
			}else{
				enum job_state estado = tarea->state;
				if(estado != SUSPENDED && estado == BACKGROUND){
					printf("Borrando trabajo actual de la lista de jobs: PID=%d command=%s",tarea->pgid,tarea->command);
					delete_job(miLista,tarea);
				}else if(estado == STOPPED && estado == BACKGROUND){
					printf("No se permiten borrar trabajos en segundo plano suspendidos");
				}
			}

		}if(strcmp(args[0],"fg")==0){

			int pos = 1; // Inicializamos a 1 ya que si args[1]==NULL se trata el primer proceso de la lista
			

			if(args[1] != NULL){
				pos = atoi(args[1]); //En caso de que no sea nulo actualizamos pos con el valor entero de args[1]
			}

			if(pos<0){
				continue;
			}

			tarea = get_item_bypos(miLista,pos);
			
			if(tarea != NULL){
				foreground = 1;
				set_terminal(tarea->pgid);
				command = strdup(tarea->command);
				if(tarea->state == STOPPED){
					killpg(tarea->pgid, SIGCONT);
				}
				pid_fork = tarea->pgid;
				delete_job(miLista,tarea);
			}else{
				continue;
			}
			
			
		}if(strcmp(args[0],"bg")==0){
			
			int pos = 1;
			

			
			if(args[1]!=NULL){
				pos = atoi(args[1]);
			}

			tarea = get_item_bypos(miLista,pos);

			if(tarea != NULL && tarea->state == STOPPED){
				tarea->state = BACKGROUND;
				killpg(tarea->pgid,SIGCONT);
			}

			continue;

		}else{

			if(foreground!=1){
				pid_fork=fork();
			}

			if(pid_fork == 0){ // child
				
				new_process_group(getpid()); // Creamos el nuevo grupo de procesos para hijo

				if(background == 0)	set_terminal(getpid()); // El hijo toma el poder del terminal
				
				FILE *inFile, *outFile;
				int fnum1,fnum2;

				if(file_in!=NULL){ // En el caso de que haya redireccion de entrada
					if(NULL != (inFile=fopen(file_in,"r"))){
						fnum1 = fileno(inFile);
						fnum2 = fileno(stdin);
						if(dup2(fnum1,fnum2)==-1){
							printf("Error redireccionando la entrada\n");
							return(-1);
						}
					}else{
						printf("Error abriendo %s\n",file_in);
						return(-1);
					}
					fclose(inFile);
				}

				if(file_out!=NULL){ // En el caso de que haya redireccion de salida
					if(NULL != (outFile=fopen(file_out,"w"))){
						fnum1 = fileno(outFile);
						fnum2 = fileno(stdout);
						if(dup2(fnum1,fnum2)==-1){
							printf("Error redireccionando la entrada\n");
							return(-1);
						}
					}else{
						printf("Error abriendo %s\n",file_out);
						return(-1);
					}
					fclose(outFile);
				}

				restore_terminal_signals();

				execvp(args[0],args);
				if(execvp(args[0],args) == -1){ //Si el comando introducido no se encuentra en la lista de comandos
					printf("Error, command not found: %s\n", args[0]);
					printf("Foreground pid: %d, command: %s, %s, info: %d\n", pid_fork, command,status_strings[status],info);
					exit(-1);
				}

			}else{ // parent
				if(background == 0){ // Si el comando se ejecuta en primer plano
					waitpid(pid_fork,&status,WUNTRACED); // Debemos esperar a que el proceso acabe
					set_terminal(getpid());
					status_res=analyze_status(status,&info);
					if(status_res == SUSPENDED){
						job * trabajo = new_job(pid_fork,command,STOPPED);
						add_job(miLista,trabajo);
					}
					printf("Foreground pid: %d, command: %s, %s, info: %d\n", pid_fork, command,status_strings[status_res],info);
				}else{ // Si el comando se ejecuta en segundo plano
					job * tarea = new_job(pid_fork,args[0],BACKGROUND);
					add_job(miLista,tarea);
					printf("Background job running... pid: %d, command:%s\n",tarea->pgid,command);
				}

				foreground=0;
			}
		}
		

	} // end while
}
