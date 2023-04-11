///////////////////////////////////////////////////////////////////////////////////////
// File Name    : 2021202078_final_advanced.c                                         //
// Date         : 2023/04/05                                                         //
// OS           : Ubuntu 16.04.5 Desktop 64bits                                      //
// Author       : Choi Kyeong Jeong                                                  //
// Student ID   : 2021202078                                                         //
// --------------------------------------------------------------------------------- //
// Title        : System Programming Assignment 1-2                                  //
// Descriptions : A C language program that implements the ls command options        //
//               (-l, -a, -la) according to the input arguments provided.            //
///////////////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <ctype.h>
#define MAX_LENGTH 1000

void listDirFiles(int a_hidden, int l_format, int S_size, int r_reverse, int h_readable, char* filename);
void printFileInfo(char* fileName, int l_format, int h_readable);
void sortByNameInAscii(char **fileList, int fileNum, int start, int r_reverse);
void sortByFileSize(char **fileList, char * dirPath, int fileNum, int start, int r_reverse);
void printPermissions(mode_t mode);
void printType(struct stat fileStat);
void printAttributes(struct stat fileStat, int h_readable);
int compareStringUpper(char* fileName1, char* fileName2);

///////////////////////////////////////////////////////////////////////////////////////
// main                                                                              //
// --------------------------------------------------------------------------------- //
// Input: char* argv[] -> Directory name or file name or option that the user enters //
//        int argc -> The number of arguments and options                            //
// output:                                                                           //
// purpose: This program distinguishes between options and arguments provided by the //
//          user, identifies the type of option, and determines whether the argument //
//          is a file or directory. It checks whether the file or directory exists   //
//          and sorts the directories alphabetically.                                //
///////////////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[]) {
    
    DIR *dirp;
    struct dirent *dir;
    struct stat st;
    int a_hidden = 0; //옵션에 -a 포함 여부(0: 미포함, 1: 포함)
    int l_format = 0; //옵션에 -l 포함 여부(0: 미포함, 1: 포함)
    int S_size = 0;
    int r_reverse = 0;
    int h_readable = 0;
    int opt; //옵션 정보

    while((opt = getopt(argc, argv, "alhSr")) != -1) {
        switch (opt)
        {
        case 'l': //옵션에 -l 포함
            l_format = 1;
            break;
        case 'a': //옵션에 -a 포함
            a_hidden = 1;
            break;
        case 'S':
            S_size = 1;
            break;
        case 'r':
            r_reverse = 1;
            break;
        case 'h':
            h_readable = 1;
        case '?':
            break;
        default:
            break;
        }
    }

    if((a_hidden == 0) && (l_format == 0) && (S_size == 0) && (r_reverse == 0) && (h_readable == 0)) //옵션이 없는 경우
        optind = 0;

    if(optind >= 2) //-al, -la의 경우에도 optind는 1
        optind = 1;

    if((argc-optind) == 1) { //인자가 없는 경우(옵션은 있을수도, 없을수도 있다)
        char currentPath[10] = "."; //현재 경로
        listDirFiles(a_hidden, l_format, S_size, r_reverse, h_readable, currentPath); //현재 디렉토리 하위 파일 출력
    }

    else {       
        for(int i = optind; i < argc; i++) { //1. 디렉토리가 아님 2. 파일 안열림 3. 옵션이 아님 => 다 합하면 존재하지 않는 파일만 추출가능
            if ((opendir(argv[i]) == NULL) && (stat(argv[i], &st)==-1) && (argv[i][0] != '-')) 
                printf("cannot access %s: No such file or directory\n", argv[i]); //error 문구 출력
        }

        sortByNameInAscii(argv, argc, optind, r_reverse);
        
        char path[MAX_LENGTH];
        getcwd(path, MAX_LENGTH);

        char* temp[MAX_LENGTH];
        if (S_size == 1) { //사이즈별로 출력
            for (int i = 0; i < argc; i++)
                temp[i] = argv[i];

            sortByFileSize(temp, path, argc, optind, r_reverse);

            for (int i = optind; i < argc; i++) { // 1. 존재하는 파일임(옵션 x) 2. 디렉토리가 아님 3. run파일이 아님 => 파일정보 출력
                
                if ((stat(temp[i], &st) != -1) && (!S_ISDIR(st.st_mode)) && (i != 0))
                    printFileInfo(temp[i], l_format, h_readable);
            }
        }

        else{
            for (int i = optind; i < argc; i++) { // 1. 존재하는 파일임(옵션 x) 2. 디렉토리가 아님 3. run파일이 아님 => 파일정보 출력
                if ((stat(argv[i], &st) != -1) && (!S_ISDIR(st.st_mode)) && (i != 0))
                    printFileInfo(argv[i], l_format, h_readable); // 파일정보 출력
            }
        }

        for(int i = optind; i < argc; i++) {
            if((stat(argv[i], &st) != -1) && S_ISDIR(st.st_mode)) {
                
                if((argc - optind > 1) && (l_format == 0)) //-l 옵션이 없고 인자로 받은 경로가 2개 이상일 경우
                    printf("%s:\n", argv[i]); //ex) a:~~~ b:~~~

                listDirFiles(a_hidden, l_format, S_size, r_reverse, h_readable, argv[i]); //디렉토리 하위 파일정보 출력
                if(l_format == 0) //-l 옵션이 없는 경우
                    printf("\n"); //입력받은 경로 간 줄바꿈 
                    
            }
        }
    }

    closedir; //디렉토리 close
    return 0; //프로그램 종료
} 

