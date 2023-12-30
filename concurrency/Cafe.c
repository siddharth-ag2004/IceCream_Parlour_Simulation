#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

#define MAX_LEN 500

#define YELLOW "\033[1;33m"
#define WHITE "\e[0;37m"
#define CYAN "\033[1;36m"
#define BLUE "\033[1;34m"
#define GREEN "\033[1;32m"
#define RED "\033[1;31m"
#define RESET "\033[0m"


typedef struct Coffee {
    char name[20];
    int ttp; // time to prepare
} Coffee;

typedef struct Customer {
    int index;
    char ctype[20];
    int t_arr;
    int t_tol;
    int is_served;
    int being_served;
    int has_arrived;
    int delay_flag;
    int serving_bar;
} Customer;

typedef struct ThreadArg {
    int barista_index;
    int B;
    int N;
    int K;
    Customer* customers;
    Coffee* coffees;
} ThreadArg;

sem_t barista;
sem_t customer_exists;
// sem_t delay1_bar;
sem_t customer_in[MAX_LEN];
sem_t customer_out[MAX_LEN];
sem_t barista_sleep[MAX_LEN];
sem_t delay1_bar[MAX_LEN];
int barista_arr[MAX_LEN];
int temp_currtime[MAX_LEN];
int barista_busy=0;
int customer_left=0;    
int avg_wtime=0;
pthread_mutex_t print_mutex;
int curr_time = 0;
int coffee_wasted = 0;

int prep_time(int K, const char *ctype, Coffee coffee[K]) {
    for (int i = 0; i < K; i++) {
        if (strcmp(ctype, coffee[i].name) == 0) {
            return coffee[i].ttp;
        }
    }
    return -1;
}

void* customer_func(void* arg)
{
    Customer* customer = (Customer*)arg;
    // int B = 0;
    // int N = 0;
    
    sem_wait(&customer_in[customer->index-1]);

    pthread_mutex_lock(&print_mutex);
    printf(WHITE "Customer %d arrives at %d second(s)\n" RESET, customer->index, customer->t_arr);
    printf(YELLOW "Customer %d orders a %s\n" RESET, customer->index, customer->ctype);
    sem_post(&customer_exists);
    pthread_mutex_unlock(&print_mutex);
    // sem_wait(&barista);


    sem_wait(&customer_out[customer->index-1]);

    if(customer->is_served==0)
    {
        pthread_mutex_lock(&print_mutex);
        if(customer->being_served==0)
            avg_wtime=avg_wtime + curr_time - customer->t_arr - 1;
        coffee_wasted++;
        customer_left++;
        printf(RED "Customer %d leaves without their order at %d second(s)\n" RESET, customer->index, curr_time);
        pthread_mutex_unlock(&print_mutex);
    }
    else
    {
        pthread_mutex_lock(&print_mutex);
        // avg_wtime=avg_wtime + temp_currtime[customer->index-1] - customer->t_arr ;
        customer_left++;
        printf(GREEN "Customer %d leaves with their order at %d second(s)\n" RESET, customer->index, curr_time);
        pthread_mutex_unlock(&print_mutex);
    }

}

void* barista_func(void* arg)
{
    sem_wait(&customer_exists);
    ThreadArg* thread_arg = (ThreadArg*)arg;
    int bar_index = thread_arg->barista_index;
    int B = thread_arg->B;
    int N = thread_arg->N;
    int K = thread_arg->K;
    Customer* cust = thread_arg->customers;
    Coffee* coffee = thread_arg->coffees;
    // printf("hello i am barista %d\n",bar_index+1);

    for(int i=0;i<N;i++)
    {
        // sem_wait(&customer_in[i]);
        if(cust[i].has_arrived == 1 && cust[i].being_served == 0)
        {
            // bar_index=i;
            // break;
        }
        else
        {
            continue;
        }
        //ENSURE THAT BARISTA WAITS 1 SECOND
        pthread_mutex_lock(&print_mutex);
        if(cust[i].delay_flag==0)
        {
            cust[i].delay_flag=1;
        }
        pthread_mutex_unlock(&print_mutex);
        cust[i].serving_bar = bar_index;

        sem_wait(&delay1_bar[cust[i].serving_bar]);

        pthread_mutex_lock(&print_mutex);
        cust[i].delay_flag=0;
        cust[i].being_served=1;
        temp_currtime[i] = curr_time;
        printf(CYAN "Barista %d begins preparing the order of customer %d at %d second(s)\n" RESET, bar_index+1, i+1, curr_time);
        avg_wtime=avg_wtime + curr_time - cust[i].t_arr;
        barista_busy++;
        int temp_prep_time = prep_time(K, cust[i].ctype, coffee);
        barista_arr[bar_index]=curr_time+temp_prep_time;
        pthread_mutex_unlock(&print_mutex);

        // sleep(temp_prep_time);
        sem_wait(&barista_sleep[bar_index]);
        
        pthread_mutex_lock(&print_mutex);
        printf(BLUE "Barista %d completes the order of customer %d at %d second(s)\n" RESET, bar_index+1, i+1, curr_time);
        barista_busy--;
        cust[i].is_served=1;
        pthread_mutex_unlock(&print_mutex);
        sem_post(&customer_out[i]);
        // sem_post(&barista);
    }
    return NULL;
}

