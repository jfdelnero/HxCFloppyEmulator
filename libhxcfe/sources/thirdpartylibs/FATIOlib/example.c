#include <stdio.h>
#include "fat_filelib.h"

int media_init()
{
    // ...
    return 1;
}

int media_read(uint32_t sector, unsigned char *buffer, uint32_t sector_count)
{
    uint32_t i;

    for (i=0;i<sector_count;i++)
    {
        // ...
        // Add platform specific sector (512 bytes) read code here
        //..

        sector ++;
        buffer += 512;
    }

    return 1;
}

int media_write(uint32_t sector, unsigned char *buffer, uint32_t sector_count)
{
    uint32_t i;

    for (i=0;i<sector_count;i++)
    {
        // ...
        // Add platform specific sector (512 bytes) write code here
        //..

        sector ++;
        buffer += 512;
    }

    return 1;
}

void main()
{
    FL_FILE *file;

    // Initialise media
    media_init();

    // Initialise File IO Library
    fiol_init();

    // Attach media access functions to library
    if (fiol_attach_media(media_read, media_write) != FAT_INIT_OK)
    {
        printf("ERROR: Media attach failed\n");
        return; 
    }

    // List root directory
    fiol_listdirectory("/");

    // Create File
    file = fiol_fopen("/file.bin", "w");
    if (file)
    {
        // Write some data
        unsigned char data[] = { 1, 2, 3, 4 };
        if (fiol_fwrite(data, 1, sizeof(data), file) != sizeof(data))
            printf("ERROR: Write file failed\n");
    }
    else
        printf("ERROR: Create file failed\n");

    // Close file
    fiol_fclose(file);

    // Delete File
    if (fiol_remove("/file.bin") < 0)
        printf("ERROR: Delete file failed\n");

    // List root directory
    fiol_listdirectory("/");

    fiol_shutdown();
}
