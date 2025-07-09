#include "multithreading.h"
#include <stdlib.h>
#include <pthread.h>

/**
 * create_task - Allocates and initializes a task
 * @entry: Task function
 * @param: Parameter to pass to the task
 * Return: Pointer to the new task
 */
task_t *create_task(task_entry_t entry, void *param)
{
	task_t *task = malloc(sizeof(task_t));

	if (!task)
		return (NULL);

	task->entry = entry;
	task->param = param;
	task->status = PENDING;
	task->result = NULL;

	if (pthread_mutex_init(&task->lock, NULL) != 0)
	{
		free(task);
		return (NULL);
	}

	return task;
}

/**
 * destroy_task - Frees memory associated with a task
 * @task: Task to destroy
 */
void destroy_task(task_t *task)
{
	if (!task)
		return;

	pthread_mutex_destroy(&task->lock);

	if (task->result)
		list_destroy((list_t *)task->result, free);

	free(task);
}

/**
 * exec_tasks - Thread entry point to execute a list of tasks
 * @tasks: Pointer to list of task_t*
 * Return: NULL
 */
void *exec_tasks(list_t const *tasks)
{
	node_t *node;

	if (!tasks)
		return (NULL);

	// Loop until no PENDING tasks are found
	while (1)
	{
		int found_pending = 0;

		for (node = tasks->head; node != NULL; node = node->next)
		{
			task_t *task = (task_t *)node->content;

			pthread_mutex_lock(&task->lock);
			if (task->status == PENDING)
			{
				task->status = STARTED;
				pthread_mutex_unlock(&task->lock);

				tprintf("[%lu] [%02lu] Started\n",
				        pthread_self(), node->index);

				void *result = task->entry(task->param);

				pthread_mutex_lock(&task->lock);
				task->result = result;
				task->status = (result != NULL) ? SUCCESS : FAILURE;
				pthread_mutex_unlock(&task->lock);

				tprintf("[%lu] [%02lu] %s\n",
				        pthread_self(), node->index,
				        (result != NULL) ? "Success" : "Failure");
			}
			else
			{
				pthread_mutex_unlock(&task->lock);
			}

			// Check again if other tasks might still be pending
			if (task->status == PENDING)
				found_pending = 1;
		}

		if (!found_pending)
			break; // Exit if no tasks are left to process
	}

	return (NULL);
}
