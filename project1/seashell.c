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
#include <curl/curl.h>

const char *sysname = "seashell";
const char *path_var;
const int SIZE = 25; // max number of locations in the path
char *path_locations[25];
FILE *shortdir_file;
char SHORTDIR_FILE[1024];
// char *payload_text;
static const char *payload_text[1024];
// static const char *payload_text[]={""};
// char **payload_ptr = payload_text;

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
// pat 5
void kdiff_all_lines(char *f1, char *f2);
void kdiff_binary(char *f1, char *f2);
// part 6
struct user_email *get_email_details();
void send_email();

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
		else if (!strcmp(command->name, "kdiff"))
		{
			if (!strcmp(command->args[1], "-b"))
			{
				kdiff_binary(command->args[2], command->args[3]);
			}
			kdiff_all_lines(command->args[2], command->args[3]);
		}
		else if (!strcmp(command->name, "email"))
		{
			send_email();
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
	strcat(line, "\n"); //home/gsa/code/comp304/comp304/project1/resources/m.mp3\n");
	// printf("%s\n",line);
	FILE *f = fopen("c.txt", "w");
	fputs(line, f);
	fclose(f);
	argv[0] = "crontab";
	argv[1] = "c.txt";
	argv[2] = NULL;
	execvp("crontab", argv);
	free(line);
}

// part 5
void kdiff_all_lines(char *f1, char *f2)
{
	FILE *file1, *file2;
	file1 = fopen(f1, "r");
	file2 = fopen(f2, "r");
	char *line1, *line2;
	size_t len1 = 0;
	size_t len2 = 0;
	ssize_t read1, read2;
	int line_no = 0;
	int total_diff = 0;
	while ((read1 = getline(&line1, &len1, file1) != -1) && (read2 = getline(&line2, &len2, file2) != -1))
	{
		if (strcmp(line1, line2))
		{

			printf("%s:Line %d: %s", f1, line_no, line1);
			printf("%s:Line %d: %s", f2, line_no, line2);
			total_diff++;
		}
		line_no++;
	}
	if (total_diff == 0)
		printf("The two files are identical.\n");
	else
		printf("%d different lines found.\n", total_diff);
	fclose(file1);
	fclose(file2);
}

void kdiff_binary(char *f1, char *f2)
{
	FILE *file1, *file2;
	char byte1, byte2;
	int diff_bytes = 0;
	file1 = fopen(f1, "rb");
	file2 = fopen(f2, "rb");
	while ((byte1 = fgetc(file1)) != EOF && (byte2 = fgetc(file2)) != EOF)
	{
		// printf("%c\t%c\n",byte1, byte2 );
		if (byte1 != byte2)
		{
			diff_bytes += 1;
		}
	}
	if (diff_bytes > 0)
		printf("%d bytes are different.\n", diff_bytes);
	else
		printf("Two files are identical.");
	// const char *fn1, *fn2;
	// strcpy(fn1, f1);
	// strcpy(fn2, f2);

	// file1 = open(fn1, O_RDONLY);
	// file2 = open(fn2, O_RDONLY);
	// // if (file1 == NULL)
	// // {
	// // 	printf("Error opening: %s\n", f1);
	// // }
	// // else if (file2 == NULL)
	// // {
	// // 	printf("Error opening: %s\n", f2);
	// // }
	// // else
	// {
	// 	unsigned char buffer1[4096], buffer2[4096];
	// 	size_t size1, size2;

	// 	int line_no = 0;
	// 	while (1)
	// 	{
	// 		size1 = read(file1, b1, 1);
	// 		size2 = read(file2, b2, 1);
	// 		if (size1 == 0 || size2 == 0)
	// 		{
	// 			break;
	// 		}
	// 		if (size1 == -1 || size2 == -1)
	// 		{
	// 			printf("Err\n");
	// 			break;
	// 		}
	// 		if (b1!=b2){
	// 			printf("Diff\n");
	// 		}
	// 	}
	// 	// while ((size1 = fread(buffer1, 1, sizeof(buffer1), file1) > 0) && (size2 = fread(buffer2, 1, sizeof(buffer2), file2) > 0))
	// 	// {
	// 	// 	printf("%s\n%s\n", buffer1, buffer2);
	// 	// }
	// }
	fclose(file1);
	fclose(file2);
}

#include <string.h>
#include <curl/curl.h>
struct user_email
{
	char *name;
	char *email;
	char *password;
};

struct user_email *get_email_details()
{
	FILE *file;
	file = fopen("user.txt", "r");
	size_t len = 0;
	ssize_t read;
	char *line;
	struct user_email *user = malloc(sizeof(struct user_email));
	read = getline(&line, &len, file);
	if (read == -1)
	{
		printf("User file empty, please provide \nname\nemail\npassword\n");
		return NULL;
	}
	line = strsep(&line, "\n");
	user->name = strdup(line);

	read = getline(&line, &len, file);
	if (read == -1)
	{
		printf("User file empty, please provide \nname\nemail\npassword\n");
		return NULL;
	}
	line = strsep(&line, "\n");
	user->email = strdup(line);

