#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <time.h>
#include <signal.h>

#define MAX_TESTS 20
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

int compile_program(const char* sourceFile);
void run_test_case(JudgeContext* context, const char* inputFile);
int compare_files(const char* file1, const char* file2);
void parse_command_line(int argc, char** argv, JudgeContext* context);
void execute_tests(JudgeContext* context);
void report_results(const JudgeContext* context);

int main(int argc, char** argv) {
    JudgeContext context;
    memset(&context, 0, sizeof(JudgeContext));
    parse_command_line(argc, argv, &context);

    if (compile_program(argv[optind]) != 0) {
        fprintf(stderr, "Compile error occurred.\n");
        context.compileErrors++;
        return EXIT_FAILURE;
    }

    execute_tests(&context);
    report_results(&context);

    return EXIT_SUCCESS;
}

void parse_command_line(int argc, char** argv, JudgeContext* context) {
    int opt;
    char* sourceFile = NULL;

    while ((opt = getopt(argc, argv, "i:a:t:")) != -1) {
        switch (opt) {
        case 'i':
            strcpy(context->inputDir, optarg);
            break;
        case 'a':
            strcpy(context->answerDir, optarg);
            break;
        case 't':
            context->timeLimit = atoi(optarg);
            if (context->timeLimit <= 0) {
                fprintf(stderr, "Invalid time limit.\n");
                exit(EXIT_FAILURE);
            }
            break;
        default:
            fprintf(stderr, "Usage: %s -i <inputdir> -a <answerdir> -t <timelimit> <sourcefile>\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    if (optind < argc) {
        sourceFile = argv[optind];
        snprintf(context->executable, sizeof(context->executable), "%s.out", sourceFile);
    } else {
        fprintf(stderr, "Source file must be specified.\n");
        exit(EXIT_FAILURE);
    }

    struct stat st;
    if (stat(context->inputDir, &st) != 0 || !S_ISDIR(st.st_mode)) {
        fprintf(stderr, "Invalid input directory.\n");
        exit(EXIT_FAILURE);
    }
    if (stat(context->answerDir, &st) != 0 || !S_ISDIR(st.st_mode)) {
        fprintf(stderr, "Invalid answer directory.\n");
        exit(EXIT_FAILURE);
    }
}

int compile_program(const char* sourceFile) {
    pid_t pid = fork();
    if (pid == 0) {
        execlp("gcc", "gcc", "-fsanitize=address", sourceFile, "-o", "program.out", NULL);
        perror("Compilation failed");
        exit(EXIT_FAILURE);
    } else if (pid > 0) {
        int status;
        wait(&status);
        return WEXITSTATUS(status);
    } else {
        perror("fork failed");
        exit(EXIT_FAILURE);
    }
}

void execute_tests(JudgeContext* context) {
    DIR* dirp = opendir(context->inputDir);
    if (!dirp) {
        perror("Failed to open input directory");
        exit(EXIT_FAILURE);
    }

    struct dirent* entry;
    while ((entry = readdir(dirp)) != NULL) {
        if (entry->d_type == DT_REG && strstr(entry->d_name, ".txt") && strncmp(entry->d_name, "output_", 7) != 0) {
            char inputPath[512];
            snprintf(inputPath, sizeof(inputPath), "%s/%s", context->inputDir, entry->d_name);
            context->totalTests++;
            printf("Processing test case: %s\n", inputPath);  // Print each processed file for debugging
            run_test_case(context, inputPath);
        }
    }
    closedir(dirp);
}

void timeout_handler(int sig) {
    fprintf(stderr, "Timeout occurred\n");
    exit(EXIT_FAILURE);  // Terminates the child process
}

void run_test_case(JudgeContext* context, const char* inputFile) {
    printf("Testing file: %s\n", inputFile);

    pid_t pid = fork();
    if (pid == 0) {
        signal(SIGALRM, timeout_handler);
        alarm(context->timeLimit);

        int inputFd = open(inputFile, O_RDONLY);
        if (inputFd < 0) {
            perror("Failed to open input file");
            exit(EXIT_FAILURE);
        }
        dup2(inputFd, STDIN_FILENO);
        close(inputFd);

        int outputFd = open("temp_output.txt", O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if (outputFd < 0) {
            perror("Failed to open output file");
            exit(EXIT_FAILURE);
        }
        dup2(outputFd, STDOUT_FILENO);
        close(outputFd);

        execl("./program.out", "program.out", NULL);
        perror("Execution failed");
        exit(EXIT_FAILURE);
    } else if (pid > 0) {
        int status;
        wait(&status);
        if (WIFSIGNALED(status) && WTERMSIG(status) == SIGALRM) {
            printf("Result for %s: Timeout\n", inputFile);
            context->timeouts++;
        } else if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
            char answerFile[512], baseName[256];
            snprintf(baseName, sizeof(baseName), "%s", strrchr(inputFile, '/') + 1);
            snprintf(answerFile, sizeof(answerFile), "%s/%s", context->answerDir, baseName);
            if (compare_files("temp_output.txt", answerFile) == 0) {
                printf("Result for %s: Passed\n", inputFile);
                context->passedTests++;
            } else {
                printf("Result for %s: Wrong Answer\n", inputFile);
            }
        } else {
            printf("Result for %s: Runtime Error\n", inputFile);
            context->runtimeErrors++;
        }
    } else {
        perror("fork failed");
        exit(EXIT_FAILURE);
    }
}

int compare_files(const char* file1, const char* file2) {
    FILE *fp1 = fopen(file1, "r");
    FILE *fp2 = fopen(file2, "r");
    if (fp1 == NULL || fp2 == NULL) {
        if (fp1) fclose(fp1);
        if (fp2) fclose(fp2);
        return 1; // 파일 열기 실패
    }

    char ch1 = fgetc(fp1);
    char ch2 = fgetc(fp2);

    while ((ch1 != EOF) && (ch2 != EOF) && (ch1 == ch2)) {
        ch1 = fgetc(fp1);
        ch2 = fgetc(fp2);
    }

    fclose(fp1);
    fclose(fp2);

    if (ch1 == ch2)
        return 0; // 파일이 동일
    else if (ch1 == EOF && ch2 == EOF)
        return 0; // 파일이 정확히 동일하게 끝남
    else
        return 1; // 파일이 다름
}

void report_results(const JudgeContext* context) {
    printf("Total Tests: %d\n", context->totalTests);
    printf("Passed: %d\n", context->passedTests);
    printf("Timeouts: %d\n", context->timeouts);
    printf("Compile Errors: %d\n", context->compileErrors);
    printf("Runtime Errors: %d\n", context->runtimeErrors);
}


