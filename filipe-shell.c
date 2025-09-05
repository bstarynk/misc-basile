/****
 * Original source code (MIT licensed) on from 
 *       https://github.com/FilipeChagasDev/small-linux-shell
 * 
 * some simple fixes by Basile Starynkevitch <basile@starynkevitch.net>
 *
 * SMALL LINUX SHELL
 * Author: Filipe Chagas
 *         ( filipe.ferraz0@gmail.com )
 *         ( github.com/filipechagasdev )
 *
 * 28 July 2020
 *
 * NOTE: This source code has not been separated into multiple files to facilitate the compilation process.
 *
 * This code file is organized into sections:
 *
 *      1 - CMD_LINE_T object features
 *      2 - Small lexer features
 *      3 - Parsing features
 *      4 - Command features
 *      5 - Main function
 */

/*** 
 * Basile note: in git commit bdceaecbc6 this has only warnings like
 *    comparison of unsigned expression in ‘>= 0’ is always true
 * using  GCC 12.2 on Linux/x86-64
 ***/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>		//contains the 'tolower' function
#include <unistd.h>		//contais the UNIX's syscall interface
#include <sys/syscall.h>	//contains syscall's numbers
#include <fcntl.h>		//flags used in 'open' syscall
#include <dirent.h>		//contains 'dirent' syscall constants
#include <wait.h>		//contains 'wait'
#include <errno.h>


#ifndef FILIPE_GIT
#error FILIPE_GIT should be defined by compilation command
#endif

const char filipe_gitid[] = FILIPE_GIT;

// ==========================================================
// =============== CMD_LINE_T OBJECT FEATURES ===============
// ==========================================================

#define MAX_TOKEN_LEN 500	//Maximun length for a token

/**
 * @brief Struct that contains the command and the arguments for each line typed by the user.
 * @param args    Array of pointers of argument strings.
 * @param nargs   Number of arguments.
 */
typedef struct
{
  char *command;		//command string
  char **args;			//array of argument strings
  size_t nargs;			//number of arguments
} cmd_line_t;

/**
 * @brief Creates an empty cmd_line_t struct buffer.
 * @return Pointer to the cmd_line_t struct buffer.
 */
cmd_line_t *
create_cmd_line ()
{
  cmd_line_t *my_cmd_line = (cmd_line_t *) malloc (sizeof (cmd_line_t));
  assert (my_cmd_line != NULL);

  //set command
  my_cmd_line->command = NULL;

  //set args as NULL
  my_cmd_line->args = NULL;

  //set nargs as 0
  my_cmd_line->nargs = 0;

  return my_cmd_line;
}

/**
 * @brief Set the command string of the cmd_line struct buffer
 * @param cmd_line    Pointer to the working cmd_line_t structure buffer
 * @param command     String that will be copied to the cmd_line buffer
 */
void
set_cmd_line_command (cmd_line_t *cmd_line, char *command)
{
  assert (cmd_line != NULL);
  assert (command != NULL);

  if (cmd_line->command != NULL)	//if cmd_line already have a command string buffer
    free (cmd_line->command);	//destroy it

  cmd_line->command = calloc (strlen (command), sizeof (char));	//create a new buffer to 'command' in cmd_line
  assert (cmd_line->command != NULL);

  strcpy (cmd_line->command, command);	//copy text from 'command' to 'cmd_line->command'
}

/**
 * @brief Initialize the arguments array of the cmd_line buffer
 * @param cmd_line    Pointer to the working cmd_line_t struct buffer
 * @param nargs       Number of arguments in the array
 */
void
init_cmd_line_args (cmd_line_t *cmd_line, size_t nargs)
{
  assert (cmd_line != NULL);
  assert (nargs > 0);

  if (cmd_line->args != NULL)	//if cmd_line already have an 'args' buffer
    free (cmd_line->args);	//destroy it

  cmd_line->args = calloc (nargs, sizeof (char *));	//create a new buffer to 'cmd_line->args'
  assert (cmd_line->args != NULL);

  cmd_line->nargs = nargs;
}

/**
 * @brief Set an argument string of the cmd_line buffer
 * @param cmd_line    Pointer to the working cmd_line_t struct buffer
 * @param arg         Argument text (string)
 * @param argi        Index of the argument in the args array of the cmd_line_t struct buffer
 */
