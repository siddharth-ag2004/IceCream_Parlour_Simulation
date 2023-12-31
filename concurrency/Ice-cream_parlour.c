#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <stdlib.h>

#define MAX_LEN 500

#define YELLOW "\033[1;33m"
#define WHITE "\e[0;37m"
#define CYAN "\033[1;36m"
#define BLUE "\033[1;34m"
#define GREEN "\033[1;32m"
#define RED "\033[1;31m"
#define ORANGE "\e[38;2;255;85;0m"
#define RESET "\033[0m"

typedef struct Machine
{
    int tm_start;
    int tm_stop;
    int stopped;
    int occupied;
} Machine;

typedef struct Topping
{
    char t_type[20];
    int q_t;
    int topping_index;
} Topping;

typedef struct Flavour
{
    char i_type[20];
    int ttp;
} Flavour;

typedef struct Icecream
{
    Flavour ic;
    int num_tops;
    Topping with_topping[100];
    int being_prepared;
    int is_served;
    int entered_new_func;
    int serving_bar;
    int delay1_flag;
} Icecream;

typedef struct Customer
{
    int index;
    int t_arr;
    int num_ic;
    Icecream order[200]; // num_ic
    int has_arrived;
    int rejected;
    int entered_func;
    int has_left;
    int order_fulfilled;
    int entered_wait;
} Customer;

typedef struct ThreadArg
{
    int machine_index;
} ThreadArg;

sem_t customer_in[MAX_LEN];
sem_t customer_out[MAX_LEN];
sem_t delay_customer[MAX_LEN];
sem_t delay1_barista[MAX_LEN];
Customer customer[MAX_LEN];
int order_flag=0;
Machine machine[MAX_LEN];
Flavour flavour[MAX_LEN];
Topping topping[MAX_LEN];
int N, K, F, T;
int cust_num = 0;
int curr_time = 0;
int used_topping[MAX_LEN];
sem_t machine_start[MAX_LEN];
sem_t machine_stop[MAX_LEN];
sem_t machine_sleep[MAX_LEN];
int machine_freenext[MAX_LEN];
sem_t ord_exist[MAX_LEN];
sem_t order_exists;
sem_t customer_exists;
pthread_mutex_t print_mutex;
pthread_mutex_t loop_mutex;
pthread_mutex_t topping_mutex;
pthread_mutex_t capacity_lock;
int machines_stopped = 0;
int flag_arr[MAX_LEN]={0};
int total_customers_left=0;
int curr_capacity=0;
int all_stopped=0;
int topping_flag=0;
int topping_over=0;
int last_time_left=-1;
//ASSUMPTION: EACH TOPPING IS ONLY USED ONCE IN ONE ORDER(EXAMPLE CHOCOLATE CARAMEL CARAMEL IS NOT VALID)

int enough_toppings(Customer cust_i)
{
    customer[cust_i.index-1].entered_func = 1;
    for (int i = 0; i < cust_i.num_ic; i++)
    {
        for (int j = 0; j < cust_i.order[i].num_tops; j++)
        {
            for (int k = 0; k < T; k++)
            {
                if (strcmp(cust_i.order[i].with_topping[j].t_type, topping[k].t_type) == 0)
                {
                    used_topping[k]++;
                }
            }
        }
    }

    for (int i = 0; i < T; i++)
    {
        if (used_topping[i] > topping[i].q_t)
        {
            for(int p=0;p<T;p++)
            {
                used_topping[p]=0;
            }
            return 0;
        }
    }

    for(int i=0;i<T;i++)
    {
        // topping[i].q_t = topping[i].q_t - used_topping[i];
        // if(topping[i].q_t == 0)
        // {
        //     topping_over--;
        // }
        used_topping[i]=0;
    }
    return 1;

}

