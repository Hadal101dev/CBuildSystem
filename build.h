#ifndef _C_BUILD_H_
#define _C_BUILD_H_


#include <stddef.h>
#include<stdint.h>

//==================================
//======== PUBLIC API ==============
//==================================
typedef struct CbsStringArray{
    const char** items;
    const int length;
}CbsStringArray;

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

typedef struct CbsCommand{
    const char *name , *description;
    void(*fnptr)(const int argc, const char** argv);
}CbsCommand;

#define TO_CBS_STRING_ARRAY(string_array) (CbsStringArray){.items = string_array,.length = sizeof(string_array)/sizeof(char*)}
void cbs_module_compile(CbsModule module);

void cbs_command_run_matching(
    const int argc,
    const char **argv,
    const int command_count,
    const CbsCommand *command_array
);
void create_compile_commands_json(const CbsModule *module_array,const int array_length);

#endif


// ============================================================================
// ============================================================================
//
//              IMPLEMENTATION SECTION
//
// ============================================================================
// ============================================================================

#define CBUILD_IMPLEMENTATION 

#ifdef CBUILD_IMPLEMENTATION
#include <stdarg.h>

#include <stdbool.h>
#include<stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <ctype.h>

static void cbs_log_error(const char* format,...){
    va_list args;
    va_start(args,format);
    fprintf(stderr,"[ERROR] ");
    vfprintf(stderr,format,args);
    fprintf(stderr,"\n");
    va_end(args);
}

static void string_replace_all_char(char* string, char target, char replacement){
    int i = 0;
    while(string[i] != '\0'){
        if(string[i] == target){
            string[i] = replacement;
        }
        i++;
    }
}

static bool string_is_null_empty_or_whitespace(const char* string){
    if(string == NULL) return true;
    if(string[0] == '\0') return true;
    
    uint32_t i = 0;
    while(string[i]!= '\0' && i < 1024){
        char c = string[i];
        if(!isspace(c)){
            return false;
        }        
    }
    return true;
}

static bool string_starts_with(const char* string, const char* prefix){
    int result = memcmp(string,prefix,strlen(prefix));
    if(result == 0)return true;
    else return false;
}

static bool string_ends_with_string(const char* string, const char* suffix){
    size_t suffix_length = strlen(suffix);
    size_t string_length = strlen(string);
    if(suffix_length > string_length) return false;

    size_t index = string_length - suffix_length;
    if(memcmp(&string[index],suffix,suffix_length) == 0) return true;
    else return false;
}
static bool string_ends_with_char(const char* string, const char suffix){
    size_t string_length = strlen(string);
    return string[string_length - 1] == suffix;
}

static bool buffer_append_string(char* buffer,uint32_t buffer_capacity,const char* string){
    size_t string_length = strlen(string);
    size_t buffer_length = strlen(buffer);
    if(buffer_length + string_length > buffer_capacity -1)//-1 for null terminator
    {
        cbs_log_error("no room in buffer for string");
        return false;
    }

    memcpy(
        &buffer[buffer_length],
        string,
        string_length
    );

    buffer[buffer_length+ string_length] = '\0';
    return true;
}

static bool buffer_append_char(char* buffer,uint32_t buffer_capacity,const char character){
    size_t buffer_length = strlen(buffer);
    if(buffer_length + 1 > buffer_capacity -1)//-1 for null terminator
    {
        cbs_log_error("no room in buffer for string");
        return false;
    }

    buffer[buffer_length] = character;
    buffer[buffer_length+1] = '\0';
    
    return true;
}

static void buffer_remove_characters_from_end(char* buffer,size_t character_count){
    size_t buffer_length = strlen(buffer);
    if(character_count >= buffer_length){
        buffer[0] = '\0';
    }else{
        size_t index = buffer_length-character_count;
        buffer[index] = '\0';
    }
}

