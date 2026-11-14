#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

typedef struct{
	uint32_t inode;
	uint32_t parent;
	char type;
	char name[32];
} Inode;

int load_inodes(Inode inodes[], int in_use[]){
	FILE *fp = fopen("inodes_list", "rb");
	if(fp == NULL){
		printf("Error reading inodes_list\n");
		return 0;
	}

	 while(1){
		 Inode temp;
		 if(fread(&temp, sizeof(Inode), 1, fp) != 1){
			break;
		 }
		 if(temp.inode >= 1024){
			printf("Invalid inode number\n");
			continue;
		 }
		 if(temp.type != 'd' && temp.type != 'f'){
			printf("Invalid type of inode\n");
			continue;
		 }

		 inodes[temp.inode] = temp;
		 in_use[temp.inode] = 1;
	 }
	 fclose(fp);
	 return 1;
}

void save_inodes(Inode inodes[], int in_use[]){
	FILE *fp = fopen("inodes_list", "wb");
	if(fp == NULL){
		printf("Error saving inodes\n");
		return;
	}

	for(int i = 0; i < 1024; i++){
		if(in_use[i] == 1){
			fwrite(&inodes[i], sizeof(Inode), 1, fp);
		}
	}
	fclose(fp);
}

int find_inodes(Inode inodes[], int in_use[], uint32_t parent, char *name){
	for(int i = 0; i < 1024; i++){
		if(in_use[i] == 1){
			char buffer[33];
			for(int j = 0; j < 32; j++){
				buffer[j] = inodes[i].name[j];
			}
			buffer[32] = '\0';

			if(inodes[i].parent == parent && strcmp(buffer, name) == 0){
				return i;
			}
		}
	}
	return -1;
}

int find_free_inode(int in_use[]){
	for(int i = 0; i < 1024; i++){
		if(in_use[i] == 0){
			return i;
		}
	}
	return -1;
}

void list_directory(Inode inodes[], int in_use[], uint32_t current_dir){
	for(int i = 0; i < 1024; i++){
		if(in_use[i] == 1){
			if(inodes[i].parent == current_dir){
				char buffer[33];
				for(int j = 0; j < 32; j++){
					buffer[j] = inodes[i].name[j];
				}
				buffer[32] = '\0';
				printf("inode: %u, type: %c, name: %s\n", inodes[i].inode, inodes[i].type, buffer);
			}
		}
	}
}

int change_directory(Inode inodes[], int in_use[], uint32_t current_dir, char *name){
	int inode = find_inodes(inodes, in_use, current_dir, name);
	if(inode == -1){
		printf("Directory not found\n");
		return current_dir;
	}

	if(inodes[inode].type != 'd'){
		printf("Not a directory\n");
		return current_dir;
	}
	return inode;
}

void make_directory(Inode inodes[], int in_use[], uint32_t current_dir, char *name){
	if(find_inodes(inodes, in_use, current_dir, name) != -1){
		printf("Directory already exists\n");
		return;
	}

	int new_inode = find_free_inode(in_use);
	if(new_inode == -1){
		printf("No free inodes available\n");
		return;
	}

	inodes[new_inode].inode = new_inode;
	inodes[new_inode].parent = current_dir;
	inodes[new_inode].type = 'd';

	for(int i = 0; i < 32; i++){
		if(name[i] == '\0'){
			inodes[new_inode].name[i] = '\0';
		}
		else{
			inodes[new_inode].name[i] = name[i];
		}
	}
	
	in_use[new_inode] = 1;
	char path[32];
	snprintf(path, sizeof(path), "%d", new_inode);
	FILE *fp = fopen(path, "w");

	if(fp == NULL){
		printf("Error creating directory\n");
		in_use[new_inode] = 0;
		return;
	}

	fprintf(fp, "%u .\n", inodes[new_inode].inode);
	fprintf(fp, "%u ..\n", current_dir);
	fclose(fp);
}

void make_file(Inode inodes[], int in_use[], uint32_t current_dir, char *name){
	if(find_inodes(inodes, in_use, current_dir, name) != -1){
		printf("File already exists\n");
		return;
	}
	int new_inode = find_free_inode(in_use);
	if(new_inode == -1){
		printf("No free inodes available\n");
		return;
	}
	inodes[new_inode].inode = new_inode;
	inodes[new_inode].parent = current_dir;
	inodes[new_inode].type = 'f';

	for(int i = 0; i < 32; i++){
		if(name[i] == '\0'){
			inodes[new_inode].name[i] = '\0';
		}
		else{
			inodes[new_inode].name[i] = name[i];
		}
	}
	in_use[new_inode] = 1;
	char path[32];
	snprintf(path, sizeof(path), "%d", new_inode);
	FILE *fp = fopen(path, "w");
	if(fp == NULL){
		printf("Error creating file\n");
		in_use[new_inode] = 0;
		return;
	}

	fprintf(fp, "%s", name);
	fclose(fp);
}

int main(int argc, char *argv[]){
	Inode inodes[1024] = {0};
	int in_use[1024] = {0};
	
	if(argc != 2){
		printf("Please enter two arguments\n");
		return 1;
	}

	if(chdir(argv[1]) != 0){
		printf("Error opening directory\n");
		return 1;
	}

	if(load_inodes(inodes, in_use) == 0){
		return 1;
	}

	if(in_use[0] == 0 || inodes[0].type != 'd'){
		printf("Root directory is invalid\n");
		return 1;
	}
	uint32_t current_dir = 0;
	char command[100];

	while(1){
		printf("> ");
		if(fgets(command, sizeof(command), stdin) == NULL){
			break;
		}

		if(strcmp(command, "exit\n") == 0){
			break;
		}

		if(strcmp(command, "ls\n") == 0){
			list_directory(inodes, in_use, current_dir);
			continue;
		}
		char input[20] = {0};
		char name[33] = {0};
		char extra[33] = {0};

		if(sscanf(command, "%19s %32s %32s", input, name, extra) == 2){
			if(strcmp(input, "mkdir") == 0){
				make_directory(inodes, in_use, current_dir, name);
			}
			else if(strcmp(input, "touch") == 0){
				make_file(inodes, in_use, current_dir, name);
			}
			else if(strcmp(input, "cd") == 0){
				current_dir = change_directory(inodes, in_use, current_dir, name);
			}
			else{
				printf("Invalid command\n");
			}
		}
		else{
			printf("Invalid command\n");
		}
	}

	save_inodes(inodes, in_use);

	return 0;
}