int enough_order_toppings(Icecream ic_ordered,Customer cust_i,int idx)
{
    pthread_mutex_lock(&topping_mutex);
    customer[cust_i.index-1].order[idx].entered_new_func = 1;
    for (int i = 0; i < ic_ordered.num_tops; i++)
    {
        for (int j = 0; j < T; j++)
        {
            if (strcmp(ic_ordered.with_topping[i].t_type, topping[j].t_type) == 0)
            {
                used_topping[j]++;
            }
        }
    }

    for (int i = 0; i < T; i++)
    {
        if (used_topping[i] > topping[i].q_t)
        {
            for(int p=0;p<T;p++)
            {
                used_topping[p]=0;
            }
            pthread_mutex_unlock(&topping_mutex);
            return 0;
        }
    }

    for(int i=0;i<T;i++)
    {
        topping[i].q_t = topping[i].q_t - used_topping[i];
        if(topping[i].q_t == 0)
        {
            topping_over--;
        }
        used_topping[i]=0;
    }
    pthread_mutex_unlock(&topping_mutex);
    return 1;
}

int prep_time(Icecream icecream_ordered)
{
    int sum = 0;
    for (int i = 0; i < F; i++)
    {
        if (strcmp(icecream_ordered.ic.i_type, flavour[i].i_type) == 0)
        {
            sum += flavour[i].ttp;
            break;
        }
    }
    return sum;
}

void *customer_func(void *arg)
{
    Customer *cust_inthread = (Customer *)arg;

    sem_wait(&customer_in[cust_inthread->index - 1]);

    // pthread_mutex_lock(&print_mutex);        //REMOVE THIS LOCK  
    pthread_mutex_lock(&capacity_lock);
    printf(WHITE "Customer %d arrives at %d second(s)\n" RESET, cust_inthread->index, cust_inthread->t_arr);
    printf(YELLOW "Customer %d orders %d ice cream(s)\n" RESET, cust_inthread->index, cust_inthread->num_ic);
    for (int i = 0; i < cust_inthread->num_ic; i++)
    {
        printf(YELLOW "Ice cream %d: %s " , i + 1, cust_inthread->order[i].ic.i_type);
        for (int j = 0; j < cust_inthread->order[i].num_tops; j++)
        {
            printf("%s ", cust_inthread->order[i].with_topping[j].t_type);
        }
        printf("\n" RESET);
    }
    // customer[cust_inthread] has_arrived = 1;
    if(curr_capacity==K || (last_time_left==curr_time && curr_capacity==K-1))
    {
        total_customers_left++;
        printf(RED "Customer %d was not serviced due to shop being full\n" RESET, cust_inthread->index);
        pthread_mutex_unlock(&capacity_lock);
        return NULL;
    }

    if(enough_toppings(*cust_inthread) == 0)
    {
        total_customers_left++;
        printf(RED "Customer %d was not serviced due to unavailability of toppings\n" RESET, cust_inthread->index);
        pthread_mutex_unlock(&capacity_lock);
        return NULL;
    }


    curr_capacity++; 

    pthread_mutex_unlock(&capacity_lock);

    customer[cust_inthread->index - 1].has_arrived = 1;
    order_flag+=cust_inthread->num_ic;
    // sem_post(&customer_exists);
    if(machines_stopped==N)
    {
        total_customers_left++;
        printf(RED "Customer %d was not serviced due to unavailability of machines\n" RESET, cust_inthread->index);
        return NULL;
    }

    // sem_wait(&delay_customer[cust_inthread->index - 1]);

    int loop_break_flag=0;
    while(customer[cust_inthread->index-1].rejected == 0 && all_stopped==0 && customer[cust_inthread->index-1].order_fulfilled < cust_inthread->num_ic)        //CHECK WHEN TO BREAK LOOOP
    {
        for (int i = 0; i < cust_inthread->num_ic; i++)
        {
            for(int j=0;j<N;j++)
            {
                if(machine[j].occupied==0 && curr_time >= machine[j].tm_start && curr_time < machine[j].tm_stop && curr_time+prep_time(customer[cust_inthread->index-1].order[i])<=machine[j].tm_stop) // PROBLEM WHEN ONE MACHINE IS CLOSING BUT SECOND MACHINE CAN OPEN
                {
                    // curr_time+prep_time(customer[cust_inthread->index-1].order[i])<=machine[i].tm_stop
                    sem_post(&ord_exist[j]);
                    loop_break_flag=1;
                }   
            }
            // sem_post(&order_exists);
            if(loop_break_flag==1)
                break;
        }
    }
    // pthread_mutex_unlock(&print_mutex);

    // sem post increase sem by cust_inthread->num_ic
    // printf("")

    sem_wait(&customer_out[cust_inthread->index - 1]);
    total_customers_left++;
    customer[cust_inthread->index - 1].has_left = 1;
    // pthread_mutex_lock(&print_mutex);
    curr_capacity--;
    last_time_left = curr_time;
    if(all_stopped==1)
    {
        // total_customers_left++;
        printf(RED "Customer %d was not serviced due to unavailability of machines\n" RESET, cust_inthread->index);
        return NULL;
    }

    if(cust_inthread->rejected == 1)
    {
        // total_customers_left++;
        printf(RED "Customer %d left at %d second(s) with an unfulfilled order\n" RESET, cust_inthread->index, curr_time);
        return NULL;
    }

    for (int i = 0; i < cust_inthread->num_ic; i++)
    {
        if (cust_inthread->order[i].is_served == 0)
        {
            total_customers_left++;
            printf("Customer %d was not serviced due to unavailability of machines\n", cust_inthread->index);
            return NULL;
        }
    }
    // total_customers_left++;
    printf(GREEN "Customer %d has collected their order(s) and left at %d second(s)\n" RESET, cust_inthread->index, curr_time);
    // pthread_mutex_unlock(&print_mutex);

    return NULL;
}

