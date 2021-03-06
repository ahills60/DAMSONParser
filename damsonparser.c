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
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <GL/glut.h>
#include <time.h>
#include <pthread.h>
// For POSIX piping
#include <unistd.h>
// For PNG files
#include <png.h>

// Program defines
#include "damsonparser.h"

// Defines:
#define MAX_CHARS       65536
// Use the Error function for warnings
#define Warning         Error

// Prototypes
void Error(const char* format, ...);
void initialisePixelStore();
void clearPixelStore();
void reshapeFunc(int newWidth, int newHeight);
void idleFunc(void);
void fadeActivity(void);
void writePNGFile(char *filename);
void keyboardFunc(unsigned char key, int xmouse, int ymouse);
void specialFunc(int key, int x, int y);
static void printToScreen(int inset, const char *format, ...);
void displayFunc(void);
void initialiseGLUT(int argc, char *argv[]);
void setPixel(int x, int y, float RVal, float GVal, float BVal);
int DAMSONHeaderCheck(char *line, int idx);
int ParseLine(char *line, int lineNo);
void ProcessFile(char *filename);
void *ProcessFileThread(void *arg);
void *ProcessPipeThread(void);

// Global Variables
char *HeaderLine1, *HeaderLine2, *HeaderLine3, *TheEndText, *LastErrorMessage = "";
int TheEnd = 0, SceneWidth = 0, SceneHeight = 0, graphicsFlag = -1, NoHeader = 0;
pthread_t procThread;

unsigned int *PixelStore;
unsigned int *ActivityStore;

// Information flags
int DisplayInfo;
int DisplayActivity;

// Use for printing to screen:
int PrintLoc;

// Text buffer:
char ScreenText[256];

// Last read instruction:
char LastReadInstruction[256];
char LastReadErrorLine1[256];
char LastReadErrorLine2[256];
char LastReadErrorRot = 0;

// The end information:
char WorkspaceMessage[256];
char ExecutionMessage[256];
char ComputingMessage[256];
char StandbyTkMessage[256];
char AvgSearchMessage[256];

// Thread for reading
pthread_t input_thread;

// Function for printing errors
void Error(const char* format, ...)
{
    va_list argpointer;
    va_start(argpointer, format);
    vsnprintf(ScreenText, 255, format, argpointer);
    printf("%s", ScreenText);
    if (LastReadErrorRot == 0)
    {
        
        memset(LastReadErrorLine1, 0, sizeof(char) * 256);
        memcpy(&LastReadErrorLine1[0], &ScreenText[0], 255);
        LastReadErrorRot = 1;
    }
    else
    {
        memset(LastReadErrorLine2, 0, sizeof(char) * 256);
        memcpy(&LastReadErrorLine2[0], &ScreenText[0], 255);
        LastReadErrorRot = 0;
    }
    va_end(argpointer);
}

void initialisePixelStore()
{
    // Ensure we have enough memory to store pixel information
    PixelStore = (unsigned int *) malloc(sizeof(unsigned int) * SceneWidth * SceneHeight);
    ActivityStore = (unsigned int *) malloc(sizeof(unsigned int) * SceneWidth * SceneHeight);
    
    // Finally, set the space to null:
    memset(PixelStore, 0, sizeof(unsigned int) * SceneWidth * SceneHeight);
    memset(ActivityStore, 0, sizeof(unsigned int) * SceneWidth * SceneHeight);
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
        ActivityStore[i] = 0 | (255 << 8) | (0 << 16) | (a << 24);
    }
}