void
set_cmd_line_arg (cmd_line_t *cmd_line, char *arg, size_t argi)
{
  assert (cmd_line != NULL);
  assert (arg != NULL);
  assert (argi >= 0 && argi < cmd_line->nargs);

  if (cmd_line->args[argi] != NULL)	//if 'args[argi]' already have a buffer
    free (cmd_line->args[argi]);	//destroy it

  cmd_line->args[argi] = calloc (strlen (arg), sizeof (char));	//create a new buffer to the arg
  assert (cmd_line->args[argi] != NULL);

  strcpy (cmd_line->args[argi], arg);	//copy text from 'arg' to the buffer
}

/**
 * @brief Free the buffer of the cmd_line_t struct with it's all contents
 * @param cmd_line    Pointer to the working cmd_line_t struct buffer
 */
void
destroy_cmd_line (cmd_line_t *cmd_line)
{
  assert (cmd_line != NULL);

  if (cmd_line->command != NULL)
    free (cmd_line->command);	//destroy 'command' string buffer

  if (cmd_line->args != NULL)	//if cmd_line have a buffer to the 'args' array
    {
      for (size_t i = 0; i < cmd_line->nargs; i++)
	{
	  if (cmd_line->args[i] != NULL)	//if 'args[i]' have a buffer
	    free (cmd_line->args[i]);	//destroy it
	}

      free (cmd_line->args);	//destroy the 'args' array buffer
    }

  free (cmd_line);		//destroy the struct
}

// ====================================================
// =============== SMALL LEXER FEATURES ===============
// ====================================================

/**
 * @brief Change each string's char to lower case
 * @param str   Pointer to the working string
 */
void
string_to_lower (char *str)
{
  assert (str != NULL);

  for (int i = 0; i < (int) strlen (str); i++)
    str[i] = tolower (str[i]);
}

/**
 * @brief Check if the string has only blank {' ', '\t', '\n'} chars
 * @param str   Pointer to the string in analysis.
 * @return 1 (true) if the string has only blank {' ', '\t', '\n'} chars. Otherwise, returns 0 (false).
 *         Note: returns 1 (true) for len=0 strings.
 */
int
blank_string (char *str)
{
  assert (str != NULL);

  for (int i = 0; i < (int) strlen (str); i++)
    {
      if (str[i] != ' ' && str[i] != '\t' && str[i] != '\n')
	return 0;
    }
  return 1;
}

/**
 * @brief Rrompt the terminal to reading a line of the tty. Return the first remaining token in stdin.
 * @param str    Pointer to the string buffer where the text will be stored.
 * @return 1 (true) if the obtained token is the last of the line in stdin. Otherwise, returns 0 (false).
 */
int
read_token (char *str)
{
  assert (str != NULL);

  char c[2];
  c[0] = getchar ();		//get the first char
  c[1] = '\0';			//end of string

  while (c[0] == ' ' || c[0] == '\t')
    c[0] = getchar ();		//jump spaces and tabs

  // now, c[0] is the first char of the token

  size_t token_len = 0;

  while (c[0] != '\n' && c[0] != ' ' && c[0] != '\t')	//the token ends with '\n' or space or tab
    {
      strcat (str, c);		//append the last obtained char to string
      token_len += 1;		//increment the token lenght
      c[0] = getchar ();	//read the next char in stdin
    }

  assert (token_len < MAX_TOKEN_LEN);

  return c[0] == '\n';		//return true if the obtained token is the last of the line in stdin
}

/**
 * @brief Prompt the terminal to reading a line of the tty. Put the tokens read at the args array of cmd_line.
 * @param cmd_line    Pointer to the working cmd_line_t struct buffer.
 * @param argi        Index of the argument read in each recursive call. It must start at 0.
 */
void
read_args (cmd_line_t *cmd_line, size_t argi)
{
  assert (cmd_line != NULL);
  assert (argi >= 0);

  //read the next token
  char str[MAX_TOKEN_LEN] = "";

  int end_of_line, is_blank;

  do
    {
      end_of_line = read_token (str);
      is_blank = blank_string (str);
    }
  while (is_blank && !end_of_line);

  if (!end_of_line)		//if stdin is not empty
    {
      read_args (cmd_line, argi + 1);	//(recursion) go to the next arg
    }
  else				//exit condition
    {
      if (is_blank && argi > 0)
	init_cmd_line_args (cmd_line, argi);
      else if (!is_blank && argi >= 0)
	init_cmd_line_args (cmd_line, argi + 1);
      else
	return;
    }

  if (argi >= 0)
    set_cmd_line_arg (cmd_line, str, argi);	//put the argument text in the cmd_line struct
}

