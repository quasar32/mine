#pragma once

typedef void task(void *);

void jobq_new(void);
void jobq_add(task *fn, void *arg);
void jobq_wait(void);