// Function to write PNG files
void writePNGFile(char *filename)
{
    int x, y, pixel_size = 3, depth = 8;
    FILE *fp;
    png_structp png_ptr = NULL;
    png_infop info_ptr = NULL;
    png_byte **row_pointers;
    
    // Now write the PNG file
    fp = fopen(filename, "wb");
    if (!fp)
    {
        Error("Error opening file for PNG creation.\n\n");
        return;
    }
    
    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (png_ptr == NULL)
    {
        Error("Error writing PNG structure\n\n");
        goto png_create_write_struct_fail;
    }
    
    info_ptr = png_create_info_struct(png_ptr);
    if (info_ptr == NULL)
    {
        Error("Error creating PNG information structure.\n\n");
        goto png_create_info_struct_fail;
    }
    
    // Set up error handling
    if (setjmp(png_jmpbuf(png_ptr)))
    {
        Error("Error encountered when writing PNG file.\n\n");
        goto png_fail;
    }
    
    // Set the image attributes
    png_set_IHDR(png_ptr, info_ptr, SceneWidth, SceneHeight, depth, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
    
    // Initialise rows of PNG file:
    row_pointers = png_malloc(png_ptr, SceneHeight * sizeof(png_byte *));
    
    for (y = 0; y < SceneHeight; ++y)
    {
        png_byte *row = png_malloc(png_ptr, sizeof(uint8_t) * SceneWidth * pixel_size);
        row_pointers[y] = row;
        for (x = 0; x < SceneWidth; ++x)
        {
            *row++ = (uint8_t) (PixelStore[(SceneHeight - y) * SceneWidth + x] & 0xFF); // R
            *row++ = (uint8_t) ((PixelStore[(SceneHeight - y) * SceneWidth + x] >> 8) & 0xFF); // G
            *row++ = (uint8_t) ((PixelStore[(SceneHeight - y) * SceneWidth + x] >> 16) & 0xFF); // B
        }
    }
    
    // Write the image data to the file pointer:
    png_init_io(png_ptr, fp);
    png_set_rows(png_ptr, info_ptr, row_pointers);
    png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);
    
    // File has been written to by this point. Tidy up.
    printf("PNG file created.\n\n");
    
    // Now free memory:
    for (y = 0; y < SceneHeight; y++)
    {
        png_free(png_ptr, row_pointers[y]);
    }
    png_free(png_ptr, row_pointers);
    
png_fail:
png_create_info_struct_fail:
    // Finally destroy the structure in memory:
    png_destroy_write_struct(&png_ptr, &info_ptr);
png_create_write_struct_fail:
    // Close file pointer
    fclose(fp);
}

// Function to take control of user input elements
void keyboardFunc(unsigned char key, int xmouse, int ymouse)
{
    switch (key)
    {
        case 'a':
        case 'A':
            // Activity
            DisplayActivity = !DisplayActivity;
            break;
        case 'i':
        case 'I':
            // Display information
            DisplayInfo = !DisplayInfo;
            break;
        case 's':
        case 'S':
            // Save image as PNG
            writePNGFile("output.png");
            break;
        case 'q':
        case 'Q':
            exit(0);
    }
    
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
        glRecti(5, PrintLoc, 480, SceneHeight - 5);
        glColor3f(1.0, 1.0, 1.0);
        PrintLoc = SceneHeight - 30;
        
        printToScreen(10, "DAMSON parser version %i.%i.%i (%s)", VERSION_MAJOR, VERSION_MINOR, VERSION_BUILD, VERSION_DATE);
        printToScreen(10, " ");
        printToScreen(10, "DAMSON information:");
        printToScreen(10, "     %s", HeaderLine1);
        printToScreen(10, "     %s", HeaderLine2);
        printToScreen(10, "     %s", HeaderLine3);
        printToScreen(10, " ");
        printToScreen(10, "Last instruction:");
        printToScreen(10, "     %s", LastReadInstruction);
        printToScreen(10, " ");
        if (LastReadErrorLine1[0] > 0)
        {
            printToScreen(10, "Last error or warning:");
            printToScreen(10, "     %s", LastReadErrorLine1);
            printToScreen(10, "     %s", LastReadErrorLine2);
            printToScreen(10, " ");
        }
        if (TheEnd)
        {
            printToScreen(10, "Runtime Summary: ");
            if (WorkspaceMessage[0] > 0)
                printToScreen(10, "     %s", WorkspaceMessage);
            if (ExecutionMessage[0] > 0)
                printToScreen(10, "     %s", ExecutionMessage);
            if (ComputingMessage[0] > 0)
                printToScreen(10, "     %s", ComputingMessage);
            if (StandbyTkMessage[0] > 0)
                printToScreen(10, "     %s", StandbyTkMessage);
            if (AvgSearchMessage[0] > 0)
                printToScreen(10, "     %s", AvgSearchMessage);
            printToScreen(10, " ");
        }
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
    
    printf("Initialising GLUT... ");
    glutInit(&argc, argv);
    printf("Done\n");
    
    glutCreateWindow("DAMSON parser visualiser");
    
    glutDisplayFunc(displayFunc);
    glutIdleFunc(idleFunc);
    glutKeyboardFunc(keyboardFunc);
    glutSpecialFunc(specialFunc);
    glutReshapeFunc(reshapeFunc);
    
    glViewport(0, 0, SceneWidth, SceneHeight);
    glLoadIdentity();
    glOrtho(0.0, SceneWidth - 1.0, 0.0, SceneHeight - 1.0, -1.0,  1.0);
    // printf("Creating visualiser thread...\n");
    // pthread_create(&input_thread, NULL, OpenVisualiser, 0);
    // printf("Visualiser thread created.\n");
}