static bool is_c_file(const char *filename){
    size_t filename_length = strlen(filename);
    if(filename_length <3) return false;

    if(filename[filename_length-2] == '.'
    && filename[filename_length-1] == 'c'){
        return true;
    }

    return false;
}

#ifdef _WIN32
#define FILE_PATH_MAX 260
#define FILE_SEPARATOR '\\'
#include<windows.h>
#elif defined(__linux__)
#define FILE_PATH_MAX 4096
#define FILE_SEPARATOR '/'
#include<unistd.h>
#include<dirent.h>
#elif defined(__APPLE__)
#include<mach-o/dyld.h>
#include<dirent.h>
#define FILE_PATH_MAX 1024
#define FILE_SEPARATOR '/'
#endif

#define COMMAND_BUFFER_SIZE 65535

static void remove_filename_from_path(char* path_buffer,uint32_t path_length){
    for (uint32_t i = path_length-1; i >= 0; i--) {
        char c = path_buffer[i];
        if(c == FILE_SEPARATOR){
            path_buffer[i+1] = '\0';
            return;
        }
    }
}

#ifdef _WIN32
static bool try_get_program_path(char* path_buffer){
    DWORD length = GetModuleFileNameA(NULL,path_buffer,FILE_PATH_MAX);
    if(length == 0 || length > FILE_PATH_MAX){
        return false;
    }
    path_buffer[length] = '\0';
    remove_filename_from_path(path_buffer,length);
    return true;
}

static void add_files_recursive_from_source_directory(char* search_path, char* command_buffer,CbsStringArray files_to_exclude){
    WIN32_FIND_DATAA find_data;
    const char wildcard = '*';
    buffer_append_char(search_path,FILE_PATH_MAX,wildcard);
    HANDLE hFind = FindFirstFileA(search_path,&find_data);
    if(hFind == INVALID_HANDLE_VALUE){
        fprintf(stderr,"couldnt find initial files with path [%s]",search_path);
        buffer_remove_characters_from_end(search_path,1);
        return;
    }
    buffer_remove_characters_from_end(search_path,1);

    do{
        if(find_data.dwFileAttributes == INVALID_FILE_ATTRIBUTES){
            continue;
        }
        if(strcmp(find_data.cFileName,".")==0
        ||strcmp(find_data.cFileName,"..")==0){
            continue;
        }

        if(find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY){
            //handle folder
            buffer_append_string(search_path,FILE_PATH_MAX,find_data.cFileName);
            buffer_append_char(search_path,FILE_PATH_MAX,FILE_SEPARATOR);
            add_files_recursive_from_source_directory(search_path,command_buffer,files_to_exclude);
            buffer_remove_characters_from_end(search_path,strlen(find_data.cFileName) +1);
        }else{
            //handle file
            if(is_c_file(find_data.cFileName)==false){
                continue;
            }
            if(files_to_exclude.length > 0){
                bool should_exclude = false;
                for (int i = 0; i<files_to_exclude.length; i++) {
                    const char* excluded_filename = files_to_exclude.items[i];
                    if(string_ends_with_string(find_data.cFileName, excluded_filename)){
                        should_exclude = true;
                        break;
                    }
                }
                if(should_exclude == false){
                    buffer_append_char(command_buffer,COMMAND_BUFFER_SIZE, '"');
                    buffer_append_string(command_buffer, COMMAND_BUFFER_SIZE, search_path);
                    buffer_append_string(command_buffer,COMMAND_BUFFER_SIZE, find_data.cFileName);
                    buffer_append_char(command_buffer,COMMAND_BUFFER_SIZE, '"');
                    buffer_append_char(command_buffer,COMMAND_BUFFER_SIZE, ' ');
                }
            } else{
                buffer_append_char(command_buffer,COMMAND_BUFFER_SIZE, '"');
                    buffer_append_string(command_buffer, COMMAND_BUFFER_SIZE, search_path);
                    buffer_append_string(command_buffer,COMMAND_BUFFER_SIZE, find_data.cFileName);
                    buffer_append_char(command_buffer,COMMAND_BUFFER_SIZE, '"');
                    buffer_append_char(command_buffer,COMMAND_BUFFER_SIZE, ' ');
            }
            
        }
    }while(FindNextFileA(hFind,&find_data));
}

