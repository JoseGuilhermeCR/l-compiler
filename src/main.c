#include "file_view.h"

#include <stdio.h>

static void print_usage(const char *arg0)
{
	fprintf(stderr, "Usage: %s <file_to_compile>\n", arg0);
}

int main(int argc, const char **argv)
{
	if (argc < 2) {
		print_usage(argv[0]);
		return -1;
	}

	const char *pathname = argv[1];

	struct file_view view;
	if (file_view_create(pathname, &view) < 0)
		return -1;

	file_view_destroy(&view);
	return 0;
}
