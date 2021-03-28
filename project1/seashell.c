#include <unistd.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h> //termios, TCSANOW, ECHO, ICANON
#include <string.h>
#include <stdbool.h>
#include <errno.h>

#include <sys/types.h>
#include <dirent.h>

const char *sysname = "seashell";
const char *path_var;
const int SIZE = 25; // max number of locations in the path
char *path_locations[25];
FILE *shortdir_file;
char SHORTDIR_FILE[1024];

enum return_codes
{
	SUCCESS = 0,
	EXIT = 1,
	UNKNOWN = 2,
};
struct command_t
{
	char *name;
	bool background;
	bool auto_complete;
	int arg_count;
	char **args;
	char *redirects[3];		// in/out redirection
	struct command_t *next; // for piping
};
/**
 * Prints a command struct
 * @param struct command_t *
 */
void print_command(struct command_t *command)
{
	int i = 0;
	printf("Command: <%s>\n", command->name);
	printf("\tIs Background: %s\n", command->background ? "yes" : "no");
	printf("\tNeeds Auto-complete: %s\n", command->auto_complete ? "yes" : "no");
	printf("\tRedirects:\n");
	for (i = 0; i < 3; i++)
		printf("\t\t%d: %s\n", i, command->redirects[i] ? command->redirects[i] : "N/A");
	printf("\tArguments (%d):\n", command->arg_count);
	for (i = 0; i < command->arg_count; ++i)
		printf("\t\tArg %d: %s\n", i, command->args[i]);
	if (command->next)
	{
		printf("\tPiped to:\n");
		print_command(command->next);
	}
}
/**
 * Release allocated memory of a command
 * @param  command [description]
 * @return         [description]
 */
int free_command(struct command_t *command)
{
	if (command->arg_count)
	{
		for (int i = 0; i < command->arg_count; ++i)
			free(command->args[i]);
		free(command->args);
	}
	for (int i = 0; i < 3; ++i)
		if (command->redirects[i])
			free(command->redirects[i]);
	if (command->next)
	{
		free_command(command->next);
		command->next = NULL;
	}
	free(command->name);
	free(command);
	return 0;
}
/**
 * Show the command prompt
 * @return [description]
 */
int show_prompt()
{
	char cwd[1024], hostname[1024];
	gethostname(hostname, sizeof(hostname));
	getcwd(cwd, sizeof(cwd));
	printf("%s@%s:%s %s$ ", getenv("USER"), hostname, cwd, sysname);
	return 0;
}
/**
 * Parse a command string into a command struct
 * @param  buf     [description]
 * @param  command [description]
 * @return         0
 */
