#include "tinyFS.h"

/* Uncomment and run this block if you want to test */
int main(int argc, char* argv[]) {
        
    tfs_mkfs("demo.dsk", 8192);    

    int status;
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

    int waffles = tfs_openFile("/foods/sweet/waffles");
    if(waffles < 0) {
        printf("open file error (%d)\n", waffles);
        exit(EXIT_FAILURE);
    }

    int pancakes = tfs_openFile("/foods/sweet/pancakes");
    if(pancakes < 0) {
        printf("open file error (%d)\n", pancakes);
        exit(EXIT_FAILURE);
    }

    int cake = tfs_openFile("/foods/sweet/cake");
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
    char s;
    int i = 0;
    status = tfs_readByte(waffles, &s);
    while(status >= 0) {
        i++;
        printf("%c", s);
        status = tfs_readByte(waffles, &s);
    }
    printf("\n");

    /* print out the contents of the disk */
    tfs_readdir();

    return 0;
}