/**
 * @brief Prompt the terminal to reading a line of the tty. Put the tokens read at the cmd_line buffer.
 * @param cmd_line    Pointer to the working cmd_line_t struct buffer.
 */
void
read_cmd_line (cmd_line_t *cmd_line)
{
  assert (cmd_line != NULL);

  //read command
  char command[MAX_TOKEN_LEN] = "";
  int end_of_line = read_token (command);

  string_to_lower (command);

  set_cmd_line_command (cmd_line, command);

  //read arguments
  if (!end_of_line)
    read_args (cmd_line, 0);
}

/**
 * @brief Print the contents of the cmd_line struct buffer.
 * @param cmd_line    Pointer to the working cmd_line_t struct buffer.
 */
void
print_cmd_line (cmd_line_t *cmd_line)
{
  assert (cmd_line != NULL);

  if (cmd_line->command != NULL)
    printf ("COMMAND: %s\n", cmd_line->command);

  printf ("ARGS:\n");
  for (size_t i = 0; i < cmd_line->nargs; i++)
    {
      if (cmd_line->args[i] != NULL)
	printf ("[%d]\t%s\n", (int) i, cmd_line->args[i]);
    }

  printf ("NARGS: %d\n", (int) cmd_line->nargs);
}

// ================================================
// =============== PARSING FEATURES ===============
// ================================================

#define ALPHABETICAL_TREE_ENTRIES 26	//Number of output edges for each vertex in the alphabetical tree

/**
 * @brief Node struct of the alphabetical tree.
 *        This tree serves to store tokens, so that the information of their existence
 *        or absence can be retrieved quickly. Each level of this tree corresponds to the
 *        index of a token character, and each edge of the tree corresponds to a lowercase
 *        letter of the alphabet.
 *        This tree algorithm will be used here as a command dictionary.
 *
 * @param cmd_callback  callback to the function that perform the command (NULL if the command does not exists)
 * @param text          Content text for variables (NULL if the variable does not exists)
 * @param next          Edges for the next tree level.
 */
typedef struct alphabetical_tree_node
{
  //callback to the function that perform the command (NULL if the command does not exists)
  void (*cmd_callback) (cmd_line_t *);
  char *text;

  struct alphabetical_tree_node *next[ALPHABETICAL_TREE_ENTRIES];
} alphabetical_tree_node_t;

/**
 * @brief Header of a alphabetical tree.
 * @param entries   Entries for the first level vertexes in the tree.
 */
typedef struct
{
  alphabetical_tree_node_t *entries[ALPHABETICAL_TREE_ENTRIES];
} alphabetical_tree_header_t;

/**
 * @brief Check if the string is compatible with the alphabetical tree
 * @param str   String in analysis
 * @return 1 (true) if str is compatible with the alphabetical tree. Otherwise, returns 0 (false).
 */
int
alphabetical_string (char *str)
{
  for (int i = 0; i < (int) strlen (str); i++)	//checks for prohibited chars in the command token
    {
      if (str[i] - 'a' >= ALPHABETICAL_TREE_ENTRIES || str[i] - 'a' < 0)	//non-alphabetic char
	return 0;
    }

  return 1;
}

/**
 * @brief Creates an empty alphabetical tree node structure.
 * @return A pointer to the node.
 */
alphabetical_tree_node_t *
create_alphabetical_tree_node ()
{
  alphabetical_tree_node_t *node =
    (alphabetical_tree_node_t *) malloc (sizeof (alphabetical_tree_node_t));
  assert (node != NULL);

  node->cmd_callback = NULL;

  for (int i = 0; i < ALPHABETICAL_TREE_ENTRIES; i++)
    node->next[i] = NULL;

  return node;
}


/**
 * @brief Creates an empty alphabetical tree.
 * @return A pointer to the header of the tree.
 */
alphabetical_tree_header_t *
create_alphabetical_tree ()
{
  alphabetical_tree_header_t *h =
    (alphabetical_tree_header_t *)
    malloc (sizeof (alphabetical_tree_header_t));
  assert (h != NULL);

  for (int i = 0; i < ALPHABETICAL_TREE_ENTRIES; i++)
    h->entries[i] = NULL;

  return h;
}

