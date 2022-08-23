#ifndef FILE_VIEW_H_
#define FILE_VIEW_H_

#include <stdint.h>

struct file_view {
	void *data;
	uint64_t size;
};

/*
 * Creates a file view from the file at pathname.
 * As the name implies, it's a view... read-only.
 * 
 *  0 indicates success.
 * -1 indicates an error.
 */
int file_view_create(const char *pathname, struct file_view *view);

/*
 * Destroys a previously created file_view.
 */
void file_view_destroy(struct file_view *view);

#endif
