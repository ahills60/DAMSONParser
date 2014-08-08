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
#include <time.h>

// Program defines
#include "damsonparser.h"

// Prototypes
int DAMSONHeaderCheck(char *line, int idx);
int ParseLine(char *line, int lineNo);
void ProcessFile(char *filename);

// Global Variables
char *HeaderLine1, *HeaderLine2, *HeaderLine3;

// This version checks the header of the DAMSON compiler output
// The index is used to inform the function of the current line number
int DAMSONHeaderCheck(char *line, int idx)
{
    char csymb[5], cnotice[10], dname[3], aname[10], progname[12], versionstr[10], version[10];
    struct tm timer;
    int scanout, n;
    unsigned int *year;
    
    switch (idx)
    {
        case 0:
            // This line is the DAMSON verion number
            scanout = sscanf(line, "%s %s %s", progname, versionstr, version);
            // Check to see if this failed or the number parsed was incorrect
            
            if (scanout == EOF || scanout < 3)
                return 0;
            // Check to see if the header is as anticipated
            if (!strcmp(progname, "DAMSON"))
                if (!strcmp(versionstr, "Version"))
                    printf("Recognised %s Compiler %s %s\n\n", progname, versionstr, version);
                else
                    return 0;
            else
                return 0;
            // Store this header line into a global variable. It may be useful
            HeaderLine1 = malloc(sizeof(char) * (14 + strlen(version)));
            // In this instance, it's easier to just add the version to the end.
            sprintf(HeaderLine1, "DAMSON Version %s", version);
            break;
        case 1:
            // This line is the copyright notice
            scanout = sscanf(line, "%s %s %s %s %u", csymb, cnotice, dname, aname, &year);
            if (scanout == EOF || scanout < 5)
                return -1;
            // Store this header line into a global variable. As before, this may be useful.
            // First allocate memory then set all entries to null.
            HeaderLine2 = malloc(sizeof(char) * strlen(line));
            memset(HeaderLine2, 0, strlen(line));
            // Next determine if the incoming line has a new line character (hint: it does normally)
            for (n = strlen(line); n > 0; n--)
                if (line[n - 1] == '\n')
                    break;
            // In the (rare) instance this may not have a new line character, say that we want the entire string.
            if (n == 0)
                n = strlen(line);
            // Copy the contents of the line to the header variable.
            memcpy(&HeaderLine2[0], &line[0], n);
            break;
        case 2:
            // This line is the time stamp that DAMSON was run
            
            // Check for the new line character.
            for (n = strlen(line); n > 0; n--)
            {
                if (line[n - 1] == '\n')
                    break;
            }
            // Remove the new line by replacing it with null if one was found
            if (n > 0)
                line[n - 1] = '\0';
            
            // Next, attempt to store the elements within the string into a timer object.
            if (strptime(line, "%a %b %d %H:%M:%S %Y", &timer) == NULL)
                return -2;
            
            // Reserve some memory and move the contents of the line to the header variable.
            HeaderLine3 = malloc(sizeof(char) * strlen(line));
            memcpy(&HeaderLine3[0], &line[0], n);
            break;
        default:
            return -3;
    }
    // If here, everything was okay.
    return 1;
}

// This function parses a line of text
int ParseLine(char *line, int lineNo)
{
    int n;
    
    // First, remove the new line character
    for (n = strlen(line); n > 0; n--)
        if (line[n - 1] == '\n')
            break;
    // In the event no new line was found:
    if (n > 0)
        line[n - 1] = '\0';
    
    // Now determine if there's something to look at:
    if (strcmp(line, ""))
    {
        // Check for no file errors
        if(!strcmp(line, "No file?"))
        {
            // No file provided. Bad.
            printf("Error: No file was passed to the DAMSON compiler.\n");
            return 0;
        }
    }
    else
    {
        // There's nothing on this line. Move on.
        return 1;
    }
}

// This function processes files.
void ProcessFile(char *filename)
{
    FILE *fp;
    int ptr = 0, justRead = 0, lineNo = 1, dcheck = 0;
    char *line = NULL;
    size_t len;
    ssize_t lsize;
    
    fp = fopen(filename, "r");
    
    // Ensure file exists and can be read:
    if (fp == NULL)
    {
        printf("\nError opening file. Ensure filename and path is valid.\n");
        return;
    }
    
    while((lsize = getline(&line, &len, fp)) != -1)
    {
        if (lineNo <= 3)
        {
            dcheck = DAMSONHeaderCheck(line, lineNo - 1);
            if (dcheck < 1)
            {
                printf("Error processing header on line %i.\n\n", lineNo);
                fclose(fp);
                return;
            }
        }
        else
        {
            dcheck = ParseLine(line, lineNo);
            if (dcheck < 1)
            {
                printf("Error processing script on line %i.\n\n", lineNo);
                fclose(fp);
                return;
            }
        }
        lineNo++;
    }
    // Once we're done, we should free up the memory that was used by the line variable
    free(line);
}

int main(int argc, char *argv[])
{
    char *currObj, *parVal, *filename = "\0";
    int i, n, a, isParam;
    
    printf("\nDAMSON Parser ");
    printf("Version: %i.%i.%i (%s)\n", VERSION_MAJOR, VERSION_MINOR, VERSION_BUILD, VERSION_DATE);
    printf("Author: Andrew Hills (a.hills@sheffield.ac.uk)\n\n");
    
    
    parVal = "";
    
    // Go through arguments (if any)
    for (i = 0; i < argc; i++)
    {
        currObj = argv[i];
        isParam = 0;
        a = strlen(argv[i]);
        // Detect the presence of an argument starter (i.e. "-"):
        for (n = 0; n < a; n++)
            if (argv[i][n] == '-')
                isParam = 1;
            else
                break;
        // An argument starter was found. Extract the characters after this
        if (isParam)
        {
            // Simply shift the contents of memory back.
            memmove(&currObj[0], &currObj[n], strlen(currObj) - n + 1);
            
            parVal = currObj;
        }
        else
        {
            // No argument starter was found. This could be the result of an option. Let's check
            if (strcmp(parVal, ""))
            {
                // There was previously a parameter, let's determine what's being set.
                if (!strcmp(parVal, "filename"))
                    filename = currObj;
                else
                    printf("Unrecognised input \"%s\"\n", parVal);
            }
            else // No parameter was defined. Skip to the next argument.
                continue;
        }
    }
    
    // Quick check to see if the filename variable was specified.
    if (filename[0] == '\0')
    {
        // No. At this point, we could check for piped input.
        printf("No input file specified\n\n");
    }
    else
    {
        // Yes. Filename specified. Let the ProcessFile function handle this request.
        printf("Input file \"%s\" specified\n\n", filename);
        
        ProcessFile(filename);
    }
    
    return 0;
}
