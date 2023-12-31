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
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "mfs.h"
#include "fsDirectory.h"

#define MAX_PATH_LENGTH 255

struct fs_diriteminfo *fs_readdir(fdDir *dirp)
{
    // if (dirp == NULL) {
    //     return NULL;
    // }
    // DirEntry *entry = getEntryFromPath(dirp->di->d_name);
    // if (getEntryFromPath(dirp->di->d_name) == NULL) {
    //     printf("Directory does not exist: %s\n", dirp->di->d_name);
    //     return NULL;
    // }
    // free(entry);
    if (dirp->dirEntryPosition < dirp->d_reclen)
    {
        return dirp->dirItemArray[dirp->dirEntryPosition++];
    }
    return NULL;
}

void *getObjPath(const char *path){
    char *nameBuffer = malloc(sizeof(char)*NAMESIZE);
    Directory *parent = parsePath(path,nameBuffer);
    if(parent == NULL){
        free(nameBuffer);
        return NULL;
    }
    DirEntry *entry = seekDirectory(parent,nameBuffer);
    free(nameBuffer);
    if(entry == NULL){
        freeDirectoryPtr(parent);
        return NULL;
    }

    void *retObject = readDEntry(entry);
    
    freeDirectoryPtr(parent);
    return retObject;


}

int replaceString(char *oldString, char *newStringBuffer)
{
    int backwardJ = strnlen(oldString, MAXSTRINGLENGTH) - 1;
    int forwardI = 0;

    while (backwardJ >= 0)
    {
        newStringBuffer[forwardI++] = oldString[backwardJ--];
    }
    newStringBuffer[forwardI] = '\0';

    return forwardI;
}

int strRemoveLastElement(char *oldString, char *newStringBuffer)
{
    char *delim = "/";
    char *intermediateStringBuffer = malloc(sizeof(char) * strnlen(oldString, MAXSTRINGLENGTH));
    char *temp = oldString;

    replaceString(oldString, intermediateStringBuffer);

    strtok(intermediateStringBuffer, delim);

    int rmTok = strlen(intermediateStringBuffer) + 1;

    char *rootCheck = strtok(NULL, delim);

    if (rootCheck == NULL)
    {
        strncpy(intermediateStringBuffer, "/", 2);
    }
    else
    {

        for (int i = 0; i < rmTok; i++)
        {
            intermediateStringBuffer[i] = '\0';
        }

        strncat(intermediateStringBuffer, oldString, strlen(oldString) - rmTok);
    }

    char *temp2 = malloc(sizeof(char) * strnlen(intermediateStringBuffer, MAXSTRINGLENGTH));
    replaceString(intermediateStringBuffer, temp2);
    int length = replaceString(temp2, newStringBuffer);

    free(intermediateStringBuffer);
    free(temp2);

    return length;
}


int fs_stat(const char *pathname, struct fs_stat *buf)
{
    DirEntry *entry = getEntryFromPath(pathname);

    if (entry == NULL)
    {
        printf("Directory does not exist: %s\n", pathname);
        return 1;
    }

    buf->st_size = entry->dirEntBlockInfo.size * getSizeofBlocks();
    buf->st_blksize = entry->dirEntBlockInfo.size;
    buf->st_blocks = (buf->st_size + 511) / 512;
    buf->st_accesstime = entry->lastAccessTime;
    buf->st_modtime = entry->modificationTime;
    buf->st_createtime = entry->creationTime;

    free(entry);
    return 0;
}

