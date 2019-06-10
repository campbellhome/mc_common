// Copyright (c) 2012-2019 Matt Campbell
// MIT license (see License.txt)

#pragma once

#include "common.h"
#include "sdict.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef u32 taskId;
static const taskId kTaskIdInvalid = 0;
typedef struct tag_task task;

void tasks_startup(void);
void tasks_shutdown(void);
void tasks_tick(void);

typedef enum tag_taskState {
	kTaskState_Pending,
	kTaskState_Running,
	kTaskState_Succeeded,
	kTaskState_Failed,
	kTaskState_Canceled,
	kTaskState_Count
} taskState;

typedef void(Task_Tick)(task *t);
typedef void(Task_StateChanged)(task *);
typedef void(Task_Reset)(task *);

void task_tick_subtasks(task *t);

typedef struct tag_tasks {
	u32 count;
	u32 allocated;
	task *data;
} tasks;

typedef struct tag_task {
	sb_t name;
	sb_t prefix;
	taskId id;
	taskState state;
	taskState prevState;
	b32 parallel;
	b32 debug;
	u8 pad[4];
	tasks subtasks;
	task *parent;
	void *userdata;
	sdict_t extraData;
	Task_Tick *tick;
	Task_StateChanged *stateChanged;
	Task_Reset *reset;
} task;

task *task_queue(task t);
task *task_queue_subtask(task *parent, task t);
void task_discard(task t);
void task_set_state(task *t, taskState state);
b32 task_started(task *t);
b32 task_done(task *t);

#if defined(__cplusplus)
}
#endif
