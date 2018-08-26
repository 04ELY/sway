#include <string.h>
#include <strings.h>
#include "sway/commands.h"
#include "sway/input/seat.h"
#include "sway/ipc-server.h"
#include "sway/output.h"
#include "sway/tree/arrange.h"
#include "sway/tree/container.h"
#include "sway/tree/view.h"
#include "sway/tree/workspace.h"
#include "list.h"

struct cmd_results *cmd_floating(int argc, char **argv) {
	struct cmd_results *error = NULL;
	if ((error = checkarg(argc, "floating", EXPECTED_EQUAL_TO, 1))) {
		return error;
	}
	struct sway_container *container =
		config->handler_context.current_container;
	if (container->type == C_WORKSPACE && container->children->length == 0) {
		return cmd_results_new(CMD_INVALID, "floating",
				"Can't float an empty workspace");
	}
	if (container->type == C_WORKSPACE) {
		// Wrap the workspace's children in a container so we can float it
		struct sway_container *workspace = container;
		container = workspace_wrap_children(container);
		workspace->layout = L_HORIZ;
		seat_set_focus(config->handler_context.seat, container);
	}

	// If the container is in a floating split container,
	// operate on the split container instead of the child.
	if (container_is_floating_or_child(container)) {
		while (container->parent->type != C_WORKSPACE) {
			container = container->parent;
		}
	}

	bool wants_floating;
	if (strcasecmp(argv[0], "enable") == 0) {
		wants_floating = true;
	} else if (strcasecmp(argv[0], "disable") == 0) {
		wants_floating = false;
	} else if (strcasecmp(argv[0], "toggle") == 0) {
		wants_floating = !container_is_floating(container);
	} else {
		return cmd_results_new(CMD_FAILURE, "floating",
			"Expected 'floating <enable|disable|toggle>'");
	}

	container_set_floating(container, wants_floating);

	struct sway_container *workspace = container_parent(container, C_WORKSPACE);
	arrange_windows(workspace);

	return cmd_results_new(CMD_SUCCESS, NULL, NULL);
}
