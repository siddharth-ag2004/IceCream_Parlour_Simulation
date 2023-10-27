// Output:

// Customer 1 arrives at 0 second(s)
// Customer 1 orders a Cappuccino
// Barista 1 begins preparing the order of customer 1 at 1 second(s)
// Customer 2 arrives at 3 second(s)
// Customer 2 orders an Espresso
// Customer 3 arrives at 3 second(s)
// Customer 3 orders an Espresso
// Barista 2 begins preparing the order of customer 2 at 4 second(s)
// Barista 2 completes the order of customer 2 at 7 second(s)
// Customer 2 leaves with their order at 7 second(s)
// Barista 2 begins preparing the order of customer 3 at 8 second(s)
// Customer 3 leaves without their order at 9 second(s)
// Barista 1 completes the order of customer 1 at 11 second(s)
// Barista 2 completes the order of customer 2 at 11 second(s)
// Customer 1 leaves with their order at 11 second(s)


#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>

typedef struct Coffee
{
    char name[20];
    int ttp;        //time to prepare
}Coffee;

typedef struct Customer
{
    int index;
    char ctype[20]; 
    int t_arr;
    int t_tol;
}Customer;

sem_t  barista;
sem_t  cust;
int curr_time = 0;

void* customer_thread(void* arg,Customer cust[]) {
    int customer_index = *((int*)arg);
    if(curr_time == cust[customer_index].t_arr)
    printf("Customer %d arrives at %d second(s)\n", customer_index, curr_time);
    return NULL;
}

void* barista_thread(void* arg, Customer cust[]) {  
    int barista_index = *((int*)arg);
    if(curr_time == cust[barista_index].t_arr)
    printf("Customer %d arrives at %d second(s)\n", barista_index, curr_time);
    return NULL;
}
    
// void order(int index, char ctype[],int cust[])
// {
//     printf("Barista %d begins preparing the order of customer %d at %d second(s)\n",3);
// }



int main()
{
    int B,K,N;
    scanf("%d %d %d",&B,&K,&N);
    Coffee coffee[K];
    for(int i=0;i<K;i++)
    {
        scanf("%s %d",coffee[i].name,&coffee[i].ttp);
    }
    Customer cust[N];
    for(int i=0;i<N;i++)
    {
        scanf("%d %s %d %d",&cust[i].index,cust[i].ctype,&cust[i].t_arr,&cust[i].t_tol);
    }

    pthread_t barista_thread[B];
    pthread_t cust_thread[N];

    sem_init(&barista,0,B);
    sem_init(&cust,0,0);

    for(int i=0;i<B;i++)
    {
        pthread_create(&barista_thread[i],NULL,barista_thread,K);
    }
    for(int i=0;i<N;i++)
    {
        pthread_create(&cust_thread[i],NULL,customer_thread,N);
    }


} 