int parse_command(char *buf, struct command_t *command)
{
	const char *splitters = " \t"; // split at whitespace
	int index, len;
	len = strlen(buf);
	while (len > 0 && strchr(splitters, buf[0]) != NULL) // trim left whitespace
	{
		buf++;
		len--;
	}
	while (len > 0 && strchr(splitters, buf[len - 1]) != NULL)
		buf[--len] = 0; // trim right whitespace

	if (len > 0 && buf[len - 1] == '?') // auto-complete
		command->auto_complete = true;
	if (len > 0 && buf[len - 1] == '&') // background
		command->background = true;

	char *pch = strtok(buf, splitters);
	command->name = (char *)malloc(strlen(pch) + 1);
	if (pch == NULL)
		command->name[0] = 0;
	else
		strcpy(command->name, pch);

	command->args = (char **)malloc(sizeof(char *));

	int redirect_index;
	int arg_index = 0;
	char temp_buf[1024], *arg;
	while (1)
	{
		// tokenize input on splitters
		pch = strtok(NULL, splitters);
		if (!pch)
			break;
		arg = temp_buf;
		strcpy(arg, pch);
		len = strlen(arg);

		if (len == 0)
			continue;										 // empty arg, go for next
		while (len > 0 && strchr(splitters, arg[0]) != NULL) // trim left whitespace
		{
			arg++;
			len--;
		}
		while (len > 0 && strchr(splitters, arg[len - 1]) != NULL)
			arg[--len] = 0; // trim right whitespace
		if (len == 0)
			continue; // empty arg, go for next

		// piping to another command
		if (strcmp(arg, "|") == 0)
		{
			struct command_t *c = malloc(sizeof(struct command_t));
			int l = strlen(pch);
			pch[l] = splitters[0]; // restore strtok termination
			index = 1;
			while (pch[index] == ' ' || pch[index] == '\t')
				index++; // skip whitespaces

			parse_command(pch + index, c);
			pch[l] = 0; // put back strtok termination
			command->next = c;
			continue;
		}

		// background process
		if (strcmp(arg, "&") == 0)
			continue; // handled before

		// handle input redirection
		redirect_index = -1;
		if (arg[0] == '<')
			redirect_index = 0;
		if (arg[0] == '>')
		{
			if (len > 1 && arg[1] == '>')
			{
				redirect_index = 2;
				arg++;
				len--;
			}
			else
				redirect_index = 1;
		}
		if (redirect_index != -1)
		{
			command->redirects[redirect_index] = malloc(len);
			strcpy(command->redirects[redirect_index], arg + 1);
			continue;
		}

		// normal arguments
		if (len > 2 && ((arg[0] == '"' && arg[len - 1] == '"') || (arg[0] == '\'' && arg[len - 1] == '\''))) // quote wrapped arg
		{
			arg[--len] = 0;
			arg++;
		}
		command->args = (char **)realloc(command->args, sizeof(char *) * (arg_index + 1));
		command->args[arg_index] = (char *)malloc(len + 1);
		strcpy(command->args[arg_index++], arg);
	}
	command->arg_count = arg_index;
	return 0;
}
void prompt_backspace()
{
	putchar(8);	  // go back 1
	putchar(' '); // write empty over
	putchar(8);	  // go back 1 again
}
/**
 * Prompt a command from the user
 * @param  buf      [description]
 * @param  buf_size [description]
 * @return          [description]
 */
int prompt(struct command_t *command)
{
	int index = 0;
	char c;
	char buf[4096];
	static char oldbuf[4096];

	// tcgetattr gets the parameters of the current terminal
	// STDIN_FILENO will tell tcgetattr that it should write the settings
	// of stdin to oldt
	static struct termios backup_termios, new_termios;
	tcgetattr(STDIN_FILENO, &backup_termios);
	new_termios = backup_termios;
	// ICANON normally takes care that one line at a time will be processed
	// that means it will return if it sees a "\n" or an EOF or an EOL
	new_termios.c_lflag &= ~(ICANON | ECHO); // Also disable automatic echo. We manually echo each char.
	// Those new settings will be set to STDIN
	// TCSANOW tells tcsetattr to change attributes immediately.
	tcsetattr(STDIN_FILENO, TCSANOW, &new_termios);

	//FIXME: backspace is applied before printing chars
	show_prompt();
	int multicode_state = 0;
	buf[0] = 0;
	while (1)
	{
		c = getchar();
		// printf("Keycode: %u\n", c); // DEBUG: uncomment for debugging

		if (c == 9) // handle tab
		{
			buf[index++] = '?'; // autocomplete
			break;
		}

		if (c == 127) // handle backspace
		{
			if (index > 0)
			{
				prompt_backspace();
				index--;
			}
			continue;
		}
		if (c == 27 && multicode_state == 0) // handle multi-code keys
		{
			multicode_state = 1;
			continue;
		}
		if (c == 91 && multicode_state == 1)
		{
			multicode_state = 2;
			continue;
		}
		if (c == 65 && multicode_state == 2) // up arrow
		{
			int i;
			while (index > 0)
			{
				prompt_backspace();
				index--;
			}
			for (i = 0; oldbuf[i]; ++i)
			{
				putchar(oldbuf[i]);
				buf[i] = oldbuf[i];
			}
			index = i;
			continue;
		}
		else
			multicode_state = 0;

		putchar(c); // echo the character
		buf[index++] = c;
		if (index >= sizeof(buf) - 1)
			break;
		if (c == '\n') // enter key
			break;
		if (c == 4) // Ctrl+D
			return EXIT;
	}
	if (index > 0 && buf[index - 1] == '\n') // trim newline from the end
		index--;
	buf[index++] = 0; // null terminate string

	strcpy(oldbuf, buf);

	parse_command(buf, command);

	// print_command(command); // DEBUG: uncomment for debugging

	// restore the old settings
	tcsetattr(STDIN_FILENO, TCSANOW, &backup_termios);
	return SUCCESS;
}
int process_command(struct command_t *command);
// part 1
void find_command_path(char *command_name, char *path);
// part 2
void handle_shortdir(struct command_t *command);
void set_shortdir_command(char *command_name);
void delete_shortdir_command(char *command_name);
int find_corresponding_location(char *command_name, char *location);
void list_shortdir_associations();
// part 3
void highlight(char *word, char *file_location, char *color);
// part 4
void schedule_alarm(char *hour, char *minute, char *song);

