Name: Kyle Lukaszek
ID: 1113798

CIS 3090: Parallel Programming
Assignment 1 - Pthread Parallelized Ray Tracer
Due: 09/29/2023

Instructions:
- Running 'make' will compile the 2 programs and create 2 executables, data and task.
- Tested on SOCS Linux Server
-  The data.c program is run with the following command:
    - ./data -t <threads> -s <scale>
    - you can also use -o to output the image to a file
    - Example: ./data -t 2 1
- The task.c program is run with the following command:
    - ./task -t <threads> -s <scale>
    - you can also use -o to output the image to a file
    - Example: ./task -t 2 1

Testing Computer Specifications:

CPU - Intel Core i5-9600k OC @ 4.9GHz, 6 cores, 6 threads
RAM - 32GB DDR4 @ 3600MHz

------------------------------------------------------------

Part 1: Data Parallelization

- Code runs faster as expected when using multiple threads
- Data is divided by columns and each thread is assigned a set of columns to work on
    - See void* render_section(void* arg) for more details
- Compared to ray.c, data.c only uses ~1.0014x more bytes of memory between scales 1-3.


   data.c Results in seconds
    Threads         1        2           3          4          5          6         7          8   
Scale
   1         0.06479 | 0.032990 | 0.031200 | 0.024478 | 0.020606 | 0.019492 | 0.020101 | 0.019757
   2        0.265081 | 0.129768 | 0.123623 | 0.097519 | 0.081076 | 0.075551 | 0.062191 | 0.065859
   3        0.577982 | 0.290299 | 0.280485 | 0.217110 | 0.184134 | 0.175478 | 0.139756 | 0.141348
   4        1.030942 | 0.517376 | 0.499206 | 0.386942 | 0.325692 | 0.298578 | 0.260817 | 0.291409
   5        1.605234 | 0.860174 | 0.776699 | 0.607085 | 0.513213 | 0.468441 | 0.369602 | 0.386147

- The results above show that the program runs faster as the number of threads increases but it seems that the speedup is most obvious when going from 1 to 2 threads. After that, the speedup is not as significant going from 2-6 threads.
- Interestingly, as the scale increases using 7-8 threads is faster than using 6 threads even though my CPU is bound to 6 at a time. The difference between 7-8 is negligible though and can be attributed to variance at lower scales.
- However, at higher scales, it seems as though using 7 threads can result in faster times at points but there is more variance. Using 8 threads never seems to peak as fast as 7 threads but is more consistent over multiple tests and the difference is negligible. 
- Data parallelism is beneficial for ray tracing and is practical at all scales and thread counts. There is a obvious difference in performance between the original code and the data parallelized code at all scales.

------------------------------------------------------------

Part 2: Task Parallelization

- Code runs ridiculously slow when using multiple threads. I imagine this is because of the overhead of creating and joining threads when it is not necessary for such a small scene.
- Ray tracing is also a very sequential process and it is difficult to parallelize the task of ray tracing which could explain why task parallelization is not a good fit.
- I made sure to use mutexes to ensure that the threads were not accessing the same data at the same time.
- I parallelized the closest intersection calculation, the light colour calculations, and the determining of shadows.
    - void* determine_current_sphere(void* arg) 
    - void* calculate_light(void* arg)
    - void* calculate_shadow(void* arg)
- Using anything more than 3 threads would also not make much sense since many of the loops are dependent on the max number of objects in the scene (spheres = 3, and lights = 3) which is 3 in this case. 
    - The code is written in such a way that would handle more objects but for testing purposes I will leave it at 3 threads since it is already incredibly slow.


task.c Results in seconds
    Threads         1          2           3
Scale
   1       26.625797 | 36.628075  | 46.790716  |
   2       99.231613 | 143.084694 | 185.380069 |
   3      224.071913 | 315.898915 | 411.448293 |


- The fastest result always comes from using 1 thread which makes sense since it has the least amount of overhead from creating and joining threads, and is most similar to the provided code in ray.c. 
    - It is still significantly slower than ray.c.
- Using 2 threads is always slower than using 1 thread and using 3 threads is always slower than using 2 threads.
- The results are consistent over multiple tests and the difference is significant between each amount of threads.
- I am not sure if I made a mistake when implementing the algorithms but I made sure to follow the example of the provided code in ray.c and I am not sure how I could have made it faster since I properly implemented pthread creation and joining.
- Task parallelism is not beneficial for ray tracing and is just ridiculously slow and impractical at all scales.