///////////////////////////////////////////////////////////////////////////////////////
// listDirFiles                                                                      //
// --------------------------------------------------------------------------------- //
// Input: int a_hidden -> option -a                                                  //
//        int l_format -> option -l                                                  //
//        cjar* filename -> file name that provided                                  //
// output:                                                                           //
// purpose: Prints sub-files in the directory specified by the filename argument     //
//          based on the options                                                     //
///////////////////////////////////////////////////////////////////////////////////////
void listDirFiles(int a_hidden, int l_format, int S_size, int r_reverse, int h_readable, char* filename) {

    DIR *dirp;
    struct dirent *dir;
    struct stat st;
    struct stat fileStat;
    int fileNum = 0; //파일의 개수
    char timeBuf[80];
    int total = 0;
    char accessPath[MAX_LENGTH];
    char accessFilename[MAX_LENGTH];

    int* isHidden = (int*)calloc(fileNum, sizeof(int));

    char dirPath[MAX_LENGTH] = {'\0', }; //출력할 절대경로
    realpath(filename, dirPath);  
    dirp = opendir(dirPath); //절대경로로 opendir

    while((dir = readdir(dirp)) != NULL) { //디렉토리 하위 파일들을 읽어들임

        strcpy(accessFilename, dirPath);
        strcat(accessFilename, "/");
        strcat(accessFilename, dir->d_name); //파일의 절대 경로 받아옴
        stat(accessFilename, &st); //파일의 절대 경로로 stat() 호출
        if(a_hidden == 1 || dir->d_name[0] != '.')
            total += st.st_blocks; //옵션(-a)에 따라 total 계산
                
        ++fileNum; //총 파일개수 count
    }

    rewinddir(dirp);

    char **fileList = (char**)malloc(sizeof(char*) * (fileNum+1));
    for(int i = 0; i < fileNum; i++) {
        
        fileList[i] = (char*)malloc(sizeof(char) * 300);
        dir = readdir(dirp);
        strcpy(fileList[i], dir->d_name); //fileList에 파일명 저장
    }

    sortByNameInAscii(fileList, fileNum, 0, r_reverse);
    if (S_size == 1) 
        sortByFileSize(fileList, dirPath, fileNum, 0, r_reverse);
    else
        sortByNameInAscii(fileList, fileNum, 0, r_reverse); //fileList를 알파벳 순으로 sort(대소문자 구분 x)
    
    if(l_format == 1) { //옵션 -l이 포함된 경우
        
        printf("Directory path: %s\n", dirPath);
        printf("total : %d\n", (int)(total/2));
    }

    for(int i = 0; i < fileNum; i++) {

        if(l_format == 1) { //옵션 -l이 포함된 경우

            if((a_hidden == 0) && fileList[i][0] == '.') //옵션 -a 여부에 따라 파일속성 출력
                continue;

            strcpy(accessPath, dirPath);
            strcat(accessPath, "/");
            strcat(accessPath, fileList[i]); //파일의 절대 경로 받아옴
            stat(accessPath, &fileStat);
            printAttributes(fileStat, h_readable);
        }

        if(a_hidden == 1 || fileList[i][0] != '.') //옵션 -a 여부에 따라 파일명 출력
            printf("%s\n", fileList[i]);
    }
}

