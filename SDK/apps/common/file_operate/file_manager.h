#ifndef __FILE_MANAGER_H__
#define __FILE_MANAGER_H__

#include "system/includes.h"
#include "typedef.h"
#include "system/fs/fs.h"

FILE *file_manager_select(struct vfscan *fs, int sel_mode, int arg);

#endif// __FILE_MANAGER_H__
