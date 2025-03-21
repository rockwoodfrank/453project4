#include "tinyFS.h"
#include "libTinyFS.h"

/* ~ PLEASE READ ~ */
/* upon the first run of the program, it should create the disk and set it up. Then, the user can look at it and inspect it
    as part of the demonstration. upon the second run of the program, the contents of the disk will be deleted to demonstrate 
    that functionality. run make clean to delete the disk again, or delete it manually. */

int status;

int main(int argc, char* argv[]) {

    if((status = tfs_mount("safetyBitWrong.dsk")) < 0) {
        printf("Mount failed due to corruption (this is the desired output). The reported Error code is (%d)\n", status);
    }

    if((status = tfs_mount("floatingFreeBlocks.dsk")) < 0) {
        printf("Mount failed due to corruption (this is the desired output). The reported Error code is (%d)\n", status);
    }

    if((status = tfs_mount("sizeMismatch.dsk")) < 0) {
        printf("Mount failed due to corruption (this is the desired output). The reported Error code is (%d)\n", status);
    }


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
        printf("\n\nReading the recipe for waffles\n");

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

        printf("Here are the disk contents before we start the deletions:\n");
        tfs_readdir();

        int steak = tfs_openFile("/foods/savory/steak");
        if(steak < 0) {
            printf("open file error (%d)\n", steak);
            exit(EXIT_FAILURE);
        }

        int waffles = tfs_openFile("/foods/sweet/waffles");
        if(waffles < 0) {
            printf("open file error (%d)\n", waffles);
            exit(EXIT_FAILURE);
        }

        /* Access some files so the timestamps get updated */

        /* here the access time gets updated */
        tfs_seek(waffles, 270);
        char s;
        status = tfs_readByte(waffles, &s);
        if(status < 0) {
            printf("readByte error (%d)\n", status);
            exit(EXIT_FAILURE);
        }

        /* here the access and modification time gets updated */
        char steakBlog[] = "Eat some yummy steak";
        status = tfs_writeFile(steak, steakBlog, sizeof(steakBlog));
        if(status < 0) {
            printf("writeFile error (%d)\n", status);
            exit(EXIT_FAILURE);
        }


        /* lets fetch the timestamps of some data */
        printf("\n\nFile data for the file \"waffles\"\n");
        tfs_readFileInfo(waffles);

        printf("\n\nFile data for the file \"steak\"\n");
        tfs_readFileInfo(steak);

        int sausage = tfs_openFile("/foods/savory/sausage");
        if(sausage < 0) {
            printf("open file error (%d)\n", sausage);
            exit(EXIT_FAILURE);
        }

        char sausageBlog[] = "Eat some yummy sausage";
        status = tfs_writeFile(sausage, sausageBlog, sizeof(sausageBlog));
        if(status < 0) {
            printf("writeFile error (%d)\n", status);
            exit(EXIT_FAILURE);
        }

        status = tfs_deleteFile(steak);
        if(status < 0) {
            printf("delete file error (%d)\n", status);
            exit(EXIT_FAILURE);
        }

        /* remove directories */
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

        /* rename the file */
        status = tfs_rename(sausage, "hotdog");
        printf("Here are the contents of the file \"hotdog\"\n");
        if(status < 0) {
            printf("rename error (%d)\n", status);
            exit(EXIT_FAILURE);
        }

        /* prove you can still access file data even after renaming */
        printf("\n\nFile data for the file \"hotdog\"\n");
        tfs_readFileInfo(sausage);

        /* ReadByte works after renaming */
        while(tfs_readByte(sausage, &s) >= 0) {
            printf("%c", s);
        }

        int hotdog = tfs_openFile("foods/savory/hotdog");
        if(hotdog < 0) {
            printf("openFile error (%d)\n", hotdog);
            exit(EXIT_FAILURE);
        }


        printf("\n\nNow, opening the file with the new name also works!! See we can print out the data\n");
        tfs_seek(hotdog, 0);
        while(tfs_readByte(hotdog, &s) >= 0) {
            printf("%c", s);
        }



        printf("\n\nHere are the disk contents after we have deleted some stuff\n");
        tfs_readdir();

        printf("\n\nEnd of Demo. Remeber to delete the disk before you rerun !! You can run \"make rmdemodisk\" to do this\n");

    }



    return 0;
}