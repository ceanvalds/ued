#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#include "ued.h"
#include "config.h"

extern struct editorConfig E;

/*** file i/o ***/

char* editorRowsToString(int* buflen)
{
	int totlen = 0;
	int j;
	for(j = 0; j < E.numrows; j++) {
		totlen += E.row[j].size + 1;
	}
	*buflen = totlen;

	char* buf = malloc(totlen);
	char* p = buf;
	for(j = 0; j < E.numrows; j++) {
		memcpy(p, E.row[j].chars, E.row[j].size);
		p += E.row[j].size;
		*p = '\n';
		p++;
	}

	return buf;
}

void editorOpen(char* filename)
{
	if(E.filename != NULL) {
		free(E.filename);
	}

	E.filename = strdup(filename);

	FILE* fp = fopen(filename, "r");
	if(!fp) {
		fp = fopen(filename, "w");
		if(!fp) {
			perror("Failed to open or create file");
			return;
		}
	}

	char* line = NULL;
	size_t linecap = 0;
	ssize_t linelen;
	while((linelen = getline(&line, &linecap, fp)) != -1) {
		while(linelen > 0 && (line[linelen - 1] == '\n' || line[linelen - 1] == '\r')) {
			linelen--;
		}

		editorInsertRow(E.numrows, line, linelen);
	}

	free(line);
	fclose(fp);

	E.dirty = 0;
}

void editorSave()
{
	if(E.filename == NULL) {
		E.filename = editorPrompt("Save as: %s (ESC to cancel)");
		if(E.filename == NULL) {
			editorSetStatusMessage("Save aborted");
			return;
		}
	}

	int len;
	char* buf = editorRowsToString(&len);

	int fd = open(E.filename, O_RDWR | O_CREAT, 0644);
	if(fd != -1) {
		if(ftruncate(fd, len) != -1) {
			if(write(fd, buf, len) == len) {
				close(fd);
				free(buf);
				E.dirty = 0;
				editorSetStatusMessage("%d bytes written to disk", len);
				return;
			}
		}
		close(fd);
	}

	free(buf);
	editorSetStatusMessage("Can't save! I/O error: %s", strerror(errno));
}

