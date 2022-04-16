#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "unistd.h"
#include "sys/wait.h"
#include "fcntl.h"
#include "ctype.h"
#include "signal.h"
#include "termios.h"
#include "poll.h"
#include "time.h"
#include "dirent.h"

#define BUF_LEN 256
#define MAX_COMMANDS 10000

struct node_
{
	char *val;
	struct node_ *next;
	struct node_ *prev;
	int lcs; // length of longest coomon substring
};
typedef struct node_ node;
typedef struct deque_
{
	int size;
	node *head;
	node *tail;

} deque;
void fill_deque(deque *d, int fd);
void pop_front(deque *d);
void push_back(deque *d, char *cmnd);
void init_deque(deque *dh);
void print_deque(deque *dh);
void get_from_back_deque(deque *dh, int i);
void clear_deque(deque* d);
void get_search_term();
int get_integer();
int calc_lcs(deque *d);
int get_lcs_value(char *str1, char *str2);
void print_node_with_given_lcs(deque *d, int lcs);
struct termios saved_attributes;


pid_t pid = 0, pid2;
char *line;
int sent_to_back = 0, exit_signal;
int get_line();
char **split_command(int *pipe_cnt);
char **tokenize(char *command, int *arg_cnt);
int cmd_handler(char **args, int arg_cnt, int fdin, int fdout,deque* d);
char *toLower(char *str, size_t len);
void SIGINT_Handler(int sig);
void SIGTSTP_Handler(int sig);
void multiWatch();

void reset_input_mode(void)
{
	tcsetattr(STDIN_FILENO, TCSANOW, &saved_attributes);
}

void set_input_mode(void)
{
	struct termios tattr;
	char *name;

	/* Make sure stdin is a terminal. */
	if (!isatty(STDIN_FILENO))
	{
		fprintf(stderr, "Not a terminal.\n");
		exit(EXIT_FAILURE);
	}

	/* Save the terminal attributes so we can restore them later. */
	tcgetattr(STDIN_FILENO, &saved_attributes);
	atexit(reset_input_mode);

	/* Set the funny terminal modes. */
	tcgetattr(STDIN_FILENO, &tattr);
	tattr.c_lflag &= ~(ICANON | ECHO); /* Clear ICANON and ECHO. */
	tattr.c_cc[VMIN] = 1;
	tattr.c_cc[VTIME] = 0;
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &tattr);
}