int main()
{
	getcwd(SHORTDIR_FILE, sizeof(SHORTDIR_FILE));
	strcat(SHORTDIR_FILE, "/shortdir_commands.txt");

	path_var = getenv("PATH");

	char *copy = strdup(path_var);
	char *token = strtok(copy, ":");
	int i = 0;
	while (token != NULL)
	{ // tokenize the path_var to extract locations in the path
		path_locations[i] = malloc(strlen(token) + 1);
		strcpy(path_locations[i], token);
		token = strtok(NULL, ":");
		i++;
	}
	free(copy);

	shortdir_file = fopen(SHORTDIR_FILE, "a+");
	fclose(shortdir_file);
	while (1)
	{
		struct command_t *command = malloc(sizeof(struct command_t));
		memset(command, 0, sizeof(struct command_t)); // set all bytes to 0

		int code;
		code = prompt(command);
		if (code == EXIT)
			break;

		code = process_command(command);
		if (code == EXIT)
			break;

		free_command(command);
	}

	printf("\n");
	return 0;
}

int process_command(struct command_t *command)
{
	int r;
	if (strcmp(command->name, "") == 0)
		return SUCCESS;

	if (strcmp(command->name, "exit") == 0)
		return EXIT;

	if (strcmp(command->name, "cd") == 0)
	{
		if (command->arg_count > 0)
		{
			r = chdir(command->args[0]);
			if (r == -1)
				printf("-%s: %s: %s\n", sysname, command->name, strerror(errno));
			return SUCCESS;
		}
	}

	pid_t pid = fork();
	if (pid == 0) // child
	{
		/// This shows how to do exec with environ (but is not available on MacOs)
		// extern char** environ; // environment variables
		// execvpe(command->name, command->args, environ); // exec+args+path+environ

		/// This shows how to do exec with auto-path resolve
		// add a NULL argument to the end of args, and the name to the beginning
		// as required by exec

		// increase args size by 2
		command->args = (char **)realloc(
			command->args, sizeof(char *) * (command->arg_count += 2));

		// shift everything forward by 1
		for (int i = command->arg_count - 2; i > 0; --i)
			command->args[i] = command->args[i - 1];

		// set args[0] as a copy of name
		command->args[0] = strdup(command->name);
		// set args[arg_count-1] (last) to NULL
		command->args[command->arg_count - 1] = NULL;

		// execvp(command->name, command->args); // exec+args+path
		// exit(0);
		/// TODO: do your own exec with path resolving using execv()
		if (!strcmp(command->name, "shortdir"))
		{

			handle_shortdir(command);
		}
		else if (!strcmp(command->name, "highlight"))
		{
			if (command->arg_count != 5)
			{
				printf("Please enter 3 arguments: word color filename\n");
			}
			else
			{
				if (strcmp(command->args[3], "r") && strcmp(command->args[3], "b") && strcmp(command->args[3], "g"))
				{
					printf("Allowed colors: r, g, b\n\tInvalid color: %s\n", command->args[3]);
				}
				else
				{
					highlight(command->args[1], command->args[2], command->args[3]);
				}
			}
		}
		else if (!strcmp(command->name, "goodMorning"))
		{
			if (command->arg_count != 4)
			{
				printf("Please enter 2 arguments: time path_to_sound\n");
			}
			else
			{
				char *time = strdup(command->args[1]);
				char *hour = strtok(time, ":");
				char *minute = strtok(NULL, ":");
				schedule_alarm(hour, minute, command->args[2]);
			}
		}
		else
		{
			char *command_path = malloc(1024);
			find_command_path(command->name, command_path);
			if (strcmp(command_path, ""))
			{
				execv(command_path, command->args);
			}
			else
			{
				printf("-%s: %s: command not found\n", sysname, command->name);
			}
		}
	}
	else
	{
		if (!command->background)
			wait(0); // wait for child process to finish
		return SUCCESS;
	}

	// TODO: your implementation here

	return UNKNOWN;
}

