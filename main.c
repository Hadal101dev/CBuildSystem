#include<stdio.h>
#include <string.h>
#include <sys/stat.h>
#include<stdbool.h>


bool file_exists(const char* path) {
    struct stat info;
    return stat(path, &info) == 0 && (info.st_mode & S_IFREG) != 0;
}

bool buffer_append_string(char* buffer,size_t buffer_capacity,const char* string){
    size_t string_length = strlen(string);
    size_t buffer_length = strlen(buffer);
    if(buffer_length + string_length > buffer_capacity -1)//-1 for null terminator
    {
        fprintf(stderr,"no room in buffer for string");
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

bool buffer_append_char(char* buffer,size_t buffer_capacity,const char character){
    size_t buffer_length = strlen(buffer);
    if(buffer_length + 1 > buffer_capacity -1)//-1 for null terminator
    {
        fprintf(stderr,"no room in buffer for string");
        return false;
    }

    buffer[buffer_length] = character;
    buffer[buffer_length+1] = '\0';
    
    return true;
}

#ifdef _WIN32
#include<windows.h>
#define FILE_PATH_MAX 260
#define FILE_SEPARATOR '\\'
void Get_Current_Directory(char* path_buffer,size_t buffer_length){
    DWORD bytes = GetCurrentDirectoryA(buffer_length, path_buffer);
    path_buffer[bytes+1] = '\0';
}

void Get_Current_Exe_Path(char* path_buffer,size_t buffer_size){
    int length = (int)GetModuleFileNameA(NULL, path_buffer, buffer_size);
    for (int i = length-1; i >= 0; i--) {
        if(path_buffer[i] == FILE_SEPARATOR){
            path_buffer[i] = '\0';
            return;
        }
    }
}

void Copy_File(const char *from, const char *to){
    bool result = CopyFileA(from,to,true);
    if(result == false){
        fprintf(stderr,"couldn't copy file from [%s] to [%s]. Errcode [%d]",
        from,to,(int)GetLastError());
    }
}

void Run_Cmd(char* cmd){
    STARTUPINFO si = { 0 };
    PROCESS_INFORMATION pi = { 0 };   

    // Create the process
    BOOL success = CreateProcessA(
        NULL,           
        cmd,        
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

    WaitForSingleObject(pi.hProcess, 10000);

    DWORD exit_code;
    GetExitCodeProcess(pi.hProcess, &exit_code);
    if (exit_code != 0) {
        fprintf(stderr,"Compiler exited with code %d", (int)exit_code);
    }

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
}

#elif defined(__linux__)
#include<unistd.h>
#define FILE_PATH_MAX 4096
#define FILE_SEPARATOR '/'
void Get_Current_Directory(char* path_buffer,size_t buffer_length){
    //TODO Implement this function
}
void Get_Current_Exe_Path(char* path_buffer,size_t buffer_size){
    //TODO Implement this function
}
void Copy_File(const char *from, const char *to){
    //TODO Implement this function
}
void Run_Cmd(char* cmd){
     //TODO Implement this function
}
#elif defined(__APPLE__)
#include<mach-o/dyld.h>
#define FILE_PATH_MAX 1024
#define FILE_SEPARATOR '/'
void Get_Current_Directory(char* path_buffer,size_t buffer_length){
    //TODO Implement this function
}
void Get_Current_Exe_Path(char* path_buffer,size_t buffer_size){
    //TODO Implement this function
}
void Copy_File(const char *from, const char *to){
    //TODO Implement this function
}
void Run_Cmd(char* cmd){
     //TODO Implement this function
}
#endif

#define CMD_BUFFER_SIZE 8192
#define COMPILER "clang"
#define LOCAL_BUILD_C_FILENAME "build.c"
#define LOCAL_BUILD_H_FILENAME "build.h"
#define TARGET_BUILD_C_FILENAME "build.c"
#define TARGET_BUILD_H_FILENAME "build.h"
#define TARGET_BUILD_EXE_FILENAME "build.exe"

typedef struct FilePaths{
    char c[FILE_PATH_MAX];
    char h[FILE_PATH_MAX];
    char exe[FILE_PATH_MAX];
    char dir[FILE_PATH_MAX];
}FilePaths;

FilePaths file_paths = {0};

void debug(const char* msg){
    printf("%s\n",msg);
}

void init_file_paths(void){    
    Get_Current_Directory(file_paths.dir,FILE_PATH_MAX);

    snprintf(file_paths.c,FILE_PATH_MAX,"%s%c%s",file_paths.dir,FILE_SEPARATOR,TARGET_BUILD_C_FILENAME);
    snprintf(file_paths.h,FILE_PATH_MAX,"%s%c%s",file_paths.dir,FILE_SEPARATOR,TARGET_BUILD_H_FILENAME);
    snprintf(file_paths.exe,FILE_PATH_MAX,"%s%c%s",file_paths.dir,FILE_SEPARATOR,TARGET_BUILD_EXE_FILENAME);
}

void command_compile(const char* compiler){

    char cmd_buffer[CMD_BUFFER_SIZE];
    memset(cmd_buffer,0,8192);

    buffer_append_string(cmd_buffer, CMD_BUFFER_SIZE, compiler);
    buffer_append_char(cmd_buffer, CMD_BUFFER_SIZE, ' ');

    buffer_append_string(cmd_buffer,CMD_BUFFER_SIZE,file_paths.c);
    buffer_append_char(cmd_buffer, CMD_BUFFER_SIZE, ' ');

    buffer_append_string(cmd_buffer,CMD_BUFFER_SIZE,"-o ");
    buffer_append_string(cmd_buffer,CMD_BUFFER_SIZE,file_paths.exe);

    debug(cmd_buffer);
    Run_Cmd(cmd_buffer);
}

void command_init(void){

    char this_exe_path[FILE_PATH_MAX]; 
    char build_c_template_path[FILE_PATH_MAX]; 
    char build_h_template_path[FILE_PATH_MAX]; 
    this_exe_path[0] = '\0';
    build_c_template_path[0] = '\0';
    build_h_template_path[0] = '\0';

    Get_Current_Exe_Path(this_exe_path,FILE_PATH_MAX);
    snprintf(build_c_template_path,FILE_PATH_MAX,"%s%c%s",this_exe_path,FILE_SEPARATOR,LOCAL_BUILD_C_FILENAME);
    snprintf(build_h_template_path,FILE_PATH_MAX,"%s%c%s",this_exe_path,FILE_SEPARATOR,LOCAL_BUILD_H_FILENAME);

    if(file_exists(file_paths.c)){
        fprintf(stdout,"%s already exists, not overwriting...\n",TARGET_BUILD_C_FILENAME);
    }else{
        Copy_File(build_c_template_path, file_paths.c);
    }

    if(file_exists(file_paths.h)){
        fprintf(stdout,"%s already exists, not overwriting...\n",TARGET_BUILD_H_FILENAME);
    }else{
        Copy_File(build_h_template_path, file_paths.h);
    }
}

void command_help(void){
    const char* help_text =
    "==========================\n"
    "===== C BUILD SYSTEM =====\n"
    "==========================\n"
    "\n"
    "Follow the link below for the README\n"
    "https://github.com/Hadal101dev/CBuildSystem/blob/main/README.md\n"
    "\n";

    fprintf(stdout,"%s",help_text);
}

void command_defer(int argc, char** argv){

    char cmd_buffer[CMD_BUFFER_SIZE];
    memset(cmd_buffer,0,8192);

    buffer_append_string(cmd_buffer,CMD_BUFFER_SIZE,file_paths.exe);
    for (int i = 1; i<argc; i++) {
        buffer_append_char(cmd_buffer,CMD_BUFFER_SIZE,' ');
        buffer_append_string(cmd_buffer,CMD_BUFFER_SIZE,argv[i]);
    }

    Run_Cmd(cmd_buffer);
}

int main(int argc, char** argv){

    if(argc < 2){
        command_help();
        return 0;
    }

    init_file_paths();
    

    if(strcmp(argv[1],"compile")==0){
        if(argc != 3){
            fprintf(stderr,"incorrect number of args. command should read \"cbs compile <compiler_exe_name>\"\n");
            return 1;
        }
        debug("compile");
        if(file_exists(file_paths.c) == false){
            fprintf(stderr,"c file: %s does not exist. Use the init command or create a new one\n",TARGET_BUILD_C_FILENAME);
            return 1;
        }
        if(file_exists(file_paths.h) == false){
            fprintf(stderr,"header file: %s. Use the init command to recreate\n",TARGET_BUILD_H_FILENAME);
            return 1;
        }
        command_compile(argv[2]);
    }else if(argc == 2 && strcmp(argv[1],"init")==0){
        debug("init");
        command_init();
    }else if(argc == 2 && strcmp(argv[1],"help")==0){
        command_help();
    }else if(file_exists(file_paths.exe)){
        debug("defer");
        command_defer(argc,argv);
    }else{
        debug("help2");
        command_help();
    }

    return 0;
}