int main()
{
	int fd_history = open("history.txt", O_CREAT | O_RDWR);
	if (fd_history < 0)
	{
		printf("error opening/creating history.txt\n");
		exit(EXIT_FAILURE);
	}
	/*
		O_CREAT create file if not exist in the specified path
		O_WRONLY only want write access
	*/
	// put all the commands in the deque
	deque *dh = (deque *)malloc(sizeof(deque));
	init_deque(dh);
	fill_deque(dh, fd_history);
	close(fd_history);
	char **commands;
	char **args;
	int pipe_cnt;
	int arg_cnt;
	int stat = EXIT_SUCCESS;

	set_input_mode();
	signal(SIGINT, SIGINT_Handler);
	signal(SIGTSTP, SIGTSTP_Handler);

	while (1)
	{
		printf("\nmy_shell>> ");
		fflush(stdout);
		int type = get_line();
		// printf("%d\n", type);

		if (type == 1) // history command detected
		{
			print_deque(dh);
			printf("\nmy_shell>> Enter the index of command to run or 0 to continue : ");
			fflush(stdout);
			int id = get_integer();
			fflush(stdout);
			if (id == 0 || id > dh->size || id < 0)
				continue;
			get_from_back_deque(dh, id);
		}
		else if (type == 2) // ctrl+r detected
		{
			printf("\nmy_shell>> Enter search term : ");
			fflush(stdout);
			get_search_term();

			int res = calc_lcs(dh);
			/*
				res = -1
				no exact match or lcs of length <=2

				res = 0
				recent exact match found with variable 'line'

				res > 0
				max lcs of all the commands
			*/
			if (res < 0)
			{
				printf("No match for search term '%s' in history\n", line);
				fflush(stdout);
				continue;
			}
			if (res >= 0)
			{
				print_node_with_given_lcs(dh, res);
				printf("my_shell>> Enter the index of command to run or 0 to continue : ");
				fflush(stdout);
				int id = get_integer();
				fflush(stdout);
				if (id == 0 || id > dh->size || id < 0)
					continue;
				get_from_back_deque(dh, id);
			}
		}

		push_back(dh,line);

		commands = split_command(&pipe_cnt);
		// printf("%d\n", pipe_cnt);

		if (pipe_cnt > 1)
		{
			int FD[2], flag = 0, fdin = 0;
			int i;
			for (i = 0; i < pipe_cnt - 1; i++)
			{

				args = tokenize(commands[i], &arg_cnt);
				if (pipe(FD) == -1)
				{
					printf("Error : Cannot create pipe\n");
					flag = 1;
					break;
				}
				if (arg_cnt > 0)
				{
					stat = cmd_handler(args, arg_cnt, fdin, FD[1],dh);
				}
				else
				{
					flag = 1;
					printf("Error : Malformed Syntax\n");
					continue;
				}
				close(FD[1]);
				fdin = FD[0];
			}

			// FD[1]

			if (!flag)
			{
				args = tokenize(commands[i], &arg_cnt);
				if (arg_cnt <= 0)
				{
					printf("Error : Malformed Syntax\n");
					continue;
				}

				stat = cmd_handler(args, arg_cnt, fdin, 1,dh);
			}
		}
		else if (pipe_cnt == 1)
		{
			args = tokenize(commands[0], &arg_cnt);
			if (strcmp(args[0], "multiWatch") == 0)
			{
				printf("\nCalling multiWatch\n");
				multiWatch();
			}
			else
			{
				if (arg_cnt <= 0)
				{
					printf("Error : Malformed Syntax\n");
					continue;
				}

				stat = cmd_handler(args, arg_cnt, 0, 1,dh);
			}
		}

		// freeing ponters for next use
		for (int i = 0; i < arg_cnt; i++)
		{
			free(args[i]);
		}
		for (int i = 0; i < pipe_cnt; i++)
		{
			free(commands[i]);
		}
		free(args);
		free(line);
		free(commands);
		args = NULL;
		line = NULL;
		commands = NULL;
		fflush(stdin);
		fflush(stdout);

		if (stat != EXIT_SUCCESS)
			break;
	}
}

void init_deque(deque *dh)
{
	dh->size = 0;
	dh->head = NULL;
	dh->tail = NULL;
}

void push_back(deque *d, char *cmnd)
{
	if (d->size == MAX_COMMANDS)
	{
		pop_front(d);
	}
	node *new_node = (node *)malloc(sizeof(node));
	new_node->val = (char *)calloc((int)strlen(cmnd) + 1, sizeof(char));
	strcpy(new_node->val, cmnd);
	new_node->next = NULL;
	new_node->prev = NULL;
	new_node->lcs = 0;
	if (d->size == 0)
	{
		d->head = new_node;
		d->tail = new_node;
	}
	else
	{
		d->tail->next = new_node;
		new_node->prev = d->tail;
		d->tail = new_node;
	}
	d->size++;
}
void pop_front(deque *d)
{
	if (d->size == 0)
	{
		// underflow
		return;
	}
	else if (d->size == 1)
	{
		free(d->head->val);
		free(d->head);
		d->head = NULL;
		d->tail = NULL;
	}
	else
	{
		node *new_head = d->head->next;
		new_head->prev = NULL;
		free(d->head->val);
		free(d->head);
		d->head = new_head;
	}
	d->size--;
}

void clear_deque(deque* d){
	node* temp = d->head;
	while (temp!=NULL)
	{
		node* next=temp->next;
		free(temp->val);
		free(temp);
		temp=next;
	}
}

void fill_deque(deque *d, int fd)
{
	int i = 0;
	char *temp_buf = (char *)calloc(BUF_LEN, sizeof(char));
	int size = 0;
	int cur_len = BUF_LEN;
	while (1)
	{
		int k = read(fd, &temp_buf[i], 1);
		if (k < 0)
		{
			printf("error in reading the history.txt file\n");
			exit(EXIT_FAILURE);
		}
		if (k == 0)
			break;
		if (temp_buf[i] == '\n')
		{
			temp_buf[i] = '\0';
			if (size > 0)
				push_back(d, temp_buf);
			i = 0;
			size = 0;
			continue;
		}
		i++;
		size++;
		if (i >= cur_len)
		{
			cur_len += BUF_LEN;
			temp_buf = (char *)realloc(temp_buf, cur_len);

			if (temp_buf == NULL)
			{
				printf("Error : Memory Allocation Failed\n");
				exit(EXIT_FAILURE);
			}
		}
	}
	if (size > 0)
	{
		temp_buf[i] = '\0';
		push_back(d, temp_buf);
	}
}