void add_flags_to_buffer(char* buffer,size_t buffer_size, CbsModule module){

    for (int i = 0; i < module.shared_compiler_flags.length; i++) {
        const char* compiler_flag = module.shared_compiler_flags.items[i];
        if(string_starts_with(compiler_flag, "-") == false){
            buffer_append_string(buffer, buffer_size, "-");
        }
        buffer_append_string(buffer, buffer_size, compiler_flag);
        buffer_append_char(buffer, buffer_size,' ');
    }

    for (int i = 0; i < module.unique_compiler_flags.length; i++) {
        const char* compiler_flag = module.unique_compiler_flags.items[i];
        if(string_starts_with(compiler_flag, "-") == false){
            buffer_append_string(buffer, buffer_size, "-");
        }
        buffer_append_string(buffer, buffer_size, compiler_flag);
        buffer_append_char(buffer, buffer_size,' ');
    }

    //add include paths
    for (int i = 0; i < module.shared_include_paths.length; i++) {
        const char* inc_path = module.shared_include_paths.items[i];

        buffer_append_string(buffer, buffer_size, "-I");
        //buffer_append_char(buffer, buffer_size,'"');
        buffer_append_string(buffer, buffer_size, inc_path);
        //buffer_append_char(buffer, buffer_size,'"');
        buffer_append_char(buffer, buffer_size,' ');
    }

    for (int i = 0; i < module.unique_include_paths.length; i++) {
        const char* inc_path = module.unique_include_paths.items[i];

        buffer_append_string(buffer, buffer_size, "-I");
        //buffer_append_char(buffer, buffer_size,'"');
        buffer_append_string(buffer, buffer_size, inc_path);
        //buffer_append_char(buffer, buffer_size,'"');
        buffer_append_char(buffer, buffer_size,' ');
    }

    //add library paths
    for (int i = 0; i < module.shared_library_paths.length; i++) {
        const char* library_path = module.shared_library_paths.items[i];

        buffer_append_string(buffer, buffer_size, "-L");
        //buffer_append_char(buffer, buffer_size,'"');
        buffer_append_string(buffer, buffer_size, library_path);
        //buffer_append_char(buffer, buffer_size,'"');
        buffer_append_char(buffer, buffer_size,' ');
    }

    for (int i = 0; i < module.unique_library_paths.length; i++) {
        const char* library_path = module.unique_library_paths.items[i];

        buffer_append_string(buffer, buffer_size, "-L");
        //buffer_append_char(buffer, buffer_size,'"');
        buffer_append_string(buffer, buffer_size, library_path);
        //buffer_append_char(buffer, buffer_size,'"');
        buffer_append_char(buffer, buffer_size,' ');
    }

    //add linker flags
    for (int i = 0; i < module.shared_linker_flags.length; i++) {
        const char* linker_flag = module.shared_linker_flags.items[i];
        if(string_starts_with(linker_flag, "-l") == false){
            buffer_append_string(buffer, buffer_size, "-l");
        }
        buffer_append_string(buffer, buffer_size, linker_flag);
        buffer_append_char(buffer, buffer_size,' ');
    }

    for (int i = 0; i < module.unique_linker_flags.length; i++) {
        const char* linker_flag = module.unique_linker_flags.items[i];
        if(string_starts_with(linker_flag, "-l") == false){
            buffer_append_string(buffer, buffer_size, "-l");
        }
        buffer_append_string(buffer, buffer_size, linker_flag);
        buffer_append_char(buffer, buffer_size,' ');
    }
}

