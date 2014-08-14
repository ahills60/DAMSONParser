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
void initialisePixelStore(int width, int height);
void clearPixelStore();
void reshapeFunc(int newWidth, int newHeight);
void idleFunc(void);
void fadeActivity(void);
void keyboardFunc(unsigned char key, int xmouse, int ymouse);
void specialFunc(int key, int x, int y);
static void printToScreen(int inset, const char *format, ...);
void displayFunc(void);
void initialiseGLUT(int argc, char *argv[]);
int DAMSONHeaderCheck(char *line, int idx);
int ParseLine(char *line, int lineNo);
void ProcessFile(char *filename);

// Global Variables
char *HeaderLine1, *HeaderLine2, *HeaderLine3, *TheEndText;
int TheEnd = 0, SceneWidth = 0, SceneHeight = 0;

unsigned int *PixelStore;
unsigned int *ActivityStore;

// Information flags
int DisplayInfo;
int DisplayActivity;

// Use for printing to screen:
int PrintLoc;

// Text buffer:
char ScreenText[256];

void initialisePixelStore(int width, int height)
{
    // Ensure we have enough memory to store pixel information
    PixelStore = (unsigned int *) malloc(sizeof(unsigned int) * width * height);
    ActivityStore = (unsigned int *) malloc(sizeof(unsigned int) * width * height);
    
    // Finally, set the space to null:
    memset(PixelStore, 0, sizeof(unsigned int) * width * height);
    memset(ActivityStore, 0, sizeof(unsigned int) * width * height);
}

// Quick function to wipe the pixel store
void clearPixelStore()
{
    // A simple wipe of the memory location:
    memset(PixelStore, 0, sizeof(unsigned int) * SceneWidth * SceneHeight);
}

// Function to define window resizing
void reshapeFunc(int newWidth, int newHeight)
{
    // We don't allow this to be resized so change it back:
    glutReshapeWindow(SceneWidth, SceneHeight);
}

// Function to define what happens in idle time
void idleFunc(void)
{
    glutPostRedisplay();
}

// Function to fade activity pixels
void fadeActivity(void)
{
    int i, a;
    for (i = 0; i < SceneWidth * SceneHeight; i++)
    {
        a = ActivityStore[i] >> 24;
        a--;
        // Ensure that a is within the bounds
        a = (a < 0) ? 0 : a;
        ActivityStore[i] = 0 | 255 < 8 | 0 << 16 | a << 24;
    }
}

// Function to take control of user input elements
void keyboardFunc(unsigned char key, int xmouse, int ymouse)
{
    
    
}

// Function to handle special keys
void specialFunc(int key, int x, int y)
{
    
    
}

// Function to print text to the screen
static void printToScreen(int inset, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vsprintf(ScreenText, format, args);
    va_end(args);
    
    glRasterPos2i(inset, PrintLoc);
    int i;
    for (i = 0; i < strlen(ScreenText); i++)
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, ScreenText[i]);
    
    PrintLoc -= 20;
}

// Function to handle what's displayed within the window
void displayFunc(void)
{
    glClear(GL_COLOR_BUFFER_BIT);
    glRasterPos2i(0, 0);
    
    // Display the contents of the pixel store to the screen
    glDrawPixels(SceneWidth, SceneHeight, GL_RGBA, GL_UNSIGNED_BYTE, &PixelStore[0]);
    
    // Display activity if desired
    if (DisplayActivity)
    {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDrawPixels(SceneWidth, SceneHeight, GL_RGBA, GL_UNSIGNED_BYTE, &ActivityStore[0]);
        glDisable(GL_BLEND);
        fadeActivity();
    }
    
    // Check to see if information should be displayed
    if (DisplayInfo)
    {
        glPushMatrix();
        glLoadIdentity();
        glOrtho(0, SceneWidth, 0, SceneHeight, -1.0, 1.0);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glColor4f(0.0, 0.0, 0.0, 0.7);
        glRecti(5, PrintLoc, 340, SceneHeight - 5);
        glColor3f(1.0, 1.0, 1.0);
        PrintLoc = SceneHeight - 30;
        
        printToScreen(10, "DAMSON parser version %i.%i.%i (%s)", VERSION_MAJOR, VERSION_MINOR, VERSION_BUILD, VERSION_DATE);
        printToScreen(10, " ");
        glDisable(GL_BLEND);
        glPopMatrix();
    }
    
    glutSwapBuffers();
}

