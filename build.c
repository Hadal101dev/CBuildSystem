//#define C_BUILD_IMPLEMENTATION
#include"build.h"
#include<stdio.h>
#include<string.h>

// ==========================================
// ============ MODULE DEFINITION ===========
// ==========================================

const CbsStringArray shared_compiler_flags = {
    .items = (const char*[]){"-Wall","-Wextra"},
    .length = 2
};

const CbsStringArray shared_include_paths = {
    .items = (const char*[]){""},
    .length = 0
};

const CbsStringArray shared_library_paths = {
    .items = (const char*[]){""},
    .length = 0
};

const CbsStringArray shared_linker_flags = {
    .items = (const char*[]){""},
    .length = 0
};

const CbsModule module_main = (CbsModule){
    .name = "main",
    .compiler = "clang", 
    .shared_compiler_flags = shared_compiler_flags, 
    .source_file_directory = "src",
    .output_file_name_with_extension = "main.exe",
};


// ==================================================
// =============== COMMANDS DEFINITION ==============
// ==================================================

void Command_Build_All(const int argc, const char** argv){
    cbs_module_compile(module_main);
}

void Command_CC(const int argc, const char** argv){
    create_compile_commands_json(&module_main, 1);
}

const CbsCommand all_commands[] = {
    (CbsCommand){
    .name = "build-all",
    .description = "builds all the modules",
    .fnptr = Command_Build_All
    },
    (CbsCommand){
        .name = "cc",
        .description = "create the compiler_commands.json",
        .fnptr = Command_CC
    }
};

// ============================================
// ================= MAIN =====================
// ============================================

int main(const int argc, const char **argv){

    if(argc < 2){
        fprintf(stderr,"No arguments passed to build system\n");
        return 1;
    }

    int command_count = sizeof(all_commands)/sizeof(CbsCommand);
    if(command_count == 0){
        fprintf(stderr,"no commands defined\n");
        return 1;
    }

    const char* command_input = argv[1]; 

    for (int i = 0; i < command_count; i++) {
        CbsCommand command = all_commands[i];
        if(strcmp(command.name,command_input) == 0){
            command.fnptr(argc,argv);
            return 0;
        }
    }

    fprintf(stderr,"command [%s] not defined in build script.\n",command_input);
    fprintf(stdout,"available commands are...\n");
    for (int i = 0; i<command_count; i++) {
        CbsCommand command = all_commands[i];
        const char* description;
        if(command.description == NULL) description = "no description";
        else description = command.description;

        fprintf(stdout,"[%s] - %s\n\n",command.name,description);
    }

    return 0;
}