///////////////////////////////////////////////////////////////////////////////////////
// printFileInfo                                                                     //
// --------------------------------------------------------------------------------- //
// Input: char* l_format -> option -l                                                //
//        int filename -> file name that provided                                    //
// output:                                                                           //
// purpose: Prints the attributes of the file based on the presence or absence of    //
//          the -l option, given the filename argument.                              //
///////////////////////////////////////////////////////////////////////////////////////
void printFileInfo(char* fileName, int l_format, int h_readable) {
    
    struct stat fileStat;
    char timeBuf[80];
    char filePath[MAX_LENGTH];

    realpath(fileName, filePath);
    stat(filePath, &fileStat);

    if(l_format == 1) //옵션 -l이 포함된 경우 파일 속성 출력
        printAttributes(fileStat, h_readable);

    printf("%s\n", fileName); //옵션 -l이 포함되지 않는 경우 파일명만 출력
}

///////////////////////////////////////////////////////////////////////////////////////
// printAttributes                                                                         //
// --------------------------------------------------------------------------------- //
// Input: struct stat fileStat -> Save information about a file (such as file size   //
//                                , owner, permissions, etc.)                        //
// output:                                                                           //
// purpose: Prints the attributes of the file using the information from the given   //
//          name of struct stat object                                               //
///////////////////////////////////////////////////////////////////////////////////////
void printAttributes(struct stat fileStat, int h_readable) {
    
    char timeBuf[80];

    printType(fileStat); // 파일 유형
    printPermissions(fileStat.st_mode); // 허가권
    printf("\t%ld\t", fileStat.st_nlink); // 링크 수
    printf("%s\t%s\t", getpwuid(fileStat.st_uid)->pw_name, getgrgid(fileStat.st_gid)->gr_name); // 파일 소유자 및 파일 소유 그룹

    if(h_readable == 1) {
        
        double size = (double)fileStat.st_size;
        int remainder = 0;
        char sizeUnit[3] = {'K', 'M', 'G'};
        int unit = 0, unitIndex = -1;

        while (size >= 1024) {
            size /= 1024;
            remainder %= 1024;
            unit = 1;
            unitIndex++;
        }

        if(unit == 1)
            printf("%.1f%c\t", size, sizeUnit[unitIndex]);
        else
            printf("%.0f\t", size);
    }
    else
        printf("%ld\t", fileStat.st_size); // 파일 사이즈

    strftime(timeBuf, sizeof(timeBuf), "%b %d %H:%M", localtime(&fileStat.st_mtime)); // 수정된 날짜 및 시간 불러오기
    printf("%s\t", timeBuf); // 수정된 날짜 및 시간 출력
}

///////////////////////////////////////////////////////////////////////////////////////
// sortByNameInAscii                                                                 //
// --------------------------------------------------------------------------------- //
// Input: char **fileList -> An array containing file names.                         //
//        int fileNum -> size of fileList                                            //
// output:                                                                           //
// purpose: Sort the filenames in alphabetical order (ignoring case) without the dot //
///////////////////////////////////////////////////////////////////////////////////////
void sortByNameInAscii(char **fileList, int fileNum, int start, int r_reverse)
{
    int* isHidden = (int*)calloc(fileNum, sizeof(int)); //hidden file인지 판별 후 저장
    
    for (int i = start; i < fileNum; i++) {
         if ((fileList[i][0] == '.') && (strcmp(fileList[i], ".") != 0) && (strcmp(fileList[i], "..") != 0)) { //hidden file인 경우
            isHidden[i] = 1; //파일명 가장 앞의 . 제거
            for (int k = 0; k < strlen(fileList[i]); k++)
                fileList[i][k] = fileList[i][k + 1];
        }
    }

    for (int i = start; i < (fileNum - 1); i++) { // 대소문자 구분 없는 알파벳 순으로 정렬
        for (int j = i + 1; j < fileNum; j++) {
            if (((compareStringUpper(fileList[i], fileList[j]) == 1) && (r_reverse == 0)) || ((compareStringUpper(fileList[i], fileList[j]) == 0) && (r_reverse == 1))) {

                char *temp = fileList[i]; // 문자열 위치 바꾸기
                fileList[i] = fileList[j];
                fileList[j] = temp;

                int temp2 = isHidden[i];
                isHidden[i] = isHidden[j];
                isHidden[j] = temp2;
            }
        }
    }

    for (int i = start; i < fileNum; i++) {
        if(isHidden[i] == 1) { //hidden file인 경우
            for(int k = strlen(fileList[i]); k >= 0; k--)
                fileList[i][k+1] = fileList[i][k];
            fileList[i][0] = '.'; //파일명 가장 앞에 . 다시 추가
        }
    }
}

