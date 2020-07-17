#ifndef __MP4_H__
#define __MP4_H__

#include <iostream>
#include <string.h>
#include <vector>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fstream>
#include <algorithm>
#include <fcntl.h>

using namespace std;

//Function Declarations
extern void get_input();
extern void clearVector();
extern void createFile(string filename);

vector<string> splitStrings;
vector<string> argsList;
bool valid = true;
bool endOfInput = false;
int commands = 0;
int fileO;
int fileI;
const char * currCommand;
int processID;

#endif