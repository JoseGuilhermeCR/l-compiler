#include "file_view.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

#include <sys/mman.h>
#include <sys/stat.h>

int file_view_create(const char *pathname, struct file_view *view)
{
	/* Open possible file as READ_ONLY. */
	int fd = open(pathname, O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "Failed to open %s: %s\n", pathname, strerror(errno));
		return -1;
	}

	/* Stat it, check that it's indeed a regular file. */
	struct stat sb;
	if (fstat(fd, &sb) < 0) {
		fprintf(stderr, "Failed to stat %s: %s\n", pathname, strerror(errno));
		goto close_out;
	}

	if (!S_ISREG(sb.st_mode)) {
		fprintf(stderr, "%s is not a regular file.\n", pathname);
		goto close_out;
	}

	assert(sb.st_size >= 0 && "A file size can't be smaller than 0.");
	view->size = (uint64_t)sb.st_size;

	/* Map the file into memory. */
	view->data = mmap(NULL, view->size, PROT_READ, MAP_SHARED, fd, 0);
	if (view->data == MAP_FAILED) {
		fprintf(stderr, "Failed to map file %s: %s\n", pathname, strerror(errno));
		goto close_out;
	}

	/* We may now close the file. */
	close(fd);
	return 0;
close_out:
	close(fd);
	return -1;
}

void file_view_destroy(struct file_view *view)
{
	munmap(view->data, view->size);
	view->data = NULL;
	view->size = 0;
}