#if 0				//unused code
void
destroy_alphabetical_subtree (alphabetical_tree_node_t *node)
{
  if (node != NULL)
    {
      for (int i = 0; i < ALPHABETICAL_TREE_ENTRIES; i++)
	destroy_alphabetical_subtree (node->next[i]);

      if (node->text != NULL)
	free (node->text);
      free (node);
    }
}

void
destroy_alphabetical_tree (alphabetical_tree_header_t *h)
{
  assert (h != NULL);

  for (int i = 0; i < ALPHABETICAL_TREE_ENTRIES; i++)
    destroy_alphabetical_subtree (h->entries[i]);
  free (h);
}
#endif

/**
 * @brief Add a token to the alphabetical tree.
 * @param h      Pointer to the header of the tree.
 * @param token  Token that will be added to the tree.
 * @param text   Text content of the token node
 */
void
insert_token_in_tree (alphabetical_tree_header_t *h, char *token,
		      void (*callback) (cmd_line_t *), char *text)
{
  assert (h != NULL);
  assert (token != NULL);

  size_t token_len = strlen (token);
  assert (token_len > 0);

  char first_char = token[0];
  int j = first_char - 'a';

  if (h->entries[j] == NULL)
    h->entries[j] = create_alphabetical_tree_node ();

  if (token_len == 1)
    {
      h->entries[j]->cmd_callback = callback;
    }
  else
    {
      alphabetical_tree_node_t *iterator = h->entries[j];
      for (int i = 1; i < (int) token_len; i++)
	{
	  j = token[i] - 'a';

	  if (iterator->next[j] == NULL)
	    iterator->next[j] = create_alphabetical_tree_node ();

	  iterator = iterator->next[j];
	}

      iterator->cmd_callback = callback;

      if (text != NULL)
	{
	  //Copy text for the node
	  iterator->text = (char *) calloc (strlen (text), sizeof (char));
	  strcpy (iterator->text, text);
	}
    }
}

/**
 * @brief Find the node of a token in the tree.
 * @param h      Pointer to the header of the tree.
 * @param token  Lowercase text (string) of the token.
 * @return A pointer to the node found, or NULL if token has not found.
 */
alphabetical_tree_node_t *
find_token_in_tree (alphabetical_tree_header_t *h, char *token)
{
  assert (h != NULL);
  assert (token != NULL);

  size_t token_len = strlen (token);
  assert (token_len > 0);

  int j = token[0] - 'a';	//Index of the entrie for the first node

  assert (j < 27);

  if (token_len == 1)		//if token has only one char
    {
      return h->entries[j];	//Return a node of the first tree's level
    }
  else
    {
      //Walk in the tree
      alphabetical_tree_node_t *iterator = h->entries[j];
      for (int i = 1; i < (int) token_len; i++)	//For each token's char (after the first char)
	{
	  if (iterator == NULL)
	    return NULL;	//If the current node is NULL, then there is no path to the token.

	  j = token[i] - 'a';	//Index of the edge for the next node
	  assert (j < 27);

	  iterator = iterator->next[j];	//Go to the next node
	}
      //The current node pointered by the iterator is the token's node.
      return iterator;
    }
}

/**
 * @brief Run the command function.
 * @param h         Pointer to the header of the alphabetical tree which has the command's token.
 * @param cmd_line  Pointer to the cmd_line_t struct buffer with the command token and its arguments.
 */
void
run_command (alphabetical_tree_header_t *h, cmd_line_t *cmd_line)
{
  assert (h != NULL);
  assert (cmd_line != NULL);

  if (!alphabetical_string (cmd_line->command))
    {
      printf ("ERROR: Commands do not have non-alphabetic characters\n");
      return;
    }

  alphabetical_tree_node_t *node = find_token_in_tree (h, cmd_line->command);

  if (node != NULL)
    node->cmd_callback (cmd_line);
  else
    printf ("command not found\n");
}

// ================================================
// =============== COMMAND FEATURES ===============
// ================================================

alphabetical_tree_header_t *dictionary = NULL;	//command dictionary
alphabetical_tree_header_t *variables = NULL;	//variables

/**
 * @brief Treatment function of the PWD command.
 * @param cmd_line  Pointer to the cmd_line_t struct buffer with the command token and its arguments.
 */
