// Step 1: Include necessary libraries and define constants
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <queue>

#define NUM_CHAIRS 3
#define NUM_STUDENTS 5

// Step 2: Declare global variables for mutex and semaphores, and a variable for the number of waiting students.
pthread_mutex_t ta_mutex;
pthread_mutex_t ta_done_mutex;
sem_t hallway_chairs;
sem_t wake_ta;
sem_t ta_done; // semaphore for students to signal they are done
pthread_cond_t waiting_students_cond;
pthread_cond_t student_done_cond[NUM_STUDENTS];
int waiting_students = 0;
int ta_busy = 0;
int helping_student_id = -1;
std::queue<int> waiting_students_queue;

// Step 3: Declare functions for TA and student threads.
void *ta_thread(void *param);
void *student_thread(void *param);

// Step 4: Implement the TA thread function.
void *ta_thread(void *param)
{
    while (1)
    {
        sem_wait(&wake_ta);
        pthread_mutex_lock(&ta_mutex);

        while (waiting_students > 0)
        {
            waiting_students--;
            ta_busy = 1;
        
            // Dequeue the next waiting student
            helping_student_id = waiting_students_queue.front();
            waiting_students_queue.pop();

            sem_post(&hallway_chairs);
            pthread_mutex_unlock(&ta_mutex);

            sleep(rand() % 5 + 1);
            printf("TA finished helping student %d.\n", helping_student_id);

            pthread_mutex_lock(&ta_done_mutex);
            pthread_cond_signal(&student_done_cond[helping_student_id]);
            pthread_mutex_unlock(&ta_done_mutex);

            // Check if there are any waiting students and help them one by one
            pthread_mutex_lock(&ta_mutex);
        }
        ta_busy = 0;
        printf("TA is napping.\n");
        pthread_mutex_unlock(&ta_mutex);
        sleep(rand() % 10 + 1);
    }
}

// Step 5: Implement the student thread function.
void *student_thread(void *param)
{
    int student_id = *((int *)param);
    while (1)
    {
        printf("Student %d is programming.\n", student_id);
        sleep(rand() % 10 + 1); // Simulate programming
        pthread_mutex_lock(&ta_mutex);
        if (!ta_busy && waiting_students < NUM_CHAIRS)
        {
            printf("Student %d needs help and wakes up the TA.\n", student_id);
            ta_busy = 1; // Set the TA as busy
            waiting_students++;

            // Enqueue the student
            waiting_students_queue.push(student_id);

            sem_post(&wake_ta); // Wake up the TA

            // printf("Student %d is getting help.\n", student_id);

            pthread_mutex_unlock(&ta_mutex);

            // Wait for the TA to signal that they are done helping this student
            pthread_mutex_lock(&ta_done_mutex);
            pthread_cond_wait(&student_done_cond[student_id], &ta_done_mutex);
            printf("Student %d has received help and is leaving.\n", student_id);
            pthread_mutex_unlock(&ta_done_mutex);
        }
        else if (waiting_students < NUM_CHAIRS)
        {
            sem_wait(&hallway_chairs);
            waiting_students++;
            waiting_students_queue.push(student_id);
            printf("Student %d is waiting in the hallway.\n", student_id);

            pthread_mutex_unlock(&ta_mutex);

            // Wait for the TA to signal that they are done helping this student
            pthread_mutex_lock(&ta_done_mutex);
            pthread_cond_wait(&student_done_cond[student_id], &ta_done_mutex);
            printf("Student %d has received help and is leaving.\n", student_id);
            pthread_mutex_unlock(&ta_done_mutex);
        }
        else
        {
            printf("Student %d will come back later (no chairs available).\n", student_id);
            pthread_mutex_unlock(&ta_mutex);
        }
    }
}

int main()
{
    srand(time(NULL));

    pthread_mutex_init(&ta_mutex, NULL);
    pthread_mutex_init(&ta_done_mutex, NULL);
    pthread_cond_init(&waiting_students_cond, NULL);
    for (int i = 0; i < NUM_STUDENTS; i++)
    {
        pthread_cond_init(&student_done_cond[i], NULL);
    }

    sem_init(&hallway_chairs, 0, NUM_CHAIRS);
    sem_init(&wake_ta, 0, 0);
    sem_init(&ta_done, 0, 0);

    pthread_t ta;
    pthread_create(&ta, NULL, ta_thread, NULL);

    pthread_t students[NUM_STUDENTS];
    int student_ids[NUM_STUDENTS];

    for (int i = 0; i < NUM_STUDENTS; i++)
    {
        student_ids[i] = i ;
        pthread_create(&students[i], NULL, student_thread, (void *)&student_ids[i]);
    }

    for (int i = 0; i < NUM_STUDENTS; i++)
    {
        pthread_join(students[i], NULL);
    }

    return 0;
}