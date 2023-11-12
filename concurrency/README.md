# 1. Cafe Sim #
If the cafe had infinite baristas, no customer would have to wait for more than 1 second to have thier order be taken, as there would always be some barista who is free. Thus average waiting time would be 1.

For wasted coffee, I maintained a variable coffee_wasted which I was incrementing whenever a customer left without being served. Thus, the total wasted coffee would be coffee_wasted.

## Implementation ##

For the Cafe Sim, I have created B barista threads (each simulating a barista), each corresponding to  and N customer threads (each simulating a customer).

I have a variable curr_time which simulates real world clock.
I have done this using a while loop which I am incrementing in int main() till all customers have left and no barista is busy preparing an order.After each iteration I sleep(1) to simulate 1 second of real world time.
Whatever happens within eachh iteration corresponds to all the events that happen in 1 second of real world time.

Initially in the main I iterate through all customers to check if the current time matches any customer's arrival time.
Each customer thread is waiting on its respective customer_in semaphore and that respective semaphore is posted when the customer arrives.
The customer thread also waits on a semaphore customer_out which is posted when the customer's order is done or when the customers tolerance time has run out.

For tolerance time, the while loop in main also checks when the current time equlas the customers arrival+tolerance time and if condition is met, sem post is done on customer_out semaphore.

Each barista thread is waiting on a semaphore customer_exists which is posted when a customer arrives.
Thus one barista thread is woken up when a customer arrives and then the customer array is iterated through to check which customer has arrived and this ensures that customer with lower index is given higher priority.

The barista then waits on a delay1_bar semaphore to ensure the condition that barista must wait atleast 1 second before taking customer's order.
This is done by implementing lock around a delay1 flag which ensures 
that the barista waits atleast 1 second before taking the order.

The barista calculates the amount of preparation time for the customer's order and stores the time at which the barista should finish preparing the order in an array. The barista then waits on a semaphore barista_sleep, which is woken when the current time is equal to the time at which the barista should finish preparing the order.
The barista then marks the customer as served and posts on the customer_out semaphore of the customer.

Whenever customer_out is posted the is_served flag is checked and if it is 0, then the customer has left without being served and the wasted count is incremented. And if the is_served flag is 1, then the customer has been served. The customer_left count is then incremented.

print_mutex lock is applied around the print statements to ensure that the output is not jumbled up and deadlocks are avoided.





# 2. Ice Cream Parlour Sim #


## Implementation ##

For the Cafe Sim, I have created B barista threads (each simulating a barista), each corresponding to  and N customer threads (each simulating a customer).

I have a variable curr_time which simulates real world clock.
I have done this using a while loop which I am incrementing in int main() till all customers have left and all machines have stopped.
Also, I have added a commented line of code in the beginning of the loop to handle the case when all finite toppings have been used up, then the parlour should close. I have commnted this part as I fely there was a discreepancy in the doubts document and the assignment document.  
After each iteration I sleep(1) to simulate 1 second of real world time.
Whatever happens within eachh iteration corresponds to all the events that happen in 1 second of real world time.

Initially in the main I iterate through all machines to check if the current time matches any machine's start time.
Each machine thread is waiting on its respective machine_start semaphore and that respective semaphore is posted when the current time is the time when the machine should start working.
Similiarly each customer thread is waiting on its respective customer_in semaphore and that respective semaphore is posted when the customer arrives.


Whenever a customer arrives he places the order, the customer thread checks if the cafe is full and if it is full, the customer leaves immediately without being served.
If capacity is not exceeded due to customer being present , then I check if the toppings are available for the customer's entire order and if they are not available, the customer leaves without being served.
A lock is placed around the above part to ensure that more than 2 customers who enter at the same time are not served if adding both exceeds capacity or that enough toppings are present for one of the customers first.
Also, if the customer arrives after all machines have stopped, then the customer leaves without being served.


