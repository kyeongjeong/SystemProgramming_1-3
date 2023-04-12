///////////////////////////////////////////////////////////////////////////////////////
// File Name    : 2021202078_final_advanced.c                                        //
// Date         : 2023/04/12                                                         //
// OS           : Ubuntu 16.04.5 Desktop 64bits                                      //
// Author       : Choi Kyeong Jeong                                                  //
// Student ID   : 2021202078                                                         //
// --------------------------------------------------------------------------------- //
// Title        : System Programming Assignment 1-3                                  //
// Descriptions : In task 1-2, implement additional options: -r (reverse order       //
//                sorting), -h (change file size units), -S (sorting based on file   //
//                size), and add functionality for wildcard usage.                   //
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
#include <fnmatch.h>
#define MAX_LENGTH 1000

void listDirFiles(int a_hidden, int l_format, int S_size, int r_reverse, int h_readable, char* filename);
void printFileInfo(char* fileName, int l_format, int h_readable);
void sortByNameInAscii(char **fileList, int fileNum, int start, int r_reverse);
void sortByFileSize(char **fileList, char * dirPath, int fileNum, int start, int r_reverse);
void printPermissions(mode_t mode);
void printType(struct stat fileStat);
void printAttributes(struct stat fileStat, int h_readable);
int compareStringUpper(char* fileName1, char* fileName2);
int wildcard(char* fileName, int isPrint);

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
    int S_size = 0; //옵션에 -S 포함 여부(0: 미포함, 1: 포함)
    int r_reverse = 0; //옵션에 -r 포함 여부(0: 미포함, 1: 포함)
    int h_readable = 0; //옵션에 -h 포함 여부(0: 미포함, 1: 포함)
    int opt; //옵션 정보
    int isNotWild[MAX_LENGTH] = {0, }; //와일드카드 포함 여부(0: 포함, 1: 미포함)
    int sortflag = 0; //정렬 교정을 위한 임시 변수

    while((opt = getopt(argc, argv, "alhSr")) != -1) { //옵션 정보 판별하기
        switch (opt)
        {
        case 'l': //옵션에 -l 포함한 경우
            l_format = 1; //l 플래그 세우기
            break;
        case 'a': //옵션에 -a 포함한 경우
            a_hidden = 1; //a 플래그 세우기
            break;
        case 'S': //옵션에 -S 포함한 경우
            S_size = 1; //S 플래그 세우기
            break;
        case 'r': //옵션에 -r 포함한 경우
            r_reverse = 1; //r 플래그 세우기
            break;
        case 'h': //옵션에 -h 포함한 경우
            h_readable = 1; //h 플래그 세우기
        case '?': //unknown
            break;
        default: //defalut
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
            if ((opendir(argv[i]) == NULL) && (stat(argv[i], &st)==-1) && (argv[i][0] != '-')) { 
                
                if(wildcard(argv[i], 0) == 0) {
                    printf("cannot access %s: No such file or directory\n", argv[i]); //error 문구 출력
                    isNotWild[i] = 1;
                }
                else
                    sortflag = 1;
            }
        }

        sortByNameInAscii(argv, argc, (optind + sortflag), r_reverse);

        for (int i = optind; i < argc; i++) {
            if ((opendir(argv[i]) == NULL) && (stat(argv[i], &st) == -1) && (argv[i][0] != '-')) {
                if (isNotWild[i] == 0)
                    wildcard(argv[i], 1);
                else
                    continue;
            }
        }

        char path[MAX_LENGTH]; //경로를 받아올 변수
        getcwd(path, MAX_LENGTH); //현재 경로 받아오기

        char* temp[MAX_LENGTH];
        if (S_size == 1) { //사이즈별로 출력
            for (int i = 0; i < argc; i++) //인자 수만큼 반복하면서
                temp[i] = argv[i]; //temp에 argv 내용 복사

            sortByFileSize(temp, path, argc, optind, r_reverse); //파일 사이즈별로 정렬

            for (int i = optind; i < argc; i++) { // 1. 존재하는 파일임(옵션 x) 2. 디렉토리가 아님 3. run파일이 아님 => 파일정보 출력
                
                if ((stat(temp[i], &st) != -1) && (!S_ISDIR(st.st_mode)) && (i != 0))
                    printFileInfo(temp[i], l_format, h_readable); //파일 정보 출력 함수
            }
        }

        else{
            for (int i = optind; i < argc; i++) { // 1. 존재하는 파일임(옵션 x) 2. 디렉토리가 아님 3. run파일이 아님 => 파일정보 출력
                if ((stat(argv[i], &st) != -1) && (!S_ISDIR(st.st_mode)) && (i != 0)) {
                    printFileInfo(argv[i], l_format, h_readable); // 파일정보 출력                
                } 
            }
        }

        for(int i = optind; i < argc; i++) { //인자 수만큼 반복
            if((stat(argv[i], &st) != -1) && S_ISDIR(st.st_mode)) { //열리는 디렉토리인 경우
                
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
// wildcard                                                                          //
// --------------------------------------------------------------------------------- //
// Input: char* patternName -> The wildcard pattern to be checked                    //
//        int isPrint -> Determination of whether to execute the output section or   //
//                       not                                                         //
// output:                                                                           //
// purpose: Prints sub-files in the directory specified by the filename argument     //
//          based on the options                                                     //
///////////////////////////////////////////////////////////////////////////////////////
int wildcard(char* patternName, int isPrint) {
    
    DIR *dirp;
    struct dirent *dir;
    int wildFlag = 0, fileNum = 0, dirNum = 0; //와일드카드의 유무, 파일 개수, 디렉토리 개수
    char *fileList[MAX_LENGTH]; //파일 리스트
    char *dirList[MAX_LENGTH]; //디렉토리 리스트
    char *dirList2[MAX_LENGTH]; //디렉토리 하위 디렉토리 리스트
    char wildPath[MAX_LENGTH] = {'\0', }; // 디렉토리 절대 경로
    char wildPath2[MAX_LENGTH] = {'\0', }; // 입력받은 경로
    char dirPath[MAX_LENGTH] = {'\0', }; //출력할 절대경로
    char pat[MAX_LENGTH] = {'\0', }; //개선된 패턴

    getcwd(wildPath, MAX_LENGTH); //현재 경로 받아옴

    if(patternName[0] != '/') //입력이 절대경로가 아닌 파일이고, /로 시작하지 않을 때 ex) A/*
        strcat(wildPath, "/"); // /을 제일 앞에 붙여줌
    
    if(strstr(patternName, wildPath) != NULL) //filename이 현재 dirPath를 포함할 때 ex)/home/Assignment/A/*
        strcpy(wildPath, patternName); //입력받은 경로로 절대경로 덮어쓰기
    else
        strcat(wildPath, patternName); //현재 경로에 입력받은 경로 이어붙이기

    char *lastSlash = strrchr(wildPath, '/'); // 가장 마지막의 '/'를 찾기
    if (lastSlash != NULL)
        *lastSlash = '\0'; // '/'를 null 문자로 바꿔 문자열을 종료

    else
        strcpy(wildPath, getcwd(wildPath, MAX_LENGTH)); //현재 위치 받아오기

    dirp = opendir(wildPath); // 절대경로로 opendir

    strcpy(wildPath2, patternName);
    lastSlash = strrchr(wildPath2, '/'); // 가장 마지막의 '/'를 찾기
    if (lastSlash != NULL)
        *lastSlash = '\0'; // '/'를 null 문자로 바꿔 문자열을 종료
    else
        strcpy(wildPath2, getcwd(wildPath, MAX_LENGTH)); //현재 위치 받아오기

    while ((dir = readdir(dirp)) != NULL) { //파일 읽어오기 
        char fileName[MAX_LENGTH] = {'\0', }; //파일이름 받아올 배열 선언 및 초기화
        strcpy(fileName, wildPath2); //입력받은 경로 불러오기
        strcat(fileName, "/"); // /를 붙이고
        strcat(fileName, dir->d_name); //읽어온 파일명 붙이기

        if(strcmp(wildPath, wildPath2) == 0) {
            
            strcpy(pat, wildPath2);
            strcat(pat, "/");
            strcat(pat, patternName);
        }
        else
            strcpy(pat, patternName);

        if((fnmatch(pat, fileName, 0) == 0) && (dir->d_name[0] != '.') && (dir->d_type == DT_DIR)) {
            dirList[dirNum] = dir->d_name; //와일드카드와 일치하는 디렉토리인 경우
            dirNum++; //디렉토리 숫자 증가
            wildFlag = 1; //인자가 와일드카드임을 표시
        }

        else if((fnmatch(pat, fileName, 0) == 0) && (dir->d_name[0] != '.')) {
            fileList[fileNum] = dir->d_name; //와일드카드와 일치하는 디렉토리 외 파일인 경우
            fileNum++; //파일 숫자 증가
            wildFlag = 1; //인자가 와일드카드임을 표시
        }
    }

    if(wildFlag == 0) //만약 와일드카드가 아니라면
        return wildFlag; //0을 반환

    if (isPrint == 1) { //출력이 필요한 경우
        sortByNameInAscii(fileList, fileNum, 0, 0); //아스키코드순으로 파일 정렬
        sortByNameInAscii(dirList, dirNum, 0, 0); //아스키코드순으로 디렉토리 정렬

        rewinddir(dirp); //dirp를 초기화
        for (int i = 0; i < fileNum; i++) //파일리스트만큼 반복하면서
            printf("%s\n", fileList[i]); //파일명 출력

        for (int i = 0; i < dirNum; i++) { //디렉토리 개수만큼 반복

            char dirPath[MAX_LENGTH]; //디렉토리 경로 저장 변수 선언 및 초기화
            strcpy(dirPath, wildPath); //절대경로를 복사
            strcat(dirPath, "/"); //경로에 / 붙이기
            strcat(dirPath, dirList[i]); //경로에 파일명 붙이기
            printf("Directory path: %s\n", dirPath);

            dirp = opendir(dirPath); //dirPath로 open
            int dirNum2 = 0; //디렉토리 하위 파일들의 개수
            while ((dir = readdir(dirp)) != NULL) { //파일 읽어오기

                if ((dir->d_name[0] != '.')) { //히든파일이 아니라면
                    dirList2[dirNum2] = dir->d_name; //하위 파일명들 저장
                    dirNum2++; //하위 파일들의 개수 세기
                }
            }
            sortByNameInAscii(dirList2, dirNum2, 0, 0); //하위 파일들도 아스키코드 순으로 정렬
            for (int j = 0; j < dirNum2; j++) //파일 개수만큼 반복하면서
                printf("%s\n", dirList2[j]); //파일명 출력
        }
    }
    return wildFlag; //와일드카드 여부 반환
}

///////////////////////////////////////////////////////////////////////////////////////
// listDirFiles                                                                      //
// --------------------------------------------------------------------------------- //
// Input: int a_hidden -> option -a                                                  //
//        int l_format -> option -l                                                  //
//        char* filename -> file name that provided                                  //
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
    getcwd(dirPath, MAX_LENGTH); //현재 경로 받아옴

    if(filename[0] != '/') //입력이 절대경로가 아닌 파일이고, /로 시작하지 않을 때 ex) A
        strcat(dirPath, "/"); // /을 제일 앞에 붙여줌
    
    if(strstr(filename, dirPath) != NULL) //filename이 현재 dirPath를 포함할 때 ex)/home/Assignment/A
        strcpy(dirPath, filename); //입력받은 경로로 절대경로 덮어쓰기
    else
        strcat(dirPath, filename); //현재 경로에 입력받은 경로 이어붙이기  
    dirp = opendir(dirPath); //절대경로로 opendir

    while((dir = readdir(dirp)) != NULL) { //디렉토리 하위 파일들을 읽어들임

        strcpy(accessFilename, dirPath); //현재 파일 경로 복사
        strcat(accessFilename, "/");
        strcat(accessFilename, dir->d_name); //파일의 절대 경로 받아옴
        stat(accessFilename, &st); //파일의 절대 경로로 stat() 호출
        if(a_hidden == 1 || dir->d_name[0] != '.')
            total += st.st_blocks; //옵션(-a)에 따라 total 계산
                
        ++fileNum; //총 파일개수 count
    }

    rewinddir(dirp); //dirp 처음으로 초기화

    char **fileList = (char**)malloc(sizeof(char*) * (fileNum+1)); //동적 할당
    for(int i = 0; i < fileNum; i++) { //파일 개수만큼 반복
        
        fileList[i] = (char*)malloc(sizeof(char) * 300); //동적 할당
        dir = readdir(dirp);
        strcpy(fileList[i], dir->d_name); //fileList에 파일명 저장
    }

    sortByNameInAscii(fileList, fileNum, 0, r_reverse); //아스키 코드순으로 정렬
    if (S_size == 1) //S 옵션 존재 시
        sortByFileSize(fileList, dirPath, fileNum, 0, r_reverse); //파일 사이즈로 재정렬
    else
        sortByNameInAscii(fileList, fileNum, 0, r_reverse); //fileList를 알파벳 순으로 sort(대소문자 구분 x)
    
    if(l_format == 1) { //옵션 -l이 포함된 경우
        
        printf("Directory path: %s\n", dirPath);
        printf("total : %d\n", (int)(total/2));
    }

    for(int i = 0; i < fileNum; i++) { //파일 개수만큼 반복

        if(l_format == 1) { //옵션 -l이 포함된 경우

            if((a_hidden == 0) && fileList[i][0] == '.') //옵션 -a 여부에 따라 파일속성 출력
                continue;

            strcpy(accessPath, dirPath); //현재 경로 받아옴
            strcat(accessPath, "/");
            strcat(accessPath, fileList[i]); //파일의 절대 경로 받아옴
            stat(accessPath, &fileStat); //파일 속성정보 불러옴
            printAttributes(fileStat, h_readable); //속성 정보 출력
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
    
    struct stat fileStat; //파일 속성정보 받아올 변수
    char timeBuf[80]; //파일 시간정보 받아올 변수
    char filePath[MAX_LENGTH]; //파일 경로 저장할 변수

    realpath(fileName, filePath); //파일의 path 불러오기
    stat(filePath, &fileStat); //절대경로로 파일 속성 받기

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
    
    char timeBuf[80]; //시간 정보 받아올 변수

    printType(fileStat); // 파일 유형
    printPermissions(fileStat.st_mode); // 허가권
    printf("\t%ld\t", fileStat.st_nlink); // 링크 수
    printf("%s\t%s\t", getpwuid(fileStat.st_uid)->pw_name, getgrgid(fileStat.st_gid)->gr_name); // 파일 소유자 및 파일 소유 그룹

    if(h_readable == 1) { //만약 h 속성이 존재한다면
        
        double size = (double)fileStat.st_size; //파일의 사이즈를 받아오기
        int remainder = 0; //1024로 나눈 나머지를 계속 저장할 변수
        char sizeUnit[3] = {'K', 'M', 'G'}; //K, M, G 단위
        int unit = 0, unitIndex = -1; //유닛 인덱스로 단위 붙이기

        while (size >= 1024) { //1024보다 클 경우
            size /= 1024; //1024로 나눈 몫
            remainder %= 1024; //1024로 나눈 나머지
            unit = 1; //단위를 붙여야 함
            unitIndex++; //유닛 인덱스 증가
        }

        if(unit == 1) //단위를 붙여야 한다면
            printf("%.1f%c\t", size, sizeUnit[unitIndex]); //소수점 아래 한 자리까지 출력
        else //안붙여도 될 정도로 작다면
            printf("%.0f\t", size); //소수점 버리고 출력
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
    
    for (int i = start; i < fileNum; i++) { //파일리스트 반복문 실행
         if ((fileList[i][0] == '.') && (strcmp(fileList[i], ".") != 0) && (strcmp(fileList[i], "..") != 0)) { //hidden file인 경우
            isHidden[i] = 1; //파일명 가장 앞의 . 제거
            for (int k = 0; k < strlen(fileList[i]); k++) //파일 글자수 반복
                fileList[i][k] = fileList[i][k + 1]; //앞으로 한 칸씩 땡기기
        }
    }

    for (int i = start; i < (fileNum - 1); i++) { // 대소문자 구분 없는 알파벳 순으로 정렬
        for (int j = i + 1; j < fileNum; j++) { //bubble sort
            if (((compareStringUpper(fileList[i], fileList[j]) == 1) && (r_reverse == 0)) || ((compareStringUpper(fileList[i], fileList[j]) == 0) && (r_reverse == 1))) {
            //만약 첫 문자열이 둘째 문자열보다 작다면
                char *temp = fileList[i]; // 문자열 위치 바꾸기
                fileList[i] = fileList[j];
                fileList[j] = temp;

                int temp2 = isHidden[i]; //히든파일인지 저장한 배열도 위치 바꾸기
                isHidden[i] = isHidden[j];
                isHidden[j] = temp2;
            }
        }
    }

    for (int i = start; i < fileNum; i++) { //리스트 반복문 돌리기
        if(isHidden[i] == 1) { //hidden file인 경우
            for(int k = strlen(fileList[i]); k >= 0; k--) //파일 길이만큼 반복
                fileList[i][k+1] = fileList[i][k]; //뒤로 한 칸씩 보내기
            fileList[i][0] = '.'; //파일명 가장 앞에 . 다시 추가
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////
// sortByFileSize                                                                    //
// --------------------------------------------------------------------------------- //
// Input: char **fileList -> An array containing file names                          //
//        char *dirPath -> path of file                                              //
//        int fileNum -> size of file                                                //
//        int start -> where to start to sort files                                  //
//        int r_reverse -> reverse sort flag                                         //
// output:                                                                           //
// purpose: Comparing two strings based on uppercase letters to determine which one  //
//          is greater.                                                              //
///////////////////////////////////////////////////////////////////////////////////////
void sortByFileSize(char **fileList, char *dirPath, int fileNum, int start, int r_reverse) {
    
    struct stat fileStat1, fileStat2; //stat of files

    for (int i = start; i < (fileNum - 1); i++) { //bubble sort
        for (int j = i + 1; j < fileNum; j++) { 
            
            char accessFilename1[MAX_LENGTH]; //first file name
            char accessFilename2[MAX_LENGTH]; //second file name

            strcpy(accessFilename1, dirPath); //경로를 파일에 복사
            strcpy(accessFilename2, dirPath); //경로를 파일에 복사
            strcat(accessFilename1, "/"); 
            strcat(accessFilename2, "/");
            strcat(accessFilename1, fileList[i]); //파일 이름을 붙여넣기
            strcat(accessFilename2, fileList[j]); //파일 이름을 붙여넣기
            
            stat(accessFilename1, &fileStat1); //filestat open
            stat(accessFilename2, &fileStat2); //filestat open

            if (fileStat1.st_size < fileStat2.st_size) { //뒤의 문자열이 더 크다면
                
                char *temp = fileList[j]; // 문자열 위치 바꾸기
                for (int k = j; k > i; k--)
                    fileList[k] = fileList[k - 1]; //앞으로 한 칸씩 당기고
                fileList[i] = temp; //첫 번째 문자열이 제일 뒤 칸으로 가기
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////
// compareStringUpper                                                                //
// --------------------------------------------------------------------------------- //
// Input: char* fileName1 -> The first string to compare                             //
//        char* fileName2 -> The second string to compare                            //
// output: 0 -> No need to swap positions, 1 -> Need to swap positions               //
// purpose: Comparing two strings based on uppercase letters to determine which one  //
//          is greater.                                                              //
///////////////////////////////////////////////////////////////////////////////////////
int compareStringUpper(char* fileName1, char* fileName2) {
    
    char* str1 = (char*)calloc(strlen(fileName1)+1, sizeof(char)); //비교할 첫 번째 문자열
    char* str2 = (char*)calloc(strlen(fileName2)+1, sizeof(char)); //비교할 두 번째 문자열

    for(int i = 0; i < strlen(fileName1); i++) //첫 번째 문자열을 돌면서
        str1[i] = toupper(fileName1[i]); //모두 대문자로 전환

    for(int i = 0; i < strlen(fileName2); i++) //두 번째 문자열을 돌면서
        str2[i] = toupper(fileName2[i]); //모두 대문자로 전환

    if((strcmp(str1, ".") == 0 || strcmp(str1, "..") == 0) && (strcmp(str2, ".") != 0)) //위치를 바꿀 필요가 없는 경우
        return 0; //0 반환

    else if((strcmp(str2, ".") == 0 || strcmp(str2, "..") == 0) && (strcmp(str1, ".") != 0)) //위치를 바꿀 필요가 있는 경우
        return 1; //1 반환

    else if(strcmp(str1, str2) > 0) //위치를 바꿀 필요가 있는 경우
        return 1; //1 반환
    
    return 0; //위치를 바꿀 필요가 없는 경우 0 반환
}

///////////////////////////////////////////////////////////////////////////////////////
// printPermissions                                                                  //
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
        printf("?"); //unknown
        break;
    }
}