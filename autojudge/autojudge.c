#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>
#include <dirent.h>
#include <signal.h>

#define BUFFER_SIZE 1024

typedef struct {
    char inputDir[256];
    char answerDir[256];
    char executable[256];
    int timeLimit;
    int totalTests;
    int passedTests;
    int timeouts;
    int compileErrors;
    int runtimeErrors;
} JudgeContext;

void parse_command_line(int argc, char** argv, JudgeContext* context);
int compile_program(const char* sourceFile);
void execute_tests(JudgeContext* context);
void run_test_case(JudgeContext* context, const char* inputFile);
int compare_output_with_answer(const char* outputFile, const char* answerFile);
void report_results(const JudgeContext* context);
void timeout_handler(int sig);

int main(int argc, char** argv) {
    JudgeContext context;
    memset(&context, 0, sizeof(JudgeContext));
    parse_command_line(argc, argv, &context);

    if (compile_program(context.executable) != 0) {
        fprintf(stderr, "Compile error.\n");
        context.compileErrors++;
        report_results(&context);
        return EXIT_FAILURE;
    }

    execute_tests(&context);
    report_results(&context);
    return EXIT_SUCCESS;
}

void parse_command_line(int argc, char** argv, JudgeContext* context) {
    int opt;
    while ((opt = getopt(argc, argv, "i:a:t:")) != -1) {
        switch (opt) {
        case 'i':
            strncpy(context->inputDir, optarg, sizeof(context->inputDir) - 1);
            break;
        case 'a':
            strncpy(context->answerDir, optarg, sizeof(context->answerDir) - 1);
            break;
        case 't':
            context->timeLimit = atoi(optarg);
            break;
        default:
            fprintf(stderr, "Usage: %s -i <inputDir> -a <answerDir> -t <timeLimit> <sourceFile>\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }
    if (optind < argc) {
        strncpy(context->executable, argv[optind], sizeof(context->executable) - 1);
    }
    else {
        fprintf(stderr, "Executable source file must be specified.\n");
        exit(EXIT_FAILURE);
    }
}

int compile_program(const char* sourceFile) {
    char command[512];
    snprintf(command, sizeof(command), "gcc -fsanitize=address -o program.out %s", sourceFile);
    return system(command); 
}

void execute_tests(JudgeContext* context) {
    DIR* dir = opendir(context->inputDir);
    struct dirent* entry;
    if (dir == NULL) {
        perror("Failed to open input directory");
        exit(EXIT_FAILURE);
    }
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG) {
            char inputFilePath[512];
            snprintf(inputFilePath, sizeof(inputFilePath), "%s/%s", context->inputDir, entry->d_name);
            run_test_case(context, inputFilePath);
            context->totalTests++;
        }
    }
    closedir(dir);
}

void run_test_case(JudgeContext* context, const char* inputFile) {
    printf("Testing: %s\n", inputFile);

    pid_t pid = fork();
    if (pid == 0) {
        
        signal(SIGALRM, timeout_handler);
        alarm(context->timeLimit);

        
        int inputFd = open(inputFile, O_RDONLY);
        dup2(inputFd, STDIN_FILENO);
        close(inputFd);

        int outputFd = open("temp_output.txt", O_WRONLY | O_CREAT | O_TRUNC, 0666);
        dup2(outputFd, STDOUT_FILENO);
        close(outputFd);

        
        execl("./program.out", "program.out", NULL);
        perror("Execution failed");
        exit(EXIT_FAILURE);
    }
    else if (pid > 0) {
        int status;
        wait(&status);
        if (WIFSIGNALED(status)) {
            int termSig = WTERMSIG(status);
            if (termSig == SIGALRM) {
                printf("Result for %s: Timeout - Execution took longer than %d seconds\n", inputFile, context->timeLimit);
                context->timeouts++;
            }
            else if (termSig == SIGSEGV) {
                printf("Result for %s: Segmentation Fault\n", inputFile);
            }
            else {
                printf("Result for %s: Terminated by signal %d\n", inputFile, termSig);
            }
        }
        else if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
            char answerFile[512], baseName[256];
            snprintf(baseName, sizeof(baseName), "%s", strrchr(inputFile, '/') + 1);
            snprintf(answerFile, sizeof(answerFile), "%s/%s", context->answerDir, baseName);
            if (compare_output_with_answer("temp_output.txt", answerFile) == 0) {
                printf("Result for %s: Passed\n", inputFile);
                context->passedTests++;
            }
            else {
                printf("Result for %s: Wrong Answer - Output does not match the expected output\n", inputFile);
                context->runtimeErrors++;
            }
        }
        else {
            printf("Result for %s: Runtime Error or Non-zero Exit Status\n", inputFile);
            context->runtimeErrors++;
        }
    }
    else {
        perror("Failed to fork");
        exit(EXIT_FAILURE);
    }
}

int compare_output_with_answer(const char* outputFile, const char* answerFile) {
    FILE* fpOut = fopen(outputFile, "r");
    FILE* fpAns = fopen(answerFile, "r");
    if (!fpOut || !fpAns) {
        if (fpOut) fclose(fpOut);
        if (fpAns) fclose(fpAns);
        return -1;
    }

    char outBuf[BUFFER_SIZE], ansBuf[BUFFER_SIZE];
    int result = 0;

    while (fgets(outBuf, sizeof(outBuf), fpOut) && fgets(ansBuf, sizeof(ansBuf), fpAns)) {
        if (strcmp(outBuf, ansBuf) != 0) {
            result = 1;
            break;
        }
    }

    if (result == 0) {
        if (fgets(outBuf, sizeof(outBuf), fpOut) || fgets(ansBuf, sizeof(ansBuf), fpAns)) {
            result = 1;
        }
    }

    fclose(fpOut);
    fclose(fpAns);
    return result;
}

void report_results(const JudgeContext* context) {
    printf("\nFinal Report:\n");
    printf("Total tests run: %d\n", context->totalTests);
    printf("Passed tests: %d\n", context->passedTests);
    printf("Timeouts: %d\n", context->timeouts);
    printf("Compile errors: %d\n", context->compileErrors);
    printf("Runtime errors: %d\n", context->runtimeErrors);
}

void timeout_handler(int sig) {
    fprintf(stderr, "Test case terminated due to timeout\n");
    exit(EXIT_FAILURE);
}