void get_from_back_deque(deque *dh, int i)
{
	int ind = 1;
	node *temp = dh->tail;
	while (1)
	{
		if (temp == NULL)
		{
			printf("\nindex out of bounds");
			fflush(stdout);
			exit(EXIT_FAILURE);
		}
		if (ind == i)
		{
			free(line);
			line = (char *)calloc(sizeof(temp->val) + 1, sizeof(char));
			strcpy(line, temp->val);
			return;
		}
		ind++;
		temp = temp->prev;
	}
}

void print_deque(deque *dh)
{
	int tot = 1000;
	printf("showing the most recent %d commands...\n", tot);
	fflush(stdout);
	node *temp = dh->tail;
	int ind = 0;
	while (temp != NULL && ind < tot)
	{
		printf("[%d] %s\n", ind + 1, temp->val);
		fflush(stdout);
		ind++;
		temp = temp->prev;
	}
	printf("\n");
	fflush(stdout);
}
void print_node_with_given_lcs(deque *dh, int lcs)
{
	if(lcs>0)
		printf("\nshowing the most recent commands having length of LCS = %d with the search term.\n", lcs);
	else printf("\nshowing the most recent command having exact match with the search term.\n");
	fflush(stdout);
	node *temp = dh->tail;
	int ind = 0;
	while (temp != NULL)
	{
		if (temp->lcs == lcs)
		{
			printf("[%d] %s\n", ind + 1, temp->val);
			fflush(stdout);
		}else if(lcs==0 && temp->lcs==-1){
			printf("[%d] %s\n", ind + 1, temp->val);
			fflush(stdout);
			return;
		}
		ind++;
		temp = temp->prev;
	}
	printf("\n");
	fflush(stdout);
}

int calc_lcs(deque *d)
{
	if (line == NULL)
		return -1;
	/*
		res = -1
		no exact match or lcs of length <=2

		res = 0
		recent exact match found with variable 'line'

		res > 0
		max lcs of all the commands
	*/

	int res = -1;
	node *temp = d->tail;
	int mx = 0;
	int n = strlen(line);
	while (temp != NULL)
	{
		temp->lcs = get_lcs_value(line, temp->val);
		if (temp->lcs == n && (int)strlen(temp->val) == n)
		{
			temp->lcs=-1; // doing this so we can identify that this is an exact match
			return 0;
		}
		mx = (mx > temp->lcs ? mx : temp->lcs);
		temp = temp->prev;
	}
	if (mx <= 2)
		return -1;
	return mx;
}

int get_lcs_value(char *str1, char *str2)
{
	if (str1 == NULL || str2 == NULL)
		return 0;
	int n = strlen(str1), m = strlen(str2);

	int **dp = (int **)malloc((n + 1) * sizeof(int *));
	for (int i = 0; i <= n; i++)
	{
		dp[i] = (int *)calloc((m + 1), sizeof(int));
	}
	int mx = 0;

	for (int i = 1; i <= n; i++)
		for (int j = 1; j <= m; j++)
			if (str1[i - 1] == str2[j - 1])
			{
				dp[i][j] = dp[i - 1][j - 1] + 1;
				mx = (mx > dp[i][j] ? mx : dp[i][j]);
			}
	for (int i = 0; i <= n; i++)
	{
		free(dp[i]);
	}
	free(dp);
	return mx;
}

