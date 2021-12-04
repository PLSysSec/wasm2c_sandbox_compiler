#if defined(_WIN32)
    // Remove warnings for strcat, strcpy as they are safely used here
    #define _CRT_SECURE_NO_WARNINGS
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>

#include "wasm-rt.h"

#if defined(_WIN32)
    // Ensure the min/max macro in the header doesn't collide with functions in std::
    #define NOMINMAX
    #include <Windows.h>
    #define LINETERM "\r\n"
#else
    #include <dlfcn.h>
    #define LINETERM "\n"
#endif

void* open_lib(char const * wasm2c_module_path) {
    #if defined(_WIN32)
        void* library = LoadLibraryA(wasm2c_module_path);
    #else
        void* library = dlopen(wasm2c_module_path, RTLD_LAZY);
    #endif

    if (!library) {
      #if defined(_WIN32)
        DWORD errorMessageID  = GetLastError();
        if (errorMessageID != 0) {
          LPSTR messageBuffer = 0;
          //The api creates the buffer that holds the message
          size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                                      NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);

          const size_t str_buff_size = size + 1;
          char* error_msg = malloc(str_buff_size);
          memcpy(error_msg, messageBuffer, str_buff_size - 1);
          error_msg[str_buff_size - 1] = '\0';
          printf("Could not load wasm2c dynamic library: %s" LINETERM, error_msg);
          free(error_msg);

          LocalFree(messageBuffer);
        }
      #else
        const char* error_msg = dlerror();
        printf("Could not load wasm2c dynamic library: %s" LINETERM, error_msg);
      #endif

      exit(1);
    }

    return library;
}


int count_words(const char sentence[ ])
{
    int counted = 0; // result

    // state:
    const char* it = sentence;
    int inword = 0;

    do switch(*it) {
        case '\0': 
        case ' ': 
            if (inword) { inword = 0; counted++; }
            break;
        default: inword = 1;
    } while(*it++);

    return counted;
}

void close_lib(void* library)
{
    #if defined(_WIN32)
      FreeLibrary((HMODULE) library);
    #else
      dlclose(library);
    #endif
}

void* symbol_lookup(void* library, const char * name) {
#if defined(_WIN32)
    void* ret = GetProcAddress((HMODULE) library, name);
#else
    void* ret = dlsym(library, name);
#endif
    if (!ret) {
        printf("Could not find symbol %s in wasm2c shared library" LINETERM, name);
    }
    return ret;
}

char* get_info_func_name(char const * wasm_module_name) {
    const char* info_func_suffix = "get_wasm2c_sandbox_info";
    char* info_func_str = malloc(strlen(wasm_module_name) + strlen(info_func_suffix) + 1);

    strcpy(info_func_str, wasm_module_name);
    strcat(info_func_str, info_func_suffix);
    return info_func_str;
}

typedef wasm2c_sandbox_funcs_t(*get_info_func_t)();
typedef void (*wasm2c_start_func_t)(void* sbx);

int main(int argc, char const *argv[])
{
    if (argc < 2) {
        printf("Expected argument: %s <path_to_shared_library> [optional_module_prefix]" LINETERM, argv[0]);
        exit(1);
    }

    char const * wasm2c_module_path = argv[1];
    char const * wasm_module_name = "";

    // if (argc >= 3) {
    //     wasm_module_name = argv[2];
    // }

    void* library = open_lib(wasm2c_module_path);
    char* info_func_name = get_info_func_name(wasm_module_name);

    get_info_func_t get_info_func = (get_info_func_t) symbol_lookup(library, info_func_name);
    wasm2c_sandbox_funcs_t sandbox_info = get_info_func();
    /*
     * Create runtime_metadata struct and pass through to create_wasm2c_sandbox func
     * */

    // TODO: maybe it makes sense to allow no fs access instead of default . ???
    wasm2c_rt_init_data init_data = {".", "", 0, "", 0, ""}; 

    int c;
    while (1)
    {
      static struct option long_options[] =
        {
          /* These options set a flag. */
          {"homedir", required_argument, NULL, 'd'},
          {"args", required_argument, NULL, 'a'},
          {"env", required_argument, NULL, 'e'},
          {"log_path", required_argument, NULL, 'l'},

          {0, 0, 0, 0}
        };
      /* getopt_long stores the option index here. */
      int option_index = 0;

      c = getopt_long (argc, argv, "l:a:e:d:", long_options, &option_index);

      switch(c)
      {
         case -1:       /* no more arguments */
         case 0:        /* long options toggles */
         break;

        case 'l':
          //printf("log file = \"%s\"\n", optarg);
          char *log_path = (char*)malloc(1024+1);
          snprintf(log_path, 1024, "%s", optarg );
          init_data.log_path = log_path;
          break;

        case 'a':
          //printf("args = \"%s\"\n", optarg);
          char *args = (char*)malloc(1024+1);
          snprintf(args, 1024, "%s", optarg );
          init_data.args = args;
          break;
         
        case 'e':
          //printf("env = \"%s\"\n", optarg);
          char *env = (char*)malloc(1024+1);
          snprintf(env, 1024, "%s", optarg );
          init_data.env = env;
          break;
        
        case 'd':
          //printf("homedir = \"%s\"\n", optarg);
          char *homedir = (char*)malloc(1024+1);
          snprintf(homedir, 1024, "%s", optarg );
          init_data.homedir = homedir;
          break;

         default:
         abort();
         return(-2);
      };

      /* Detect the end of the options. */
      if (c == -1)
        break;
    }

    init_data.argc = count_words(init_data.args);
    init_data.envc = count_words(init_data.env);
    //printf("argc = %d\n", init_data.argc);

    


    void* sandbox = sandbox_info.create_wasm2c_sandbox(&init_data);
    if (!sandbox) {
        printf("Error: Could not create sandbox" LINETERM);
        exit(1);
    }

    wasm2c_start_func_t start_func = (wasm2c_start_func_t) symbol_lookup(library, "w2c__start");
    start_func(sandbox);
    sandbox.destroy_wasm2c_sandbox();

    free(info_func_name);
    close_lib(library);
    return 0;
}
