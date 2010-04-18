                                                                                                                             
#include <unistd.h>                                                                                                          
#include <stdlib.h>                                                                                                          
#include <stdio.h>                                                                                                           
#include <string.h>                                                                                                          
#include <stdarg.h>                                                                                                          
                                                                                                                             
#define PNG_DEBUG 3                                                                                                          
#include <png.h>    

                                                                                                                             
#ifdef __cplusplus                                                                                                           
extern "C" {                                                                                                                 
#endif /* __cplusplus */ 

void run_png_sequence(void);
extern png_bytep * read_png_file(char* file_name);
int get_png_height(void);                                                                                                    
int get_png_width(void);                                                                                                   
                                                                                                                             
#ifdef __cplusplus                                                                                                           
}
#endif /* __cplusplus */ 