int get_line()
{
	line = (char *)malloc(sizeof(char) * BUF_LEN);

	int cur_len = BUF_LEN;
	char c;
	int idx = 0;

	char *last_token = (char *)calloc(BUF_LEN, sizeof(BUF_LEN));
	int idx_l = 0;
	int cur_len_l = BUF_LEN;

	// Reading command from user
	while (1)
	{
		// c = getchar(stdin);
		read(STDIN_FILENO, &c, 1);
		// printf("%c\n", c);
		if (c == 18)
		{
			// printf("ctrl+r detected\n");
			fflush(stdout);
			return 2;
		}
		if (c == '\n')
		{
			line[idx] = '\0';
			last_token[idx_l] = '\0';
			putchar(c);
			break;
		}
		if(c == 127){
			if(idx){
				idx--;
				line[idx]='\0';
				putchar('\b');
				putchar(' ');
				putchar('\b');
			}
			fflush(stdout);
			continue;
		}
		if (c == '\t')
		{
			/*
				assumed all files and folders will
				have simple english characters wihtout
				spaces, otherwise presing tab results
				in undefined autocompletes
			*/

			/*
				first copying all file names in a 2d char array
				then doing the checking
			*/

			last_token[idx_l] = '\0';
			int sz = idx_l;

			struct dirent *directory_entry;
			DIR *dr = opendir(".");
			if (dr == NULL)
			{
				printf("Could not open current directory\n");
				exit(EXIT_FAILURE);
			}
			int mx_size = 100;
			char **file_names = (char **)malloc(mx_size * (sizeof(char *)));
			int file_index = 0;
			while ((directory_entry = readdir(dr)) != NULL)
			{
				if (!strcmp(directory_entry->d_name, ".") ||
					!strcmp(directory_entry->d_name, "..") ||
					strlen(directory_entry->d_name) < sz ||
					strncmp(directory_entry->d_name, last_token, sz))
					continue;

				// size of file name must be greater than
				// or equal to the size of the last token
				// and all characters of last token
				// must match with it

				if (file_index == mx_size)
				{
					mx_size += 100;
					file_names = (char **)realloc(file_names, mx_size * (sizeof(char *)));
					if (file_names == NULL)
					{
						printf("error in memory reallocation.\n");
						exit(EXIT_FAILURE);
					}
				}
				file_names[file_index] = (char *)malloc((sizeof(directory_entry->d_name) + 1) * (sizeof(char)));
				strcpy(file_names[file_index], directory_entry->d_name);
				file_index++;
			}
			closedir(dr);

			if (file_index == 0)
				continue; // no matches

			if (file_index == 1)
			{
				// just print the one match
				for (int i = sz; i < strlen(file_names[0]); i++)
				{
					c = file_names[0][i];

					line[idx] = c;
					if (c == ' ')
					{
						idx_l = -1;
						last_token[0] = '\0';
					}
					else
						last_token[idx_l] = c;

					putchar(c);
					fflush(stdout);

					idx++;
					idx_l++;
					if (idx >= cur_len)
					{
						cur_len += BUF_LEN;
						line = (char *)realloc(line, cur_len);

						if (line == NULL)
						{
							printf("Error : Memory Allocation Failed\n");
							exit(EXIT_FAILURE);
						}
					}
					if (idx_l >= cur_len_l)
					{
						cur_len_l += BUF_LEN;
						last_token = (char *)realloc(last_token, cur_len_l);

						if (last_token == NULL)
						{
							printf("Error : Memory Allocation Failed\n");
							exit(EXIT_FAILURE);
						}
					}
				}
				continue;
			}

			// now more than one matches so give options
			for (int i = 0; i < file_index; i++)
			{
				printf("\n[%d] %s", i + 1, file_names[i]);
				fflush(stdout);
			}
			printf("\nPlease enter the corresponding index : ");
			fflush(stdout);
			int req_index = get_integer();
			fflush(stdout);
			if (req_index < 0)
				req_index = 1;

			line[idx] = '\0';
			printf("my_shell>> %s", line);
			fflush(stdout);
			for (int i = sz; i < strlen(file_names[req_index - 1]); i++)
			{
				c = file_names[req_index - 1][i];
				line[idx] = c;
				if (c == ' ')
				{
					idx_l = -1;
					last_token[0] = '\0';
				}
				else
					last_token[idx_l] = c;

				putchar(c);
				fflush(stdout);

				idx++;
				idx_l++;
				if (idx >= cur_len)
				{
					cur_len += BUF_LEN;
					line = (char *)realloc(line, cur_len);

					if (line == NULL)
					{
						printf("Error : Memory Allocation Failed\n");
						exit(EXIT_FAILURE);
					}
				}
				if (idx_l >= cur_len_l)
				{
					cur_len_l += BUF_LEN;
					last_token = (char *)realloc(last_token, cur_len_l);

					if (last_token == NULL)
					{
						printf("Error : Memory Allocation Failed\n");
						exit(EXIT_FAILURE);
					}
				}
			}
			continue;
		}

		line[idx] = c;
		if (c == ' ')
		{
			idx_l = -1;
			last_token[0] = '\0';
		}
		else
			last_token[idx_l] = c;

		putchar(c);
		fflush(stdout);

		idx++;
		idx_l++;
		if (idx >= cur_len)
		{
			cur_len += BUF_LEN;
			line = (char *)realloc(line, cur_len);

			if (line == NULL)
			{
				printf("Error : Memory Allocation Failed\n");
				exit(EXIT_FAILURE);
			}
		}
		if (idx_l >= cur_len_l)
		{
			cur_len_l += BUF_LEN;
			last_token = (char *)realloc(last_token, cur_len_l);

			if (last_token == NULL)
			{
				printf("Error : Memory Allocation Failed\n");
				exit(EXIT_FAILURE);
			}
		}
	}

	if (strcmp(line, "history") == 0)
		return 1;

	// printf("%d\n", strlen(line));
	// printf("%s\n", line);
	return 0;
}

