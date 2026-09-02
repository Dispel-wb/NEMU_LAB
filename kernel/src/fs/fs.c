#include "common.h"
#include <sys/ioctl.h>
#include <string.h>

typedef struct {
	char *name;
	uint32_t size;
	uint32_t disk_offset;
} file_info;

typedef struct {
	bool opened;
	uint32_t offset;
} Fstate;

enum {SEEK_SET, SEEK_CUR, SEEK_END};

/* This is the information about all files in disk. */
static const file_info file_table[] __attribute__((used)) = {
	{"1.rpg", 188864, 1048576}, {"2.rpg", 188864, 1237440},
	{"3.rpg", 188864, 1426304}, {"4.rpg", 188864, 1615168},
	{"5.rpg", 188864, 1804032}, {"abc.mkf", 1022564, 1992896},
	{"ball.mkf", 134704, 3015460}, {"data.mkf", 66418, 3150164},
	{"desc.dat", 16027, 3216582}, {"fbp.mkf", 1128064, 3232609},
	{"fire.mkf", 834728, 4360673}, {"f.mkf", 186966, 5195401},
	{"gop.mkf", 11530322, 5382367}, {"map.mkf", 1496578, 16912689},
	{"mgo.mkf", 1577442, 18409267}, {"m.msg", 188232, 19986709},
	{"mus.mkf", 331284, 20174941}, {"pat.mkf", 8488, 20506225},
	{"rgm.mkf", 453202, 20514713}, {"rng.mkf", 4546074, 20967915},
	{"sss.mkf", 557004, 25513989}, {"voc.mkf", 1997044, 26070993},
	{"wor16.asc", 5374, 28068037}, {"wor16.fon", 82306, 28073411},
	{"word.dat", 5650, 28155717},
};

#define NR_FILES (sizeof(file_table) / sizeof(file_table[0]))

static Fstate file_state[NR_FILES + 3];

int fs_ioctl(int fd, uint32_t request, void *p) {
	assert(request == TCGETS);
	return (fd >= 0 && fd <= 2 ? 0 : -1);
}

void ide_read(uint8_t *, uint32_t, uint32_t);
void ide_write(uint8_t *, uint32_t, uint32_t);
void serial_printc(char);

int fs_open(const char *pathname, int flags) {
	uint32_t i;
	(void)flags;
	while(pathname[0] == '.' && pathname[1] == '/') pathname += 2;

	/* WB的作业，可借鉴，请勿直接复制粘贴 */
	for(i = 0; i < NR_FILES; i ++) {
		if(strcmp(pathname, file_table[i].name) == 0) {
			int fd = i + 3;
			assert(!file_state[fd].opened);
			file_state[fd].opened = true;
			file_state[fd].offset = 0;
			return fd;
		}
	}
	panic("file not found: %s", pathname);
	return -1;
}

uint32_t fs_read(int fd, void *buf, uint32_t len) {
	if(fd >= 0 && fd <= 2) return 0;
	assert(fd >= 3 && fd < (int)(NR_FILES + 3));
	assert(file_state[fd].opened);

	const file_info *file = &file_table[fd - 3];
	uint32_t remain = file->size - file_state[fd].offset;
	if(len > remain) len = remain;
	ide_read(buf, file->disk_offset + file_state[fd].offset, len);
	file_state[fd].offset += len;
	return len;
}

uint32_t fs_write(int fd, const void *buf, uint32_t len) {
	uint32_t i;
	if(fd == 1 || fd == 2) {
		for(i = 0; i < len; i ++) serial_printc(((const char *)buf)[i]);
		return len;
	}
	if(fd == 0) return 0;
	assert(fd >= 3 && fd < (int)(NR_FILES + 3));
	assert(file_state[fd].opened);

	const file_info *file = &file_table[fd - 3];
	uint32_t remain = file->size - file_state[fd].offset;
	if(len > remain) len = remain;
	ide_write((uint8_t *)buf, file->disk_offset + file_state[fd].offset, len);
	file_state[fd].offset += len;
	return len;
}

int32_t fs_lseek(int fd, int32_t offset, int whence) {
	assert(fd >= 3 && fd < (int)(NR_FILES + 3));
	assert(file_state[fd].opened);
	const file_info *file = &file_table[fd - 3];
	int32_t next = 0;

	switch(whence) {
		case SEEK_SET: next = offset; break;
		case SEEK_CUR: next = (int32_t)file_state[fd].offset + offset; break;
		case SEEK_END: next = (int32_t)file->size + offset; break;
		default: panic("invalid lseek whence: %d", whence);
	}
	assert(next >= 0 && next <= (int32_t)file->size);
	file_state[fd].offset = next;
	return next;
}

int fs_close(int fd) {
	if(fd >= 0 && fd <= 2) return 0;
	assert(fd >= 3 && fd < (int)(NR_FILES + 3));
	assert(file_state[fd].opened);
	file_state[fd].opened = false;
	file_state[fd].offset = 0;
	return 0;
}