// function to initialise GLUT window and output
void initialiseGLUT(int argc, char *argv[])
{
    DisplayInfo = 0;
    DisplayActivity = 0;
    glutInitWindowSize(SceneWidth, SceneHeight);
    
    // Set up the window position:
    glutInitWindowPosition(0, 0);
    glutInitDisplayMode(GLUT_RGBA | GLUT_ALPHA | GLUT_DOUBLE);
    
    glutInit(&argc, argv);
    
    glutCreateWindow("DAMSON parser visualiser");
    
    glutDisplayFunc(displayFunc);
    glutIdleFunc(idleFunc);
    glutKeyboardFunc(keyboardFunc);
    glutSpecialFunc(specialFunc);
    glutReshapeFunc(reshapeFunc);
    
    glViewport(0, 0, SceneWidth, SceneHeight);
    glLoadIdentity();
    glOrtho(0.0, SceneWidth - 1.0, 0.0, SceneHeight - 1.0, -1.0,  1.0);
}

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
    char *tempString;
    int n, m = -1, drawLoc, lBrack = -1, rBrack = -1, eqsign = -1, comsign = -1, x, y, scanout;
    float RVal, GVal, BVal;
    
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
        if (!TheEnd)
        {
            // Check for no file errors
            if (!strcmp(line, "No file?"))
            {
                // No file provided. Bad.
                printf("Error: No file was passed to the DAMSON compiler.\n");
                return 0;
            }
            // Are we at the end?
            if (strlen(line) > 10)
            {
                tempString = malloc(sizeof(char) * 11);
                memset(tempString, 0, 11);
                memcpy(&tempString[0], &line[0], 10);
                if (!strcmp(tempString, "Workspace:"))
                {
                    // Recognised keyword. It's highly probable we're at the end.
                    // Raise the end flag.
                    TheEnd = 1;
                    free(tempString);
                    // ANd return to the calling function
                    return 2;
                }
                free(tempString);
            } else if (strlen(line) < 6)
            {
                // Line is too short to be anything useful
                return 3;
            }
            // If here, we're not at the end. Look for recognisable input
            for (n = 0; n < (strlen(line) - 4); n++)
            {
                tempString = malloc(sizeof(char) * 5);
                memset(tempString, 0, 5);
                memcpy(&tempString[0], &line[n], 4);
                if (!strcmp(tempString, "draw"))
                {
                    // Found the word draw. We can try parsing this information
                    m = n;
                    free(tempString);
                    break;
                }
                free(tempString);
            }
            // Check to see if these are equal. If so, then draw was found
            if (m == n)
            {
                drawLoc = m;
                for (n = drawLoc + 4; n < strlen(line); n++)
                {
                    switch(line[n])
                    {
                        case '(':
                            if (lBrack < 0)
                                lBrack = n;
                            else
                            {
                                // Too many brackets
                                printf("Warning: Disfigured draw call on line %i. Too many parentheses.\n", lineNo);
                                return 4;
                            }
                            break;
                        case ')':
                            if (lBrack < 0)
                            {
                                printf("Warning: Parentheses could be out of order on line %i\n", lineNo);
                                return 5;
                            }
                            if (rBrack < 0)
                                rBrack = n;
                            else
                            {
                                // Too many brackets
                                printf("Warning: Disfigured draw call on line %i. Too many parentheses.\n", lineNo);
                                return 5;
                            }
                            break;
                        case '=':
                            if (rBrack < 0)
                            {
                                printf("Warning: Equals encountered before closing parentheses on line %i.\n", lineNo);
                            }
                            if (eqsign < 0)
                                eqsign = n;
                            else
                            {
                                // Too many brackets
                                printf("Warning: Disfigured draw call on line %i. Too many equals.\n", lineNo);
                                return 6;
                            }
                            break;
                        case ',':
                            if (comsign >= 0)
                            {
                                printf("Warning: Multiple commas encountered on line %i.\n", lineNo);
                                return 7;
                            }
                            if (lBrack >= 0 && rBrack < 0)
                            {
                                comsign = n;
                            }
                        // default:
                            // Do nothing.
                    }
                }
                
                // By this point, we should know where we are:
                if (lBrack < 0 || rBrack < 0 || eqsign < 0 || comsign < 0)
                {
                    // Invalid line.
                    return 8;
                }
                
                // Check scene dimensions
                if (SceneHeight == 0 || SceneWidth == 0)
                {
                    printf("Error: Found draw command before scene dimensions defined on line %i.\n", lineNo);
                    return 0;
                }
                
                // If here, we have everything we need. Let's try parsing some of this information
                
                tempString = malloc(sizeof(char) * (rBrack - lBrack + 1));
                memset(tempString, 0, (rBrack - lBrack + 1));
                memcpy(&tempString[0], &line[lBrack + 1], rBrack - lBrack);
                scanout = sscanf(tempString, "%i, %i", &x, &y);
                free(tempString);
                
                if (scanout == EOF || scanout < 2)
                {
                    printf("Could not parse coordinates from draw command on line %i\n", lineNo);
                    return 9;
                }
                
                tempString = malloc(sizeof(char) * (strlen(line) - eqsign));
                memset(tempString, 0, strlen(line) - eqsign);
                memcpy(&tempString[0], &line[eqsign + 1], strlen(line) - eqsign - 1);
                scanout = sscanf(tempString, "%f %f %f", &RVal, &GVal, &BVal);
                free(tempString);
                
                if (scanout == EOF || scanout < 3)
                {
                    printf("Could not parse RGB values from draw command on line %i\n", lineNo);
                    return 9;
                }
                
                // If here, we've successfully extracted RGB values and coordinate information.
                printf("X: %i Y: %i --> R: %f G: %f B: %f\n", x, y, RVal, GVal, BVal);
                return 100;
            }
            // No draw keyword was found. This could be a scene definition
            if (strlen(line) > 17)
            {
                for (n = 0; n < strlen(line); n++)
                {
                    tempString = malloc(sizeof(char) * 10);
                    memset(tempString, 0, 10);
                    memcpy(&tempString[0], &line[n], 9);
                    if (!strcasecmp(tempString, "dimension"))
                    {
                        // Found the word draw. We can try parsing this information
                        m = n;
                        free(tempString);
                        break;
                    }
                    free(tempString);
                }
                if (m != n)
                {
                    // Assume this is debug information.
                    return 3;
                }
                
                // If here, we know the word dimensions. Try to find the word scene too.
                for (n = 0; n < strlen(line); n++)
                {
                    tempString = malloc(sizeof(char) * 6);
                    memset(tempString, 0, 6);
                    memcpy(&tempString[0], &line[n], 5);
                    if (!strcasecmp(tempString, "scene"))
                    {
                        // Found the word draw. We can try parsing this information
                        m = n;
                        free(tempString);
                        break;
                    }
                    free(tempString);
                }
                if (m != n)
                {
                    // Assume this is debug information.
                    return 3;
                }
                
                // If here, we have the keywords scene and dimensions. We can therefore parse this:
                for (n = strlen(line) - 1; n >= 0; n--)
                    if ((line[n] < '0' || line[n] > '9') && (line[n] != ' ' && line[n] != '\t'))
                        break;
                if (n == 0)
                {
                    printf("Warning: Could not recognise scene description on line %i.\n", lineNo);
                    return 3;
                }
                tempString = malloc(sizeof(char) * (strlen(line) - n + 2));
                memset(tempString, 0, strlen(line) - n + 2);
                memcpy(&tempString[0], &line[n + 1], strlen(line) - n + 1);
                
                printf("I parsed: \"%s\"\n", tempString);
                
                scanout = sscanf(tempString, "%i %i", &SceneWidth, &SceneHeight);
                free(tempString);
                if (scanout == EOF || scanout < 2)
                {
                    printf("Warning: Unable to understand scene description on line %i.\n", lineNo);
                    return 3;
                }
                
                printf("Scene dimensions recognised (%i x %i)\n", SceneWidth, SceneHeight);
            }
            
            // Assume this is debug information.
            return 3;
        }
        else
        {
            // This is the end...
            return 2;
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
    
    printf("Finished parsing input.\n\n");
    
    return 0;
}