	read = getline(&line, &len, file);
	if (read == -1)
	{
		printf("User file empty, please provide \nname\nemail\npassword\n");
		return NULL;
	}
	line = strsep(&line, "\n");
	user->password = strdup(line);

	printf("Sending email as %s <%s>\n", user->name, user->email);
	fclose(file);
	return user;
}

struct upload_status
{
	int lines_read;
};

void get_recepients(struct curl_slist *recipients)
{
}

void payload_dummy()
{
}

static size_t payload_source(char *ptr, size_t size, size_t nmemb, void *userp)
{
	struct upload_status *upload_ctx = (struct upload_status *)userp;
	const char *data;

	if ((size == 0) || (nmemb == 0) || ((size * nmemb) < 1))
	{
		return 0;
	}

	data = payload_text[upload_ctx->lines_read];

	if (data)
	{
		size_t len = strlen(data);
		memcpy(ptr, data, len);
		upload_ctx->lines_read++;
// printf("Data length: %ld\n", len);
		return len;
	}

	return 0;
}
void get_payload(struct user_email *user, struct curl_slist *recipients)
{
	int line_no = 0;
	FILE *file;
	char byte;
	file = fopen("email.txt", "r");
	size_t len = 0;
	ssize_t read;
	char *line, *to, *cc, *email, *subject, *body;
	// payload_text = malloc(1);
	read = getline(&line, &len, file);
	// char **payload_ptr = payload_text;

	// payload_ptr[line_no]=malloc(line);
	payload_text[line_no++] = strdup(line);
	printf("Payload: %s\n", payload_text[line_no - 1]);
	// omit first TO:
	strsep(&line, " ");
	while (strcmp((to = strsep(&line, ";")), "\n"))
	{
		if (strcmp(to, " "))
		{
			// recepient name
			strsep(&to, "<");
			email = strsep(&to, ">");
			
			recipients = curl_slist_append(recipients, email);
			// printf("%s\n", recipients);
			printf("%s\n", email);
		}
	}

	char *p = malloc(1);
	// p="";
	p = strdup("From: ");
	strcat(p, user->name);
	strcat(p, " ");
	strcat(p, user->email);
	strcat(p, "\n");
	// payload_text[line_no]=malloc(p);
	payload_text[line_no++] = strdup(p);
	printf("Payload: %s\n", payload_text[line_no - 1]);

	printf("%s\n", p);

	read = getline(&line, &len, file);
	// strcat(payload_text, line);
	// payload_text[line_no]=malloc(line);
	payload_text[line_no++] = strdup(line);
	printf("Payload: %s\n", payload_text[line_no - 1]);

	// omit first TO:
	strsep(&line, " ");
	while (strcmp((cc = strsep(&line, ";")), "\n"))
	{
		if (strcmp(cc, " "))
		{
			// recepient name
			strsep(&cc, "<");
			email = strsep(&cc, ">");
			recipients = curl_slist_append(recipients, email);
		}
	}

	// read subject
		// payload_text[line_no++] = strdup("\n");
	read = getline(&line, &len, file);
	printf("%s\n", line);
	// payload_text[line_no]=malloc(line);
	payload_text[line_no++] = strdup(line);
	printf("%s\nPayload subject: %s\n", line, payload_text[line_no - 1]);

	// strcat(payload_text, line);
	// strcat(payload_text, "\n");
	strsep(&line, " ");
	subject = strsep(&line, "\n");
	printf("Email subject: %s\n", subject);

	// read message body
	read = getline(&line, &len, file);
	strsep(&line, " ");
	body = strdup(line);
	payload_text[line_no++] = strdup("\n");
	payload_text[line_no++] = strdup(line);
	while ((read = getline(&line, &len, file)) != -1)
	{
		payload_text[line_no++] = strdup(line);

		strcat(body, line);
	}
	printf("Email body: %s\n", body);

	fclose(file);

	// strcat(payload_text, body);
	// strcat(payload_text, NULL);
	payload_text[line_no++] = NULL;
	// return payload_text;
	// for (int i=0; i<)
}

// char *get_payload_old(struct user_email *user, struct curl_slist *recipients)
// {
// 	FILE *file;
// 	char byte;
// 	int diff_bytes = 0;
// 	file = fopen("email.txt", "r");
// 	size_t len = 0;
// 	ssize_t read;
// 	char *line, *to, *cc, *email, *subject, *body, *payload_text;
// 	payload_text = malloc(1);
// 	read = getline(&line, &len, file);

// 	strcat(payload_text, line);
// 	// omit first TO:
// 	strsep(&line, " ");
// 	while (strcmp((to = strsep(&line, ";")), "\n"))
// 	{
// 		if (strcmp(to, " "))
// 		{
// 			// recepient name
// 			strsep(&to, "<");
// 			email = strsep(&to, ">");
// 			recipients = curl_slist_append(recipients, email);

