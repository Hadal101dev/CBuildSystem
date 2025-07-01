# CBuildSystem
a simple build system for C projects, written in C. Currently only implemented for Windows, but shouldn't be too tough to implement Linux and Mac. Meant for small to medium sized C-based projects without the mental overhead of having to deal with makefiles or batch scripts.

Step 1: Clone repo to directory.

Step 2: Set path in environment variable (so you can use it from command line)

Step 3: From terminal, navigate to project directory and type "cbs init"

Step 4:adjust the build.c template file to match your project. (read below to see how it works)

Step 5: type "cbs compile" to build the build.c file into build.exe

Step 6: run any custom commands you've created from the terminal using "cbs <command> <optional_args>"

======== HOW IT WORKS ============

when you run a command from the terminal "cbs <command> <additional args>" it will first check if it's a system command like "init" or "compile". If it isn't, it will check for the "build.exe" in your project directory. If it exists, then it will run build.exe and pass in the same command and args you typed, allowing them to be handled by the the code in your build.c file. This lets you define custom commands and map them to C functions. (remember that argv[0] will be "cbs", the command itself will be argv[1].

There is some overhead since it's effectively an exe calling into another exe which, when building, calls into the compiler exe thru the command line.

There is no incremental building built-in. you can define custom commands for each module or groups of modules, but it wont automatically check for you unless you code it yourself.

Also you can bypass the cbs command if you want to just type ./build.exe <args>. Set it up to use cbs cuz i'm just that lazy.

Current System Commands...
"init" - creates build.c and build.h files at project directory. Files are copied from cbs.exe directory.
"compile" - compiles the build.c into a build.h with the specified compiler. e.g. "cbs compile <compiler>"
"help" - just links you here lol.

NOTE: the build.h file comes with a utility method for building a compiler_commands.json if you need it. It's included in the build.c template file under the "cc" command.


======== CUSTOM COMMANDS ============

This system uses the CbsCommand struct
```c
typedef struct CbsCommand{
    const char *name , *description;
    void(*fnptr)(const int argc, const char** argv);
}CbsCommand;
```

You can define your own commands and  bind them to functions as long as they follow the signature of returning void and taking in and const int and const char**. 

```c
void do_thing(int argc, char** argv){
    // run custom c code
}

const CbsCommand command_do_thing = {
    .name = "do-thing",
    .description = "does a thing",
    .fnptr = do_thing
}
```

By default the generated build.c file will define and loop thru an array of commands and check to see whether argv[1] matches the command name. If it does it will call the bound function. 

================ BUILD MODULES / PROJECT SETUP ==============

This build system uses the concept of "modules". A module is defined loosely as a group of source files that are combined to produce an output file and any additional flags or compiler arguments associated with that group and output.

this is defined thru the CbsModule struct.
```c
typedef struct CbsModule{
    const char *name;
    const char *compiler;
    const CbsStringArray shared_compiler_flags;
    const CbsStringArray unique_compiler_flags;
    const CbsStringArray shared_include_paths;
    const CbsStringArray unique_include_paths;
    const CbsStringArray shared_library_paths;
    const CbsStringArray unique_library_paths;
    const CbsStringArray shared_linker_flags;
    const CbsStringArray unique_linker_flags;
    const char *source_file_directory;
    const CbsStringArray additional_source_file_paths;
    const CbsStringArray source_files_to_exclude;
    const char *output_file_name_with_extension;
    const char *output_directory;
} CbsModule;
```
a CbsStringArray is just a wrapper around a regular array and adds the length...
```
typedef struct CbsStringArray{
    const char** items;
    const int length;
}CbsStringArray;
```
it can be defined directly...
```c
const CbsStringArray example_cbs_array = {
    .items = (const char*[]){"-Wall","-Wextra"},
    .length = 2 //when defined like this, length must be user defined
};
```
but you'll have to make sure you manually assign the length.

alternatively, you can define a regular array and use the TO_CBS_STRING_ARRAY macro...
```c
const char* example_c_array[] = {
    "-Wall","Wextra"
};
const CbsModule example_module_1 = (CbsModule){
    .shared_compiler_flags = TO_CBS_STRING_ARRAY(example_c_array)
};
```
this automatically calculates the length for you.

When filling out the CbsModule struct certain fields are manditory and others are optional. 
Manditory will have [M] and optional will be have [O].

```c
const CbsModule example_module_2 = (CbsModule){
    .name = "main",                         //[M] name of module, used as id
    .compiler = "clang",                    //[M] compiler exe name (clang, gcc, cl, etc)
    .source_file_directory = NULL,          //[M] relative path. searches recursively in all subfolders
    .output_file_name_with_extension = NULL,//[M]

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
```

compiler flags,include paths, library paths and linker flags all have 2 separate array, one for shared values across modules and one for unique values per module. This is mostly for bigger projects with multiple modules so you don't have to repeat stuff often.

:IMPORTANT: include paths and library paths should NOT include the -I and -L prefixes.
compiler flags and linker flags can include the '-' and '-l' prefixes or leave them out. Doesnt matter.

:IMPORTANT: unique flags for different output types like '-shared' for .dll or .so, must be explicitly included. This system wont detect and add them automatically.