void
pwd_command (cmd_line_t *cmd_line)
{
  if (cmd_line->nargs != 0)
    {
      printf ("ERROR: The 'pwd' command has no arguments\n");
      print_cmd_line (cmd_line);
    }
  else
    {
      char wd_name[MAX_TOKEN_LEN];
      syscall (SYS_getcwd, wd_name, MAX_TOKEN_LEN);

      printf ("%s\n", wd_name);	//linux syscall 'getcwd' to get the current working dir name
    }
}

/**
 * @brief Treatment function of the CD command.
 * @param cmd_line  Pointer to the cmd_line_t struct buffer with the command token and its arguments.
 */
void
cd_command (cmd_line_t *cmd_line)
{
  if (cmd_line->nargs != 1)
    {
      printf ("ERROR: The 'cd' command has 1 argument\n");
      print_cmd_line (cmd_line);
    }
  else
    {
      int retval = syscall (SYS_chdir, cmd_line->args[0]);	//linux syscall 'chdir' to change the current working dir name

      if (retval == -1)		//error flag
	printf ("ERROR: Cannot change the working directory path for that\n");
    }
}

/**
 * @brief Treatment function of the EXIT command.
 * @param cmd_line  Pointer to the cmd_line_t struct buffer with the command token and its arguments.
 */
void
exit_command (cmd_line_t *cmd_line)
{
  if (cmd_line->nargs != 0)
    {
      printf ("ERROR: The 'exit' command has no arguments\n");
      print_cmd_line (cmd_line);
    }
  else
    {
      syscall (SYS_exit, EXIT_SUCCESS);	//linux syscall 'exit' to close the program
    }
}

/**
 * @brief Linux's dirent (directory entrie) struct. Do not change it.
 * @param d_ino     Inode number (32 bits)
 * @param d_off     Offset to next linux_dirent (32 bits)
 * @param d_reclen  Length of this linux_dirent (16 bits)
 * @param d_name    Filename (null-terminated)
 */
typedef struct linux_dirent
{
  unsigned long d_ino;
  unsigned long d_off;
  unsigned short d_reclen;
  char d_name[];
} linux_dirent_t;

#define DENTS_BUFFER_SIZE 1024*1024*5

/**
 * @brief Print name and type of each entrie in the dirents buffer.
 *        This function is used by the LS command.
 *
 * @param buffer        Pointer to the dirents buffer
 * @param n_entries     Number of dirents in the buffer
 */
void
print_entries (void *buffer, size_t n_entries)
{
  assert (buffer != NULL);

  linux_dirent_t *entrie = (linux_dirent_t *) buffer;	//first entrie

  for (size_t addr_offset = 0; addr_offset < n_entries;
       addr_offset += entrie->d_reclen)
    {
      entrie = (struct linux_dirent *) (buffer + addr_offset);	//get current entrie

      //the last byte of the entrie is its type flag
      char entrie_type =
	*(char *) ((unsigned long) entrie + entrie->d_reclen - 1);

      if (strcmp (entrie->d_name, "..") && strcmp (entrie->d_name, "."))	//ignore '..' and '.' dir entries
	{
	  switch (entrie_type)	//print small entrie's type
	    {
	    case DT_DIR:
	      printf ("[DIR]");
	      break;		//directory entrie
	    case DT_REG:
	      printf ("[FILE]");
	      break;		//file entrie
	    case DT_LNK:
	      printf ("[LINK]");
	      break;		//link entrie
	    case DT_SOCK:
	    case DT_CHR:
	    case DT_BLK:
	    case DT_FIFO:
	      printf ("[SYS]");
	      break;		//system entrie (devices, sockets and pipes)
	    default:
	      printf ("[UNK]");	//unknow entrie type
	    }

	  printf ("\t%s\t\t", entrie->d_name);	//print entrie's name

	  switch (entrie_type)	//print system entrie's type description
	    {
	    case DT_SOCK:
	      printf ("(network socket)");
	      break;
	    case DT_CHR:
	      printf ("(char device)");
	      break;
	    case DT_BLK:
	      printf ("(block device)");
	      break;
	    case DT_FIFO:
	      printf ("(pipe)");
	      break;
	    }

	  printf ("\n");	//break line
	}
    }
}

/**
 * @brief Treatment function of the LS command.
 * @param cmd_line  Pointer to the cmd_line_t struct buffer with the command token and its arguments.
 */