void *machine_func(void *arg)
{
    // CUSTOMER INDEX IS 1 INDEXED AND MACHINE INDEX IS 0 INDEXED

    ThreadArg *thread_arg = (ThreadArg *)arg;
    int machine_index = thread_arg->machine_index;
    sem_wait(&machine_start[machine_index]); // MACHINE STOP NOT YET PUT

    printf(ORANGE "Machine %d has started working at %d second(s)\n" RESET, machine_index + 1, curr_time);

    flag_arr[machine_index]=1;
    while(1)
    {
    for (int i = 0; i < cust_num; i++)
    {
        sem_wait(&ord_exist[machine_index]);
        if(machine[machine_index].stopped == 1)
        {
            machines_stopped++;
            printf(ORANGE "Machine %d has stopped working at %d second(s)\n" RESET, machine_index + 1, curr_time);
            if(machines_stopped == N)
            {
                all_stopped=1;
                for(int p=0;p<cust_num;p++)
                    sem_post(&customer_out[p]);
                // sem_post(&customer_out[2]);
            }
            return NULL;
        }

        if(customer[i].has_arrived == 1 && customer[i].has_left == 0)
        {
            for (int j = 0; j < customer[i].num_ic; j++)
            {
                pthread_mutex_lock(&print_mutex);
                if(customer[i].rejected == 1 || (customer[i].entered_func==0 && enough_toppings(customer[i]) == 0) || (customer[i].order[j].entered_new_func==0 && enough_order_toppings(customer[i].order[j],customer[i],j) == 0) || all_stopped==1)
                {
                    if(customer[i].entered_wait==0)
                    {
                        if(customer[i].order[j].delay1_flag==0)
                        {
                            customer[i].order[j].delay1_flag=1;
                        }
                        customer[i].order[j].serving_bar=machine_index;
                        if(curr_time!=machine[machine_index].tm_start || (curr_time==machine[machine_index].tm_start) && curr_time == customer[i].t_arr)
                            sem_wait(&delay1_barista[machine_index]);  

                        customer[i].order[j].delay1_flag=0;
                    }
                    customer[i].rejected = 1;
                    customer[i].entered_wait=1;
                    sem_post(&customer_out[i]);
                    pthread_mutex_unlock(&print_mutex);
                    break;
                }
                else
                {
                    if (customer[i].order[j].being_prepared == 0 && prep_time(customer[i].order[j]) + curr_time+ (curr_time!=machine[machine_index].tm_start) < machine[machine_index].tm_stop)
                    {
                        // printf("order recvd\n");
                    }
                    else
                    {
                        pthread_mutex_unlock(&print_mutex);
                        continue;
                    }
                }
                if(customer[i].rejected == 0)
                {
                // order_flag--;
                customer[i].order[j].being_prepared = 1;
                customer[i].order_fulfilled++;
                customer[i].order[j].serving_bar = machine_index;

                if(customer[i].order[j].delay1_flag==0)
                {
                    customer[i].order[j].delay1_flag=1;
                }

                if(curr_time!=machine[machine_index].tm_start || (curr_time==machine[machine_index].tm_start) && curr_time == customer[i].t_arr)
                    sem_wait(&delay1_barista[machine_index]);  

                customer[i].order[j].delay1_flag=0;
                printf(CYAN "Machine %d starts preparing ice cream %d of customer %d at %d second(s)\n" RESET, machine_index + 1, j + 1, i + 1, curr_time);
                machine[machine_index].occupied = 1;
                machine_freenext[machine_index] = curr_time + prep_time(customer[i].order[j]);
                // printf("prep time: %d\n",prep_time(customer[i].order[j]));
                pthread_mutex_unlock(&print_mutex);

                sem_wait(&machine_sleep[machine_index]);

                pthread_mutex_lock(&print_mutex);
                customer[i].order[j].is_served = 1;
                printf(BLUE "Machine %d completes preparing ice cream %d of customer %d at %d second(s)\n" RESET, machine_index + 1, j + 1, i + 1, curr_time);
                machine[machine_index].occupied = 0;
                pthread_mutex_unlock(&print_mutex);
                //Customer Leaves If All Icecreams Are Served
                // if(j==customer[i].num_ic-1)
                // {
                //     sem_post(&customer_out[i]);
                // }
                int temp_sum=0;
                for(int k=0;k<customer[i].num_ic;k++)
                {
                    if(customer[i].order[k].is_served==1)
                    {
                        temp_sum++;
                    }
                }
                if(temp_sum==customer[i].num_ic)
                {
                    sem_post(&customer_out[i]);
                }
                }
                else
                {
                    pthread_mutex_unlock(&print_mutex);
                }
            }
        }
    }
    }
}

