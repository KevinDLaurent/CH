/* ch.c.
auteur: Kevin Laurent (20062981)
date: fevrier 2019
problèmes connus:

  */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>

// Signatures des fonctions auxilieres
char * get_line();
char * parse_quotes(char *input_copy);
int configure_cmd(char cmd_type[], char *args[], char *options[], int buff);
int exec_cmd(char *ex[], int back_process, int redirect, char *out_file);
int parse_cmd(char *input);
int parse_logic(char *input, int len);
int parse_if(char *input, int len);

// retourne un input sur STDIN
char * get_line()
{
  char *line = malloc(sizeof(char) * 2);
  char c;
  int i = 0;

  // Tant que on arrive pas a la fin de l'input, reallouer un buffer plus grand
  while((c = getchar()) != '\n' && c != EOF) {
    line = realloc(line, (sizeof(char)) * (i+1));
    line[i++] = c;
  }

  line[i] = '\0';
  return line;
}

// Appel system execvp()
int exec_cmd(char *ex[], int back_process, int redirect, char *out_file)
{
  int status;

  pid_t pid = fork();
  // Si l'enfant
  if (pid == 0)
  {
    // Si le flag pour la redirection (>) est allume
    if (redirect == 1)
    {
      // ouvrir (ou creer fichier), changer STDIN au fichier, executer la commande
      int out = open(out_file, O_WRONLY | O_APPEND | O_CREAT, S_IRUSR | S_IWUSR);
      dup2(out, 1);
      close(out);
      execvp(ex[0], ex);
      return EXIT_FAILURE;
    }
    else
    {
      execvp(ex[0], ex);
      return EXIT_FAILURE;
    }
    
  }
  // Si c'est le parent
  else if (pid > 0)
  {
    // Si le flag de back_process est allume, on attend brievement pour imprimer CH> apres
    // le output de execvp()
    if (back_process == 1)
    {
      usleep(100000);
      return EXIT_FAILURE;
    }
    else
    {
      waitpid(-1, &status, 0);
      return status;
    }
  }
  else if (pid < 0)
  {
    printf("Cannot fork");
    return EXIT_FAILURE;
  }
}

// Detection des symboles > et &
int configure_cmd(char cmd_type[], char *args[], char *options[], int buff)
{
  int status;
  int back_process = 0;
  int redirect = 0;
  char *out_file;
  char *ex[2*sizeof(buff)] = {0};
  ex[0] = cmd_type;
  int i = 0;
  int j = 0;

  // On combine args et options dans ex
  while (options[i] != NULL)
  {
    ex[i+1] = options[i];
    i++;
  }
  while (args[j] != NULL)
  {
    // Si le dernier arg est &, flag allume
    if (*args[j] == '&')
    {
      back_process = 1;
      j++;
    }
    // Si le dernier arg est >, flag allume
    else if (*args[j] == '>')
    {
      redirect = 1;
      // le fichier de sortie viens apres >
      out_file = args[j+1];
      j = j + 2;
    }
    else
    {
      ex[i+1] = args[j++];
      i++;
    }
    
  }
  status = exec_cmd(ex, back_process, redirect, out_file);
}

// Separer un string par les guillemets.
char * parse_quotes(char *input_copy)
{
  char quote[2] = "\"";
  char *quotes = strtok(input_copy, quote);

  if (quotes != NULL)
    quotes = strtok(NULL, quote);
  // Exemple: un deux "trois quatre" cinq
  // quotes = trois quatre
  return quotes;
}

// Separer le input en arguments et options
int parse_cmd(char *input)
{
  int status;
  char *cmd;
  char *quotes;
  char space[2] = " ";
  int buff = strlen(input)*sizeof(char);
  char input_copy[buff];
  char *args[sizeof(input)] = {0};
  char *options[sizeof(input)] = {0};
  char *cmd_type;
  char *left_quote;
  char *right_quote;
  int string_flag = 0;
  int i = 0;
  int j = 0;

  // On passe une copie de input pour trouver les guillemets
  strcpy(input_copy, input);
  quotes = parse_quotes(input_copy);

  if (input != NULL)
  {
    // Separer l'input sur les espaces
    cmd = strtok(input, space);
    cmd_type = cmd;
    cmd = strtok (NULL, space);
    
    while (cmd != NULL)
    {
      // des flags si on retrouve des guillemets dans le string
      left_quote = strrchr(cmd, '\"');
      right_quote = strrchr(cmd+1, '\"');
      // Si on trouve le guillemet ouvrant
      if (left_quote != NULL && string_flag == 0)
      {
        string_flag = 1;
      }
      // Si ou trouve un caratere - sur un mot, ex: -e
      if (cmd[0] == '-' && strcmp(cmd, "-") != 0)
      {
        options[i++] = cmd;
      }
      // Si on lit pas un string, mais des arguments
      else if (string_flag == 0)
      {
        args[j++] = cmd;
      }
      // Si on trouve le guillemet fermant
      if (right_quote != NULL && string_flag == 1)
      {
        string_flag = 0;
        args[j++] = quotes;
      }
      // avance au prochain mot dans l'input
      cmd = strtok(NULL, space);
    }

    if (strcmp(cmd_type, "echo") == 0 || strcmp(cmd_type, "ls") == 0 || 
        strcmp(cmd_type, "man") == 0  || strcmp(cmd_type, "cat") == 0 ||
        strcmp(cmd_type, "tail") == 0 || strcmp(cmd_type, "sleep") == 0 ||
        strcmp(cmd_type, "grep") == 0)
    {
      status = configure_cmd(cmd_type, args, options, buff);
      return status;
    }
    // true retourne 0 par defaut car 0 == status de termination true
    else if (strcmp(cmd_type, "true") == 0)
    {
      status = 0;
      return status;
    }
    // false retourne 1 car status de termination > 0 implique q'il echoue
    else if (strcmp(cmd_type, "false") == 0)
    {
      status = 1;
      return status;
    }
    // commande pour quitter
    else if (strcmp (cmd_type, "quit") == 0)
    {
      free(input);
      fprintf (stdout, "Bye!\n");
      exit (0);
    }
    // La commande n'est pas implemente
    else
    {
      fprintf (stdout, "Invalid command.\n");
      return EXIT_FAILURE;
    }
  }
}