// 			// printf("%s\n", email);
// 		}
// 	}

// 	strcat(payload_text, "From: ");
// 	strcat(payload_text, user->name);
// 	strcat(payload_text, "<");
// 	strcat(payload_text, user->email);
// 	strcat(payload_text, ">\n");

// 	read = getline(&line, &len, file);
// 	strcat(payload_text, line);
// 	// omit first TO:
// 	strsep(&line, "\"");
// 	while (strcmp((cc = strsep(&line, "\"")), "\n"))
// 	{
// 		if (strcmp(cc, " "))
// 		{
// 			// recepient name
// 			strsep(&cc, "<");
// 			email = strsep(&cc, ">");
// 			recipients = curl_slist_append(recipients, email);
// 		}
// 	}

// 	// read subject
// 	read = getline(&line, &len, file);
// 	strcat(payload_text, line);
// 	strcat(payload_text, "\n");
// 	strsep(&line, " ");
// 	subject = strsep(&line, "\n");
// 	printf("Email subject: %s\n", subject);

// 	// read message body
// 	read = getline(&line, &len, file);
// 	strsep(&line, " ");
// 	body = strdup(line);
// 	while ((read = getline(&line, &len, file)) != -1)
// 	{
// 		strcat(body, line);
// 	}
// 	printf("Email body: %s\n", body);

// 	fclose(file);

// 	strcat(payload_text, body);
// 	strcat(payload_text, NULL);

// 	return payload_text;
// }
void send_email()
{
	struct user_email *user = get_email_details();
	if (user == NULL)
	{
		printf("Hii");
		return;
	}
	CURL *curl;
	CURLcode res = CURLE_OK;
	struct curl_slist *recipients = NULL;
	// curl_easy_reset(curl);
	curl = curl_easy_init();
	if (curl)

	{
		const char **p;
		long infilesize;
		struct upload_status upload_ctx;

		upload_ctx.lines_read = 0;

		// /* Set username and password */
		curl_easy_setopt(curl, CURLOPT_USERNAME, user->email);
		curl_easy_setopt(curl, CURLOPT_PASSWORD, user->password);

		curl_easy_setopt(curl, CURLOPT_URL, "smtp://smtp.gmail.com:587");
		// curl_easy_setopt(curl, CURLOPT_URL, "smtps://smtp.gmail.com:465");
		curl_easy_setopt(curl, CURLOPT_USE_SSL, (long)CURLUSESSL_ALL);

		// payload_text =
		get_payload(user, recipients);


		// printf("P: %s\n", payload_text[1]);
		printf("Hi malloc\n");
		char *from = malloc(1);
		printf("Byei malloc\n");
		from = strdup(user->name);
		strcat(from, " ");
		strcat(from, user->email);
		strcat(from, "( ");
		printf("%s\n", from);
		printf("Byei malloc\n");
		curl_easy_setopt(curl, CURLOPT_MAIL_FROM, from);
		curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);
		curl_easy_setopt(curl, CURLOPT_READFUNCTION, payload_source);

		printf("Byei malloc\n");

		printf("Passed payload source\n");
		curl_easy_setopt(curl, CURLOPT_READDATA, &upload_ctx);
		curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
		printf("Passed payload upload\n");

		// char **payload_ptr = payload_text;
		infilesize = 0;
		// infilesize = sizeof(payload_text)/8;
		// printf("Payload text size: %ld\n", sizeof(payload_text));
		// char *p_text=malloc(size_of(payload_text));
		// size_t offset=0;
		// for (size_t i = 0; i < sizeof(payload_text); i++)
		// {
		// 	if (payload_text[i] != NULL)
		// 		infilesize += (long)strlen(payload_text[i]);
		// }
		// const char **p_text = payload_text;
		// // p_text=strdup(payload_text);
		for (p = payload_text; *p; ++p)
		{
			printf("%s",*p);
			infilesize += (long)strlen(*p);
			// infilesize += (long)strlen(*p);
		}
		// for (int i = 0; i < 5; i++)
		// {
		// 	printf("%s\n",payload_text[i]);
		// 	// printf("%s\n", payload_text[i]);
		// 	if (payload_text[i] != NULL)
		// 		infilesize += strlen(payload_text[i]);
		// }
		printf("%ld\n", infilesize);
		curl_easy_setopt(curl, CURLOPT_INFILESIZE, infilesize);
		/* Since the traffic will be encrypted, it is very useful to turn on debug
     * information within libcurl to see what is happening during the transfer.
     */
		printf("Passed file size\n");
		curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
		/* Perform the append */
		res = curl_easy_perform(curl);
		printf("Passed perform %d", res);
		/* Check for errors */
		if (res != CURLE_OK)
			fprintf(stderr, "curl_easy_perform() failed: %s\n",
					curl_easy_strerror(res));
		/* Free the list of recipients */
		curl_slist_free_all(recipients);

		/* Always cleanup */
		curl_easy_cleanup(curl);
		curl_easy_reset(curl);
	}
}