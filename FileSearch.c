#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <dirent.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <semaphore.h>


#define THREADS 5
#define FILENAME "XYZ"

struct info {
    pthread_mutex_t* mutex;
    sem_t* sem;
    char dirpath[256];
    int fd;
};

void searchFile(pthread_mutex_t* mutex, char dirpath[256], int fd, sem_t* sem)
{
    int buffsize = 0;
    while(dirpath[buffsize] != '\0')
    {
        buffsize++;
    }
    char path[buffsize];
    strncpy(path, dirpath, buffsize);
    path[buffsize] = '\0';

    struct dirent* entry;
    DIR* dirp = opendir(path);

    if(dirp == NULL)
    {
        perror("opendir");
        pthread_exit(NULL);
    }

    while((entry = readdir(dirp)) != NULL)
    {
        if(strcmp(entry->d_name, FILENAME) == 0)
        {
            int buffsize = sizeof(path) + sizeof(FILENAME) + 1; // +1 for \n
            char buff[buffsize];
            sprintf(buff, "%s/%s\n", path, FILENAME);
            pthread_mutex_lock(mutex);
            write(fd, buff, buffsize);
            pthread_mutex_unlock(mutex);
        }
        int newbuffsize = buffsize + sizeof(entry->d_name);
        char new_path[newbuffsize];
        sprintf(new_path,"%s/%s", path, entry->d_name);
        struct stat stat_struct;
        if(stat(new_path, &stat_struct) != 0)
        {
            perror("stat");
            pthread_exit(NULL);
        }

        if(S_ISDIR(stat_struct.st_mode) && strcmp(".", entry->d_name) != 0 && strcmp("..", entry->d_name))
        {
            searchFile(mutex, new_path, fd, sem);    
        }
    }
    closedir(dirp);
    return;
}
void* pthread_searchFile(void* information)
{
    struct info* info = (struct info*) information;
    sem_post(info->sem);
    searchFile(info->mutex, info->dirpath, info->fd, info->sem);
    pthread_exit(NULL);
}

int main(int argc, char* argv[])
{
    if(argc != THREADS+1)
    {
        perror("argc");
        return -1;
    }
    int fd = open("res", O_CREAT | O_TRUNC | O_WRONLY, 0666);
    if(fd == -1)
    {
        perror("open");
        return -1;
    }

    sem_t sem;
    sem_init(&sem, 0, 0);
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_t thr[THREADS];

    struct info info;
    info.mutex = &mutex;
    info.fd = fd;
    info.sem = &sem;
    for(int i = 0; i < THREADS; i++)
    {
        strcpy(info.dirpath, argv[i+1]);
        pthread_create(&thr[i], NULL, pthread_searchFile, &info);
        sem_wait(info.sem);
    }

    for(int i = 0; i < THREADS; i++)
    {
        pthread_join(thr[i], NULL);
    }
    close(fd);
    printf("Success!\n");

    return 0;
}