void
ls_command (cmd_line_t *cmd_line)
{
  assert (cmd_line != NULL);

  char dir_name[MAX_TOKEN_LEN];	//buffer to working dir name

  // STEP 1 - GET THE PATH OF THE DIRECTORY THAT WILL BE LOAD

  if (cmd_line->nargs > 1)
    {
      printf ("ERROR: The ls command has 0 or 1 arguments\n");
      print_cmd_line (cmd_line);
      return;
    }
  else if (cmd_line->nargs == 1)
    strcpy (dir_name, cmd_line->args[0]);
  else
    syscall (SYS_getcwd, dir_name, MAX_TOKEN_LEN);	//linux syscall 'getcwd' to get the current working dir name

  // STEP 2 - LOAD THE DIRECTORY AS A DESCRIPTOR

  int fd = open (dir_name, O_RDONLY | O_DIRECTORY);	//open the working dir as a file descriptor

  if (fd == -1)			//file descriptor equal to -1 is an error flag
    {
      printf ("ERROR: Cannot load that directory descriptor\n");
      return;
    }

  // STEP 3 - GET DIRECTORY ENTRIES FROM THE DESCRIPTOR AND PRINT ITS INFORMATIONS

  void *buffer = malloc (DENTS_BUFFER_SIZE);	//buffer to dir entries

  long n_read = syscall (SYS_getdents, fd, buffer, DENTS_BUFFER_SIZE);	//getdents syscall to get dir entries

  //the 'getdents' syscall must be repeated as long as there are unread entries in the descriptor.
  while (n_read != 0)		//do it while the 'getdents' syscall gets most than zero entries
    {
      if (n_read == -1)		//n_read equal to -1 is an error flag
	{
	  printf ("ERROR: Cannot read entries of that directory\n");
	  return;
	}

      print_entries (buffer, n_read);	//print entries informations

      //repeat 'getdents' syscall to get remaining dir entries
      n_read = syscall (SYS_getdents, fd, buffer, DENTS_BUFFER_SIZE);
    }
}

/**
 * @brief Treatment function of the EXEC command.
 * @param cmd_line  Pointer to the cmd_line_t struct buffer with the command token and its arguments.
 */
void
exec_command (cmd_line_t *cmd_line)
{
  assert (cmd_line != NULL);

  if (cmd_line->nargs < 1)
    {
      printf ("ERROR: The ls command has at least 1 argument\n");
      print_cmd_line (cmd_line);
      return;
    }

  int my_pid = fork ();		//fork the current process

  if (my_pid == 0)		//if this is the child process
    {
      //build the argv array
      char **argv = (char **) calloc (cmd_line->nargs + 1, sizeof (char *));

      for (int i = 0; i < (int) cmd_line->nargs; i++)
	argv[i] = cmd_line->args[i];

      argv[cmd_line->nargs] = NULL;	//argv must be NULL-terminated

      //change the current process to 'args[0]'
      int ret_val = execv (cmd_line->args[0], argv);

      if (ret_val == -1)	//error flag
	{
	  printf ("ERROR: Cannot execute \'%s\'\n", cmd_line->args[0]);
	  exit (0);
	}
    }

  ///Basile: this was obviously wrong: wait(2);
  int ws = 0;
  pid_t wpid = wait (&ws);
  if (wpid < 0)
    {
      printf ("ERROR: wait failed (%s) for command  \'%s\'\n",
	      strerror (errno), cmd_line->args[0]);
    }
}

/**
 * @brief Treatment function of the SET command.
 * @param cmd_line  Pointer to the cmd_line_t struct buffer with the command token and its arguments.
 */
void
set_command (cmd_line_t *cmd_line)
{
  assert (cmd_line != NULL);
  assert (variables != NULL);

  if (cmd_line->nargs != 3)
    {
      printf ("ERROR: The set command has 3 arguments\n");
      print_cmd_line (cmd_line);
      return;
    }

  char *dest_var = cmd_line->args[0];	//destination variable name
  char *text = NULL;		//text that will be stored in the variable

  if (!alphabetical_string (dest_var))
    {
      printf ("ERROR: Variable's names must have only alphabetical chars\n");
      return;
    }

  if (!strcmp (cmd_line->args[1], "as"))	// variable <- text
    {
      text = cmd_line->args[2];
    }
  else if (!strcmp (cmd_line->args[1], "like"))	// variable <- variable
    {
      char *origin_var = cmd_line->args[2];

      if (!alphabetical_string (origin_var))
	{
	  printf
	    ("ERROR: Variable's names must have only alphabetical chars\n");
	  return;
	}

      alphabetical_tree_node_t *n =
	find_token_in_tree (variables, origin_var);
      text = n->text;
    }
  else
    {
      printf ("ERROR: Missing \'as\' or \'like\' (lowercase) statement.\n"
	      "The syntax of \'store\' command is: set [var] as [text].\n"
	      "\tor: set [dest_var] like [origin_var]\n"
	      "Example: set dev_path as \\dev\n"
	      "Example: set dev_path_2 like dev_path\n");
      return;
    }

  insert_token_in_tree (variables, dest_var, NULL, text);
}