// Parsing de && et ||, fait de maniere recursive
int parse_logic(char *input, int len)
{
  int status;
  char left_buffer[len];
  char right_buffer[len];
  char *and = strstr(input, " && ");
  char *or = strstr(input, " || ");

  // Si il existe un instance de && ou || dans l'input
  if (and != NULL && or != NULL)
  {
    // Si le && est premier, separer sur cette operateur la partie gauche et droite
    if (and - input < or - input)
    {
      memcpy(left_buffer, &input[0], (and - input));
      left_buffer[and - input] = '\0';
      strcpy(right_buffer, &input[and - input + 4]);
      status = parse_cmd(left_buffer);
      // Si la commande de gauche est true
      if (status == 0)
      {
        // execute la droite
        status = parse_if(right_buffer, len);
        return status;
      }
    }
    else
    {
      // operateur || premier. Seperation: gauche || droite
      memcpy(left_buffer, &input[0], (or - input));
      left_buffer[or - input] = '\0';
      strcpy(right_buffer, &input[or - input + 4]);
      status = parse_cmd(left_buffer);
      // Si gauche est false
      if (status > 0)
      {
        // execute la droite
        status = parse_if(right_buffer, len);
        return status;
      }
    }
  }
  // Si on trouve && dans l'input, separation: commande gauche && commande droite
  else if (and != NULL)
  {
    memcpy(left_buffer, &input[0], (and - input));
    left_buffer[and - input] = '\0';
    strcpy(right_buffer, &input[and - input + 4]);
    status = parse_cmd(left_buffer);
    // Si la commande de gauche est true
    if (status == 0)
    {
      // execute la droite
      status = parse_if(right_buffer, len);
      return status;
    }
  }
  // Si on a || dans la commande
  else if (or != NULL)
  {
    // separe gauche || droite
    memcpy(left_buffer, &input[0], (or - input));
    left_buffer[or - input] = '\0';
    strcpy(right_buffer, &input[or - input + 4]);
    status = parse_cmd(left_buffer);
    // Si gauche est false
    if (status > 0)
    {
      // execute droite
      status = parse_if(right_buffer, len);
      return status;
    }
  }
  // Aucun operateur logique
  else
  {
    status = parse_cmd(input);
    return status;
  }
}

// Parsing de if ... done et if ... done && OU if ... done ||. 
// Les deux derniere de maniere recursive
int parse_if(char *input, int len)
{
  int status;
  char condition_buffer[len];
  char clause_buffer[len];
  char left_buffer[len];
  char right_buffer[len];
  char *condition = strstr(input, "if ");
  char *clause = strstr(input, " ; do ");
  char *done = strrchr(input, 'd');

  // input est un seul if statement
  if (condition != NULL && clause != NULL && done != NULL && strlen(input) == done-input+4 && 
      condition-input == 0)
  {
    // Executer la condition
    memcpy(condition_buffer, &input[3], clause - input - 3);
    condition_buffer[clause - input - 3] = '\0';
    status = parse_logic(condition_buffer, len);
    // Si la condition retourne true (status de execvp() retourne 0) on execute la clause
    if (status == 0)
    {
      memcpy(clause_buffer, &clause[6], done - clause - 9);
      clause_buffer[done - clause - 9] = '\0';
      status = parse_if(clause_buffer, len);
      return status;
    }
  }
  // input est un if statement avec && ou || a la fin
  else if (condition != NULL && clause != NULL && done != NULL && strlen(input) > done-input+4)
  {
    if (input[done-input+5] == '&' && input[done-input+6] == '&')
    {
      memcpy(left_buffer, &input[0], done - input + 4);
      left_buffer[done - input + 4] = '\0';
      status = parse_if(left_buffer, len);
      // Si le IF a gauche retourne true (status de execvp() retourne 0)
      // execute la prochaine commande apres l'operateur logique
      if (status == 0)
      {
        strcpy(right_buffer, &input[done - input + 8]);
        status = parse_if(right_buffer, len);
      }
    }
    else if (input[done-input+5] == '|' && input[done-input+6] == '|')
    {
      memcpy(left_buffer, &input[0], done - input + 4);
      left_buffer[done - input + 4] = '\0';
      status = parse_if(left_buffer, len);
      // Si le IF a gauche retourne false (status de execvp() retourne > 0)
      // execute la commande apres l'operateur logique
      if (status > 0)
      {
        strcpy(right_buffer, &input[done - input + 8]);
        status = parse_if(right_buffer, len);
      }
    }
  }
  else
  {
    status = parse_logic(input, len);
    return status;
  }
}

int main (void)
{
  char *input;
  int status;
  int len;
  fprintf (stdout, "%% Entered CH shell. To exit use the command 'quit'.\n");

  // Boucle principale qui prend l'entree pour chaque commande
  while (1)
  {
    fprintf (stdout, "CH> ");
    input = get_line();
    input = strtok(input, "\n");
    if (input != NULL)
    {
      len = strlen(input);
      status = parse_if(input, len);
    }
    // Collection de child processus execute an arriere-plan
    waitpid(0, &status, WNOHANG);
    free(input);
  }
}