// Shortcut method for populating the pixelstore and activitystore variables
void setPixel(int x, int y, float RVal, float GVal, float BVal)
{
    int idx = y * SceneWidth + x, iR = (int) ((RVal > 1.0 ? 1.0 : (RVal < 0 ? 0 : RVal)) * 255), iG = (int) ((GVal > 1.0 ? 1.0 : (GVal < 0 ? 0 : GVal)) * 255), iB = (int) ((BVal > 1.0 ? 1.0 : (BVal < 0 ? 0 : BVal)) * 255);
    
    // printf("At <%i, %i>, RGB %f, %f, %f is %i, %i, %i\n", x, y, RVal, GVal, BVal, iR, iG, iB);
    
    PixelStore[idx] = iR | (iG << 8) | (iB << 16);
    ActivityStore[idx] = 0 | (255 << 8) | (0 << 16) | (255 << 24);
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
                Error("Error: No file was passed to the DAMSON compiler.\n");
                return 0;
            }
            tempString = malloc(sizeof(char) * 8);
            memset(tempString, 0, 8);
            memcpy(&tempString[0], &line[0], 7);
            
            // Lookout for the timeout command.
            if (!strcmp(tempString, "Timeout"))
            {
                // Free memory and ignore.
                free(tempString);
                return 3;
            }
            free(tempString);
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
                    // Copy line to workspace variable
                    memcpy(&WorkspaceMessage[0], &line[0], strlen(line));
                    // And return to the calling function
                    return 2;
                }
                free(tempString);
            } else if (strlen(line) < 6)
            {
                // Line is too short to be anything useful
                return 3;
            }
            
            // Store the read line for printing in the visualiser
            memset(LastReadInstruction, 0, 256);
            memcpy(&LastReadInstruction[0], &line[0], (strlen(line) > 255) ? 255 : strlen(line));
            
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
                                Warning("Warning: Disfigured draw call on line %i. Too many parentheses.\n", lineNo);
                                return 4;
                            }
                            break;
                        case ')':
                            if (lBrack < 0)
                            {
                                Warning("Warning: Parentheses could be out of order on line %i\n", lineNo);
                                return 5;
                            }
                            if (rBrack < 0)
                                rBrack = n;
                            else
                            {
                                // Too many brackets
                                Warning("Warning: Disfigured draw call on line %i. Too many parentheses.\n", lineNo);
                                return 5;
                            }
                            break;
                        case '=':
                            if (rBrack < 0)
                            {
                                Warning("Warning: Equals encountered before closing parentheses on line %i.\n", lineNo);
                            }
                            if (eqsign < 0)
                                eqsign = n;
                            else
                            {
                                // Too many brackets
                                Warning("Warning: Disfigured draw call on line %i. Too many equals.\n", lineNo);
                                return 6;
                            }
                            break;
                        case ',':
                            if (comsign >= 0)
                            {
                                Warning("Warning: Multiple commas encountered on line %i.\n", lineNo);
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
                    Error("Error: Found draw command before scene dimensions defined on line %i.\n", lineNo);
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
                    Error("Could not parse coordinates from draw command on line %i\n", lineNo);
                    return 9;
                }
                
                tempString = malloc(sizeof(char) * (strlen(line) - eqsign));
                memset(tempString, 0, strlen(line) - eqsign);
                memcpy(&tempString[0], &line[eqsign + 1], strlen(line) - eqsign - 1);
                scanout = sscanf(tempString, "%f %f %f", &RVal, &GVal, &BVal);
                // printf("%i: \"%s\"\n", lineNo, tempString);
                free(tempString);
                
                if (scanout == EOF || scanout < 3)
                {
                    Error("Could not parse RGB values from draw command on line %i\n", lineNo);
                    
                    return 9;
                }
                
                // If here, we've successfully extracted RGB values and coordinate information.
                // printf("X: %i Y: %i --> R: %f G: %f B: %f\n", x, y, RVal, GVal, BVal);
                
                // Just check that the pixel values do not exceed the scene dimensions
                if (x > SceneWidth || x < 0)
                {
                    Error("Pixel draw x coordinate is outside scenery dimensions on line %i\n", lineNo);
                    
                    return 10;
                }
                if (y > SceneHeight || y < 0)
                {
                    Error("Pixel draw y coordinate is outside scenery dimensions on line %i\n", lineNo);
                    
                    return 10;
                }
                
                setPixel(x, y, RVal, GVal, BVal);
                return 100;
            }
            // Let's check for the keyword "error".
            if (strlen(line) > 5)
            {
                tempString = malloc(sizeof(char) * 6);
                for (n = 0; n < strlen(line) - 6; n++)
                {
                    // Copy the contents of the string to memory
                    memset(tempString, 0, sizeof(char) * 6);
                    memcpy(&tempString[0], &line[n], sizeof(char) * 5);
                    if (!strcasecmp(tempString, "error"))
                    {
                        // Yes, it's an error message. Best way to handle this is to print this error
                        // and recommend further debugging outside the DAMSON parser. Finally, safely
                        // free any memory and return with an error condition.
                        Error("An error was encountered in DAMSON:\n");
                        Error("     %s\n", line);
                        Error("Please debug outside the DAMSON parser environment.\n");
                        free(tempString);
                        return -1;
                    }
                    
                }
                // Not an error. Free up memory.
                free(tempString);
            }
            
            // No draw keyword was found. This could be a scene definition
            if (strlen(line) > 17)
            {
                for (n = 0; n < strlen(line) - 10; n++)
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
                for (n = 0; n < strlen(line) - 6; n++)
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
                    Error("Warning: Could not recognise scene description on line %i.\n", lineNo);
                    return 3;
                }
                tempString = malloc(sizeof(char) * (strlen(line) - n + 2));
                memset(tempString, 0, strlen(line) - n + 2);
                memcpy(&tempString[0], &line[n + 1], strlen(line) - n + 1);
                
                scanout = sscanf(tempString, "%i %i", &SceneWidth, &SceneHeight);
                free(tempString);
                if (scanout == EOF || scanout < 2)
                {
                    Error("Warning: Unable to understand scene description on line %i.\n", lineNo);
                    return 3;
                }
                
                printf("Scene dimensions recognised (%i x %i)\n", SceneWidth, SceneHeight);
                initialisePixelStore();
                graphicsFlag = 1;
            }
            
            // Assume this is debug information.
            return 3;
        }
        else
        {
            // This is the end...
            // There are only so many possibilities that can be displayed in "the end":
            switch (line[0])
            {
                case 'E':
                    // Execution time
                    memcpy(&ExecutionMessage[0], &line[0], strlen(line));
                    break;
                case 'C':
                    // Computing time
                    memcpy(&ComputingMessage[0], &line[0], strlen(line));
                    break;
                case 'S':
                    // Standby ticks
                    memcpy(&StandbyTkMessage[0], &line[0], strlen(line));
                    break;
                case 'A':
                    // Average Search Length
                    memcpy(&AvgSearchMessage[0], &line[0], strlen(line));
                    break;
                default:
                    Warning("Warning: Unrecognised line in DAMSON end summary.\n");
            }
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
    int lineNo = 1, dcheck = 0;
    char *line = NULL;
    size_t len;
    ssize_t lsize;
    
    fp = fopen(filename, "r");
    
    // Ensure file exists and can be read:
    if (fp == NULL)
    {
        Error("\nError opening file. Ensure filename and path is valid.\n");
        return;
    }
    
    while((lsize = getline(&line, &len, fp)) != -1)
    {
        if (lineNo <= 3 && !NoHeader)
        {
            dcheck = DAMSONHeaderCheck(line, lineNo - 1);
            if (dcheck < 1)
            {
                Error("Error processing header on line %i.\n\n", lineNo);
                fclose(fp);
                graphicsFlag = -1;
                return;
            }
        }
        else
        {
            dcheck = ParseLine(line, lineNo);
            if (dcheck < 1)
            {
                Error("Error processing script on line %i.\n\n", lineNo);
                fclose(fp);
                graphicsFlag = -1;
                return;
            }
        }
        lineNo++;
    }
    // Once we're done, we should free up the memory that was used by the line variable
    free(line);
    fclose(fp);
}

