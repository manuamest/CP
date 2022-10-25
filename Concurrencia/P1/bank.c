#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include "options.h"
#include <stdbool.h>

#define MAX_AMOUNT 20


bool printtransfers = true;

struct bank {
    int num_accounts;        // number of accounts
    int *accounts;           // balance array
    pthread_mutex_t *mutex;  // mutex for accounts
};

struct cont{
    int iters;
    pthread_mutex_t *mutex;
};

struct args {
    int          thread_num;  // application defined thread #
    int          delay;       // delay between operations
    int          iterations;  // number of operations
    int          net_total;   // total amount deposited by this thread
    struct bank *bank;        // pointer to the bank (shared with other threads)
    struct cont *cont;
};

struct thread_info {
    pthread_t    id;    // id returned by pthread_create()
    struct args *args;  // pointer to the arguments
};

// Threads run on this function
void *deposit(void *ptr)
{

    struct args *args =  ptr;
    int amount, account, balance;
    int i = args->iterations;

    while(i!=0) {
        pthread_mutex_lock(args->cont->mutex);
        args->cont->iters++;

        amount  = rand() % MAX_AMOUNT;
        while (amount == 0)
            amount  = rand() % MAX_AMOUNT;
        account = rand() % args->bank->num_accounts;
        pthread_mutex_lock(&args->bank->mutex[account]);
        if (args->iterations < args->cont->iters){         // unha vez feitas todas as iteracións forzamos a saida dos threads que quedan soltos
            pthread_mutex_unlock(&args->bank->mutex[account]);
            pthread_mutex_unlock(args->cont->mutex);
            break;
        }
        printf("Thread %d depositing %d on account %d\n",
               args->thread_num, amount, account);

        balance = args->bank->accounts[account];

        if(args->delay) usleep(args->delay); // Force a context switch



        balance += amount;

        if(args->delay) usleep(args->delay);

        args->bank->accounts[account] = balance;
        if(args->delay) usleep(args->delay);

        args->net_total += amount;
        if (args->iterations == args->cont->iters){
            pthread_mutex_unlock(&args->bank->mutex[account]);
            pthread_mutex_unlock(args->cont->mutex);
            break;
        }
        pthread_mutex_unlock(args->cont->mutex);
        pthread_mutex_unlock(&args->bank->mutex[account]);
        i--;
    }
    return NULL;
}

void *transfer(void *ptr){

    struct args *args =  ptr;
    int amount, account1, account2;
    int i = args->iterations;
    while(i!=0) {
        account1 = rand() % args->bank->num_accounts;     // QUEN ENVÍA
        while (args->bank->accounts[account1] == 0)       // se a conta non ten saldo non pode facer transferencia
            account1 = rand() % args->bank->num_accounts;
        account2 = rand() % args->bank->num_accounts;      // QUEN RECIBE
        while(account1==account2){                        // a conta que recibe non pode ser a mesma que a que envía
            account2 = rand() % args->bank->num_accounts;
        }
        pthread_mutex_lock(&args->bank->mutex[account1]);
        if(args->delay) usleep(args->delay);
        if (pthread_mutex_trylock(&args->bank->mutex[account2])){ // TRYLOCK
            pthread_mutex_unlock(&args->bank->mutex[account1]);                     //Impedimos que el while baje
            continue;
        }
        if (args->iterations <= args->cont->iters) {
            pthread_mutex_unlock(&args->bank->mutex[account1]);// unlock
            pthread_mutex_unlock(&args->bank->mutex[account2]);
            break;
        }
        amount = rand() % args->bank->accounts[account1]; // máximo da conta á que esteamos accedendo

        if(args->delay) usleep(args->delay);
        printf("Account %d transfering %d to account %d\n",
               account1, amount, account2);

        args->bank->accounts[account2] += amount; // sumamos o que recibe
        args->bank->accounts[account1] -= amount; // restamos o que da
        if(args->delay) usleep(args->delay);

        pthread_mutex_unlock(&args->bank->mutex[account1]);// unlock
        pthread_mutex_unlock(&args->bank->mutex[account2]);
        i--;
    }
    return NULL;

}

void *printTransfer(void *ptr){
    struct args *args = ptr;
    int bank_total = 0;
    for (int i = 0; i < args->bank->num_accounts; i++) {
        bank_total += args->bank->accounts[i];
    }
    while (printtransfers) {

        printf("\nAccount balance\n");
        for (int i = 0; i < args->bank->num_accounts; i++) {
            printf("%d: %d\n", i, args->bank->accounts[i]);
        }
        printf("Total: %d\n", bank_total);
        if (args->delay) usleep(args->delay);
    }
    return NULL;
}