/**
 * @brief Treatment function of the PRINT command.
 * @param cmd_line  Pointer to the cmd_line_t struct buffer with the command token and its arguments.
 */
void
print_command (cmd_line_t *cmd_line)
{
  assert (cmd_line != NULL);

  if (cmd_line->nargs < 1)
    {
      printf ("ERROR: The print command has at least 1 argument\n");
      print_cmd_line (cmd_line);
      return;
    }

  for (int i = 0; i < (int) cmd_line->nargs; i++)
    {
      if (cmd_line->args[i][0] == '$' && cmd_line->args[i][1] != '$')	//arg is a variable name
	{
	  char *var_name = &cmd_line->args[i][1];

	  if (!alphabetical_string (var_name))
	    {
	      printf
		("ERROR: Variable's names have only alphabetical chars\n");
	      return;
	    }

	  /// Basile adds GITID and TIMESTAMP as magical variable names
	  if (!strcmp (var_name, "GITID"))
	    {
	      printf ("%s ", FILIPE_GIT);
	    }

	  else if (!strcmp (var_name, "TIMESTAMP"))
	    {
	      printf ("%s ", __DATE__ "@" __TIME__);
	    }

	  else
	    {
	      alphabetical_tree_node_t *n =
		find_token_in_tree (variables, var_name);

	      if (n == NULL)
		{
		  printf ("\nERROR: Variable \'%s\' not found\n", var_name);
		  return;
		}

	      printf ("%s ", n->text);
	    }
	}
      else if (cmd_line->args[i][0] == '$' && cmd_line->args[i][1] == '$')	//arg begins with '$' but is not a variable name
	printf ("%s ", &cmd_line->args[i][1]);
      else
	printf ("%s ", cmd_line->args[i]);
    }
  printf ("\n");
}

/**
 * @brief Treatment function of the UV command.
 * @param cmd_line  Pointer to the cmd_line_t struct buffer with the command token and its arguments.
 */
void
uv_command (cmd_line_t *cmd_line)
{
  assert (cmd_line != NULL);

  if (cmd_line->nargs < 2)
    {
      printf ("ERROR: The uv command has at least 2 arguments\n");
      print_cmd_line (cmd_line);
      return;
    }

  cmd_line_t *new_cmd_line = create_cmd_line ();
  set_cmd_line_command (new_cmd_line, cmd_line->args[0]);
  init_cmd_line_args (new_cmd_line, cmd_line->nargs - 1);

  for (int i = 1; i < (int) cmd_line->nargs; i++)
    {
      if (cmd_line->args[i][0] == '$' && cmd_line->args[i][1] != '$')	//arg is a variable name
	{
	  char *var_name = &cmd_line->args[i][1];

	  if (!alphabetical_string (var_name))
	    {
	      printf
		("ERROR: Variable's names have only alphabetical chars\n");
	      return;
	    }

	  alphabetical_tree_node_t *n =
	    find_token_in_tree (variables, var_name);

	  if (n == NULL)
	    {
	      printf ("\nERROR: Variable \'%s\' not found\n", var_name);
	      return;
	    }

	  set_cmd_line_arg (new_cmd_line, n->text, i - 1);
	}
      else if (cmd_line->args[i][0] == '$' && cmd_line->args[i][1] == '$')	//arg begins with '$' but is not a variable name
	set_cmd_line_arg (new_cmd_line, &cmd_line->args[i][1], i - 1);
      else
	set_cmd_line_arg (new_cmd_line, cmd_line->args[i], i - 1);
    }

  run_command (dictionary, new_cmd_line);
  destroy_cmd_line (new_cmd_line);

}

