/**************************************************************
 * Class:  CSC-415-03 Fall 2023
 * Names: Edmund Huang, Jimmy Pan, Juan Estrada, Kripa Pokhrel
 * Student IDs: 918426293, 920950183, 923058731, 922961998
 *
 * GitHub Name: EdmUUUndo
 *
 * Group Name: HumanOS
 *
 * Project: Basic File System
 *
 * File: mfs.c
 *
 * Description: functions to handle shell commands
 **************************************************************/
#include "mfs.h"

int fs_mkdir(const char *pathname, mode_t mode)
{
    // Ensure input parameter are valid, name is not empty and follows conventions

    // Find free blocks getfreeblocks()
    // Creat Directory Entry ()
    // Update parent directory
    // writing to second storage LBAwrite
}

int fs_isDir(const char *pathname)
{
    // Initialize the return value to indicate it's not a directory
    int isDirectory = 0;

    // Get the directory entry from the given path
    DirEntry *entry = getEntryFromPath(path);

    // Check if the entry exists and is a directory
    if (entry != NULL && entry->isDir == 1)
    {
        // Set the return value to indicate it's a directory
        isDirectory = 1;
    }

    // Free the allocated memory for the directory entry
    free(entry);

    // Return the result
    return isDirectory;
}

int fs_isFile(char *path)
{
    // Initialize the return value to 0 (not a file)
    int retValue = 0;

    // Get the DirEntry associated with the path
    DirEntry *entry = getEntryFromPath(path);

    // Check if the DirEntry is not NULL and represents a file (not a directory)
    if (entry != NULL && entry->isDir == 0)
    {
        retValue = 1; // Set return value to 1 (file found)
    }

    // Free the DirEntry as it's no longer needed
    free(entry);

    // Return the determined value (1 if file, 0 otherwise)
    return retValue;
}

Directory *fs_opendir(const char *pathname)
{
    // Check if the given pathname is a directory
    if (!fs_isDir(pathname)) {
        return NULL; // The specified path is not a directory.
    }

    // Buffer to store the last component of the path
    char nameBuffer[NAMESIZE];

    // Get the parent Directory using the parsePath function
    Directory *parent = parsePath(pathname, nameBuffer);

    // If parsePath fails or the parent is not a directory, return NULL
    if (parent == NULL) {
        return NULL;
    }

    // Search for the DirEntry in the parent Directory
    DirEntry *dirEntry = searchDirectory(parent, nameBuffer);

    // Free the parent Directory as it's no longer needed
    freeDirectoryPtr(parent);

    // Check if the DirEntry is not found or is not a directory
    if (dirEntry == NULL || dirEntry->isDir != 1) {
        return NULL; // The specified directory does not exist.
    }

    // Open the specified directory and return it
    Directory *openedDir = (Directory *)readDirEntry(dirEntry);
    
    // Return the opened directory
    return openedDir;
}


/**
 * Parses the given path and returns the corresponding Directory.
 * Also populates the provided nameBuffer with the last component of the path.
 */
Directory *parsePath(const char *path, char *nameBuffer)
{
    // Reset nameBuffer to NULL in case it's not empty
    memset(nameBuffer, '\0', sizeof(char) * NAMESIZE);

    // Tokenize the path using '/' and ',' as delimiters
    char **arguments = malloc(strlen(path) * sizeof(char *));
    int counter = 0;

    // Allocate memory for a temporary copy of the path
    char *tempPath = malloc((strlen(path) + 1) * sizeof(char));
    strncpy(tempPath, path, strlen(path) + 1);

    // If path[0] is '/', it's an absolute path
    if (strncmp(tempPath, "/", 1) == 0)
    {
        arguments[counter] = "/";
        counter++;
    }

    // Tokenize the path and store each component in arguments array
    char *token = strtok(tempPath, "/ ,");
    while (token != NULL)
    {
        arguments[counter] = token;
        counter++;
        token = strtok(NULL, "/ ,");
    }

    counter--;

    // Create Directory pointer
    Directory *tempDir;

    // Check if the first argument is "/"
    if (strncmp(arguments[0], "/", 1) == 0)
    {
        // If it is, tempDir points to the Root directory
        tempDir = getRootDirectory();
    }
    else
    {
        // Otherwise, tempDir points to the current working directory (cwd)
        tempDir = getCWD();
    }

    DirEntry *tempEntry;
    Directory *oldDir;

    // Traverse through the DirEntry array to find the last component of the path
    for (int i = 1; i < counter; i++)
    {
        tempEntry = searchDirectory(tempDir, arguments[i]);
        if (tempEntry != NULL && tempEntry->isDir == 1)
        {
            // Update tempDir to the found subdirectory
            oldDir = tempDir;
            tempDir = (Directory *)readDirEntry(tempEntry);
            freeDirectoryPtr(oldDir);
        }
        else
        {
            // Clean up and return NULL if a component is not found or is not a directory
            freeDirectoryPtr(tempDir);
            free(arguments);
            return NULL;
        }
    }

    // Copy the last component to nameBuffer
    strncpy(nameBuffer, arguments[counter], strlen(arguments[counter]));

    // Clean up and return the final Directory pointer
    free(arguments);
    return tempDir;
}

/**
 * Retrieves the DirEntry associated with the given path.
 */
DirEntry *getEntryFromPath(const char *path)
{
    // Buffer to store the last component of the path
    char *nameBuffer = malloc(sizeof(char) * NAMESIZE);

    // Get the parent Directory using the parsePath function
    Directory *parent = parsePath(path, nameBuffer);

    // If parsePath fails, free resources and return NULL
    if (parent == NULL)
    {
        free(nameBuffer);
        return NULL;
    }

    // Search for the DirEntry in the parent Directory
    DirEntry *retObject = searchDirectory(parent, nameBuffer);

    // Free the nameBuffer as it's no longer needed
    free(nameBuffer);

    // If the DirEntry is not found, free resources and return NULL
    if (retObject == NULL)
    {
        freeDirectoryPtr(parent);
        return NULL;
    }

    // Copy the DirEntry to a new object
    retObject = copyDirEntry(retObject);

    // Free the parent Directory as it's no longer needed
    freeDirectoryPtr(parent);

    // Return the DirEntry pointer
    return retObject;
}