// void TakeInput()
// {
//     scanf("%d %d %d %d", &N, &K, &F, &T);
//     int num_toppings;
//     for (int i = 0; i < N; i++)
//     {
//         scanf("%d %d", &machine[i].tm_start, &machine[i].tm_stop);
//         machine[i].stopped = 0;
//         machine[i].occupied = 0;
//     }
//     for (int i = 0; i < F; i++)
//     {
//         scanf("%s %d", flavour[i].i_type, &flavour[i].ttp);
//     }
//     for (int i = 0; i < T; i++)
//     {
//         scanf("%s %d", topping[i].t_type, &topping[i].q_t);
//         if(topping[i].q_t == -1)
//         {
//             topping[i].q_t = 10000;
//         }
//         else
//         {
//             topping_flag=1;
//             topping_over++;
//         }
//         used_topping[i] = 0;
//         topping[i].topping_index = i;
//     }
//     // printf("enter number of customers\n");
//     scanf("%d", &cust_num); // not giving input
//     for (int idx = 0; idx < cust_num; idx++)
//     {
//         scanf("%d", &customer[idx].index);
//         scanf("%d", &customer[idx].t_arr);
//         scanf("%d", &customer[idx].num_ic);
//         for (int j = 0; j < customer[idx].num_ic; j++)
//         {
//             scanf("%s", customer[idx].order[j].ic.i_type);
//             scanf("%d", &num_toppings);
//             customer[idx].order[j].num_tops = num_toppings;
//             for (int i = 0; i < num_toppings; i++)
//             {
//                 scanf("%s", customer[idx].order[j].with_topping[i].t_type);
//             }
//             customer[idx].order[j].being_prepared = 0;
//             customer[idx].order[j].is_served = 0;
//             customer[idx].order[j].entered_new_func = 0;
//             customer[idx].order[j].serving_bar = -1;
//             customer[idx].order[j].delay1_flag = 0;
//         }
//         customer[idx].has_arrived = 0;
//         customer[idx].rejected = 0;
//         customer[idx].entered_func = 0;
//         customer[idx].has_left = 0;
//         customer[idx].order_fulfilled = 0;
//     }

