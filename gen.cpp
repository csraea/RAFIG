#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <ctype.h>
#include <errno.h>
#include <stdint.h>
#include <sys/stat.h>

#define KB 1024

struct Options {
    char infile[128] = "null";
    long long size = KB;
    size_t count = 1;
} opts;

void PrintHelpMessage(char* msg){
    if(msg != NULL) fprintf(stderr, "Error: %s, errno: %d\n", msg, errno);
    else {
        printf("\033[1m" "--- RAFIG ---\n" "\033[22m");
        printf("RAndom FIle Generator: a tool to generate random file(s) of specified size.\n");
        printf("\033[1m" "Usage:" "\033[22m" " [OPTION]...\n"); 
        printf("\033[1m" " -i. --infile:""\033[22m" "    a file to get contents seeking from random position.\n");
        printf("\033[1m" " -s, --size:""\033[22m" "      a size of an output file, 1KB by default.\n");
        printf("\033[1m" " -c, --count:""\033[22m" "     number of output files to generate.\n");
    }
}

void ParseOptions(int argc, char** argv) {
    // extern int optind;
    extern char* optarg;
    char c = 0;

    while (1)
    {        
        int option_index = 0;
        
        static struct option long_options[] = {
            { "infile",    required_argument, 0, 'i' },
            { "size",      required_argument, 0, 's' }, 
            { "count",     required_argument, 0, 'c' },
            { "help",      no_argument,       0, 'h' },
            { 0,             0,               0,  0  },
        };
        c = getopt_long(argc, argv, "i:s:c:h", long_options, &option_index);
        if (c == -1)
            break; 
        
        switch (c)
        {
            case 'i':
                strcpy(opts.infile, optarg);
                break;
            case 's':
            {
                char *tmpptr = optarg;
                if((opts.size = strtoll(optarg, &tmpptr, 10)) <= 0 || tmpptr != optarg){
                    if((strstr(optarg, "KB") != NULL || strstr(optarg, "MB") != NULL) && isdigit(*optarg)){
                        char number[64] = { 0 };
                        for(uint8_t i = 0; i < strlen(optarg) && isdigit(optarg[i]); i++)
                            number[i] = optarg[i];
                        if(strstr(optarg, "KB") != NULL)
                            opts.size = atoll(number) * KB;
                        else
                            opts.size = atoll(number) * KB * KB;
                    } else {
                        PrintHelpMessage((char *)"invalid size specified");
                        exit(EXIT_FAILURE);
                    }
                }
            } break;
            case 'c':
                if((opts.count = atoi(optarg)) <= 0){
                    PrintHelpMessage((char *)"invalid number of files to generate is specified");
                    exit(EXIT_FAILURE);
                }
                break;
            case '?': case 'h':
            default:
                PrintHelpMessage(NULL);
                exit(EXIT_FAILURE);
        }
    }
};

ssize_t Copy(int fd1, int fd2, off_t fileOffset1, off_t fileOffset2, long long amount) {
    char buf[BUFSIZ];
    memset(buf, 0, sizeof(buf));
    ssize_t ret = 0;

    while ((ret = pread (fd1, buf, (amount > BUFSIZ) ? BUFSIZ : amount, fileOffset1)) != 0 && (amount - ret >= 0)) {
        if (ret == -1) {
            if (errno == EINTR) continue; // handling some frequent interruptions
            return -1;
        }
        fileOffset1 += ret;
        if((ret = pwrite(fd2, buf, ret, fileOffset2)) != 0){
            if (ret == -1) {
                if (errno == EINTR) continue; // handling some frequent interruptions
                return -1;
            }
        }
        fileOffset2 += ret;
        amount -= (int) ret;
    }

    return ret;
}

void Magic(){

    srand(time(NULL));

    struct stat buf = {0};

    if(stat("./out", &buf) == -1) {
        mkdir("./out", 0777);
    }
    
    if(strcmp(opts.infile, "null") != 0){
        int fd1 = open(opts.infile, O_RDONLY);

        if(stat(opts.infile, &buf) == -1){
            PrintHelpMessage((char *)"stat() exited with failure");
            exit(EXIT_FAILURE);
        }

        if(buf.st_size < opts.size){
            PrintHelpMessage((char *)"infile size is smaller than size of a file to generate");
            exit(EXIT_FAILURE);
        }

        for(size_t i = 0; i < opts.count; i++){

            off64_t r = (off64_t) rand() % (buf.st_size - opts.size);
            
            char filename[32] = { 0 };
            sprintf(filename, "./out/rand%zu.bin", i+1);
            int fd2 = open(filename, O_CREAT | O_TRUNC | O_WRONLY, 0777);

            ssize_t res = Copy(fd1, fd2, r, 0, opts.size);
            if(res == -1) {
                PrintHelpMessage((char *)"file read/write exited with failure");
                close(fd1);
                close(fd2);
                exit(EXIT_FAILURE);
            }

            close(fd2);
        }

        close(fd1);

    } else {

        for(size_t i = 0; i < opts.count; i++){

            char filename[32] = { 0 };
            sprintf(filename, "./out/rand%zu.bin", i+1);
            int fd2 = open(filename, O_CREAT | O_WRONLY | O_TRUNC | O_APPEND, 0777);

            for(size_t i = 0; i < opts.size; i++){
                int r = rand() % 255;
                ssize_t res = write(fd2, &r, 1);
                if(res == -1) {
                    PrintHelpMessage((char *)"file write exited with failure");
                    close(fd2);
                    exit(EXIT_FAILURE);
                }
            }

            close(fd2);
        }
    }
}


int main(int argc, char **argv){

    ParseOptions(argc, argv);

    Magic();

    exit(EXIT_SUCCESS);
}