int fs_rmdir(const char *pathname)
{
    char *nameBuffer = malloc(sizeof(char) * NAMESIZE);
    Directory *folder = parsePath(pathname, nameBuffer);
    if (folder == NULL)
    {
        printf("Path is invalid\n");
        free(nameBuffer);
        return -1;
    }
    DirEntry *entries = seekDirectory(folder, nameBuffer);
    free(nameBuffer);

    if (entries == NULL || entries->isDirectory == 0)
    {
        printf("A File with that name does not exist\n");
        freeDirectoryPtr(folder);
        return -1;
    }
    Directory *oldDirectory = readDEntry(entries);
    if (isDirectoryEmpty(oldDirectory) == 0)
    {
        printf("Directory is not empty. Cannot delete\n");
        return -1;
    }

    deleteDirectory(oldDirectory);
    unassignDEntry(entries);
    writeDirectory(folder);

    free(oldDirectory);
    freeDirectoryPtr(folder);
    return 0;
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
        tempEntry = seekDirectory(tempDir, arguments[i]);
        if (tempEntry != NULL && tempEntry->isDirectory == 1)
        {
            // Update tempDir to the found subdirectory
            oldDir = tempDir;
            tempDir = (Directory *)readDEntry(tempEntry);
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
    DirEntry *retObject = seekDirectory(parent, nameBuffer);

    // Free the nameBuffer as it's no longer needed
    free(nameBuffer);

    // If the DirEntry is not found, free resources and return NULL
    if (retObject == NULL)
    {
        freeDirectoryPtr(parent);
        return NULL;
    }

    // Copy the DirEntry to a new object
    retObject = copyDEntry(retObject);

    // Free the parent Directory as it's no longer needed
    freeDirectoryPtr(parent);

    // Return the DirEntry pointer
    return retObject;
}

int fs_mkdir(char *pathname, mode_t mode)
{
    int retValue = 0;
    char *nameBuffer = malloc(sizeof(char) * NAMESIZE);
    Directory *parentDir = parsePath(pathname, nameBuffer);
    if (parentDir == NULL)
    {
        printf("Path is not valid\n try again!");
        retValue = -1;
    }
    else if (seekDirectory(parentDir, nameBuffer) != NULL)
    {
        printf("That name is in use, change it please!");
        retValue = -1;
    }
    else
    {
        DirEntry *newEntry = createDEntry(nameBuffer, sizeof(Directory), 1);
        retValue = assignDEntryToDirectory(newEntry, parentDir);
        Directory *newDir = createDirectory(newEntry, parentDir);
        writeDirectory(newDir);
        writeDirectory(parentDir);
        free(newEntry);
    }
    free(nameBuffer);
    freeDirectoryPtr(parentDir);
    return retValue;
}

int fs_isDir(char *pathname)
{
    // Initialize the return value to indicate it's not a directory
    int isDirectory = 0;

    // Get the directory entry from the given path
    DirEntry *entry = getEntryFromPath(pathname);

    // Check if the entry exists and is a directory
    if (entry != NULL && entry->isDirectory == 1)
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
    if (entry != NULL && entry->isDirectory == 0)
    {
        retValue = 1; // Set return value to 1 (file found)
    }

    // Free the DirEntry as it's no longer needed
    free(entry);

    // Return the determined value (1 if file, 0 otherwise)
    return retValue;
} 

fdDir *fs_opendir(char *pathname)
{
      Directory *dir = NULL;
    if (strncmp(pathname, "/", 2) != 0)
    {
        dir = (Directory *)getObjPath(pathname);
        if (dir == NULL)
        {
            printf("Failed to open directory\n");
            return NULL;
        }
    }
    else
    {
        dir = getRootDirectory();
    }

    fdDir *returnDirInfo = malloc(sizeof(fdDir));
    struct fs_diriteminfo **itemArray = malloc(sizeof(struct fs_diriteminfo *) * MAXDIRENTRIES);
    returnDirInfo->dirItemArray = itemArray;
    int i = 0;
    while (i < MAXDIRENTRIES && dir->dirArray[i].isUse == 0)
    {
        struct fs_diriteminfo *dirInfo = malloc(sizeof(struct fs_diriteminfo));
        strncpy(dirInfo->d_name, dir->dirArray[i].fileName, NAMESIZE);
        dirInfo->d_reclen = dir->dirArray[i].dirEntBlockInfo.size * getSizeofBlocks();
        dirInfo->fileType = dir->dirArray[i].isDirectory;
        returnDirInfo->dirItemArray[i] = dirInfo;
        i++;
    }
    returnDirInfo->d_reclen = i;
    returnDirInfo->dirEntryPosition = 0;
    returnDirInfo->directoryStartLocation = dir->dirArray[0].dirEntBlockInfo.currentBlock;

    setCWD(dir);
    return returnDirInfo;

    /*
    // Check if the given pathname is a directory
    if (!fs_isDir(pathname))
    {
        return NULL; // The specified path is not a directory.
    }

    // Buffer to store the last component of the path
    char nameBuffer[NAMESIZE];

    // Get the parent Directory using the parsePath function
    Directory *parent = parsePath(pathname, nameBuffer);

    // If parsePath fails or the parent is not a directory, return NULL
    if (parent == NULL)
    {
        return NULL;
    }*/
}

int fs_closedir(fdDir *dirp)
{
    // Check if dirp is NULL
    if (dirp == NULL)
    {
        return -1; // Invalid argument.
    }
   for (int i = 0; i < dirp->d_reclen; i++)
    {
        free(dirp->dirItemArray[i]);
    }
    free(dirp->dirItemArray);
    free(dirp);
    return 0;
}

char *fs_getcwd(char *pathname, size_t size)
{               
    return getCWDPath();
}

// Helper function for fs_delete()
int removeEntryFromDirectory(Directory *dir, DirEntry *entryToRemove)
{
    if (dir == NULL || entryToRemove == NULL)
    {
        return -1; // Invalid arguments.
    }

    int found = 0;
    for (int i = 0; i < MAXDIRENTRIES; i++)
    {
        if (&dir->dirArray[i] == entryToRemove)
        {
            found = 1;

            // Reset the entryToRemove
            memset(entryToRemove, 0, sizeof(DirEntry));

            break; // Exit the loop after finding and "removing" the entry.
        }
    }

    if (found)
    {
        return 0; // Entry "removed" successfully.
    }
    else
    {
        return -1; // Entry not found in the directory.
    }
}
int fs_delete(char *pathname)
{ 
    char *nameBuffer = malloc(sizeof(char) * NAMESIZE);
    Directory *parentDir = parsePath(pathname, nameBuffer);
    if (parentDir == NULL)
    {
        printf("Path is invalid\n");
        free(nameBuffer);
        return -1;
    }

    DirEntry *entry = seekDirectory(parentDir, nameBuffer);
    free(nameBuffer);

    if (entry == NULL || entry->isDirectory == 1)
    {
        printf("A File with that name does not exist\n");
        freeDirectoryPtr(parentDir);
        return -1;
    }
    deleteDEntry(entry);

    unassignDEntry(entry);
    writeDirectory(parentDir);
    freeDirectoryPtr(parentDir);
    return 0;
    /*
    // Step 1: Get the DirEntry associated with the specified filename
    DirEntry *entryToDelete = getEntryFromPath(filename);

    // Step 2: Check if the DirEntry exists and is a file
    if (entryToDelete == NULL || entryToDelete->isDirectory == 1)
    {
        // Entry not found or is a directory, return an error
        printf("File not found: %s\n", filename);
        free(entryToDelete);
        return -1;
    }

    // Step 3: If the DirEntry is a file, delete it from its parent directory
    // Get the parent directory of the file
    char nameBuffer[NAMESIZE];
    Directory *parentDir = parsePath(filename, nameBuffer);

    // Remove the file's entry from the parent directory
    if (removeEntryFromDirectory(parentDir, entryToDelete) == 0)
    {
        // Successfully removed the file from the directory
        printf("File deleted: %s\n", filename);
        // Free associated resources
        free(entryToDelete);
        writeDirectory(parentDir);   // Write changes to disk
        freeDirectoryPtr(parentDir); // Free the parent directory
        return 0;
    }
    else
    {
        // Failed to remove the file from the directory
        printf("Failed to delete file: %s\n", filename);
        free(entryToDelete);
        freeDirectoryPtr(parentDir);
        return -1;
    }
    */
}


int fs_setcwd(char *buf){
     if (strncmp(buf, ".", 2) == 0)
    {
        return 0;
    }

    if (strncmp(buf, "/", 2) == 0)
    {
        setCWDPath("/");
        return setCWD(getRootDirectory());
    }

    Directory *newCWD = getObjPath(buf);
    if (newCWD == NULL)
    {
        printf("Not such a file\n");
        return -1;
    }

    int setRet = setCWD(newCWD);
    if (setRet == 0)
    {
        if (strncmp(buf, "/", 1) != 0)
        {
            char *newCWDPath = malloc(sizeof(char) * MAXSTRINGLENGTH);
            int offset = 0;
            strncpy(newCWDPath, getCWDPath(), MAXSTRINGLENGTH);
            while (strncmp(buf + offset, "../", 3) == 0)
            {
                strRemoveLastElement(newCWDPath, newCWDPath);
                offset += 3;
            }
            if (strncmp(buf + offset, "..", 3) == 0)
            {
                strRemoveLastElement(newCWDPath, newCWDPath);
                setCWDPath(newCWDPath);
            }
            else
            {
                if (strncmp(getCWDPath(), "/", 1) != 0 || strlen(getCWDPath()) != 1)
                {

                    strcat(newCWDPath, "/");
                }
                setCWDPath(strncat(newCWDPath, buf + offset, DIRMAX_LEN));
            }
            free(newCWDPath);
        }
        else
        {
            setCWDPath(buf);
            printf("%s is now the new cwd\n", buf);
        }
    }
    else
    {
        printf("Failed to change current working directory\n");
        setRet = -1;
    }

    return setRet;
}



    
/*
    // Parse the path and get the parent directory
    char nameBuffer[NAMESIZE];
    Directory *parent = parsePath(pathname, nameBuffer);

    // Check if parsing the path failed
    if (parent == NULL)
    {
        printf("Invalid path\n");
        return -1;
    }

    // Find the directory entry in the parent directory
    DirEntry *dirEntry = seekDirectory(parent, nameBuffer);

    // Check if the directory entry doesn't exist
    if (dirEntry == NULL)
    {
        // Generate an absolute path
        char absolutePath[32];
        snprintf(absolutePath, sizeof(absolutePath), "%s/%s", getCWDPath(), pathname);

        // Try to parse the absolute path
        Directory *absPathDir = parsePath(absolutePath, nameBuffer);

        // Check if parsing the absolute path failed
        if (absPathDir == NULL)
        {
            printf("Directory does not exist: %s\n", pathname);
            freeDirectoryPtr(parent);
            return -1;
        }

        // Set the new current working directory
        int result = setCWD(absPathDir);

        // Free allocated resources
        freeDirectoryPtr(parent);
        freeDirectoryPtr(absPathDir);

        return result;
    }

    // Set the new current working directory
    int result = setCWD(readDEntry(dirEntry));

    // Free allocated resources
    freeDirectoryPtr(parent);

    return result;*/