void find_command_path(char *command_name, char *path)
{
	/**
	 * iterates over the folders in the path and copies the absolute path 
	 * of the command with given name to *path
	 */
	int i;
	for (i = 0; i < sizeof(path_locations); i++)
	{
		if (path_locations[i] != NULL)
		{
			char *file_name;
			DIR *d;
			struct dirent *dir;
			d = opendir(path_locations[i]);
			if (d != NULL)
			{
				while ((dir = readdir(d)) != NULL)
				{
					if (!strcmp(dir->d_name, ".") || !strcmp(dir->d_name, ".."))
					{
					}
					else
					{
						file_name = dir->d_name;
						if (!strcmp(file_name, command_name))
						{
							// printf("FOUND: file_name: \"%s\"\n", file_name);
							char *p = malloc(1);
							strcat(p, path_locations[i]);
							strcat(p, "/");
							strcat(p, file_name);
							strcpy(path, p);
							closedir(d);
							return;
						}
					}
				}
				closedir(d);
			}
		}
	}
	strcpy(path, "");
	return;
}

void handle_shortdir(struct command_t *command)
{

	if (!strcmp(command->args[1], "set"))
	{
		if (command->arg_count != 4)
		{
			printf("Please enter 2 arguments: shortdir set shortcut\n");
		}
		else
		{
			printf("Setting\n");
			set_shortdir_command(command->args[2]);
		}
	}
	else if (!strcmp(command->args[1], "jump"))
	{
		if (command->arg_count != 4)
		{
			printf("Please enter 2 arguments: shortdir jump shortcut\n");
		}
		else
		{
			char *location = malloc(1024);
			find_corresponding_location(command->args[2], location);
			if (strcmp(location, ""))
			{
				printf("%s\n", location);
				chdir(location);
			}
			else
			{
				printf("Invalid location: %s\n", command->args[2]);
			}
			free(location);
		}
	}
	else if (!strcmp(command->args[1], "del"))
	{
		if (command->arg_count != 4)
		{
			printf("Please enter 2 arguments: shortdir del shortcut\n");
		}
		else
		{
			// printf("Deleting\n");
			delete_shortdir_command(command->args[2]);
		}
	}

	else if (!strcmp(command->args[1], "clear"))
	{
		if (command->arg_count != 3)
		{
			printf("Please enter 1 argument: shortdir clear\n");
		}
		else
		{
			remove(SHORTDIR_FILE);
			shortdir_file = fopen(SHORTDIR_FILE, "a+");
			fclose(shortdir_file);
		}
	}
	else if (!strcmp(command->args[1], "list"))
	{
		if (command->arg_count != 3)
		{
			printf("Please enter 1 argument: shortdir list\n");
		}
		else
		{
			list_shortdir_associations();
		}
	}
	else
	{
		printf("Invalid argument for shortdir: %s\n", command->args[1]);
	}
}

void set_shortdir_command(char *command_name)
{
	/**
	 * checks if the command exists in the shortdir file,
	 * if it exists deletes the first command,
	 * adds the command to shortdir file
	 */
	char *string = malloc(1024);
	char cwd[1024];
	getcwd(cwd, sizeof(cwd));
	char c[1024];
	strcpy(c, command_name);
	strcat(strcat(strcat(c, " "), cwd), "\n");
	find_corresponding_location(command_name, string);
	if (strcmp(string, ""))
	{
		printf("Replacing old command for shortdir %s...\n", command_name);
		delete_shortdir_command(command_name);
	}
	shortdir_file = fopen(SHORTDIR_FILE, "a+");
	fputs(c, shortdir_file);
	fclose(shortdir_file);
	free(string);
}