/**
 * @brief Print text content for each node in the alphabetical subtree
 * @param node  Pointer to the root of the alphabetical subtree
 */
void
print_subtree_texts (alphabetical_tree_node_t *node)
{
  if (node != NULL)
    {
      if (node->text != NULL)
	printf ("%s\n", node->text);

      for (int i = 0; i < ALPHABETICAL_TREE_ENTRIES; i++)
	print_subtree_texts (node->next[i]);
    }
}

/**
 * @brief Treatment function of the HELP command.
 * @param cmd_line  Pointer to the cmd_line_t struct buffer with the command token and its arguments.
 */
void
help_command (cmd_line_t *cmd_line)
{
  assert (cmd_line != NULL);

  if (cmd_line->nargs != 0)
    printf ("WARNING: The \'help\' command has no arguments\n\n");

  printf ("Command line syntax:\n"
	  "* No arguments:\t\t[command]\n"
	  "* Single argument:\t[command] [arg]\n"
	  "* N arguments:\t\t[command] [arg_0] [arg_1] ... [arg_{N-1}]\n"
	  "\n" "---- COMMANDS ----\n\n");

  for (int i = 0; i < ALPHABETICAL_TREE_ENTRIES; i++)
    {
      print_subtree_texts (dictionary->entries[i]);
    }
}


// =============================================
// =============== MAIN FUNCTION ===============
// =============================================

int
main ()
{

  printf ("Small Linux Shell\n"
	  "By Filipe Chagas\n"
	  "\t( filipe.ferraz0@gmail.com )\n"
	  "\t( github.com/filipechagasdev )\n"
	  "Type 'help' to see the list of commands\n\n");

  //Building variables tree
  variables = create_alphabetical_tree ();

  //Building the dictionary of commands
  dictionary = create_alphabetical_tree ();

  insert_token_in_tree (dictionary, "pwd", pwd_command,
			"* PWD (Print Working Directory)\n"
			"\tArguments: no arguments.\n"
			"\tDescription: Print current working directory path.\n");

  insert_token_in_tree (dictionary, "cd", cd_command,
			"* CD (Change Directory)\n"
			"\tArguments: path.\n"
			"\tDescription: Change working directory path. \n");

  insert_token_in_tree (dictionary, "exit", exit_command,
			"* EXIT\n"
			"\tArguments: no arguments.\n"
			"\tDescription: Close the shell.\n");

  insert_token_in_tree (dictionary, "ls", ls_command,
			"* LS (List)\n"
			"\tArguments: path (optional).\n"
			"\tDescription: Lists entries in the directory (argument directory or working directory).\n");

  insert_token_in_tree (dictionary, "exec", exec_command,
			"* EXEC (Execute) \n"
			"\tArguments: path, arg0, arg1, ..., argn.\n"
			"\tDescription: Execute the *path* file.\n" "\n");

  insert_token_in_tree (dictionary, "help", help_command,
			"* HELP\n"
			"\tArguments: no arguments.\n"
			"\tDescription: Print informations about this shell.\n");

  insert_token_in_tree (dictionary, "set", set_command,
			"* SET\n"
			"\tArguments: varname, as, text\n"
			"\t\t destvarname like originvarname\n"
			"\tDescription: With \'as\', set the text as variable's content.\n"
			"\t\t With \'like\', copy contents from the origin var to the destination var.\n");

  insert_token_in_tree (dictionary, "print", print_command,
			"* PRINT\n"
			"\tArguments: $varname or text, ... (n-times)\n"
			"\tDescription: Print contents of arguments.\n");

  insert_token_in_tree (dictionary, "uv", uv_command,
			"* UV (Using Variables)\n"
			"\tArguments: command, $varname or text, ... (n-times)\n"
			"\tDescription: Use any command with variables.\n");

  //Runtime loop
  cmd_line_t *cmd_line;
  while (1)
    {
      cmd_line = create_cmd_line ();	//new command line buffer

      printf (">>> ");

      read_cmd_line (cmd_line);	//read command line

      if (strlen (cmd_line->command) == 0)
	continue;		//Ignore empty command line

      run_command (dictionary, cmd_line);	//run command line

      destroy_cmd_line (cmd_line);	//discard command line buffer
    }

  return 0;
}

/****************
 **                           for Emacs...
 ** Local Variables: ;;
 ** compile-command: "make filipe-shell" ;;
 ** End: ;;
 ****************/
