# CBuildSystem
a simple build system for C written in C. Currently only implemented for Windows, but shouldn't be too tough to implement Linux and Mac. Meant for small to medium sized C-based projects without the mental overhead of having to deal with makefiles or batch scripts.

Step 1: Clone repo to directory.

Step 2: Set path in environment variable (so you can use it from command line)

Step 3: From terminal, navigate to project directory and type "cbs init"

Step 4: read and adjust the build.c template file. There's an "examples" section at the top. I'll post below as well for convenience.

Step 5: type "cbs compile" to build the build.c file into build.exe

Step 6: run any custom commands you've created from the terminal


======== CUSTOME COMMANDS ============
This build system lets you define custom commands to call from the terminal. For example you can have a "build" command that will build your project, a "run" command to run the executable, a "recompile-dlls" to only rebuild dll files, etc. You define the custom commands and code them yourself using C in the "build.c" file.

The way it works is that when you type in "cbs <command>", if it's not a hardcoded command like "init" or "compile" it will check to see if a build.exe file exists at the project directory, if it does it will pass the command to the build.exe, which lets you define how to process it in your build.c file. 

You can define CbsCommand structs
```c
typedef struct CbsCommand{
    const char *name , *description;
    void(*fnptr)(const int argc, const char** argv);
}CbsCommand;
```

and bind them to functions

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

By default the generated build.c file will loop thru an array of defined commands and check to see whether argv[1] matches the command name. If it does it will call the bound function. 

================ BUILD MODULES ==============

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

