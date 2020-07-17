/* Cassie Willis
 *
 * CS 4414 - Operating Systems
 * Spring 2018
 * 4-18-18
 *
 * MP4 - Writing Your Own Shell
 *
 * The purpose of this project is to become familiar with shell scripting. 
 * This project recieves input from STDIN and appropriately performes file 
 * opening or piping operations from |, <, and > operators, returning either 
 * return information to STDOUT or exit codes to STDERR
 * Refer to the writeup for complete details.
 *
 * Compile with MAKE
 *
 */

// > should send STDOUT to a file, create the file if it doesn't exist, error if cannot create
// < should send from file to STDIN, error if file cannot be read
// | redirects STDOUT of first to STDIN of second
//wait for subprocesses to finish and print return values to STDOUT, one per line in order listed

#include "MP4.h"
using namespace std;


//Parse a single line of input into a vector of strings
void get_input() {
	string input;
	cout << ">";
	fflush(stdout);
	getline(cin, input);

	//Make sure input is not too long
	if (input.size() > 100) {
		fprintf(stdout, "invalid input\n");
		valid = false;
	}

	//Check if end of input and can exit shell
	if (input.empty() || input == "exit" || input == "Exit" || input == "EXIT" || input == " " || input == "EOF") {
		endOfInput = true;
		return;
	}

	//Make sure the input contains only valid characters
	int size = input.size();
	int i;
	for (i = 0; i < size; i++) {
		if(!isalnum(input[i]) && input[i] != '|' && input[i] != '>' && input[i] != '<') {
			if(input[i] != ' ' && input[i] != '/' && input[i] != '.' && input[i] != '-' && input[i] != '_') {
				fprintf(stdout, "invalid input\n");
				valid = false;
				break;
			}
		}
	}

	//Take the input string and separate by spaces into a vector of strings
	istringstream iss(input);
	for(string input; iss >> input; ) {
		splitStrings.push_back(input); 
	}

	//Make sure first and last inputs are words not operations
	if(splitStrings[splitStrings.size()-1] == "|" || splitStrings[splitStrings.size()-1] == ">" || 
		splitStrings[splitStrings.size()-1] == "<" || splitStrings[0] == "|" || splitStrings[0] == ">" || 
		splitStrings[0] == "<") valid = false;

	//Make sure operations are not connected to words
	for(unsigned int j = 0; j < input.size(); j++) {
		if (((input[j] == '|') || (input[j] == '<') || (input[j] == '>')) 
			&& ((input[j-1] != ' ') || (input[j+1] != ' '))) {
			valid = false;
		}
	}

	//Check to make sure < and > do not come before/after pipes where not allowed
	// < input file redirection cannot have a pipe before it
	// > output file redirection cannot have a pipe after it
	for(unsigned int x = 0; x < input.size(); x++) {
		if(input[x] == '<') { //check for pipe before <
			for(unsigned int e = 0; e < x; e++) {
				if(input[e] == '|') {
					valid = false;
				}
			}
		}
		else if(input[x] == '>') { //check for pipe after >
			for(unsigned int e = x+1; e < input.size(); e++) {
				if(input[e] == '|') {
					valid = false;
				}
			}
		}
	}

	for (unsigned int k = 0; k < splitStrings.size(); k++) {
		if (splitStrings[k] == "|") commands++;
	}
	commands ++;

	//Separate commands/arguments in a line
	stringstream in(input);
	string partial;
	while(getline(in, partial, '|')) argsList.push_back(partial);
}

//After each line is processed, clear the vector to process the next line
void clearVector() {
	splitStrings.clear();
	argsList.clear();
	commands = 0;
	valid = true;
	currCommand = NULL;
}

//create a new file
void createFile(const char * filename) {
	FILE *newFile;
	newFile = fopen((const char*)filename, "w+");
	if(newFile == NULL) fprintf(stdout, "invalid input\n"); 
	else fclose(newFile); //create new file and check if created
}

