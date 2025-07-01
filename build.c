//#define C_BUILD_IMPLEMENTATION
#include"build.h"
#include<stdio.h>
#include<string.h>

#ifdef _C_BUILD_EXAMPLES
// ==========================================
// ================ EXAMPLES ================
// ==========================================


//[NOTE] CbsStringArray groups the actual array and the length
const CbsStringArray example_cbs_array = {
    .items = (const char*[]){"-Wall","-Wextra"},
    .length = 2 //when defined like this, length must be user defined
};

//[NOTE] You can also just define a regular array and use
//       the TO_CBS_STRING_ARRAY macro, which calculates the length for you
const char* example_c_array[] = {
    "-Wall","Wextra"
};
const CbsModule example_module_1 = (CbsModule){
    .shared_compiler_flags = TO_CBS_STRING_ARRAY(example_c_array)
};


//[NOTE] This build system uses the concept of "modules". A module is defined loosely as
        // a group of source files that are combined to produce an output file and
        // any additional flags or compiler arguments associated with that group and output


//[NOTE] The following is an example of the CbsModule struct with its fields
//[NOTE] fields with the [M] are manditory, [O] are optional
const CbsModule example_module_2 = (CbsModule){
    .name = "main",                         //[M] name of module, used as id
    .compiler = "clang",                    //[M] compiler exe name (clang, gcc, cl, etc)
    .source_file_directory = NULL,          //[M] where to search for c files (recursive)
    .output_file_name_with_extension = NULL,//[M] 
    
    //[NOTE] when making a .dll or .so, it's currently up to the user to 
            //remember to add required flags like "-shared"

    //[NOTE] there's shared and unique arrays for compiler flags, include paths,
    //      library paths and linker flags. Just so you dont have to retype the
    //      same values if they're shared amoungst modules

    //[NOTE] in compiler flags the "-" is optional,
            //in linker flags the "-l" is optional,
            //for include and library paths, do NOT add the "-I" or "-L"

    .shared_compiler_flags = NULL,          //[O]
    .unique_compiler_flags = NULL,          //[O]

    .shared_include_paths = NULL,           //[O]
    .unique_include_paths = NULL,           //[O]

    .shared_library_paths = NULL,           //[O]
    .unique_library_paths = NULL,           //[O]

    .shared_linker_flags = NULL,            //[O]
    .unique_linker_flags = NULL,            //[O]

    .additional_source_file_paths = NULL,   //[O] for additional files that arent in src folder. absolute paths.
    .source_files_to_exclude = NULL,        //[O] to exclude c files in source folder, just file names with extension
    .output_directory = NULL,               //[O] Use relative path. if null, its the root directory. 
};



//[INFO] you can define your own command line commands and bind them to custom functions
        //Use CbsCommand structs to define your custom commands...



//[INFO] Function to be run when command is called, must have these const args 
void Command_Do_Thing(const int argc, const char** argv){
    // Do stuff here
}
const CbsCommand example_command = (CbsCommand){
    //[INFO] The name is what you would type in the command line to run it
    .name = "do-thing",
    //[INFO] Optional description for printing available commands with descriptions
    .description = "do some thing",
    //[INFO] The function pointer to the function that should get run
    .fnptr = Command_Do_Thing
};

#endif



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