void sortByFileSize(char **fileList, char * dirPath, int fileNum, int start, int r_reverse) {
    
    struct stat fileStat1, fileStat2;

    for (int i = start; i < (fileNum - 1); i++) {
        for (int j = i + 1; j < fileNum; j++) {
            
            char accessFilename1[MAX_LENGTH];
            char accessFilename2[MAX_LENGTH];

            strcpy(accessFilename1, dirPath);
            strcpy(accessFilename2, dirPath);
            strcat(accessFilename1, "/");
            strcat(accessFilename2, "/");
            strcat(accessFilename1, fileList[i]);
            strcat(accessFilename2, fileList[j]);
            
            stat(accessFilename1, &fileStat1);
            stat(accessFilename2, &fileStat2);

            if (fileStat1.st_size < fileStat2.st_size) {
                
                char *temp = fileList[j]; // 문자열 위치 바꾸기
                for (int k = j; k > i; k--)
                    fileList[k] = fileList[k - 1];
                fileList[i] = temp;
            }
        }
    }
}

int compareStringUpper(char* fileName1, char* fileName2) {
    
    char* str1 = (char*)calloc(strlen(fileName1)+1, sizeof(char));
    char* str2 = (char*)calloc(strlen(fileName2)+1, sizeof(char));

    for(int i = 0; i < strlen(fileName1); i++)
        str1[i] = toupper(fileName1[i]);

    for(int i = 0; i < strlen(fileName2); i++)
        str2[i] = toupper(fileName2[i]);

    if((strcmp(str1, ".") == 0 || strcmp(str1, "..") == 0) && (strcmp(str2, ".") != 0))
        return 0;

    else if((strcmp(str2, ".") == 0 || strcmp(str2, "..") == 0) && (strcmp(str1, ".") != 0))
        return 1;

    else if(strcmp(str1, str2) > 0)
        return 1;
    
    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////
// printPermissions                                                                 //
// --------------------------------------------------------------------------------- //
// Input: mode_t mode -> represents the permission information of a file.            //                                         
// output:                                                                           //
// purpose: Printing file permissions for user, group, and others.                   //
///////////////////////////////////////////////////////////////////////////////////////
void printPermissions(mode_t mode) {
    printf((mode & S_IRUSR) ? "r" : "-"); //user-read
    printf((mode & S_IWUSR) ? "w" : "-"); //user-write
    printf((mode & S_IXUSR) ? "x" : "-"); //user-execute
    printf((mode & S_IRGRP) ? "r" : "-"); //group-read
    printf((mode & S_IWGRP) ? "w" : "-"); //group-write
    printf((mode & S_IXGRP) ? "x" : "-"); //group-execute
    printf((mode & S_IROTH) ? "r" : "-"); //other-read
    printf((mode & S_IWOTH) ? "w" : "-"); //other-write
    printf((mode & S_IXOTH) ? "x" : "-"); //other-execute
}

///////////////////////////////////////////////////////////////////////////////////////
// printType                                                                    //
// --------------------------------------------------------------------------------- //
// Input: struct stat fileStat -> Save information about a file (such as file size   //
//                                , owner, permissions, etc.)                        //
// output:                                                                           //
// purpose: Printing file type(regular file, directory, symbolic link, etc.)         //
///////////////////////////////////////////////////////////////////////////////////////
void printType(struct stat fileStat) {

    switch (fileStat.st_mode & __S_IFMT) {
    case __S_IFREG: //regular file
        printf("-");
        break;
    case __S_IFDIR: //directory
        printf("d");
        break;
    case __S_IFLNK: //symbolic link
        printf("l");
        break;
    case __S_IFSOCK: //socket
        printf("s");
        break;
    case __S_IFIFO: //FIFO(named pipe)
        printf("p");
        break;
    case __S_IFCHR: //character device
        printf("c");
        break;
    case __S_IFBLK: //block device
        printf("b");
        break;
    default:
        printf("?");
        break;
    }
}