int main() {
	while(1) {

		get_input();

		//If end of input, exit shell
		if(endOfInput == true) {
			break;
		}

		if(valid == true) { //input line is valid input
			//create pipes
			int pipes[commands-1][2]; //Pipes = #token groups - 1
			for (int y = 0; y < commands-1; y++) {
				if((pipe(pipes[y])) < 0) fprintf(stdout, "invalid input\n");
			}

			//create child processes
			pid_t pid;
			for(int p = 0; p < commands; p++) {
				processID = p;
				pid = fork();
				//fork not successful
				if (pid < 0) {
					fprintf(stdout, "invalid input\n");
					return -1;
				}

				//child process
				else if(pid == 0) { 
					bool fileOutput = false; //checks if file redirection is required
					bool fileInput = false;

					//Piping dup2 - curr token group = i, read from pipe i, write to pipe i-1
					//STDIN = 0	  /  STDOUT = 1
					//If first command, write to i
					if(p == 0) {
						dup2(pipes[p][1], 1);
						close(pipes[p][0]);
						close(pipes[p][1]);
						for(int l = p; l < commands-1; l++) close(pipes[l][1]);
					}
					//If last command, read from i-1
					else if(p == commands-1) {
						dup2(pipes[p-1][0], 0);
						close(pipes[p-1][0]);
						close(pipes[p-1][1]);
					}
					//If middle command
					else {
						dup2(pipes[p][1], 1);
						dup2(pipes[p-1][0], 0);
						close(pipes[p][0]);
						close(pipes[p][1]);
						close(pipes[p-1][0]);
						close(pipes[p-1][1]);
						for(int l = p; l < commands-1; l++) close(pipes[l][1]);
					}

					//Each argument in token group into char array
					vector<string> temp;
					string tempArg = argsList[p];
					istringstream iss(tempArg);
					for(string tempArg; iss >> tempArg; ) {
						temp.push_back(tempArg); 
					}
					int vSize = temp.size();
					const char* args[vSize];
					for(int i = 0; i < vSize; i++) {
						args[i] = temp[i].c_str();
					}
					args[vSize] = NULL;

					//Each argument in token group before redirect into char array
					string tempA = argsList[p];
					istringstream isa(tempA);
					vector<string> beforeRedirect;
					for(string tempA; isa >> tempA; ) {
						if((tempA.compare(">") != 0) && (tempA.compare("<") != 0)) {
							beforeRedirect.push_back(tempA); 
						}
						else break;
					}

					int rSize = beforeRedirect.size();
					const char* argsBeforeRedirect[rSize];
					for(int i = 0; i < rSize; i++) {
						argsBeforeRedirect[i] = beforeRedirect[i].c_str();
					}
					argsBeforeRedirect[rSize] = NULL;

					//Open files for redirection	
					for(int oc = 0; oc < vSize-1; oc++) {
						//output - open as write, need to check if exists
					 	if(strcmp(args[oc], ">") == 0) {
							fileOutput = true; 
							char cwd[100];   
							getcwd(cwd, 100);
							string tempPath = args[oc+1];
							if(args[oc+1][0] != '/') {
								tempPath = string(cwd) + '/' + args[oc+1];
							}
							//file does not exist - create file
							if(access(tempPath.c_str(), F_OK) == -1) {
								createFile(tempPath.c_str());
							}
							fileO = open(tempPath.c_str(), O_WRONLY);
							if(fileO < 0) {
								fprintf(stdout, "invalid input\n");
								return -1; 
							}
							break;
						 }
						 //input - open as read
						 if(strcmp(args[oc], "<") == 0) {
						 	fileInput = true;
						 	char cwd[100];   
							getcwd(cwd, 100);
							string tempPath = args[oc+1];
							if(args[oc+2][0] != '/') {
								tempPath = string(cwd) + '/' + args[oc+1];
							}
							fileI = open(tempPath.c_str(), O_RDONLY);
							if(fileI < 0) {
								fprintf(stdout, "invalid input\n");
								return -1; 
							}
							break;
						 }
					}

					//For commands: If path is not absolute, get cwd and attach to path (command at beginning of each pipe)
					char cwd[100];   
					getcwd(cwd, 100);
					string fullPath;
					if(args[0][0] != '/') {
						fullPath = string(cwd) + '/' + temp[0];
						temp[0] = fullPath;
					}
					else {
						fullPath = temp[0];
					}

					//Run the command
					//dup2(new, old) - redirect file output, 1 = stdout
					if(fileOutput) {
						fileOutput = false;
						dup2(fileO, STDOUT_FILENO);
						close(fileO);

					}
					if(fileInput) {
						fileInput = false;
						dup2(fileI, STDIN_FILENO);
						close(fileI);
					}

					currCommand = temp[0].c_str();
					execv((const char*)fullPath.c_str(), (char**)argsBeforeRedirect);
					_exit(-1);
				}

				//parent process, wait for children to end
				else {
					//close all read pipes
					for (int y = 0; y < commands-1; y++) {
						close(pipes[y][1]);
					}

					//wait for child to finish, print exit code
					int status;
					waitpid(pid, &status,0);
					if(WIFEXITED(status) > 0) {
						fprintf(stderr, "%d\n", WEXITSTATUS(status));
					}
					else fprintf(stdout, "invalid input\n");
				}
			}

			//close all pipes
			for (int y = 0; y < commands-1; y++) {
				for(int s = 0; s < 2; s++) {
					close(pipes[y][s]);
				}
			}

		}
		
		else {
			fprintf(stdout, "invalid input\n");
		}

		clearVector();
	} 

	return 0;
}