void *ProcessFileThread(void *arg)
{
    char *filename = (char *) arg;
    
    ProcessFile(filename);
    
    printf("File read complete.\n\n");
}

// This function processes files.
void ProcessPipe()
{
    int ptr = 0, lineNo = 0, dcheck = 0, c, tries = 0;
    char line[MAX_CHARS], ch;
    
    // Initialise the line
    memset(line, 0, MAX_CHARS);
    
    while((c = getchar()) && tries < 5)
    {
        if (c == EOF)
        {
            tries++;
            continue;
        }
            
        // Convert int to char
        ch = (char) c;
        // Add this character to the buffer
        line[ptr++] = (char) ch;
        
        // Check to see if the buffer should be emptied and the command processed.
        if (ch == '\0' || ch == '\n')
        {
            lineNo++;
            if (lineNo > 0 && lineNo <= 3 && !NoHeader)
            {
                dcheck = DAMSONHeaderCheck(&line, lineNo - 1);
                if (dcheck < 1)
                {
                    Error("Error processing header on line %i.\n\n", lineNo);
                    graphicsFlag = -1;
                    return;
                }
            }
            else if (lineNo > 3 || NoHeader)
            {
                dcheck = ParseLine(&line, lineNo);
                if (dcheck < 1)
                {
                    Error("Error processing script on line %i.\n\n", lineNo);
                    graphicsFlag = -1;
                    return;
                }
            }
            
            // Reset the line buffer
            ptr = 0;
            memset(line, 0, MAX_CHARS);
        }
    }
    // Check to see if the buffer is empty
    if (ptr > 0)
    {
        // Contents of buffer should be processed
        lineNo++;
        if (lineNo <= 3 && !NoHeader)
        {
            dcheck = DAMSONHeaderCheck(line, lineNo - 1);
            if (dcheck < 1)
            {
                Error("Error processing header on line %i.\n\n", lineNo);
                graphicsFlag = -1;
                return;
            }
        }
        else
        {
            dcheck = ParseLine(line, lineNo);
            if (dcheck < 1)
            {
                Error("Error processing script on line %i.\n\n", lineNo);
                graphicsFlag = -1;
                return;
            }
        }
    }
    // Once we're done, we should free up the memory that was used by the line variable
}