int main() {
    int B, K, N;
    scanf("%d %d %d", &B, &K, &N);
    Coffee coffee[K];
    for (int i = 0; i < K; i++) 
    {
        scanf("%s %d", coffee[i].name, &coffee[i].ttp);
    }
    Customer cust[N];
    for (int i = 0; i < N; i++) 
    {
        scanf("%d %s %d %d", &cust[i].index, cust[i].ctype, &cust[i].t_arr, &cust[i].t_tol);
        cust[i].is_served = 0;
        cust[i].being_served = 0;
        cust[i].has_arrived = 0;
        cust[i].delay_flag = 0;
        cust[i].serving_bar = -1;
        temp_currtime[i]=-1;
    }
    for(int i=0;i<B;i++)
    {
        barista_arr[i]=-1;
    }

    sem_init(&barista, 0, B);
    sem_init(&customer_exists, 0, 0);
    // sem_init(&delay1_bar,0,0);
    pthread_mutex_init(&print_mutex, NULL);

    for(int i=0;i<N;i++)
    {
        sem_init(&customer_in[i],0,0);
    }
    for(int i=0;i<N;i++)
    {
        sem_init(&customer_out[i],0,0);
    }
    for(int i=0;i<B;i++)
    {
        sem_init(&delay1_bar[i],0,0);
    }
    for(int i=0;i<B;i++)
    {
        sem_init(&barista_sleep[i],0,0);
    }

    pthread_t barista_thread[B];
    pthread_t cust_thread[N];
    int exit_flag=0;
    ThreadArg thread_args[B];

    // curr_time = cust[0].t_arr;

    for (int i = 0; i < N; i++) 
    {
        Customer* customer = &cust[i];
        pthread_create(&cust_thread[i], NULL, customer_func, customer);
    }

    for (int i = 0; i < B; i++) 
    {
        thread_args[i].barista_index = i;
        thread_args[i].B = B;
        thread_args[i].N = N;
        thread_args[i].K = K;
        thread_args[i].customers = cust;
        thread_args[i].coffees = coffee;
        pthread_create(&barista_thread[i], NULL, barista_func, &thread_args[i]);
    }

    // while(curr_time<(cust[N-1].t_arr + cust[N-1].t_tol))
    while(!(customer_left==N && barista_busy==0))
    {
        for(int i=0;i<N;i++)
        {
            if(cust[i].t_arr == curr_time)
            {
                cust[i].has_arrived=1;
                sem_post(&customer_in[i]);
            }
        }
        for(int i=0;i<N;i++)
        {
            if(curr_time == cust[i].t_arr + cust[i].t_tol + 1)
            {
                sem_post(&customer_out[i]);
            }
        }
        for(int i=0;i<N;i++)
        {
            if(cust[i].delay_flag==1)
            {
                sem_post(&delay1_bar[cust[i].serving_bar]);
            }
        }
        for(int i=0;i<B;i++)
        {
            if(curr_time == barista_arr[i])
            {
                sem_post(&barista_sleep[i]);
            }
        }
        sleep(1);
        curr_time++;
    }


    for (int i = 0; i < N; i++) {
        pthread_join(cust_thread[i], NULL);
    }

    // for (int i = 0; i < B; i++) {
    //     pthread_join(barista_thread[i], NULL);
    // }
        // printf("cusr left = %d barista busy = %d\n",customer_left,barista_busy);

    pthread_mutex_destroy(&print_mutex);
    // sem_destroy(&barista);
    for(int i=0;i<N;i++)
    {
        sem_destroy(&customer_in[i]);
        sem_destroy(&customer_out[i]);
    }

    // for(int i=0;i<N;i++)
    // {
    //     if(cust[i].being_served==0)
    //     {

    //     }
    // }

    printf("\naverage wait time : %f second(s)\n", (float)avg_wtime/N);
    printf("\n%d coffee wasted\n", coffee_wasted);

    return 0;
}
