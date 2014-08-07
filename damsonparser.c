/* 
This reads DAMSON output and draws any recognised output.
Those lines that aren't recognised by this application are
sent to a log that can be reviewed.

  Author: Andrew Hills (a.hills@sheffield.ac.uk)
 Version: 0.1 (07/08/2014)
*/

// Standard IO defines
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <GL/glut.h>

// Program defines
#include "damsonparser.h"

int main(int argc, char *argv[])
{
    char *currObj, *parVal, *filename;
    int i, isParam;
    
    printf("\nDAMSON Parser");
    printf("Version: %i.%i.%i (%s)\n", VERSION_MAJOR, VERSION_MINOR, VERSION_BUILD, VERSION_DATE);
    printf("Author: Andrew Hills (a.hills@sheffield.ac.uk)\n\n");
    
    // Go through arguments (if any)
    for (i = 0; i < argc; i++)
    {
        currObj = argv[i];
        isParam = 0;
        a = strlen(argv[i]);
        
    }
    
}