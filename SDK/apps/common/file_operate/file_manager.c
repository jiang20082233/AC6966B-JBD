#include "file_manager.h"
#include "app_config.h"

FILE *file_manager_select(struct vfscan *fs, int sel_mode, int arg)
{
    return fselect(fs, sel_mode, arg);
}


