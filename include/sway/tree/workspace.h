#ifndef _SWAY_WORKSPACE_H
#define _SWAY_WORKSPACE_H

#include <stdbool.h>
#include "sway/tree/container.h"

struct sway_view;

struct sway_workspace {
	struct sway_container *swayc;
	struct sway_container *fullscreen;
	list_t *floating; // struct sway_container
	list_t *output_priority;
	bool urgent;
};

extern char *prev_workspace_name;

struct sway_container *workspace_get_initial_output(const char *name);

char *workspace_next_name(const char *output_name);

bool workspace_switch(struct sway_container *workspace,
		bool no_auto_back_and_forth);

struct sway_container *workspace_by_number(const char* name);

struct sway_container *workspace_by_name(const char*);

struct sway_container *workspace_output_next(struct sway_container *current);

struct sway_container *workspace_next(struct sway_container *current);

struct sway_container *workspace_output_prev(struct sway_container *current);

struct sway_container *workspace_prev(struct sway_container *current);

bool workspace_is_visible(struct sway_container *ws);

bool workspace_is_empty(struct sway_container *ws);

void workspace_output_raise_priority(struct sway_container *workspace,
		struct sway_container *old_output, struct sway_container *new_output);

void workspace_output_add_priority(struct sway_container *workspace,
		struct sway_container *output);

struct sway_container *workspace_output_get_highest_available(
		struct sway_container *ws, struct sway_container *exclude);

void workspace_detect_urgent(struct sway_container *workspace);

void workspace_for_each_container(struct sway_container *ws,
		void (*f)(struct sway_container *con, void *data), void *data);

struct sway_container *workspace_find_container(struct sway_container *ws,
		bool (*test)(struct sway_container *con, void *data), void *data);

/**
 * Wrap the workspace's tiling children in a new container.
 * The new container will be the only direct tiling child of the workspace.
 * The new container is returned.
 */
struct sway_container *workspace_wrap_children(struct sway_container *ws);

void workspace_add_floating(struct sway_container *workspace,
		struct sway_container *con);

#endif
