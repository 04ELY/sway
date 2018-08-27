#define _POSIX_C_SOURCE 200809L
#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_output_layout.h>
#include "sway/tree/arrange.h"
#include "sway/tree/container.h"
#include "sway/output.h"
#include "sway/tree/workspace.h"
#include "sway/tree/view.h"
#include "list.h"
#include "log.h"

static void apply_horiz_layout(struct sway_container *parent) {
	size_t num_children = parent->children->length;
	if (!num_children) {
		return;
	}
	size_t parent_offset = 0;
	if (parent->parent->layout == L_TABBED) {
		parent_offset = container_titlebar_height();
	} else if (parent->parent->layout == L_STACKED) {
		parent_offset = container_titlebar_height() *
			parent->parent->children->length;
	}
	size_t parent_height = parent->height - parent_offset;

	// Calculate total width of children
	double total_width = 0;
	for (size_t i = 0; i < num_children; ++i) {
		struct sway_container *child = parent->children->items[i];
		if (child->width <= 0) {
			if (num_children > 1) {
				child->width = parent->width / (num_children - 1);
			} else {
				child->width = parent->width;
			}
		}
		container_remove_gaps(child);
		total_width += child->width;
	}
	double scale = parent->width / total_width;

	// Resize windows
	wlr_log(WLR_DEBUG, "Arranging %p horizontally", parent);
	double child_x = parent->x;
	for (size_t i = 0; i < num_children; ++i) {
		struct sway_container *child = parent->children->items[i];
		wlr_log(WLR_DEBUG,
				"Calculating arrangement for %p:%d (will scale %f by %f)",
				child, child->type, child->width, scale);
		child->x = child_x;
		child->y = parent->y + parent_offset;
		child->width = floor(child->width * scale);
		child->height = parent_height;
		child_x += child->width;

		// Make last child use remaining width of parent
		if (i == num_children - 1) {
			child->width = parent->x + parent->width - child->x;
		}
		container_add_gaps(child);
	}
}

static void apply_vert_layout(struct sway_container *parent) {
	size_t num_children = parent->children->length;
	if (!num_children) {
		return;
	}
	size_t parent_offset = 0;
	if (parent->parent->layout == L_TABBED) {
		parent_offset = container_titlebar_height();
	} else if (parent->parent->layout == L_STACKED) {
		parent_offset =
			container_titlebar_height() * parent->parent->children->length;
	}
	size_t parent_height = parent->height + parent_offset;

	// Calculate total height of children
	double total_height = 0;
	for (size_t i = 0; i < num_children; ++i) {
		struct sway_container *child = parent->children->items[i];
		if (child->height <= 0) {
			if (num_children > 1) {
				child->height = parent_height / (num_children - 1);
			} else {
				child->height = parent_height;
			}
		}
		container_remove_gaps(child);
		total_height += child->height;
	}
	double scale = parent_height / total_height;

	// Resize
	wlr_log(WLR_DEBUG, "Arranging %p vertically", parent);
	double child_y = parent->y + parent_offset;
	for (size_t i = 0; i < num_children; ++i) {
		struct sway_container *child = parent->children->items[i];
		wlr_log(WLR_DEBUG,
				"Calculating arrangement for %p:%d (will scale %f by %f)",
				child, child->type, child->height, scale);
		child->x = parent->x;
		child->y = child_y;
		child->width = parent->width;
		child->height = floor(child->height * scale);
		child_y += child->height;

		// Make last child use remaining height of parent
		if (i == num_children - 1) {
			child->height =
				parent->y + parent_offset + parent_height - child->y;
		}
		container_add_gaps(child);
	}
}

static void apply_tabbed_or_stacked_layout(struct sway_container *parent) {
	if (!parent->children->length) {
		return;
	}
	size_t parent_offset = 0;
	if (parent->parent->layout == L_TABBED) {
		parent_offset = container_titlebar_height();
	} else if (parent->parent->layout == L_STACKED) {
		parent_offset =
			container_titlebar_height() * parent->parent->children->length;
	}
	size_t parent_height = parent->height - parent_offset;
	for (int i = 0; i < parent->children->length; ++i) {
		struct sway_container *child = parent->children->items[i];
		container_remove_gaps(child);
		child->x = parent->x;
		child->y = parent->y + parent_offset;
		child->width = parent->width;
		child->height = parent_height;
		container_add_gaps(child);
	}
}

static void arrange_children_of(struct sway_container *parent);

static void arrange_floating(list_t *floating) {
	for (int i = 0; i < floating->length; ++i) {
		struct sway_container *floater = floating->items[i];
		if (floater->type == C_VIEW) {
			view_autoconfigure(floater->sway_view);
		} else {
			arrange_children_of(floater);
		}
		container_set_dirty(floater);
	}
}

