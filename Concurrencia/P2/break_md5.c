#include <sys/types.h>
#include <openssl/md5.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>
#define PASS_LEN 6
#define THREADS_NUM 100

struct data {
    char **md5;
    int tries;
    int hashes_num;
    pthread_mutex_t *mutex_tries;
    pthread_mutex_t *mutex_hash;
};

struct args {
    int id;
    struct data *data;
};

struct thread_info {
    pthread_t    id;    // id returned by pthread_create()
    struct args *args;  // pointer to the arguments
};

bool stop = false; // Condición para que pare de bascar o password e que pare a barra

//NO SE TOCA
long ipow(long base, int exp)
{
    long res = 1;
    for (;;)
    {
        if (exp & 1)
            res *= base;
        exp >>= 1;
        if (!exp)
            break;
        base *= base;
    }
    return res;
}

//NO SE TOCA
long pass_to_long(char *str) {
    long res = 0;
    for(int i=0; i < PASS_LEN; i++)
        res = res * 26 + str[i]-'a';
    return res;
};

//NO SE TOCA
void long_to_pass(long n, unsigned char *str) {  // str should have size PASS_SIZE+1
    for(int i=PASS_LEN-1; i >= 0; i--) {
        str[i] = n % 26 + 'a';
        n /= 26;
    }
    str[PASS_LEN] = '\0';
}

int hex_value(char c) {
    if (c>='0' && c <='9')
        return c - '0';
    else if (c>= 'A' && c <='F')
        return c-'A'+10;
    else if (c>= 'a' && c <='f')
        return c-'a'+10;
    else return 0;
}

void hex_to_num(char *str, unsigned char *hex) {
    for(int i=0; i < MD5_DIGEST_LENGTH; i++)
        hex[i] = (hex_value(str[i*2]) << 4) + hex_value(str[i*2 + 1]);
}

void *break_pass(void *ptr) {
    struct args *args = ptr;
    unsigned char res[MD5_DIGEST_LENGTH];
    unsigned char *pass = malloc((PASS_LEN + 1) * sizeof(char));
    unsigned char md5_num[MD5_DIGEST_LENGTH];
    char *aux;
    long bound = ipow(26, PASS_LEN); // we have passwords of PASS_LEN
    // lowercase chars =>
    //     26 ^ PASS_LEN  different cases
    int local_tries = 0;

    for(long i=args->id; i < bound && !stop; i+=THREADS_NUM) {
        pthread_mutex_lock(args->data->mutex_hash);
        if (args->data->hashes_num>0) {
            pthread_mutex_unlock(args->data->mutex_hash);
            local_tries++;
            if (local_tries == 1000) {     // Deste xeito temos menos contención no mutex e actualizamos menos frecuentemente

                pthread_mutex_lock(args->data->mutex_tries);
                args->data->tries += local_tries;
                pthread_mutex_unlock(args->data->mutex_tries);
                local_tries = 0;
            }

            long_to_pass(i, pass);
            MD5(pass, PASS_LEN, res);
            pthread_mutex_lock(args->data->mutex_hash);

            for (int j = 0; j < args->data->hashes_num; j++) {  // Comproba todos os hashes que quedan

                hex_to_num(args->data->md5[j], md5_num);
                if (0 == memcmp(res, md5_num, MD5_DIGEST_LENGTH)) { // Comprobacion de igualdade
                    printf("\nFOUND!  ->  %s: %s\n\n", args->data->md5[j], pass);

                    for (int a = j; a < (args->data->hashes_num - 1); a++) {    // Borramos o hash que foi atopado da lista de hash que quedan
                        aux = args->data->md5[a];
                        args->data->md5[a] = args->data->md5[a + 1];
                        args->data->md5[a + 1] = aux;
                    }

                    args->data->hashes_num--;
                    if (args->data->hashes_num == 0)
                        stop = true;


                    break; // Found it!
                }
            }
            pthread_mutex_unlock(args->data->mutex_hash);

        }else{
            pthread_mutex_unlock(args->data->mutex_hash);
            break;
        }
    }
    free(pass);
    return NULL;
}

void *progress_bar(void *ptr) {
    struct args *args = ptr;
    int i = 0;
    char bar[51];
    double prctg;
    long bound = ipow(26, PASS_LEN);
    int iters;
    memset(bar, 0, sizeof(bar));

    while(!stop){
        pthread_mutex_lock(args->data->mutex_hash);
        if (args->data->hashes_num == 0) {
            stop = true;
        }
        pthread_mutex_unlock(args->data->mutex_hash);

        pthread_mutex_lock(args->data->mutex_tries);
        prctg = 100 * ((double) args->data->tries / (double) bound);
        iters = args->data->tries;
        pthread_mutex_unlock(args->data->mutex_tries);
        usleep(100000);
        iters = args->data->tries-iters;

        if (!stop){
            printf("[%-50s] [%.2f%%][%dpsw/s]\r", bar, (prctg), iters*10);
            fflush(stdout);
            while((int)prctg / 2 >= i && prctg >= 2)
                bar[i++] = '#';
        }
    }
    return NULL;
}
struct thread_info *start_threads(struct data *data)
{
    struct thread_info *threads;
    threads = malloc(sizeof(struct thread_info)*(THREADS_NUM + 1));
    if (threads == NULL) {
        printf("Not enough memory\n");
        exit(1);
    }

    for (int i = 0; i < THREADS_NUM + 1; i++) {

        // Create num_thread threads running swap()
        threads[i].args = malloc(sizeof(struct args));
        threads[i].args->id = i;
        threads[i].args->data = data;

        if (i < 1) {
            if (0 != pthread_create(&threads[i].id, NULL, progress_bar, threads[i].args)) {
                printf("Could not create thread #%d", i);
                exit(1);
            }
        } else {

            if (0 != pthread_create(&threads[i].id, NULL, break_pass, threads[i].args)) {
                printf("Could not create thread #%d", i);
                exit(1);
            }
        }
    }
    return threads;
}

void waitt(struct thread_info *threads, struct data *data) {
    printf("hola");
    int loops = THREADS_NUM + 1;

    // Wait for the threads to finish
    for (int i = 0; i < (loops); i++) {
        pthread_join(threads[i].id, NULL);
    }

    for (int i = 0; i < (loops); i++)
        free(threads[i].args);

    pthread_mutex_destroy(data->mutex_tries);
    pthread_mutex_destroy(data->mutex_hash);

    free(data->mutex_tries);
    free(data->mutex_hash);
    free(data->md5);
    free(threads);
}
void init_data(struct data *data, int argc, char *argv[]) {
    data->md5 = malloc(sizeof(char*)*(argc-1)); // Reservamos memoria para todos os hashes
    data->mutex_tries = malloc(sizeof(pthread_mutex_t));
    data->mutex_hash = malloc(sizeof(pthread_mutex_t));
    if (data->mutex_tries == NULL || data->md5 == NULL || data->mutex_hash == NULL){
        printf("Could not create Thread\n");
        exit(1);
    }
    pthread_mutex_init(data->mutex_tries, NULL);
    pthread_mutex_init(data->mutex_hash, NULL);
    data->tries = 0;
    data->hashes_num = argc - 1; // Num de hashes

    for (int i = 1; i < argc ; i++) {    // Inicializamos cada md5
        data->md5[i-1] = argv[i];
    }
}

int main(int argc, char *argv[]) {
    struct thread_info *thrs;
    struct data data;
    if(argc < 2) {
        printf("Use: %s string\n", argv[0]);
        exit(0);
    }

    init_data(&data,argc,argv);

    thrs = start_threads(&data);
    waitt(thrs, &data);
    return 0;
}