void add_files_to_compile_commands_recursive(HANDLE file,char* path_buffer,CbsModule module,bool *first_block){
    WIN32_FIND_DATAA find_data;
    const char wildcard = '*';
    buffer_append_char(path_buffer,FILE_PATH_MAX,wildcard);
    HANDLE hFind = FindFirstFileA(path_buffer,&find_data);
    if(hFind == INVALID_HANDLE_VALUE){
        fprintf(stderr,"couldnt find initial files with path [%s]",path_buffer);
        buffer_remove_characters_from_end(path_buffer,1);
        return;
    }
    buffer_remove_characters_from_end(path_buffer,1);

    do{
        if(find_data.dwFileAttributes == INVALID_FILE_ATTRIBUTES){
            continue;
        }
        if(strcmp(find_data.cFileName,".")==0
        ||strcmp(find_data.cFileName,"..")==0){
            continue;
        }

        if(find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY){
            //handle folder
            buffer_append_string(path_buffer,FILE_PATH_MAX,find_data.cFileName);
            buffer_append_char(path_buffer,FILE_PATH_MAX,FILE_SEPARATOR);
            add_files_to_compile_commands_recursive(file,path_buffer,module,first_block);
            buffer_remove_characters_from_end(path_buffer,strlen(find_data.cFileName) +1);
        }else{
            //handle file
            if(is_c_file(find_data.cFileName)==false){
                continue;
            }
            const size_t block_buffer_size = FILE_PATH_MAX *3;
            char block_buffer[block_buffer_size];
            block_buffer[0] = '\0';

            if(*first_block == false){
                buffer_append_char(block_buffer,block_buffer_size,',');    
            }
            buffer_append_string(block_buffer,block_buffer_size,"\n\t{\n\t\t\"directory\" : \"");
            buffer_append_string(block_buffer,block_buffer_size,path_buffer);
            buffer_append_string(block_buffer,block_buffer_size,"\",\n\t\t\"command\" : \"");
            add_flags_to_buffer(block_buffer,block_buffer_size,module);
            buffer_append_string(block_buffer,block_buffer_size,"\",\n\t\t\"file\" : \"");
            buffer_append_string(block_buffer,block_buffer_size,find_data.cFileName);
            buffer_append_string(block_buffer,block_buffer_size,"\"\n\t}");
            string_replace_all_char(block_buffer, '\\', '/');
            
            WriteFile(file,
                block_buffer,
                strlen(block_buffer),
                NULL,
                NULL
            );
            *first_block = false;
        }
    }while(FindNextFileA(hFind,&find_data));
}

void create_compile_commands_json(const CbsModule *module_array,const int array_length){
    
    char path_buffer[FILE_PATH_MAX];
    DWORD bytes = GetCurrentDirectoryA(FILE_PATH_MAX, path_buffer);
    path_buffer[bytes+1] = '\0';

    const char* cc = "compile_commands.json";
    buffer_append_char(path_buffer,FILE_PATH_MAX,FILE_SEPARATOR);
    buffer_append_string(path_buffer, FILE_PATH_MAX,cc);
    
    HANDLE file = CreateFileA(
        path_buffer,
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );
    WriteFile(file,"[\n",2,NULL,NULL);
    
    if(file == INVALID_HANDLE_VALUE){
        fprintf(stderr,"couldn't create file at [%s]\n",path_buffer);
        buffer_remove_characters_from_end(path_buffer, strlen(cc));
        return;
    }

    buffer_remove_characters_from_end(path_buffer, strlen(cc));
    bool first_block = true;
    for(int i = 0; i < array_length; i++){
        CbsModule module = module_array[i];
        if(module.source_file_directory != NULL){
            size_t chars_to_remove = strlen(module.source_file_directory);
            buffer_append_string(path_buffer, FILE_PATH_MAX, module.source_file_directory);
            if(string_ends_with_char(module.source_file_directory,FILE_SEPARATOR)==false){
                buffer_append_char(path_buffer, FILE_PATH_MAX, FILE_SEPARATOR);
                chars_to_remove++;
            }
            add_files_to_compile_commands_recursive(file, path_buffer, module, &first_block);
            buffer_remove_characters_from_end(path_buffer,chars_to_remove);
        }else{
            add_files_to_compile_commands_recursive(file, path_buffer, module, &first_block);
        }
    }
    WriteFile(file,"\n]\n",3,NULL,NULL);

    CloseHandle(file);
}