static void arrange_children_of(struct sway_container *parent) {
	if (config->reloading) {
		return;
	}
	wlr_log(WLR_DEBUG, "Arranging layout for %p %s %fx%f+%f,%f", parent,
		parent->name, parent->width, parent->height, parent->x, parent->y);

	// Calculate x, y, width and height of children
	switch (parent->layout) {
	case L_HORIZ:
		apply_horiz_layout(parent);
		break;
	case L_VERT:
		apply_vert_layout(parent);
		break;
	case L_TABBED:
	case L_STACKED:
		apply_tabbed_or_stacked_layout(parent);
		break;
	case L_NONE:
		apply_horiz_layout(parent);
		break;
	}

	// Recurse into child containers
	for (int i = 0; i < parent->children->length; ++i) {
		struct sway_container *child = parent->children->items[i];
		if (parent->has_gaps && !child->has_gaps) {
			child->has_gaps = true;
			child->gaps_inner = parent->gaps_inner;
			child->gaps_outer = parent->gaps_outer;
		}
		if (child->type == C_VIEW) {
			view_autoconfigure(child->sway_view);
		} else {
			arrange_children_of(child);
		}
		container_set_dirty(child);
	}
}

static void arrange_workspace(struct sway_container *workspace) {
	if (config->reloading) {
		return;
	}
	struct sway_container *output = workspace->parent;
	struct wlr_box *area = &output->sway_output->usable_area;
	wlr_log(WLR_DEBUG, "Usable area for ws: %dx%d@%d,%d",
			area->width, area->height, area->x, area->y);
	workspace_remove_gaps(workspace);

	double prev_x = workspace->x;
	double prev_y = workspace->y;
	workspace->width = area->width;
	workspace->height = area->height;
	workspace->x = output->x + area->x;
	workspace->y = output->y + area->y;

	// Adjust any floating containers
	double diff_x = workspace->x - prev_x;
	double diff_y = workspace->y - prev_y;
	if (diff_x != 0 || diff_y != 0) {
		for (int i = 0; i < workspace->sway_workspace->floating->length; ++i) {
			struct sway_container *floater =
				workspace->sway_workspace->floating->items[i];
			container_floating_translate(floater, diff_x, diff_y);
			double center_x = floater->x + floater->width / 2;
			double center_y = floater->y + floater->height / 2;
			struct wlr_box workspace_box;
			container_get_box(workspace, &workspace_box);
			if (!wlr_box_contains_point(&workspace_box, center_x, center_y)) {
				container_floating_move_to_center(floater);
			}
		}
	}

	workspace_add_gaps(workspace);
	container_set_dirty(workspace);
	wlr_log(WLR_DEBUG, "Arranging workspace '%s' at %f, %f", workspace->name,
			workspace->x, workspace->y);
	if (workspace->sway_workspace->fullscreen) {
		struct sway_container *fs = workspace->sway_workspace->fullscreen;
		fs->x = workspace->parent->x;
		fs->y = workspace->parent->y;
		fs->width = workspace->parent->width;
		fs->height = workspace->parent->height;
		if (fs->type == C_VIEW) {
			view_autoconfigure(fs->sway_view);
		} else {
			arrange_children_of(fs);
		}
		container_set_dirty(fs);
	} else {
		arrange_floating(workspace->sway_workspace->floating);
		arrange_children_of(workspace);
	}
}

static void arrange_output(struct sway_container *output) {
	if (config->reloading) {
		return;
	}
	const struct wlr_box *output_box = wlr_output_layout_get_box(
			root_container.sway_root->output_layout,
			output->sway_output->wlr_output);
	output->x = output_box->x;
	output->y = output_box->y;
	output->width = output_box->width;
	output->height = output_box->height;
	container_set_dirty(output);
	wlr_log(WLR_DEBUG, "Arranging output '%s' at %f,%f",
			output->name, output->x, output->y);
	for (int i = 0; i < output->children->length; ++i) {
		struct sway_container *workspace = output->children->items[i];
		arrange_workspace(workspace);
	}
}

static void arrange_root() {
	if (config->reloading) {
		return;
	}
	struct wlr_output_layout *output_layout =
		root_container.sway_root->output_layout;
	const struct wlr_box *layout_box =
		wlr_output_layout_get_box(output_layout, NULL);
	root_container.x = layout_box->x;
	root_container.y = layout_box->y;
	root_container.width = layout_box->width;
	root_container.height = layout_box->height;
	container_set_dirty(&root_container);
	for (int i = 0; i < root_container.children->length; ++i) {
		struct sway_container *output = root_container.children->items[i];
		arrange_output(output);
	}
}

void arrange_windows(struct sway_container *container) {
	switch (container->type) {
	case C_ROOT:
		arrange_root();
		break;
	case C_OUTPUT:
		arrange_output(container);
		break;
	case C_WORKSPACE:
		arrange_workspace(container);
		break;
	case C_CONTAINER:
		arrange_children_of(container);
		container_set_dirty(container);
		break;
	case C_VIEW:
		view_autoconfigure(container->sway_view);
		container_set_dirty(container);
		break;
	case C_TYPES:
		break;
	}
}