void get_search_term()
{
	line = (char *)malloc(sizeof(char) * BUF_LEN);

	int cur_len = BUF_LEN;
	int idx = 0;

	// Reading command from user
	// assuming each command is of max lengfth BUF_LEN
	while (1)
	{
		read(STDIN_FILENO, line + idx, 1);

		if (line[idx] == '\n' || idx + 1 == BUF_LEN)
		{
			putchar('\n');
			line[idx] = '\0';
			fflush(stdout);
			break;
		}
		if(line[idx] == 127){
			if(idx){
				idx--;
				line[idx]='\0';
				putchar('\b');
				putchar(' ');
				putchar('\b');
			}
			fflush(stdout);
			continue;
		}

		putchar(line[idx]);
		fflush(stdout);
		idx++;
	}
}

int get_integer()
{
	int ans = 0;
	int res = 0;
	char c;

	// Reading integer from user
	while (1)
	{
		read(STDIN_FILENO, &c, 1);

		if (c == '\n')
		{
			putchar('\n');
			fflush(stdout);
			return ans;
		}

		if (c >= '0' && c <= '9')
		{
			putchar(c);
			fflush(stdout);
			ans *= 10;
			ans += c - '0';
		}
		else
		{
			// invalid character
			// so return -1
			return -1;
		}
	}
	return -1;
}

char **split_command(int *pipe_cnt)
{
	// Finding the number of pipes and respective commands

	*pipe_cnt = 0;
	char **commands = (char **)malloc(sizeof(char *) * BUF_LEN);
	// if(commands == NULL){
	// 	printf("Error : Memory Allocation Failed\n");
	// 	exit(EXIT_FAILURE);
	// }
	int cur_len = strlen(line) + 10;
	for (int i = 0; i < BUF_LEN; i++)
	{
		commands[i] = (char *)malloc(sizeof(char) * cur_len);
	}

	int i = 0, j = 0, c_idx = 0;
	cur_len = BUF_LEN;
	char p = '|', sq = '\'', dq = '\"';
	int line_len = strlen(line);

	while (i < line_len)
	{
		if (line[i] == p)
		{
			if (strlen(commands[c_idx]) <= 0)
			{
				printf("Error : Malformed Syntax\n");
				return NULL;
			}
			commands[c_idx][j] = '\0';
			c_idx++;
			if (c_idx >= cur_len)
			{
				cur_len += BUF_LEN;
				commands = (char **)realloc(commands, cur_len);
				if (commands == NULL)
				{
					printf("Error : Memory Allocation Failed\n");
					exit(EXIT_FAILURE);
				}
				for (int k = c_idx; k < cur_len; k++)
				{
					commands[i] = (char *)malloc(sizeof(char) * line_len);
				}
			}
			j = 0;
			i++;
		}
		else if (line[i] == sq || line[i] == dq)
		{
			char q = line[i];
			commands[c_idx][j] = line[i];
			i++;
			j++;
			while (i < line_len && line[i] != q)
			{
				commands[c_idx][j] = line[i];
				i++;
				j++;
			}
			if (line[i] != q)
			{
				printf("Error : Malformed Syntax\n");
				exit(EXIT_FAILURE);
			}
			i++;
			j++;
		}
		else
		{
			commands[c_idx][j] = line[i];
			i++;
			j++;
		}
	}
	commands[c_idx][j] = '\0';
	commands[c_idx + 1] = NULL;
	*pipe_cnt = c_idx + 1;
	return commands;
}