static void run_command_line(char* command_line){
    STARTUPINFO si = { 0 };
    PROCESS_INFORMATION pi = { 0 };   

    // Create the process
    BOOL success = CreateProcessA(
        NULL,           
        command_line,        
        NULL,           
        NULL,           
        FALSE,          
        0,              
        NULL,           
        NULL,        
        &si,            
        &pi      
    );

    if (!success) {
        DWORD last_error =GetLastError();
        printf("CreateProcess failed (%d).\n", (int)last_error);
        return;
    }

    WaitForSingleObject(pi.hProcess, INFINITE);

    DWORD exit_code;
    GetExitCodeProcess(pi.hProcess, &exit_code);
    if (exit_code != 0) {
        cbs_log_error("Compiler exited with code %d", (int)exit_code);
    }

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
}
#elif defined(__linux__)
static bool try_get_program_path(char* path_buffer){
    ssize_t length = readlink("/proc/self/exe", path_buffer, FILE_PATH_MAX - 1);
    if (length == -1) {
        return false; // readlink failed
    }
    path_buffer[length] = '\0'; // Null-terminate the string
    remove_filename_from_path(path_buffer,length);
    return true;
}
static void add_files_recursive_from_source_directory(char* search_path, char* command_buffer){
    //TODO implement this method
}
static void create_compile_commands_json(CbsModule* module_array,int array_length){
       //TODO implement this method
}
static void run_command_line(char* command){
    //fork(command);
    //TODO implement this method properly
}
#elif defined(__APPLE__)
static bool try_get_program_path(char* path_buffer){
    uint32_t size = FILE_PATH_MAX;
    if (_NSGetExecutablePath(path_buffer, &size) != 0) {
        // Buffer too small, but size now contains required length
        path_buffer[0] = '\0';
        return false;
    } 
    return true;
}
static void add_files_recursive_from_source_directory(char* search_path, char* command_buffer){
    //TODO implement this method
}
static void create_compile_commands_json(CbsModule* module_array,int array_length){
       //TODO implement this method
}
static void create_compile_commands_json(CbsModule* module_array,int array_length){
       //TODO implement this method
}
static void run_command_line(char* command){
    //exec(command);
    //TODO implement this method properly
}
    
#endif



static bool directory_exists(const char *path){
    struct stat info;
    if(stat(path,&info) != 0){
        return false;
    }
    return (info.st_mode & S_IFDIR) != 0;
}

