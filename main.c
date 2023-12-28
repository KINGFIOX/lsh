#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define LSH_RL_BUFSIZE 1024
#define LSH_TOK_BUFSIZE 64
#define LSH_TOK_DELIM " \t\r\n\a"

/**
 * @brief 这个也可以使用getline来代替
 * 
 * @return char* 
 */
char* lsh_read_line(void)
{
    int bufsize = LSH_RL_BUFSIZE;
    int position = 0;
    char* buffer = malloc(sizeof(char) * bufsize);
    int c;

    // 分配失败
    if (!buffer) {
        fprintf(stderr, "lsh: allocation error\n");
        exit(EXIT_FAILURE);
    }

    while (1) {
        // 从标准输入中读取
        c = getchar();

        // 写入buffer
        if (c == EOF || c == '\n') {
            buffer[position] = '\0';
            return buffer;
        } else {
            buffer[position] = c;
        }
        position++;

        // 超出buffer，重新分配
        if (position >= bufsize) {
            bufsize += LSH_RL_BUFSIZE;
            buffer = realloc(buffer, bufsize);
            if (!buffer) {
                fprintf(stderr, "lsh: allocation error\n");
                exit(EXIT_FAILURE);
            }
        }
    }
}

char** lsh_split_line(char* line)
{
    int bufsize = LSH_TOK_BUFSIZE; // tok就是：分开
    int position = 0;
    char** tokens = malloc(bufsize * sizeof(char*));
    char* token;
    if (!tokens) {
        fprintf(stderr, "lsh: allocation error\n");
        exit(EXIT_FAILURE);
    }

    token = strtok(line, LSH_TOK_DELIM); // 一个一个分开
    while (token != NULL) {
        tokens[position] = token;
        position++;

        // 单词数量超过bufsize
        if (position >= bufsize) {
            bufsize += LSH_TOK_BUFSIZE;
            tokens = realloc(tokens, bufsize * sizeof(char*));
            if (!token) {
                fprintf(stderr, "lsh: allocation error\n");
                exit(EXIT_FAILURE);
            }
        }
        // 这个语法，黑马的课有讲，是要NULL
        token = strtok(NULL, LSH_TOK_DELIM);
    }
    // 最后一个位置是NULL
    tokens[position] = NULL;
    return tokens;
}

// 执行 环境变量 中的程序
int lsh_launch(char** args)
{
    pid_t pid, wpid;
    int status;

    pid = fork();

    if (pid == 0) {
        // args[0] 是可执行文件
        // args 是其他参数
        if (execvp(args[0], args) == -1) {
            perror("lsh");
        }
        exit(EXIT_FAILURE);
    } else if (pid < 0) {
        perror("lsh");
    } else {
        do {
            // waitpid的第三个参数是options
            // WUNTRACED --> return  if  a  child  has  stopped
            // 这个wpid没用到
            wpid = waitpid(pid, &status, WUNTRACED);
            // WIF EXITED --> returns true if the child terminated normally
            // WIF SIGNALED --> returns true if the child process was terminated by a signal
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }

    return 1;
}

// bash: which cd --> cd: shell built-in command
char* builtin_str[] = { "cd", "help", "exit" };

int lsh_num_builtins()
{
    return sizeof(builtin_str) / sizeof(char*);
}

int lsh_cd(char** args)
{
    if (args[1] == NULL) {
        fprintf(stderr, "lsh: expected argument to \"cd\"\n");
    } else {
        // chdir() changes the current working directory
        if (chdir(args[1]) != 0) {
            perror("lsh");
        }
    }
    return 1;
}

int lsh_help(char** args)
{
    int i;
    printf("wangfiox's LSH\n");
    printf("type programs names and arguments, and hit enter. \n");
    printf("the following are build in:\n");

    for (i = 0; i < lsh_num_builtins(); i++) {
        printf("  %s\n", builtin_str[i]);
    }

    printf("use the man command for information on other programs.\n");
    return 1;
}

int lsh_exit(char** args)
{
    return 0;
}

int (*builtin_func[])(char**) = { &lsh_cd, &lsh_help, &lsh_exit };

/**
 * @brief 执行程序
 * 
 * @param args 
 * @return int 
 */
int lsh_execute(char** args)
{
    int i;
    if (args[0] == NULL) {
        return 1;
    }
    // 检查是不是内置的 command
    for (i = 0; i < lsh_num_builtins(); i++) {
        if (strcmp(args[0], builtin_str[i]) == 0) {
            return (*builtin_func[i])(args);
        }
    }
    return lsh_launch(args);
}

void lsh_loop(void)
{
    char* line;
    char** args;
    int status;

    do {
        printf("> ");
        line = lsh_read_line(); // 读取
        args = lsh_split_line(line); // 解析
        status = lsh_execute(args); // 执行，返回状态变量 何时退出

        free(line);
        free(args);
    } while (status);
}

int main(int argc, char** argv)
{
    lsh_loop();
    return EXIT_SUCCESS;
}