char **tokenize(char *command, int *arg_cnt)
{
	char **args;
	int cur_len = BUF_LEN;
	int cmd_len = strlen(command);
	int i = 0, t_idx = 0, j = 0;

	args = (char **)malloc(sizeof(char *) * cur_len);

	for (int i = 0; i < cur_len; i++)
	{
		args[i] = (char *)malloc(sizeof(char) * cmd_len);
	}

	char sq = '\'', dq = '\"';

	while (i < cmd_len)
	{
		if (command[i] == sq || command[i] == dq)
		{
			char q = command[i];
			args[t_idx][j] = command[i];
			i++;
			j++;

			while (i < cmd_len && command[i] != q)
			{
				args[t_idx][j] = command[i];
				i++;
				j++;
			}

			args[t_idx][j] = command[i];
			i++;
			j++;
			args[t_idx][j] = '\0';

			if (strlen(args[t_idx]) > 0)
			{
				t_idx++;
				j = 0;
			}
		}
		else if (command[i] == ' ' || command[i] == '\t' || command[i] == '\r' || command[i] == '\n')
		{
			args[t_idx][j] = '\0';
			if (strlen(args[t_idx]) > 0)
			{
				t_idx++;
			}
			j = 0;
			i++;
		}
		else
		{
			args[t_idx][j] = command[i];
			j++;
			if (i == cmd_len - 1)
			{
				args[t_idx][j] = '\0';
				if (strlen(args[t_idx]))
					t_idx++;
				args[t_idx] = NULL;
			}
			i++;
		}

		if (t_idx >= cur_len)
		{
			cur_len += BUF_LEN;
			args = (char **)realloc(args, cur_len);
			if (args == NULL)
			{
				printf("Error : Memory Allocation Failed\n");
				exit(EXIT_FAILURE);
			}

			for (int k = t_idx; k < cur_len; k++)
			{
				args[k] = (char *)malloc(sizeof(char) * cmd_len);
			}
		}
	}

	*arg_cnt = t_idx;
	return args;
}

int cmd_handler(char **args, int arg_cnt, int fdin, int fdout,deque *d)
{
	int stat = 0;
	sent_to_back = 0;

	if (strcmp(toLower(args[0], strlen(args[0])), "exit") == 0)
	{
		int fd_history = open("history.txt", O_CREAT | O_TRUNC | O_RDWR );
		if (fd_history < 0)
		{
			printf("error opening/creating history.txt\n");
			exit(EXIT_FAILURE);
		}
		node* temp = d->head;
		while (temp!=NULL)
		{
			write(fd_history,temp->val,strlen(temp->val));
			write(fd_history,"\n",1);
			temp=temp->next;
		}
		close(fd_history);
		clear_deque(d);
		exit(EXIT_SUCCESS);
	}
	pid = fork();

	if (pid < 0)
	{
		printf("Error : FORK Failed\n");
		exit(EXIT_FAILURE);
	}
	if (pid == 0)
	{ // Child Process
		if (fdout != 1)
		{
			dup2(fdout, 1);
			close(fdout);
		}
		if (fdin != 0)
		{
			dup2(fdin, 0);
			close(fdin);
		}

		int k = arg_cnt, flag = 0;

		char **pre = (char **)malloc(sizeof(char *) * arg_cnt);

		for (int i = 0; i < arg_cnt; i++)
		{
			if (strcmp(args[i], "<") == 0)
			{
				int new_fdin = open(args[i + 1], O_RDONLY);
				dup2(new_fdin, 0);
			}

			if (strcmp(args[i], ">") == 0)
			{
				int new_fdout = open(args[i + 1], O_CREAT | O_TRUNC | O_WRONLY, 0666);
				dup2(new_fdout, 1);
			}

			if (!flag)
			{
				if (strcmp(args[i], "<") == 0 || strcmp(args[i], ">") == 0 || strcmp(args[i], "&") == 0)
				{
					k = i;
					flag = 1;
				}
			}
		}
		int i;
		for (i = 0; i < k; i++)
		{
			pre[i] = args[i];
		}
		pre[i] = NULL;

		if (execvp(pre[0], pre) == -1)
		{
			printf("Error : Failed to execute %s\n", pre[0]);
			exit(EXIT_FAILURE);
		}
	}
	else
	{ // Parent Process
		if (strcmp(args[arg_cnt - 1], "&") != 0)
		{
			do
			{
				pid2 = waitpid(pid, &stat, WUNTRACED);
				// printf("From child %d\n", stat);
			} while (!WIFEXITED(stat) && !WIFSIGNALED(stat) && !sent_to_back);
			if (sent_to_back)
			{
				kill(pid, SIGCONT);
			}
		}
	}

	return 0;
}