void *ProcessPipeThread(void)
{
    ProcessPipe();
    printf("Pipe read complete.\n");
    
}

int main(int argc, char *argv[])
{
    char *currObj, *parVal = "", *filename = "\0";
    int i, n, a, isParam;
    
    printf("\nDAMSON Parser ");
    printf("Version: %i.%i.%i (%s)\n", VERSION_MAJOR, VERSION_MINOR, VERSION_BUILD, VERSION_DATE);
    printf("Author: Andrew Hills (a.hills@sheffield.ac.uk)\n\n");
    
    // Initialise variables
    memset(LastReadInstruction, 0, 256);
    memset(LastReadErrorLine1, 0, 256);
    memset(LastReadErrorLine2, 0, 256);
    memset(WorkspaceMessage, 0, 256);
    memset(ExecutionMessage, 0, 256);
    memset(ComputingMessage, 0, 256);
    memset(StandbyTkMessage, 0, 256);
    memset(AvgSearchMessage, 0, 256);
    
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
            if (!strcmp(parVal, "noheader"))
                NoHeader = 1;
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
                    Error("Unrecognised input \"%s\"\n", parVal);
            }
            else // No parameter was defined. Skip to the next argument.
                continue;
        }
    }
    
    // Quick check to see if the filename variable was specified.
    if (filename[0] == '\0')
    {
        // No. At this point, we could check for piped input.
        if (isatty(fileno(stdin)))
        {
            // Connection is via a terminal session
            Error("No input file specified\n\n");
        }
        else
        {
            // Connection is not connected to a terminal. Could be a pipe or file.
            
            graphicsFlag = 0;
            pthread_create(&procThread, NULL, ProcessPipeThread, 0);
        }
    }
    else
    {
        // Yes. Filename specified. Let the ProcessFile function handle this request.
        printf("Input file \"%s\" specified\n\n", filename);
        
        // Set the graphics flag and then create a thread
        graphicsFlag = 0;
        pthread_create(&procThread, NULL, ProcessFileThread, (void *) filename);
    }
    while(graphicsFlag == 0)
    {
        // Do nothing
    }
    if (graphicsFlag == 1)
    {
        initialiseGLUT(argc, argv);
        glutMainLoop();
    }
    // printf("Finished parsing input.\n\n");
    // pthread_join(input_thread, &status);
    
    exit(0);
}