void delete_shortdir_command(char *command_name)
{
	/**
	 * checks the contents of the shortdir file,
	 * finds the line containing the given command name
	 * copies each line except for the corresponding line to a swap file
	 * and replaces the original file
	 */
	int line;
	char *location = malloc(1024);
	line = find_corresponding_location(command_name, location);
	if (line < 0)
	{
		printf("Shortdir command doesn't exit: %s\n", command_name);
	}
	else
	{
		FILE *swap = fopen("swap", "w");
		shortdir_file = fopen(SHORTDIR_FILE, "r");
		int current_line = 0;

		char ch = fgetc(shortdir_file);
		while (ch != EOF)
		{
			if (current_line != line)
			{
				fputc(ch, swap);
			}
			if (ch == '\n')
			{
				current_line++;
			}
			ch = fgetc(shortdir_file);
		}
		fclose(shortdir_file);
		fclose(swap);
		remove(SHORTDIR_FILE);
		rename("swap", SHORTDIR_FILE);
	}

	free(location);
}

int find_corresponding_location(char *command_name, char *location)
{
	/**
	 * searches for the command_name in the shortdir files and if found
	 * copies it to location and returns the line number,
	 * otherwise copies "" to location and returns -1
	 */
	shortdir_file = fopen(SHORTDIR_FILE, "a+");
	char ch;
	char string[255];
	int line_no = 0;

	while (fscanf(shortdir_file, "%s", string) == 1)
	{
		if (!strcmp(string, command_name))
		{
			fscanf(shortdir_file, "%s", string);
			strcpy(location, string);
			fclose(shortdir_file);
			return line_no;
		}
		line_no++;
		fscanf(shortdir_file, "%s", string);
	}
	fclose(shortdir_file);
	strcpy(location, "");
	return (-1);
}

void list_shortdir_associations()
{
	/**
	 * lists all shortdir associations
	 */
	printf("Existing file associations:\n");
	shortdir_file = fopen(SHORTDIR_FILE, "r");
	char *line;
	size_t len = 0;
	ssize_t read;
	while ((read = getline(&line, &len, shortdir_file) != -1))
	{
		printf("\t%s", line);
	}
	fclose(shortdir_file);
}

// part 3

#define KRED "\x1B[31m"
#define KGRN "\x1B[32m"
#define KBLU "\x1B[34m"
#define ANSI_COLOR_RESET "\x1b[0m"

void highlight(char *word, char *file_location, char *color)
{
	FILE *file = fopen(file_location, "r");
	char *string = malloc(1024);

	while (fscanf(file, "%s", string) == 1 || fscanf(file, "%[^\n]", string) == 1)
	{
		// new line
		if (!strcasecmp(word, string))
		{
			if (!strcmp(color, "r"))
				printf("%s%s%s ", KRED, string, ANSI_COLOR_RESET);
			else if (!strcmp(color, "b"))
				printf("%s%s%s ", KBLU, string, ANSI_COLOR_RESET);
			else if (!strcmp(color, "g"))
				printf("%s%s%s ", KGRN, string, ANSI_COLOR_RESET);
		}
		else
		{
			printf("%s ", string);
		}
	}
	free(string);
	fclose(file);
}

// part 4
void schedule_alarm(char *hour, char *minute, char *song)
{
	/**
	 * schedules the song to be played at hour:minute
	 */
	char *line = malloc(1024);
	char *argv[3];
	strcat(line, minute);
	strcat(line, " ");
	strcat(line, hour);
	strcat(line, " * * * XDG_RUNTIME_DIR=/run/user/$(id -u) DISPLAY=:0.0 /usr/bin/rhythmbox-client --play ");
	strcat(line, song);
	strcat(line, "\n");//home/gsa/code/comp304/comp304/project1/resources/m.mp3\n");
	// printf("%s\n",line);
	FILE *f = fopen("c.txt", "w");	
	fputs(line, f);
	fclose(f);
	argv[0]="crontab";
	argv[1]="c.txt";
	argv[2]=NULL;
	execvp("crontab", argv);
	free(line);
}