#include "tinyFS.h"
#include "libTinyFS.h"

int status;

int main(int argc, char* argv[]) {

    /* upon the first run of the program, it should create the disk and set it up. Then, the user can look at it and inspect it
       as part of the demonstration. upon the second run of the program, the contents of the disk will be deleted to demonstrate 
       that functionality. run make clean to delete the disk again, or delete it manually. */
        
    /* if you can't mount... */
    if(tfs_mount("demo.dsk") < 0) {
        /* make a new disk and set it up */
        status = tfs_mkfs("demo.dsk", 8192);
        if(status < 0) {
            return status;
        }
          
        status = tfs_mount("demo.dsk");
        if(status < 0) {
            printf("Mount error (%d)\n", status);
            exit(EXIT_FAILURE);
        }


        /* Creating a file in the root directory */
        int readme = tfs_openFile("/README");
        if(readme < 0) {
            printf("Checking sydney status %d\n", readme);
        }

        /* Create a foods directory */
        status = tfs_createDir("/foods");
        if(status < 0) {
            printf("create dir error (%d)\n", status);
            exit(EXIT_FAILURE);
        }

        status = tfs_createDir("/foods/savory");
        if(status < 0) {
            printf("create dir error (%d)\n", status);
            exit(EXIT_FAILURE);
        }

        status = tfs_createDir("/foods/sweet");
        if(status < 0) {
            printf("create dir error (%d)\n", status);
            exit(EXIT_FAILURE);
        }

        status = tfs_createDir("/foods/fastfood");
        if(status < 0) {
            printf("create dir error (%d)\n", status);
            exit(EXIT_FAILURE);
        }

        int waffles = tfs_openFile("/foods/sweet/waffles");
        if(waffles < 0) {
            printf("open file error (%d)\n", waffles);
            exit(EXIT_FAILURE);
        }

        int pancakes = tfs_openFile("foods/sweet/pancakes");
        if(pancakes < 0) {
            printf("open file error (%d)\n", pancakes);
            exit(EXIT_FAILURE);
        }

        int cake = tfs_openFile("foods/sweet/cake");
        if(pancakes < 0) {
            printf("open file error (%d)\n", cake);
            exit(EXIT_FAILURE);
        }

        int sausage = tfs_openFile("/foods/savory/sausage");
        if(sausage < 0) {
            printf("open file error (%d)\n", sausage);
            exit(EXIT_FAILURE);
        }

        int chicken = tfs_openFile("/foods/savory/chicken");
        if(sausage < 0) {
            printf("open file error (%d)\n", chicken);
            exit(EXIT_FAILURE);
        }

        int steak = tfs_openFile("/foods/savory/steak");
        if(sausage < 0) {
            printf("open file error (%d)\n", steak);
            exit(EXIT_FAILURE);
        }

        int snickerdoodle = tfs_openFile("/foods/sweet/snickerdoodle");
        if(snickerdoodle >= 0) {
            printf("Trying to create file \"snickerdoodle\" which is too long did not result in an error\n");
            exit(EXIT_FAILURE);
        }

        int apple = tfs_openFile("/foods/fruits/apple");
        if(apple >= 0) {
            printf("Trying to create file \"apple\" in non-existant directory \"fruits\" did not result in an error.\n");
            exit(EXIT_FAILURE);
        }

        int sausage2 = tfs_openFile("/foods/savory/sausage");
        if(sausage2 < 0 || sausage2 == sausage) {
            printf("Failed to open duplicate file");
            exit(EXIT_FAILURE);
        }

        tfs_closeFile(sausage2);

        /* lets do some writing !! */
        char waffles_recipe[] = "Take a dash of love and a lit bit of sugar. Put it in your waffle iron and make some deliciousness !!"
                                "And then stir ! And then stir ! And then stir ! And then stir ! And then stir ! And then stir ! And then stir ! And then stir ! "
                                "And then stir ! And then stir ! And then stir ! And then stir ! And then stir ! And then stir ! And then stir ! And then stir ! ";
        status = tfs_writeFile(waffles, waffles_recipe, strlen(waffles_recipe));
        if(status < 0) {
            printf("write fail (%d)", status);
            exit(EXIT_FAILURE);
        }


        /* Not lets read what we just wrote !! The recipe for delicious waffles :) */
        printf("Reading the recipe for waffles\n");

        char s;
        int i = 0;
        status = tfs_readByte(waffles, &s);
        while(status >= 0) {
            i++;
            printf("%c", s);
            status = tfs_readByte(waffles, &s);
        }

        /* print out the contents of the disk */
        printf("\n\nThe entire contents of the disk:\n");
        tfs_readdir();

    } else {
        /* lets delete some files */

        printf("Here are the disk contents before we start the deletions \n");
        tfs_readdir();

        int steak = tfs_openFile("/foods/savory/steak");
        if(steak < 0) {
            printf("open file error (%d)\n", steak);
            exit(EXIT_FAILURE);
        }

        char steakBlog[] = "Eat some yummy steak";
        status = tfs_writeFile(steak, steakBlog, sizeof(steakBlog));

        printf("\n\nFile data for the file \"steak\"\n");
        tfs_readFileInfo(steak);

        int sausage = tfs_openFile("/foods/savory/sausage");
        if(sausage < 0) {
            printf("open file error (%d)\n", sausage);
            exit(EXIT_FAILURE);
        }

        status = tfs_deleteFile(steak);
        if(status < 0) {
            printf("delete file error (%d)\n", status);
            exit(EXIT_FAILURE);
        }

        status = tfs_removeAll("foods/sweet");
        if(status < 0) {
            printf("removeAll error (%d)\n", status);
            exit(EXIT_FAILURE);
        }

        status = tfs_removeDir("foods/fastfood");
        if(status < 0) {
            printf("removeDir error (%d)\n", status);
            exit(EXIT_FAILURE);
        }

        status = tfs_rename(sausage, "hotdog");
        if(status < 0) {
            printf("rename error (%d)\n", status);
            exit(EXIT_FAILURE);
        }

        printf("\n\nFile data for the file \"hotdog\"\n");
        tfs_readFileInfo(sausage);

        printf("\n\nHere are the disk contents after we have deleted some stuff\n");
        tfs_readdir();

        printf("\n\nEnd of Demo. Remeber to delete the disk before you rerun !! You can run \"make rmdemodisk\" to do this\n");

    }



    return 0;
}