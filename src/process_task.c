// Copyright (c) 2012-2019 Matt Campbell
// MIT license (see License.txt)

#include "process_task.h"
#include "bb_array.h"

void task_process_tick(task *_t)
{
	task_process *t = (task_process *)_t->taskData;
	if(t->process) {
		processTickResult_t res = process_tick(t->process);
		if(res.done) {
			if(res.exitCode == 0) {
				task_set_state(_t, kTaskState_Succeeded);
			} else {
				task_set_state(_t, kTaskState_Failed);
			}
		}
	} else {
		task_set_state(_t, kTaskState_Failed);
	}
	task_tick_subtasks(_t);
}

void task_process_statechanged(task *_t)
{
	task_process *t = (task_process *)_t->taskData;
	if(_t->state == kTaskState_Running) {
		t->process = process_spawn(sb_get(&t->dir), sb_get(&t->cmdline), t->spawnType, _t->debug ? kProcessLog_All : kProcessLog_None).process;
		if(!t->process) {
			task_set_state(_t, kTaskState_Failed);
		}
	}
}

void task_process_reset(task *_t)
{
	task_process *t = (task_process *)_t->taskData;
	sb_reset(&t->dir);
	sb_reset(&t->cmdline);
	if(t->process) {
		process_free(t->process);
		t->process = NULL;
	}
	free(_t->taskData);
	_t->taskData = NULL;
}

task process_task_create(const char *name, processSpawnType_t spawnType, const char *dir, const char *cmdlineFmt, ...)
{
	task t = { BB_EMPTY_INITIALIZER };
	sb_append(&t.name, name);
	t.tick = task_process_tick;
	t.stateChanged = task_process_statechanged;
	t.reset = task_process_reset;
	t.taskData = malloc(sizeof(task_process));
	if(t.taskData) {
		task_process *p = t.taskData;
		memset(p, 0, sizeof(*p));
		sb_append(&p->dir, dir);
		va_list args;
		va_start(args, cmdlineFmt);
		sb_va_list(&p->cmdline, cmdlineFmt, args);
		va_end(args);
		p->spawnType = spawnType;
	}
	return t;
}