// }

void TakeNewInput()
{
//    scanf("%d %d %d %d", &N, &K, &F, &T);
    char input1[1000];
    fgets(input1,sizeof(input1),stdin);
    sscanf(input1,"%d %d %d %d", &N, &K, &F, &T);
    int num_toppings;
    for (int i = 0; i < N; i++)
    {
        // scanf("%d %d", &machine[i].tm_start, &machine[i].tm_stop);
        fgets(input1,sizeof(input1),stdin);
        sscanf(input1,"%d %d", &machine[i].tm_start, &machine[i].tm_stop);
        machine[i].stopped = 0;
        machine[i].occupied = 0;
    }
    for (int i = 0; i < F; i++)
    {
        // scanf("%s %d", flavour[i].i_type, &flavour[i].ttp);
        fgets(input1,sizeof(input1),stdin);
        sscanf(input1,"%s %d", flavour[i].i_type, &flavour[i].ttp);
    }
    for (int i = 0; i < T; i++)
    {
        // scanf("%s %d", topping[i].t_type, &topping[i].q_t);
        fgets(input1,sizeof(input1),stdin);
        sscanf(input1,"%s %d", topping[i].t_type, &topping[i].q_t);
        if(topping[i].q_t == -1)
        {
            topping[i].q_t = 10000;
        }
        else
        {
            topping_flag=1;
            topping_over++;
        }
        used_topping[i] = 0;
        topping[i].topping_index = i;
    }
    
    char total_input[5000];
    while(1)
    {
        fgets(total_input,sizeof(total_input),stdin);
        int len1 = strlen(total_input);
        if(total_input[0]=='\n' || total_input[0]=='\0')
        {

            break;
        }
        sscanf(total_input,"%d %d %d",&customer[cust_num].index,&customer[cust_num].t_arr,&customer[cust_num].num_ic);
        for (int j = 0; j < customer[cust_num].num_ic; j++) 
        {
            // fgets(input1, sizeof(input1), stdin);
            // sscanf(input1, "%s", customer[cust_num].order[j].ic.i_type);

            num_toppings = 0;
            // while (1) 
            // {
                char topping_line[1000];
                fgets(topping_line, sizeof(topping_line), stdin);

                size_t len = strlen(topping_line);
                if (len > 0 && topping_line[len - 1] == '\n') {
                    topping_line[len - 1] = '\0';
                }

                if (topping_line[0] == '\n' || topping_line[0] == '\0') {
                    break;
                }

                char* token = strtok(topping_line, " ");
                if (token != NULL) {
                    strcpy(customer[cust_num].order[j].ic.i_type, token);

                // printf("icecream type: %s\n", customer[cust_num].order[j].ic.i_type);
                num_toppings = 0;
                while ((token = strtok(NULL, " ")) != NULL) {
                    strcpy(customer[cust_num].order[j].with_topping[num_toppings].t_type, token);
                    // printf("topping type: %s\n", customer[cust_num].order[j].with_topping[num_toppings].t_type);
                    num_toppings++;
                }
                customer[cust_num].order[j].num_tops = num_toppings;
                // printf("num_toppings: %d\n", num_toppings);
                }
            // }
            // printf("once\n");
            customer[cust_num].order[j].being_prepared = 0;
            customer[cust_num].order[j].is_served = 0;
            customer[cust_num].order[j].entered_new_func = 0;
            customer[cust_num].order[j].serving_bar = -1;
            customer[cust_num].order[j].delay1_flag = 0;
            // customer[cust_num].order[j].num_tops = num_toppings;
            // getchar();

        }
        // printf("second\n");
        customer[cust_num].has_arrived = 0;
        customer[cust_num].rejected = 0;
        customer[cust_num].entered_func = 0;
        customer[cust_num].has_left = 0;
        customer[cust_num].order_fulfilled = 0;
        customer[cust_num].entered_wait = 0;

        cust_num++;
    }
   
}

