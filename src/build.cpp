// Copyright Seong Woo Lee. All Rights Reserved.

#include "basic/log.h"
#include "shared.h"
#include "os/os.h"
#include "material/codegen.h"

int main_entry(int argc, char **argv)
{
    shared_init();

    material_codegen(shared->shader_compiler, 
                     tprint(S("%Sshaders/material/"), shared->data_path), 
                     tprint(S("%Sgenerated/"), shared->data_path));

    shared_shutdown();

    log_info(S("Done."));
    return 0;
}
