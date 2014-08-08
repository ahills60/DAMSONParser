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
    char *datestring, csymb[5], cnotice[10], dname[3], aname[10], progname[12], versionstr[10], version[10];
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
            if (!strcmp(progname, "DAMSON"))
                if (!strcmp(versionstr, "Version"))
                    printf("Recognised %s Compiler %s %s\n\n", progname, versionstr, version);
                else
                    return 0;
            else
                return 0;
            HeaderLine1 = malloc(sizeof(char) * (14 + strlen(version)));
            sprintf(HeaderLine1, "DAMSON Version %s", version);
            break;
        case 1:
            // This line is the copyright notice
            scanout = sscanf(line, "%s %s %s %s %u", csymb, cnotice, dname, aname, &year);
            if (scanout == EOF || scanout < 5)
                return -1;
            HeaderLine2 = malloc(sizeof(char) * strlen(line));
            memset(HeaderLine2, 0, strlen(line));
            for (n = strlen(line); n > 0; n--)
                if (line[n - 1] == '\n')
                    break;
            if (n == 0)
                n = strlen(line);
            memcpy(&HeaderLine2[0], &line[0], n);
            break;
        case 2:
            // This line is the time stamp that DAMSON was run
            for (n = strlen(line); n > 0; n--)
            {
                if (line[n - 1] == '\n')
                    break;
            }
            if (n == 0)
                n = strlen(line);
            // Remove the new line by replacing it with null
            line[n] = '\0';
            if (strptime(line, "%a %b %d %H:%M:%S %Y", &timer) == NULL)
                return -2;
            
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
    int c, ptr = 0, justRead = 0, lineNo = 1, dcheck = 0;
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
                printf("Error processing header on line %i.\n", lineNo);
                fclose(fp);
                return;
            }
        }
        else
        {
            dcheck = ParseLine(line, lineNo);
            if (dcheck < 1)
            {
                printf("Error processing script on line %i.\n", lineNo);
                fclose(fp);
                return;
            }
        }
        lineNo++;
    }
    free(line);
    
    // while((c == getc(fp)) != EOF)
//     {
//         if (c == '\n')
//         {
//             if (ptr > 0)
//             {
//                 if (lineNo <= 3)
//                 {
//                     dcheck = DAMSONHeaderCheck(line, lineNo - 1);
//                     if (dcheck < 1)
//                     {
//                         printf("Error processing header on line %i.\n", lineNo);
//                         fclose(fp);
//                         return;
//                     }
//
//                 }
//                 else
//                 {
//                     dcheck = ParseLine(line, lineNo);
//                     if (dcheck < 1)
//                     {
//                         printf("Error processing header on line %i.\n", lineNo);
//                         fclose(fp);
//                         return;
//                     }
//                 }
//                 // Reset the pointer and line space:
//                 ptr = 0;
//                 memset(line, 0, sizeof(char) * COMMANDLENGTH);
//             }
//             // increment line number:
//             lineNo++;
//         }
//
//     }
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
        for (n = 0; n < a; n++)
            if (argv[i][n] == '-')
                isParam = 1;
            else
                break;
        if (isParam)
        {
            memmove(&currObj[0], &currObj[n], strlen(currObj) - n + 1);
            
            parVal = currObj;
        }
        else
        {
            if (strcmp(parVal, ""))
                if (!strcmp(parVal, "filename"))
                    filename = currObj;
                else
                    printf("Unrecognised input \"%s\"\n", parVal);
            else
                continue;
        }
    }
    
    if (filename[0] == '\0')
    {
        printf("No input file specified\n\n");
    }
    else
    {
        printf("Input file \"%s\" specified\n\n", filename);
        
        ProcessFile(filename);
    }
    
    return 0;
}