int main()
{
    TakeNewInput();
    sem_init(&customer_exists, 0, 0);
    sem_init(&order_exists, 0, 0);
    for (int i = 0; i < N; i++)
    {
        sem_init(&machine_start[i], 0, 0);
        sem_init(&machine_stop[i], 0, 0);
        sem_init(&machine_sleep[i], 0, 0);
        sem_init(&ord_exist[i], 0, 0);
        sem_init(&delay1_barista[i], 0, 0);
        machine_freenext[i] = -1;
    }
    for (int i = 0; i < cust_num; i++)
    {
        sem_init(&customer_in[i], 0, 0);
        sem_init(&customer_out[i], 0, 0);
        sem_init(&delay_customer[i], 0, 0);
    }

    pthread_mutex_init(&print_mutex, NULL);
    pthread_mutex_init(&loop_mutex, NULL);
    pthread_mutex_init(&topping_mutex, NULL);
    pthread_mutex_init(&capacity_lock, NULL);

    pthread_t machine_thread[N];
    pthread_t cust_thread[cust_num];
    ThreadArg thread_args[N];

    for (int i = 0; i < cust_num; i++)
    {
        // printf("test1\n");
        Customer *cst = &customer[i];
        // printf("test2\n");
        pthread_create(&cust_thread[i], NULL, customer_func, cst);
    }

    for (int i = 0; i < N; i++)
    {
        thread_args[i].machine_index = i;
        pthread_create(&machine_thread[i], NULL, machine_func, &thread_args[i]);
        // printf("test3\n");
    }

    // while (curr_time < 1000)
    while((machines_stopped < N || total_customers_left < cust_num))
    {
        // printf("tops : flag = %d over = %d\n",topping_flag,topping_over);
        //comment out below line if implementation of parlour closing is required
        // if(topping_flag == 1 && topping_over==0)
        // {
        //     printf("Parlour closed due to unavailibility of toppings.\n");
        //     exit(1);
        // }
        for (int i = 0; i < N; i++)
        {
            if (curr_time == machine[i].tm_start)
            {
                // printf("machine which starts : %d\n", i+1);
                sem_post(&machine_start[i]);
            }
        }
        for (int i = 0; i < N; i++)
        {
            if (curr_time == machine_freenext[i])
            {
                sem_post(&machine_sleep[i]);
            }
        }
        for (int i = 0; i < cust_num; i++)
        {
            if (curr_time == customer[i].t_arr)
            {
                sem_post(&customer_in[i]);
            }
        }
        // for(int i=0;i<cust_num;i++)
        // {
        //     if(curr_time == customer[i].t_arr + 1)
        //     {
        //         sem_post(&delay_customer[i]);
        //     }
        // }
        for(int i=0;i<cust_num;i++)
        {
            for(int j=0;j<customer[i].num_ic;j++)
            {
                if(customer[i].order[j].delay1_flag==1)
                {
                    sem_post(&delay1_barista[customer[i].order[j].serving_bar]);
                }
            }
        }
        for(int i=0;i<N;i++)
        {
            if(curr_time == machine[i].tm_stop)
            {
                machine[i].stopped = 1;
                // machines_stopped++;
                sem_post(&ord_exist[i]);
                // sem_post(&order_exists);
            }
        }
        sleep(1);
        curr_time++;
    }
    
    printf("Parlour closed\n");

    for(int i=0;i<cust_num;i++)
    {
        pthread_join(cust_thread[i], NULL);
    }
    for(int i=0;i<N;i++)
    {
        pthread_join(machine_thread[i], NULL);
    }

    pthread_mutex_destroy(&print_mutex);
    pthread_mutex_destroy(&loop_mutex);
    sem_destroy(&customer_exists);
    sem_destroy(&order_exists);
    for (int i = 0; i < N; i++)
    {
        sem_destroy(&machine_start[i]);
        sem_destroy(&machine_stop[i]);
        sem_destroy(&machine_sleep[i]);
        sem_destroy(&ord_exist[i]);
    }
    for (int i = 0; i < cust_num; i++)
    {
        sem_destroy(&customer_in[i]);
        sem_destroy(&customer_out[i]);
        sem_destroy(&delay_customer[i]);
    }
}