void cbs_module_compile(CbsModule module){
    if(string_is_null_empty_or_whitespace(module.name)){
        cbs_log_error("[name] variable in the module struct is NULL. Must provide name");
        return;
    }
    if(string_is_null_empty_or_whitespace(module.compiler)){
        cbs_log_error("[compiler] variable in module [%s] is null,empty or whitespace",module.name);
        return;
    }
    if(string_is_null_empty_or_whitespace(module.output_directory)){
        cbs_log_error("[output_directory] variable in module [%s] is null,empty or whitespace",module.name);
        return;
    }
    if(string_is_null_empty_or_whitespace(module.output_file_name_with_extension)){
        cbs_log_error("[output_file_name_with_extension] variable in module [%s] is null,empty or whitespace",module.name);
        return;
    }
    if(string_is_null_empty_or_whitespace(module.source_file_directory)){
        cbs_log_error("[source_file_directory] variable in module [%s] is null,empty or whitespace",module.name);
        return;
    }
    
    if(directory_exists(module.output_directory)==false){
        cbs_log_error("output directory [%s] in module [%s] doesnt exist",module.output_directory,module.name);
        return;
    }
    if(directory_exists(module.source_file_directory)==false){
        cbs_log_error("source file directory [%s] in module [%s] doesnt exist",module.output_directory,module.name);
        return;
    }
    char command_buffer[COMMAND_BUFFER_SIZE];

    //add compiler to command buffer
    buffer_append_string(command_buffer, COMMAND_BUFFER_SIZE, module.compiler);
    buffer_append_char(command_buffer, COMMAND_BUFFER_SIZE,' ');

    //add compiler flags
    for (int i = 0; i < module.shared_compiler_flags.length; i++) {
        const char* compiler_flag = module.shared_compiler_flags.items[i];
        if(string_starts_with(compiler_flag, "-") == false){
            buffer_append_string(command_buffer, COMMAND_BUFFER_SIZE, "-");
        }
        buffer_append_string(command_buffer, COMMAND_BUFFER_SIZE, compiler_flag);
        buffer_append_char(command_buffer, COMMAND_BUFFER_SIZE,' ');
    }
    for (int i = 0; i < module.unique_compiler_flags.length; i++) {
        const char* compiler_flag = module.unique_compiler_flags.items[i];
        if(string_starts_with(compiler_flag, "-") == false){
            buffer_append_string(command_buffer, COMMAND_BUFFER_SIZE, "-");
        }
        buffer_append_string(command_buffer, COMMAND_BUFFER_SIZE, compiler_flag);
        buffer_append_char(command_buffer, COMMAND_BUFFER_SIZE,' ');
    }

    //add include paths
    for (int i = 0; i < module.shared_include_paths.length; i++) {
        const char* inc_path = module.shared_include_paths.items[i];

        buffer_append_string(command_buffer, COMMAND_BUFFER_SIZE, "-I");
        buffer_append_char(command_buffer, COMMAND_BUFFER_SIZE,'"');
        buffer_append_string(command_buffer, COMMAND_BUFFER_SIZE, inc_path);
        buffer_append_char(command_buffer, COMMAND_BUFFER_SIZE,'"');
        buffer_append_char(command_buffer, COMMAND_BUFFER_SIZE,' ');
    }
    for (int i = 0; i < module.unique_include_paths.length; i++) {
        const char* inc_path = module.unique_include_paths.items[i];

        buffer_append_string(command_buffer, COMMAND_BUFFER_SIZE, "-I");
        buffer_append_char(command_buffer, COMMAND_BUFFER_SIZE,'"');
        buffer_append_string(command_buffer, COMMAND_BUFFER_SIZE, inc_path);
        buffer_append_char(command_buffer, COMMAND_BUFFER_SIZE,'"');
        buffer_append_char(command_buffer, COMMAND_BUFFER_SIZE,' ');
    }

    //add library paths
    for (int i = 0; i < module.shared_library_paths.length; i++) {
        const char* library_path = module.shared_library_paths.items[i];

        buffer_append_string(command_buffer, COMMAND_BUFFER_SIZE, "-L");
        buffer_append_char(command_buffer, COMMAND_BUFFER_SIZE,'"');
        buffer_append_string(command_buffer, COMMAND_BUFFER_SIZE, library_path);
        buffer_append_char(command_buffer, COMMAND_BUFFER_SIZE,'"');
        buffer_append_char(command_buffer, COMMAND_BUFFER_SIZE,' ');
    }
    for (int i = 0; i < module.unique_library_paths.length; i++) {
        const char* library_path = module.unique_library_paths.items[i];

        buffer_append_string(command_buffer, COMMAND_BUFFER_SIZE, "-L");
        buffer_append_char(command_buffer, COMMAND_BUFFER_SIZE,'"');
        buffer_append_string(command_buffer, COMMAND_BUFFER_SIZE, library_path);
        buffer_append_char(command_buffer, COMMAND_BUFFER_SIZE,'"');
        buffer_append_char(command_buffer, COMMAND_BUFFER_SIZE,' ');
    }

    //add linker flags
    for (int i = 0; i < module.shared_linker_flags.length; i++) {
        const char* linker_flag = module.shared_linker_flags.items[i];
        if(string_starts_with(linker_flag, "-l") == false){
            buffer_append_string(command_buffer, COMMAND_BUFFER_SIZE, "-l");
        }
        buffer_append_string(command_buffer, COMMAND_BUFFER_SIZE, linker_flag);
        buffer_append_char(command_buffer, COMMAND_BUFFER_SIZE,' ');
    }
    for (int i = 0; i < module.unique_linker_flags.length; i++) {
        const char* linker_flag = module.unique_linker_flags.items[i];
        if(string_starts_with(linker_flag, "-l") == false){
            buffer_append_string(command_buffer, COMMAND_BUFFER_SIZE, "-l");
        }
        buffer_append_string(command_buffer, COMMAND_BUFFER_SIZE, linker_flag);
        buffer_append_char(command_buffer, COMMAND_BUFFER_SIZE,' ');
    }

    //add source files
    char search_path[FILE_PATH_MAX];

    if(try_get_program_path(search_path)==false){
        cbs_log_error("failed to get executable path");
        return;
    }

    if(string_is_null_empty_or_whitespace(module.source_file_directory) == false){
        buffer_append_string(search_path,FILE_PATH_MAX, module.source_file_directory);
        size_t src_file_dir_len = strlen(module.source_file_directory);
        if(module.source_file_directory[src_file_dir_len-1] != FILE_SEPARATOR){
            buffer_append_char(search_path,FILE_PATH_MAX,FILE_SEPARATOR);
        }
    }

    add_files_recursive_from_source_directory(search_path,command_buffer,module.source_files_to_exclude);
    
    for(int i = 0; i<module.additional_source_file_paths.length; i++){
        const char* additional_src_file_path = module.additional_source_file_paths.items[i];
        buffer_append_string(command_buffer, COMMAND_BUFFER_SIZE, additional_src_file_path);
        buffer_append_char(command_buffer, COMMAND_BUFFER_SIZE, ' ');
    }

    // add output
    buffer_append_string(command_buffer, COMMAND_BUFFER_SIZE, "-o ");
    buffer_append_string(command_buffer, COMMAND_BUFFER_SIZE, module.output_directory);
    size_t output_directory_length = strlen(module.output_directory);
    if(module.output_directory[output_directory_length-1] != FILE_SEPARATOR){
        buffer_append_char(command_buffer,COMMAND_BUFFER_SIZE,FILE_SEPARATOR);
    }
    buffer_append_string(command_buffer, COMMAND_BUFFER_SIZE, module.output_file_name_with_extension);

    run_command_line(command_buffer);
}

void cbs_command_run_matching(
    const int argc,
    const char **argv,
    const int command_count,
    const CbsCommand *command_array
){
    if(argc <2){
        cbs_log_error("no arguments passed to build script");
        return;
    }
    if(command_array == NULL){
        cbs_log_error("NULL commands array");
        return;
    }
    if(command_count < 1){
        cbs_log_error("passing zero or negative array length");
        return;
    }

    const char *command_name = argv[1];

    for(int i = 0; i < command_count; i++){
        CbsCommand command = command_array[i];
        if(strcmp(command_name,command.name)==0){
            if(command.fnptr== NULL){
                cbs_log_error("command [%s] has null fn ptr",command_name);
            }else{
                command.fnptr(argc, argv);
            }
            return;
        }
    }

    cbs_log_error("command [%s] not found in array\nDefined commands are as follows...\n",command_name);
    for (uint32_t i = 0; i < command_count; i++) {
        printf("%s\n",command_array[i].name);
    }

}

#endif
