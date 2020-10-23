#include <task.h>
#include <include/kernel/sched.h>

struct spinlock oom_lock;

extern struct task **tasks;
extern pid_t pid_max;

void oom_init() {
	spin_init(&oom_lock, "oom_lock");
}

static int badness_score(struct task *task) {
	int score = 0;

	if (task->task_type == TASK_TYPE_USER) {
		score = task->nactive_pages + task->nswapped_pages;
	}

	return score;
}

int oom_kill() {
	int worst_score = 0, score = 0;
	struct task *worst_task = NULL;

	spin_lock(&oom_lock);

	for (pid_t pid = 0; pid < pid_max; ++pid) {
		if (tasks[pid]) {
			 score = badness_score(tasks[pid]);
			 if (score > worst_score) {
			 	worst_score = score;
			 	worst_task = tasks[pid];
			 }
		}
	}

	if (worst_task != NULL) {
		task_destroy(worst_task);
		spin_unlock(&oom_lock);
		return 0;
	}

	spin_unlock(&oom_lock);
	return -1;
}