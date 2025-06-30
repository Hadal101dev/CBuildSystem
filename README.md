# CBuildSystem
a simple build system for C written in C. Currently only implemented for Windows, but shouldn't be too tough to implement Linux and Mac. Meant for small to medium sized C-based projects without the mental overhead of having to deal with makefiles or batch scripts.

Step 1: Clone repo to directory.

Step 2: Set path in environment variable (so you can use it from command line)

Step 3: From terminal, navigate to project directory and type "cbs init"

Step 4: read and adjust the build.c template file. There's an "examples" section at the top. I'll post below as well for convenience.

Step 5: type "cbs compile" to build the build.c file into build.exe

Step 6: run any custom commands you've created from the terminal


This build system lets you define custom commands to call from the terminal. For example you can have a "build" command that will build your project, a "run" command to run the executable, a "recompile-dlls" to only rebuild dll files, etc. You define the custom commands and code them yourself using C in the "build.c" file.

When you run "cbs init" it also clones a build.h header only library for handling project building and command definitions. It's currently only set up for windows because i don't have a linux or mac to test it on, but there's only like 3 OS specific functions thats shouldn't be too hard to implement if needed.

When cloning the build.h and build.c files, it just copies whats in the same directory as the cbs.exe, so you can alter those templates if you want different default scripts.

Here's some examples of how the build.c file works. The following text can also be found in the build.c file when it's created...
```c
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
```