// start opt.num_threads threads running on deposit.
struct thread_info *start_threads(struct options opt, struct bank *bank, void *func, struct cont *cont)
{
    int i;
    struct thread_info *threads;
    printf("creating %d threads\n", opt.num_threads);
    threads = malloc(sizeof(struct thread_info) * (opt.num_threads));
    if (func == transfer){
        threads = malloc(sizeof(struct thread_info) * (opt.num_threads+1));
    }
    if (threads == NULL) {
        printf("Not enough memory\n");
        exit(1);
    }
    // Create num_thread threads running swap()
    for (i = 0; i < opt.num_threads; i++) {
        threads[i].args = malloc(sizeof(struct args));

        threads[i].args -> thread_num = i;
        threads[i].args -> net_total  = 0;
        threads[i].args -> bank       = bank;
        threads[i].args -> cont       = cont;
        threads[i].args -> delay      = opt.delay;
        threads[i].args -> iterations = opt.iterations;
        if (0 != pthread_create(&threads[i].id, NULL, func, threads[i].args)) {          // ESTRUCTURA PARA AS TRANSFERENCIAS(?)
            printf("Could not create thread #%d", i);
            exit(1);
        }

        cont->iters = 0;

    }
    //Tread para printear cuentas
    if (func == transfer){
        i++;
        threads[i].args = malloc(sizeof(struct args));
        threads[i].args -> thread_num = i;
        threads[i].args -> bank       = bank;
        threads[i].args -> delay      = opt.delay;
        pthread_create(&threads[i].id, NULL, printTransfer, threads[i].args);
    }
    return threads;
}

// Print the final balances of accounts and threads
void print_balances(struct bank *bank, struct thread_info *thrs, int num_threads) {
    int total_deposits=0, bank_total=0;
    printf("\nNet deposits by thread\n");
    for(int i=0; i < num_threads; i++) {
        printf("%d: %d\n", i, thrs[i].args->net_total);
        total_deposits += thrs[i].args->net_total;
    }
    printf("Total: %d\n", total_deposits);
    printf("\nAccount balance\n");
    for(int i=0; i < bank->num_accounts; i++) {
        printf("%d: %d\n", i, bank->accounts[i]);
        bank_total += bank->accounts[i];
    }
    printf("Total: %d\n", bank_total);
}

// wait for all threads to finish, print totals, and free memory
void waitt(struct options opt, struct bank *bank, struct cont *cont, struct thread_info *threads, char fun) {
    int loops = opt.num_threads;
    if(fun == 'T')
        loops = opt.num_threads+1;
    // Wait for the threads to finish
    for (int i = 0; i < (loops); i++) {
        pthread_join(threads[i].id, NULL);
    }
    if (fun == 'D'){
        print_balances(bank, threads, opt.num_threads);
    }
    for (int i = 0; i < (loops); i++)
        free(threads[i].args);
    free(threads);
    for (int j = 0; j < bank->num_accounts; ++j) {
        pthread_mutex_destroy(&bank->mutex[bank->num_accounts]);
    }
    if (fun == 'T'){
        printtransfers = false;
    }
}

// allocate memory, and set all accounts to 0
void init_accounts(struct bank *bank, struct cont *cont, int num_accounts) {
    bank->num_accounts = num_accounts;
    bank->accounts     = malloc(bank->num_accounts * sizeof(int));
    bank->mutex = malloc(bank->num_accounts * sizeof(pthread_mutex_t));     // RESERVAMOS MEMORIA PARA O MUTEX

    for(int i=0; i < bank->num_accounts; i++) {
        pthread_mutex_init(&bank->mutex[i], NULL);
        bank->accounts[i] = 0;
    }

    cont->mutex = malloc(bank->num_accounts*sizeof(pthread_mutex_t));
}

int main (int argc, char **argv)
{
    struct options      opt;
    struct bank         bank;
    struct cont     cont;
    struct thread_info *thrs1;
    struct thread_info *thrs2;

    srand(time(NULL));

    // Default values for the options
    opt.num_threads  = 5;
    opt.num_accounts = 10;
    opt.iterations   = 100;
    opt.delay        = 10;

    read_options(argc, argv, &opt);

    init_accounts(&bank, &cont, opt.num_accounts);
    
    printf("\n---------- DEPOSITS ----------\n");
    thrs1 = start_threads(opt, &bank,deposit, &cont);   // METINLLE DEPOSIT COMO A OPERACIÓN QUE TEN QUE FACER
    waitt(opt, &bank, &cont, thrs1, 'D');
    
    printf("\n---------- TRANSFERS ----------\n");
    thrs2 = start_threads(opt, &bank,transfer, &cont);// FACER O MESMO PARA TRANSFER
    waitt(opt, &bank, &cont, thrs2, 'T');
    printtransfers = false;

    free(bank.accounts);
    free(bank.mutex);
    return 0;
}