char *toLower(char *str, size_t len)
{
	char *str_l = calloc(len + 1, sizeof(char));

	for (size_t i = 0; i < len; ++i)
	{
		str_l[i] = tolower((unsigned char)str[i]);
	}
	return str_l;
}

void SIGINT_Handler(int sig)
{
	exit_signal = 1;
	if (pid != 0)
		kill(pid, SIGINT);
	// else printf("\nmy_shell>> ");
}

void SIGTSTP_Handler(int sig)
{
	printf("\nSending possible children to background\n");
	sent_to_back = 1;
	// kill(pid, SIGCONT);
}

void multiWatch()
{
	char **commands;
	commands = (char **)malloc(sizeof(char *) * BUF_LEN);
	int idx = 0, stat;
	int j = 0;
	int i = 0;
	int line_len = strlen(line);
	// printf("%s\n", line);
	while (i < line_len)
	{
		while (i < line_len && line[i] != '\"')
			i++;
		i++;
		if (i >= line_len)
			break;
		commands[idx] = (char *)malloc(sizeof(char) * line_len);

		while (i < line_len && line[i] != '\"')
		{
			commands[idx][j] = line[i];
			j++;
			i++;
		}
		i++;
		commands[idx][j] = '\0';
		idx++;
		j = 0;
	}
	commands[idx] = NULL;
	int arg_cnt = 0;
	pid_t child_pids[idx];
	int fd_out[idx];

	for (int i = 0; i < idx; i++)
	{
		char **args = tokenize(commands[i], &arg_cnt);
		// printf("%s\n", commands[i]);
		fflush(stdout);
		int FD[2];
		if (pipe(FD) == -1)
		{
			printf("Pipe Error\n");
			return;
		}
		fd_out[i] = FD[0];
		pid = fork();

		if (pid < 0)
		{
			printf("Error : FORK Failed\n");
			exit(EXIT_FAILURE);
		}
		if (pid == 0)
		{ // Child Process

			dup2(FD[1], 1);
			if (execvp(args[0], args) == -1)
			{
				printf("Error : Failed to execute %s\n", args[0]);
				exit(EXIT_FAILURE);
			}
		}
		else
		{ // Parent Process

			child_pids[i] = pid;
		}
	}
	struct pollfd fds[idx];
	for (int i = 0; i < idx; i++)
	{
		fds[i].fd = fd_out[i];
		fds[i].events = POLLIN;
	}
	int timeout = 500;
	char buff[BUF_LEN];
	exit_signal = 0;
	while (1)
	{
		int ret = poll(fds, idx, timeout);
		for (int i = 0; i < idx; i++)
		{
			if (fds[i].revents & POLLIN)
			{
				int n = read(fds[i].fd, buff, BUF_LEN - 1);
				if (n <= 0)
					continue;
				buff[n] = '\0';
				printf("\"%s \", %lu\n", commands[i], (unsigned long)time(NULL));
				printf("<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-\n");
				printf("%s\n\n", buff);
				printf("->->->->->->->->->->->->->->->->->->->\n\n");
				fflush(stdout);
			}
		}
		if (exit_signal)
			break;
	}
}