In contrast to Cafe Sim where each customer has only 1 order so, only 1 barista can prepare a customer's order, here, I iterate through all the orders of the customer and check if any machine is free and if the machine is free, I check if the machine can prepare the order before it closes and if it can, then the machine prepares the order by posting the semaphore for that machine, whcih that machine waits on initially.
The customer then waits on a semaphore customer_out once all its orders have begun being prepared by some machine.
Once customer out is posted, the customer checks if all its orders have been prepared and if they have been prepared, then the customer leaves.
If the customer was rejected then I print cprresponding message and If semaphore was posted due to all machines stopping then I print corresponding message.


The machine thread for each machine having been started, first checks if its stopped flag is 1, if it is 1, then the machine has stopped and the machine thread exits.The stop flag becomes 1 in the while loop in int main when the current time is equal to the time when the machine should stop working.
Then, to give higher priority to a customer who with lower index (when more than one customer arrives at a particular time), I iterate through all customers and check if any customer has arrived and if a customer has arrived, I check if the customer has any order which can be prepared by the machine and if it can, then the machine prepares the order by posting the semaphore for that machine, which that machine waits on initially. 
The machine checks if the order can be prepared by it if it has enough toppings and if it can prepare the order before it closes.
If the above conditions match then the machine uses up the toppings (which is a critical part I have locked to prevent two threads from accessing at same time) and waits 1 second before preparing the order after which it waits on a semaphore machine_sleep which is posted when the current time is equal to the time at which the machine should finish preparing the order to simulate the machine being busy while preparing order.


Once a machine has finished preparing an order, it checks if all orders have beein prepared for that customer and if condition is met, then the customer_out semaphore of the customer is posted.






### Minimizing Incomplete Orders: Describe your approach to redesign the simulation to minimize incomplete orders, given that incomplete orders can impact the parlor’s reputation. Note that the parlor’s reputation is unaffected if orders are instantly rejected due to topping shortages. ###

In my impleqmentation, I have ensured that a customer is not served if the toppings are not available for the entire order of the customer. However, I have not ensured that a customer is not served if the toppings are not available for a part of the order of the customer.

I would have a variable storing the total time all machines can run for beginning from the curr_time called total_time.
To ensure that a customer is not served if the toppings are not available for a part of the order of the customer, I would reserve the toppings for the entire order for that customer and precompute the total preparing time of the entire order. If the preparing time of the entire order is less than total_time, then the customer is served and his prep_time is subtracted from total_time, else the customer is not served. Similiarly for allother customers. 
Although this might lead to more customers being rejected, they would be rejected immediately and so parlor's reputation would not be affected.


### Ingredient Replenishment: Assuming ingredients can be replenished by contacting the nearest supplier, outline how you would adjust the parlour order acceptance/rejection process based on ingredient availability. ### 

Lets say the nearest supplier takes t_supply time to replenish the ingredients.
If a customer makes an order consisting of toppings which are not available , I would sem_wait (out the customer to sleep) for t_suply time, after which he would be woken up and allowed to place an order. In the duration of the customer sleeping , if annother customer enters the shop and makes an order, but due to the sleeping customer, the capacity of the shop is full, I would ask the wake the sleeping customer thread and ask him to leave without being served. In the case , the capacity is not full, no change would be made to the sleeping customer thread. This is an optimal strategy considering full capacity as, in the case when the new customer's order's only has toppings which are available, the customer would be served and we would have same number of served customers and in case , when the new customer's order could also not be fulfilled due to shortage of toppings, the previous sleeping customer thread would exit and a new thread would sleep in its place. Thus, we would have same number of unserved customers. Thus, the parlour's reputation would not be affected.


### Unserviced Orders: Suggest ways by which we can avoid/minimise the number of unserviced orders or customers having to wait until the parlor closes. ### 

To minimise the number of unserviced orders I would remove the condition where all toppings of the customer's entire order must be available, instead I wold only check for a particular order. This would reduce the number of unserviced customers and they would still get their partial order. They would wait till all machines close, however, they can leave with those completed orders, instead of nothing.
For the problem of customers waiting till end of parlour closing, my implementation in the first question of total time would reduce the number customers waiting till end of parlour closing by decrementing total time by only the minimum preparation time order of the customer. So, if the minimum preparation time order of a customer can also not be fulfilled due to all machines closing later, they will leave without being served immediately and not have to wait till the parlour closes(all machines stop).

