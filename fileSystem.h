#pragma once
#include<iostream>
#include<string>
#include<string.h>
#include<stdio.h>
#include<stdlib.h>
#include<windows.h>
#include <malloc.h>
#include <signal.h>
#include <time.h>
#pragma warning(disable:4996)
using namespace std;
char FILENAME[30];			//��ǰ�ļ���
char rootname[30];			//��ǰ�û����û���
char rootpath[30];			//��ǰ�û�����Ŀ¼
char FLAGROOT[30];
#define INODE_BLOCKNUM 12 //ÿ���ļ����ռ�ݵĴ��̿��С
#define BLOCKSIZE 1024	  //ÿ�����̿��СΪ1024�ֽ�
#define BLOCKNUM 65536	  //���̿������Ϊ65536��
#define INODENUM 1024	  //inodeһ����1024��
#define MAX_NAME 30		  //name�������Ϊ30���ַ�
#define USERS 100		  //�����100���û�
#define MAX_OPENFILE 5      //���ͬʱ��5���ļ�
//������ �洢������Ϣ
typedef struct SuperBlock {
	int blocksize;
	int blocknum;
	int inodenum;
	int blockfree;               //ʣ��block��
	int inodefree;               //ʣ��inode��
	int blockBitmap[BLOCKNUM];  //block����
	int inodeBitmap[INODENUM];  //inode����
	int length;                  //�û�����
}SuperBlock;   

SuperBlock superBlock;          //�ļ�ϵͳ����Ϣ

typedef struct INode {
	char filename[10];//�ļ���
	int ino;//inode���,��FCB������ϵ
	int isdir;//�Ƿ�Ϊ�ļ�Ŀ¼
	int parent;//��inode�ĸ�Ŀ¼
	int length;//�ļ����ʾ���С��Ŀ¼���ʾ���ļ�����
	int blockNum[INODE_BLOCKNUM];//INODE��block���
	int blockIndex;//blockNum������±�
	int blockNumLength[INODE_BLOCKNUM];//ÿ��block�Ѿ��õ��Ŀռ䣬�����ļ�(��Ŀ¼) ��ʾ����д����ֽ���,�����ļ�Ŀ¼��˵��¼����д���fcb����
	char owner[30];//�ļ������û�		
}INode,*INodePtr;


typedef struct FCB {
	int ino;//�ļ����
	char filename[10];//�ļ���
	int isdir;//0Ϊ�ļ���1Ϊ�ļ�Ŀ¼
	char owner[30];//�ļ������û�
}FCB,*FcbPtr;

//�û���Ϣ ������������������
typedef struct User {
	char username[30];
	char password[30];
}User, *UserPtr;

int currentDir;//��ǰ��¼Ŀ¼��inode��

FILE *fp;//��ǰ�û����ļ�ָ��
char openFile[MAX_OPENFILE][100];//��ǰ����open״̬���ļ���
int openDir[MAX_OPENFILE];//��ǰ����open״̬���ļ�����Ŀ¼��inode��
char openPath[MAX_OPENFILE][100] = { "","","","",""}; //��ǰ����open״̬���ļ��ľ���·��
const int superBlockSize = sizeof(superBlock);
const int inodeSize = sizeof(INode);
const int fcbSize = sizeof(FCB);
const int userSize = sizeof(User);
const int Offset = superBlockSize + INODENUM * inodeSize + BLOCKNUM * BLOCKSIZE;
char *argv[10];                 //ָ������
int argc;                        //��¼ָ����
char path[100] = "";             //��¼��ǰĿ¼
int FLAG = 0;                    //�������
int cdFlag = 1;                  //cd�����ִ�����
int openfilenum = 0;
char content_temp[65536];
//						  0      1		2			3			4		5		6		7		8		9				10		11		12		13		14	  15		16			17		
const char* syscmd[] = { "cd", "dir", "treedir", "deldir", "mkdir", "create", "open", "close", "help", "showpath", "write", "read", "copy", "exit", "import", "export", "xcopydir", "clear"};
//���ҵ�ǰĿ¼���ļ���Ϊname���ļ���inode���
//flag=0 ��ʾΪ�ļ� flag=1 ��ʾΪĿ¼
//����ֵΪinode���
int findInodeNum(char *name, int flag) {
	int fileInodeNum = -1;
	INodePtr parentInode = (INodePtr)malloc(inodeSize);
	FcbPtr newfcb = (FcbPtr)malloc(fcbSize);
	//�����ļ��ڲ�ָ��ָ��ǰ�ļ�����Ӧinode����λ��
	fseek(fp, superBlockSize + currentDir * inodeSize, SEEK_SET);
	//��ȡ���ݵ����õ�parentInode�ڵ�
	fread(parentInode, inodeSize, 1, fp);
	//��ȡ���ļ������ļ�����ռ�õĴ��̿�
	for (int j = 0; j < parentInode->blockIndex; ++j) {
		fseek(fp, superBlockSize + inodeSize * INODENUM + parentInode->blockNum[j] * BLOCKSIZE, SEEK_SET);
		for (int i = 0; i < parentInode->blockNumLength[j]; i++) {
			fread(newfcb, fcbSize, 1, fp);
			if ((newfcb->isdir == flag) && (strcmp(name, newfcb->filename) == 0)) {
				fileInodeNum = newfcb->ino;
				break;
			}
		}
		if (fileInodeNum != -1)
			break;
	}
	free(newfcb);
	free(parentInode);
	return fileInodeNum;
}
//ʵ��Ŀ¼���л�
void cd(char* name)
{
	if (argc > 2)
	{
		printf("�����������,����\n");
		cdFlag = 0;
		return;
	}
	//int fileInodeNum;
	INodePtr fileInode = (INodePtr)malloc(inodeSize);
	if (strcmp(name, "..") == 0) 
	{
		//��ת����ǰ�ļ��е���һ�� cd ..
		fseek(fp, superBlockSize + currentDir * inodeSize, SEEK_SET);
		fread(fileInode, inodeSize, 1, fp);
		currentDir = fileInode->parent;
		char temp[100] = "";
		int cnt = 0;
		for (int i = strlen(path) - 1; i >= 0; --i) {
			cnt++;
			if (path[i] == '/')
				break;
		}
		strncpy(temp, path, strlen(path) - cnt);
		strcpy(path, temp);
	}
	else if (strcmp(name, "/") == 0) { 
		//���ظ�Ŀ¼ cd /
		fseek(fp, superBlockSize, SEEK_SET);
		fread(fileInode, inodeSize, 1, fp);
		currentDir = fileInode->parent;
		strcpy(path, "/");
	}
	else if (name[0] == '/') { 
		char* subName;
		//strtok�������ڷֽ��ַ���Ϊһ���ַ���
		//����ֵ
		//�ú������ر��ֽ�ĵ�һ�����ַ��������û�пɼ������ַ������򷵻�һ����ָ�롣
		subName = strtok(name, "/");
		int f = 1;
		char temp[100] = "";
		strcpy(temp, path);//�Ȱѵ�ǰ·���ŵ�temp����ʱ�洢
		int tempInodenum = currentDir;//��¼�µ�ǰINode���
		fseek(fp, superBlockSize, SEEK_SET);//�����ļ�ϵͳ��
		fread(fileInode, inodeSize, 1, fp);//�ҵ���һ��INode��Ӧ���ļ���Ϣ
		currentDir = fileInode->parent;//���õ�ǰ�ļ���Ϊ��Ŀ¼
		strcpy(path, "/");
		while (subName != NULL) {
			if (strcmp(subName, "") != 0) {
				int fileInodeNum = findInodeNum(subName, 1);//�ҵ����ļ��ж�ӦINode���
				if (fileInodeNum == -1) {
					printf("Ŀ¼������!\n");
					cdFlag = 0;
					f = 0;
					break;
				}
				else {
					currentDir = fileInodeNum;//����ǰ�ļ��б�Ÿ���
					if (strlen(path) > 1)strcat(path, "/");//����·��
					strcat(path, subName);
				}
			}
			subName = strtok(NULL, "/");
		}
		if (!f) {
			//���û�ҵ� ��ԭ����ĵ�ǰ�ļ��С�·��
			currentDir = tempInodenum;
			strcpy(path, temp);
		}
	}
	else if (strcmp(name, ".") != 0) {
		//��ת����ǰ�ļ��е����ļ��� cd subName/....
		char* subName;
		subName = strtok(name, "/");
		int f = 1;
		char temp[100] = ".";
		strcpy(temp, path);
		int tempInodenum = currentDir;
		while (subName != NULL) {
			if (strcmp(subName, "") != 0) {
				int fileInodeNum = findInodeNum(subName, 1);
				if (fileInodeNum == -1) {
					printf("Ŀ¼������!\n");
					cdFlag = 0;
					f = 0;
					break;
				}
				else {
					currentDir = fileInodeNum;
					if (strlen(path) > 1)strcat(path, "/");
					strcat(path, subName);
				}
			}
			subName = strtok(NULL, "/");
		}
		if (!f) {
			currentDir = tempInodenum;
			strcpy(path, temp);
		}
	}
	free(fileInode);
}
//�����ļ����ļ�Ŀ¼
//flag=1 �����ļ��� flag=0�����ļ�
int creatFlag = 1;
void createFile(char* name, int flag)
{
	FLAG = 1;
	if (flag==1&&argc > 2) {
		printf("��������\n");
		FLAG = 0;
		return;
	}
	if (flag == 0 && argc > 3) {
		printf("��������\n");
		FLAG = 0;
		return;
	}
	if (strcmp(name, "") == 0) {
		printf("�������\n"); 
		FLAG = 0;
		return;
	}
	//rootpath��ǰ�û�����Ŀ¼ path��ǰλ��
	if (strlen(rootpath) > strlen(path)) {
		FLAG = 0;
	}
	//�����ڵ�ǰ�û�Ŀ¼�´����ļ�
	for (int i = 0; i < strlen(rootpath); ++i) {
		if (rootpath[i] != path[i]) {
			FLAG = 0;
			break;
		}
	}
	if (!FLAG) {
		printf("��Ȩ��\n");
		return;
	}
	creatFlag = 1;//��ʾ���Դ���
	INodePtr fileInode = (INodePtr)malloc(inodeSize);
	fseek(fp, superBlockSize + currentDir * inodeSize, SEEK_SET);
	//������ǰ���ļ��е�inode
	fread(fileInode, inodeSize, 1, fp);
	FcbPtr ptrFcb = (FcbPtr)malloc(fcbSize);
	int f = 1;
	for (int i = 0; i < fileInode->blockIndex; ++i) {
		fseek(fp, superBlockSize + INODENUM * inodeSize + fileInode->blockNum[i] * BLOCKSIZE, SEEK_SET);
		for (int j = 0; j < fileInode->blockNumLength[i]; ++j) {
			fread(ptrFcb, fcbSize, 1, fp);
			if (flag == ptrFcb->isdir && strcmp(ptrFcb->filename, name) == 0) {
				f = 0;
				break;
			}
		}
		if (!f)
			break;
	}

	if (!f) {
		printf("�Ѵ��ڸ��ļ���Ŀ¼\n"); 
		FLAG = 0;
		free(ptrFcb);
		free(fileInode);
		return;
	}
	if (superBlock.inodefree == 0) {
		printf("�ռ䲻��\n"); 
		FLAG = 0;
		free(ptrFcb);
		free(fileInode);
		return;
	}
	bool isSpaceEnough = false;
	//�����inode���䵽�����һ�����̿鲻���Է���һ��fcb,�򷵻ص����ļ�Ŀ¼���̿���������
	for (int i = 0; i < INODE_BLOCKNUM; i++)
	{
		if (BLOCKSIZE - fileInode->blockNumLength[i] * fcbSize >= fcbSize)
		{
			isSpaceEnough = true;
			break;
		}
	}
	if (!isSpaceEnough)
	{
		printf("�����ļ�Ŀ¼���̿���������\n");
		return;
	}
	int inodeIndex = -1;
	//����λͼ
	for (int i = 0; i < INODENUM; ++i) {
		if (superBlock.inodeBitmap[i] == 0) {
			inodeIndex = i;
			superBlock.inodefree--;
			superBlock.inodeBitmap[i] = 1;
			break;
		}
	}
	//����д���ļ�
	rewind(fp);
	fwrite(&superBlock, superBlockSize, 1, fp);

	FcbPtr newFcb = (FcbPtr)malloc(fcbSize);
	INodePtr newInode = (INodePtr)malloc(inodeSize);
	strcpy(newInode->filename, name);//�����ļ�/�ļ�����
	newInode->isdir = flag;//�����ļ����ļ�������
	//�ļ���ŵĴ��̿���
	memset(newInode->blockNum, 0, sizeof(newInode->blockNum));
	//�ļ���ŵ�ÿ�����̿����ļ���������Ϊ0
	memset(newInode->blockNumLength, 0, sizeof(newInode->blockNumLength));
	newInode->parent = currentDir;//���׽ڵ�Ϊ��ǰ�ļ���
	newInode->blockIndex = 0;//blockNum������±�
	newInode->ino = inodeIndex;//���ǵڼ���inode��
	newInode->length = 0;//�ļ����к���0���ļ��������ļ�����Ϊ0
	if (strcmp(FLAGROOT, "") != 0) {
		strcpy(newInode->owner, FLAGROOT);
		strcpy(newFcb->owner, FLAGROOT);
	}
	else {
		strcpy(newInode->owner, rootname);
		strcpy(newFcb->owner, rootname);
	}
	//ͬ������FCB
	strcpy(newFcb->filename, name);
	newFcb->isdir = flag;
	newFcb->ino = inodeIndex;
	f = 1;
	//�����ǰ���̿�Ϊ0�飬����ǰһ�����̿��Ѿ�ռ��������Ҫ�ٴӿ��еĴ��̿�����һ��
	if (fileInode->blockIndex == 0 || (fileInode->blockNumLength[fileInode->blockIndex - 1] + 1) * fcbSize > BLOCKSIZE) {
		int block_Index = -1;
		for (int i = 0; i < BLOCKNUM; ++i) {
			if (superBlock.blockBitmap[i] == 0) {
				if (superBlock.blockfree == 0) {
					f = 0;
					break;
				}
				superBlock.blockfree--;
				block_Index = i;

				superBlock.blockBitmap[i] = 1;
				break;
			}
		}
		if (f) {
			fileInode->blockNum[fileInode->blockIndex] = block_Index;
			fileInode->blockNumLength[fileInode->blockIndex] = 0;
			fileInode->blockIndex++;
		}
	}
	if (f) {
		fseek(fp, superBlockSize + INODENUM * inodeSize + fileInode->blockNum[fileInode->blockIndex - 1] * BLOCKSIZE + fileInode->blockNumLength[fileInode->blockIndex - 1] * fcbSize, SEEK_SET);
		fwrite(newFcb, fcbSize, 1, fp);
		fileInode->blockNumLength[fileInode->blockIndex - 1]++;
		fileInode->length++;
		fseek(fp, superBlockSize + currentDir * inodeSize, SEEK_SET);
		fwrite(fileInode, inodeSize, 1, fp);
		rewind(fp);
		fwrite(&superBlock, superBlockSize, 1, fp);
		fseek(fp, superBlockSize + newInode->ino * inodeSize, SEEK_SET);
		fwrite(newInode, inodeSize, 1, fp);
	}
	else {
		printf("�ռ䲻��\n"); 
		FLAG = 0;
	}
	free(newInode);
	free(newFcb);
	free(fileInode);
	free(ptrFcb);
	fflush(fp);
}
void mkdir(char* name)
{
	//�����ļ���Ϊname���ļ���
	createFile(name,1);

}
void setPath()
{
	if (path[0] != '/')
	{
		char temp[] = "/";
		strcat(temp, path);
		strcpy(path, temp);
	}
	printf("OS:%s:%s%c>", rootname, path, (strcmp(rootname, "root") == 0) ? '#' : '$');
}
//�û���¼
int user_register(char* name, char* password)
{
	rewind(fp);
	fread(&superBlock, superBlockSize, 1, fp);
	User user;
	int user_number = superBlock.length;//��ȡ�û�����
	fseek(fp, Offset, SEEK_SET);//�û���Ϣ�ڴ��̵�λ��
	for (int i = 1; i <= user_number; i++)
	{
		fread(&user, sizeof(User), 1, fp);
		//�û����ظ�
		if (strcmp(user.username, name) == 0)
		{
			return 0;
		}
	}
	//���´����û�����
	strcpy(user.username, name);
	strcpy(user.password, password);
	fseek(fp, Offset + user_number * userSize, SEEK_SET);
	fwrite(&user, userSize, 1, fp);
	superBlock.length++;
	rewind(fp);
	fwrite(&superBlock, superBlockSize, 1, fp);
	fflush(fp);
	return 1;
	
}
int login(char *name,char*password)
{
	rewind(fp);
	fread(&superBlock, superBlockSize, 1, fp);
	User user;
	int cnt = superBlock.length;//superBlock.lengthΪ�û�����
	fseek(fp, Offset, SEEK_SET);//offsetΪƫ����
	for (int i = 0; i < cnt; i++)
	{
		fread(&user, sizeof(User), 1, fp);//ÿ�ζ���һ���û�����
		if (strcmp(name,user.username)==0&&strcmp(password,user.password)==0)
		{
			return 1;
		}
	}
	return 0;

}
void openFileSystem() 
{
	for (int i = 0; i < MAX_OPENFILE; i++)
	{
		strcpy(openFile[i], "");//openFile��ǰ����open״̬���ļ���
		openDir[i] = -1;//��ǰ����open״̬���ļ�����Ŀ¼��inode��
	}
	if ((fp = fopen(FILENAME, "rb+")) == NULL) {
		//rb+����д��һ���������ļ���ֻ�����д���ݡ�
		printf("�򿪴���\n");
		exit(1);
	}
	rewind(fp);//ʹ�ļ����»ص���ʼ��λ��
	fread(&superBlock, superBlockSize, 1, fp);//�����ļ�ϵͳ����Ϣ
	strcpy(path, "/");//path�ǵ�ǰĿ¼
	currentDir = 0;//��ǰ�ļ��ж�Ӧ��inode
	if (strcmp(rootname, "root") != 0) {//�����ǰ���û�������root
		strcpy(FLAGROOT, rootname);//�ѵ�ǰ�û�������FLAGROOT
		char temp[100];
		strcpy(temp, rootname);//�ѵ�ǰ�û�������temp
		strcpy(rootname, "root");//�ѵ�ǰ�û�������Ϊroot
		if (findInodeNum(temp, 1) == -1) {//����Ҳ����û���Ϊtemp���ļ���
			mkdir(temp);//����һ���û���Ϊtemp���ļ���
		}
		cd(temp);//��ת���û���Ϊtemp���ļ���
		strcpy(rootname, temp);//�������õ�ǰ�û����û���rootnameΪtemp
	}
	else 
	{
		/*char temp[100] = "root";
		if (findInodeNum(temp, 1) == -1)
		{
			mkdir(temp);
		}
		cd(temp);*/
		strcpy(rootname, "root");
	}
	strcpy(rootpath, path);
	strcpy(FLAGROOT, "");
}
int splitOrder(char* cmd)
{
	FLAG = 1;
	char temp[MAX_NAME];

	int length = 0;//cmd����
	char *ptr_temp;

	argc = 0;//��������
	//�Կո�ָ�cmd����
	for (length = 0, ptr_temp = cmd; *ptr_temp != '\0'; ++ptr_temp) {
		if (*ptr_temp != ' ') {
			while (*ptr_temp != ' ' && (*ptr_temp != '\0')) {
				temp[length] = *ptr_temp;
				length++;
				ptr_temp++;
			}
			argv[argc] = (char *)malloc(length + 1);
			strncpy(argv[argc], temp, length);
			argv[argc][length] = '\0';
			argc++;

			length = 0;
			if (*ptr_temp == '\0')
				break;
		}
	}
	if (argc != 0) {
		int i;
		for (i = 0; i < 18 && strcmp(argv[0], syscmd[i]) != 0; ++i);
		return i;
	}
	else
		return 24;
	return 0;
}
void ls(int ls_number) 
{
	if (argc > 2) {
		printf("����������࣬����\n");
		return;
	}
	int ls_dir = ls_number;
	printf("<TYPE>%16s%16s%16s%16s\n", "NAME", "INODENUMBER", "OWNER", "LENGTH");
	int dirCount = 0;
	int fileCount = 0;
	int CurTotalFileSize = 0;
	FcbPtr ptrFcb = (FcbPtr)malloc(fcbSize);
	INodePtr parentInode = (INodePtr)malloc(inodeSize);     //ָ��ǰĿ¼
	//parentInode ָ��ǰĿ¼��inode
	fseek(fp, superBlockSize + ls_dir * inodeSize, SEEK_SET);
	fread(parentInode, inodeSize, 1, fp);
	for (int i = 0; i < parentInode->blockIndex; ++i) {
		for (int j = 0; j < parentInode->blockNumLength[i]; ++j) {
			fseek(fp, superBlockSize + INODENUM * inodeSize + parentInode->blockNum[i] * BLOCKSIZE + j * fcbSize, SEEK_SET);
			fread(ptrFcb, fcbSize, 1, fp);
			if (ptrFcb->isdir == 1) {
				printf("<DIR> %16s%16d", ptrFcb->filename, ptrFcb->ino);
				int num = ptrFcb->ino;
				fseek(fp, superBlockSize + num * inodeSize, SEEK_SET);
				INodePtr ptRinode = (INodePtr)malloc(inodeSize);
				fread(ptRinode, inodeSize, 1, fp);
				printf("%16s%16d\n", ptRinode->owner, ptRinode->length);
				free(ptRinode);
				dirCount++;
			}
			else {
				printf("<FILE>%16s%16d", ptrFcb->filename, ptrFcb->ino);
				int num = ptrFcb->ino;
				fseek(fp, superBlockSize + num * inodeSize, SEEK_SET);
				INodePtr ptRinode = (INodePtr)malloc(inodeSize);
				fread(ptRinode, inodeSize, 1, fp);
				printf("%16s%16d\n", ptRinode->owner, ptRinode->length);
				free(ptRinode);
				fileCount++;
			}
		}
	}
	printf("\n%d���ļ�\n", fileCount);
	printf("%d��Ŀ¼\n", dirCount);
	free(ptrFcb);
	free(parentInode);
}
int getDirNumber(char* path)
{
	char temp_argv[100];
	strcpy(temp_argv, path);
	//int fileInodeNum;
	//int tempInodenum = currentDir;
	int f = 1;
	if (argc == 1)
	{
		//˵��ָ���� dir (��)����Ӧ��չʾ��ǰĿ¼�µ������ļ���Ϣ
		return currentDir;
	}
	else
	{
		char* subName;
		int temp_dir = currentDir;
		if(temp_argv[0]=='/')currentDir = 0;
		int flag = 1;
		subName = strtok(temp_argv, "/");
		if (subName == NULL&&strcmp(temp_argv,"/")!=0)flag = 0;
		while (subName != NULL)
		{
			if (strcmp(subName, "") != 0)
			{
				int fileInodeNum = findInodeNum(subName, 1);
				if (fileInodeNum == -1)
				{
					flag = 0;
					break;
				}
				currentDir = fileInodeNum;
			}
			subName = strtok(NULL, "/");
		}
		int ret;
		if (flag)
		{
			ret = currentDir;
		}
		else
		{
			ret = -1;
		}
		currentDir = temp_dir;
		return ret;
		
	}
	
}
void help()
{
	cout << "\t\tcd [����·��]                                   �л�����·��Ϊ��ǰĿ¼" << endl;
	cout << "\t\tdir ([�����ļ�·��])                            �и���·�����ļ�Ŀ¼" << endl;
	cout << "\t\ttreedir ([�����ļ�·��])                        ѭ���г�����·���µ���������Ŀ¼���ļ���ʽ������������ʾ" << endl;
	cout << "\t\tmkdir [��������]                                �ڵ�ǰĿ¼�´����������ֵ�Ŀ¼" << endl;
	cout << "\t\tdeldir [�����ļ�·��]                           ɾ������·���Ŀ�Ŀ¼" << endl;
	cout << "\t\tcreate ([�����ļ�·��]) [�����ļ���]            ����ָ��·���¸����ļ������ļ�" << endl;
	cout << "\t\tshowpath                                        ��ʾ��ǰ�򿪵��ļ�·��"   << endl;
	cout << "\t\topen ([�����ļ�·��]) [�����ļ���]              ��ָ��·���¸����ļ������ļ�" << endl;
	cout << "\t\tclose ([�����ļ�·��]) [�����ļ���]             �ر�ָ��·���¸����ļ������ļ�" << endl;
	cout << "\t\tread ([�����ļ�·��]) [�����ļ���]              ��ȡָ��·���¸����ļ������ļ�" << endl;
	cout << "\t\twrite ([�����ļ�·��]) [�����ļ���]             дָ��·���¸����ļ������ļ�" << endl;
	cout << "\t\tcopy ([Դ�ļ�·��]) [�����ļ���] [Ŀ���ļ�·��] ����Դ�ļ�·���¸����ļ������ļ���Ŀ���ļ�·��"<< endl;
	cout << "\t\txcopydir [�����ļ�Ŀ¼·��] [����·��]          ����ĳ��Ŀ¼����������ͬ���������︴�Ƶ�������·����" << endl;
	cout << "\t\texport  ([Դ�ļ�·��]) [�ļ���] [����·��]      ����ϵͳ����Ŀ¼�µĸ����ļ����뵽Windows�µĸ���·����" << endl;
	cout << "\t\timport  [�����ļ�] [����·��]					 ��Windows�µ��ļ����뵽��ϵͳ����·����" << endl;
	cout << "\t\tclear                                           �����Ļ" << endl;
	cout << "\t\texit                                            �˳��ļ�ϵͳ" << endl;
}
void treedir(int now_dir,int layer)
{
	INodePtr fileInode = (INodePtr)malloc(inodeSize);
	for (int i = 0; i < layer; i++) cout << "   ";
	cout << "|--";
	fseek(fp, superBlockSize + now_dir * inodeSize, SEEK_SET);
	fread(fileInode, inodeSize, 1, fp);
	cout << fileInode->filename << endl;
	if (fileInode->isdir == 1)
	{
		int count = 1; 
		INodePtr fileInode2 = (INodePtr)malloc(inodeSize);
		for (int j = 1; j < INODENUM && count<= fileInode->length; j++)
		{
			fseek(fp, superBlockSize +j * inodeSize, SEEK_SET);
			fread(fileInode2, inodeSize, 1, fp);
			if (fileInode2->parent == fileInode->ino&&strlen(fileInode2->filename)!=0&&superBlock.inodeBitmap[j]!=0)
			{
				//cout << fileInode2->filename << endl;
				treedir(j, layer + 1);
				count++;
			}
		}
		free(fileInode2);
	}
	free(fileInode);
}
void DelBlankDir(int dir_number)
{
	FLAG = 1;
	//rootpath��ǰ�û�����Ŀ¼ path��ǰλ��
	if (strlen(rootpath) > strlen(path)) {
		FLAG = 0;
	}
	//�����ڵ�ǰ�û�Ŀ¼�´����ļ�
	for (int i = 0; i < strlen(rootpath); ++i) {
		if (rootpath[i] != path[i]) {
			FLAG = 0;
			break;
		}
	}
	if (!FLAG) {
		printf("��Ȩ��\n");
		return;
	}
	if (dir_number == 0)
	{
		printf("��Ŀ¼�޷���ɾ��\n");
		return;
	}
	INodePtr filenode = (INodePtr)malloc(inodeSize);
	fseek(fp, superBlockSize + dir_number * inodeSize, SEEK_SET);
	fread(filenode, inodeSize, 1, fp);
	if (filenode->length == 0&&filenode->isdir==1)
	{
		int parentDir = filenode->parent;
		fseek(fp, superBlockSize + parentDir * inodeSize, SEEK_SET);
		INodePtr parent_node = (INodePtr)malloc(inodeSize);
		fread(parent_node,inodeSize, 1, fp);
		FcbPtr fcb = (FcbPtr)malloc(fcbSize);
		int index = -1;
		for (int i = 0; i < parent_node->blockIndex; i++)
		{
			fseek(fp, superBlockSize + INODENUM * inodeSize + parent_node->blockNum[i] * BLOCKSIZE, SEEK_SET);
			for (int j = 1; j <= parent_node->blockNumLength[i]; j++)
			{
				fread(fcb, fcbSize, 1, fp);
				if (fcb->ino == dir_number&&fcb->isdir==1)
				{
					index = i;
					break;
				}
			}
			if (index != -1)break;
		}
		fseek(fp, superBlockSize + INODENUM * inodeSize + parent_node->blockNum[index] * BLOCKSIZE, SEEK_SET);
		FcbPtr newfcb=(FcbPtr) malloc(fcbSize*parent_node->blockNumLength[index]);
		for (int i = 0; i < parent_node->blockNumLength[index]; ++i) {
			fread(&newfcb[i], fcbSize, 1, fp);
		}
		fseek(fp, superBlockSize + INODENUM * inodeSize + parent_node->blockNum[index] * BLOCKSIZE, SEEK_SET);
		for (int i = 0; i < BLOCKSIZE; ++i) {
			fputc(0, fp);
		}
		fseek(fp, superBlockSize + INODENUM * inodeSize + parent_node->blockNum[index] * BLOCKSIZE, SEEK_SET);
		for (int i = 0; i < parent_node->blockNumLength[index]; ++i) {
			if (newfcb[i].isdir == 1 && newfcb[i].ino==dir_number)continue;
			fwrite(&newfcb[i], fcbSize, 1, fp);
		}
		parent_node->length--;//�ļ�����1
		parent_node->blockNumLength[index]--;//fcb����-1
		fseek(fp, superBlockSize + parent_node->ino * inodeSize, SEEK_SET);
		fwrite(parent_node, inodeSize, 1, fp);
		rewind(fp);
		superBlock.inodefree++;
		superBlock.inodeBitmap[dir_number] = 0;
		fwrite(&superBlock, superBlockSize, 1, fp);
		fflush(fp);
	}
	else
	{
		printf("Ŀ¼��Ϊ��,�޷�ɾ��\n");
	}
}
void open(char* name)
{
	int fileInodeNum = -1;
	INodePtr parentInode = (INodePtr)malloc(inodeSize);
	fseek(fp, superBlockSize + inodeSize * currentDir, SEEK_SET);
	fread(parentInode, inodeSize, 1, fp);
	FcbPtr ptrFcb = (FcbPtr)malloc(fcbSize);
	for (int i = 0; i < parentInode->blockIndex; ++i) {
		fseek(fp, superBlockSize + INODENUM * inodeSize + parentInode->blockNum[i] * BLOCKSIZE, SEEK_SET);
		for (int j = 0; j < parentInode->blockNumLength[i]; ++j) {
			fread(ptrFcb, fcbSize, 1, fp);
			if (ptrFcb->isdir == 0 && strcmp(ptrFcb->filename, name) == 0) {
				if (strcmp(ptrFcb->owner, rootname) == 0 || strcmp(rootname, "root") == 0)
					fileInodeNum = ptrFcb->ino;
				else
					fileInodeNum = -2;
				break;
			}
		}
		if (fileInodeNum != -1)
			break;
	}
	free(ptrFcb);
	free(parentInode);
	if (fileInodeNum == -1) {
		printf("�ļ�������\n");
		return;
	}
	if (fileInodeNum == -2) {
		printf("��Ȩ��\n");
		return;
	}
	for (int i = 0; i < MAX_OPENFILE; i++)
	{
		if (strcmp(openFile[i], name) == 0 && openDir[i] == currentDir) {
			printf("�Ѿ����˸��ļ�\n");
			return;
		}
	}
	/*if (strcmp(openFile, "") != 0) {
		printf("���ļ��Ѿ����򿪣����ȹر��Ѿ��򿪵��ļ�\n");
		return;
	}*/
	if (openfilenum >= MAX_OPENFILE)
	{
		printf("��ʧ�ܣ��ļ��������ﵽ���ޣ�\n");
		return;
	}
	openfilenum++;
	cout << "�ļ��򿪳ɹ���" << endl;
	int index = -1;
	for (int i = 0; i < MAX_OPENFILE; i++)
	{
		if (strcmp(openFile[i], "") == 0)
		{
			strcpy(openFile[i], name);
			openDir[i] = currentDir;
			index = i;
			break;
		}
	}
	//strcpy(openPath, path);
	if (argc == 3)
	{
		if(argv[1][0]=='/')strcpy(openPath[index], argv[1]);//����·��
		else
		{
			strcpy(openPath[index], path);
			if(strcmp(path,"/")!=0)strcat(openPath[index], "/");
			strcat(openPath[index], argv[1]);
		}
		strcat(openPath[index], "/");
		strcat(openPath[index], argv[2]);
	}
	else
	{
		strcpy(openPath[index], path);
		if (strcmp(path, "/") != 0)strcat(openPath[index], "/");
		strcat(openPath[index], argv[1]);
	}
	cout <<"�ļ�·��:"<< openPath[index] << endl;
}
void close(char*name,int dirnum)
{
	bool isitopened = false;
	int index = -1;
	//�жϵ�ǰ�ļ��Ƿ��
	for (int i = 0; i < MAX_OPENFILE; i++)
	{
		if (dirnum==openDir[i]){
			isitopened = true;
			index = i;
			break;
		}
	}
	if (isitopened == false)
	{
		printf("���ļ�û�б���\n");
		return;
	}
	strcpy(openFile[index], "");
	strcpy(openPath[index], "");
	openDir[index] = -1;
	openfilenum--;
}
void write(char* name,int dirnum) {
	//FLAG = 0;
	//char name[100] = "";
	bool isitopen = false;
	for (int i = 0; i < MAX_OPENFILE; i++)
	{
		if (dirnum == openDir[i]&&strcmp(name,openFile[i])==0) {
			isitopen = true;
			break;
		}
	}
	if (isitopen == false)
	{
		printf("����open�ļ�\n");
		return;
	}
	//strcpy(name, openFile);
	//char temp[100] = "", temp1[100];
	//strcpy(temp, path);
	//strcpy(temp1, openPath);
	//cd(openPath);
	int fileInodeNum = findInodeNum(name, 0);
	//int fileInodeNum = writenum;
	INodePtr fileInode = (INodePtr)malloc(inodeSize);
	FcbPtr ptrFcb = (FcbPtr)malloc(fcbSize);
	char c;
	if (fileInodeNum == -1)
		printf("%s�ļ�������\n", name);
	else {
		//�ҵ��ļ�inode

		fseek(fp, superBlockSize + fileInodeNum * inodeSize, SEEK_SET);

		fread(fileInode, inodeSize, 1, fp);

		//�ҵ���һ��д���λ��
		int blockIndex =0;
		int byte_pos = 0;
		if (fileInode->blockIndex != 0) blockIndex = fileInode->blockIndex - 1;
		byte_pos = fileInode->blockNumLength[blockIndex];
		int original_blockindex = blockIndex;
		int original_byte_pos = byte_pos;
		if(blockIndex!=0)fseek(fp, superBlockSize+INODENUM * inodeSize + fileInode->blockNum[blockIndex] * BLOCKSIZE+byte_pos, SEEK_SET);
		printf("�������ļ�����(��$��β):\n");
		//int i = 0;//����д����ַ�����
		int increaseblocknum = 0;
		//bool addnewblock = false;
		while ((c = getchar()) != '$') {
			//д��һ�����̿���߸�inode��㻹û�б�������̿�
			if (blockIndex==0||BLOCKSIZE-byte_pos<2) {
				//��Ϊһ�����������ֽ����Ե����̿�ʣ��1��0ʱ������һ�����̿�
				//cout << byte_pos % BLOCKSIZE << endl;
				//byte_pos = byte_pos % BLOCKSIZE;
				//cout << BLOCKSIZE << endl;
				blockIndex = -1;
				byte_pos = 0;
				for (int j = 0; j < BLOCKNUM; ++j) {
					if (superBlock.blockBitmap[j] == 0) {
						superBlock.blockfree--;
						superBlock.blockBitmap[j] = 1;
						blockIndex = j;
						break;
					}
				}
				if (blockIndex != -1) {
					increaseblocknum++;
					fileInode->blockNum[fileInode->blockIndex] = blockIndex;
					fileInode->blockNumLength[fileInode->blockIndex] = 0;
					fseek(fp, superBlockSize + INODENUM * inodeSize + fileInode->blockNum[fileInode->blockIndex] * BLOCKSIZE, SEEK_SET);
					fileInode->blockIndex++;
				}
			}
			if (blockIndex != -1&&fileInode->blockIndex<=INODE_BLOCKNUM) 
			{
				//cout << c << endl;
				fputc(c, fp);
				byte_pos++;
				fileInode->blockNumLength[fileInode->blockIndex - 1]++;
			}
		}
		for (int i = original_blockindex; i < fileInode->blockIndex; i++)
		{
			if (i == original_blockindex) fileInode->length += fileInode->blockNumLength[i] - original_blockindex;
			else fileInode->length += fileInode->blockNumLength[i];
		}
		c = getchar();
		if (blockIndex == -1) {
			printf("�ռ䲻��,��Щδд�룡\n");
		}
		else if (fileInode->blockIndex > INODE_BLOCKNUM)
		{
			printf("�����ļ��ռ�ﵽ����,��Щδд�룡\n");
		}
		else
		{
			printf("�ļ�д��ɹ���\n");
		}
		rewind(fp);
		fwrite(&superBlock, superBlockSize, 1, fp);
		//����inode
		fseek(fp, superBlockSize + fileInodeNum * inodeSize, SEEK_SET);
		fwrite(fileInode, inodeSize, 1, fp);
	}
	free(fileInode);
	free(ptrFcb);
	fflush(fp);
	//cd(temp);
	//strcpy(openPath, temp1);
}
void read(char* name,int dirnum)
{
	//ss
	bool isitopen = false;
	for (int i = 0; i < MAX_OPENFILE; i++)
	{
		if (dirnum == openDir[i]&&strcmp(name, openFile[i]) == 0) {
			isitopen = true;
			break;
		}
	}
	if (isitopen == false)
	{
		printf("����open�ļ�\n");
		return;
	}
	//char c;
	INodePtr fileInode = (INodePtr)malloc(inodeSize);
	INodePtr parentInode = (INodePtr)malloc(inodeSize);
	int fileInodeNum = findInodeNum(name, 0);
	if (fileInodeNum == -1) {
		printf("%s�ļ�������\n", name);
	}
	else {
		fseek(fp, superBlockSize + fileInodeNum * inodeSize, SEEK_SET);
		fread(fileInode, inodeSize, 1, fp);
		for (int i = 0; i < fileInode->blockIndex; ++i) {
			fseek(fp, superBlockSize + INODENUM * inodeSize + fileInode->blockNum[i] * BLOCKSIZE, SEEK_SET);
			/*for (int j = 0; j < fileInode->blockNumLength[i]; ++j) {
				c = fgetc(fp);
				putchar(c);
			}*/
			char content[BLOCKSIZE];
			fread(content,fileInode->blockNumLength[i], 1, fp);
			content[fileInode->blockNumLength[i]] = '\0';
			cout << content;
		}
		cout << endl;
	}
	free(fileInode);
	free(parentInode);
}
void showOpenFilePath()
{
	cout << "��ǰ�򿪵��ļ�����Ϊ" << openfilenum << endl;
	for (int i = 0; i < MAX_OPENFILE; i++)
	{
		if(openDir[i]!=-1)cout << openPath[i] << endl;
	}	
}
int cpFlag = 1;
void cp(char* name, int targetIno)
{
	for (int i = 0; i < MAX_OPENFILE; i++)
	{
		if (strcmp(name, openFile[i]) == 0 && openDir[i] == currentDir) {
			printf("����close�ļ�\n");
			cpFlag = 0;
			return;
		}
	}
	char content[1000008] = "";
	int fileInodeNum = findInodeNum(name, 0);
	if (fileInodeNum == -1) {
		cpFlag = 0;
		printf("�Ҳ���%s�ļ�\n", name);
		return;
	}
	int cnt = 0;
	//Դ�ļ����ļ���inode
	INodePtr fileInode = (INodePtr)malloc(inodeSize);
	fseek(fp, superBlockSize + inodeSize * fileInodeNum, SEEK_SET);
	fread(fileInode, inodeSize, 1, fp);
	for (int i = 0; i < fileInode->blockIndex; ++i) {
		fseek(fp, superBlockSize + inodeSize * INODENUM + fileInode->blockNum[i] * BLOCKSIZE, SEEK_SET);
		/*for (int j = 0; j < fileInode->blockNumLength[i]; ++j) {
			content[cnt++] = fgetc(fp);
		}*/
		fread(content_temp, fileInode->blockNumLength[i], 1, fp);
		//cout << content_temp << endl;
		cnt += fileInode->blockNumLength[i];
		strcat(content, content_temp);
		//cout << content << endl;
		strcpy(content_temp,"");
	}
	//------------------------------------------------------------------------
	INodePtr targetInode = (INodePtr)malloc(inodeSize);//Ŀ���ļ��е�inode
	fseek(fp, superBlockSize + targetIno * inodeSize, SEEK_SET);
	fread(targetInode, inodeSize, 1, fp);
	//����Ҫ�ж�Ŀ���ļ����Ƿ��ܷ������fcb
	//�����inode���䵽�����һ�����̿鲻���Է���һ��fcb,�򷵻ص����ļ�Ŀ¼���̿���������
	bool isSpaceEnough = false;
	for (int i = 0; i < INODE_BLOCKNUM; i++)
	{
		if (BLOCKSIZE -targetInode->blockNumLength[i] * fcbSize >= fcbSize)
		{
			isSpaceEnough = true;
			break;
		}
	}
	if (!isSpaceEnough)
	{
		printf("�����ļ�Ŀ¼���̿���������\n");
		return;
	}
	//�жϴ���ʣ��ռ��Ƿ��㹻
	int block_need = cnt / BLOCKSIZE;
	if (superBlock.blockfree < block_need||superBlock.inodefree==0) {
		printf("���̿ռ䲻��\n");
		cpFlag = 0;
		return;
	}
	//����inode��Ϣ,����1��inode
	int inodeIndex = -1;
	//����λͼ
	for (int i = 0; i < INODENUM; ++i) {
		if (superBlock.inodeBitmap[i] == 0) {
			inodeIndex = i;
			superBlock.inodefree--;
			superBlock.inodeBitmap[i] = 1;
			break;
		}
	}
	//����д���ļ�
	rewind(fp);
	fwrite(&superBlock, superBlockSize, 1, fp);
	content[cnt] = '\0';
	FcbPtr newFcb = (FcbPtr)malloc(fcbSize);
	INodePtr newInode = (INodePtr)malloc(inodeSize);
	strcpy(newInode->filename, name);//�����ļ�/�ļ�����
	newInode->isdir = 0;
	//�ļ���ŵĴ��̿���
	memset(newInode->blockNum, 0, sizeof(newInode->blockNum));
	//�ļ���ŵ�ÿ�����̿����ļ����������ļ���������Ϊ0
	memset(newInode->blockNumLength, 0, sizeof(newInode->blockNumLength));
	newInode->parent = targetIno;//���׽ڵ�Ϊ��ǰ�ļ���
	newInode->blockIndex = 0;//blockNum������±�
	newInode->ino = inodeIndex;//���ǵڼ���inode��
	newInode->length = 0;//�ļ����к���0���ļ��������ļ�����Ϊ0
	if (strcmp(FLAGROOT, "") != 0) {
		strcpy(newInode->owner, FLAGROOT);
		strcpy(newFcb->owner, FLAGROOT);
	}
	else {
		strcpy(newInode->owner, rootname);
		strcpy(newFcb->owner, rootname);
	}
	//ͬ������FCB
	strcpy(newFcb->filename, name);
	newFcb->isdir = 0;
	newFcb->ino = inodeIndex;
	int f = 1;
	//�����ǰ���̿�Ϊ0�飬����ǰһ�����̿��Ѿ�ռ��������Ҫ�ٴӿ��еĴ��̿�����һ��
	if (targetInode->blockIndex == 0 || (targetInode->blockNumLength[targetInode->blockIndex - 1] + 1) * fcbSize > BLOCKSIZE) {
		int block_Index = -1;
		for (int i = 0; i < BLOCKNUM; ++i) {
			if (superBlock.blockBitmap[i] == 0) {
				if (superBlock.blockfree == 0) {
					f = 0;
					break;
				}
				superBlock.blockfree--;
				block_Index = i;
				superBlock.blockBitmap[i] = 1;
				break;
			}
		}
		if (f) {
			targetInode->blockNum[targetInode->blockIndex] = block_Index;
			targetInode->blockNumLength[targetInode->blockIndex] = 0;
			targetInode->blockIndex++;
		}
	}
	if (f) {
		//cout << newFcb->filename << endl;
		fseek(fp, superBlockSize + INODENUM * inodeSize + targetInode->blockNum[targetInode->blockIndex - 1] * BLOCKSIZE + targetInode->blockNumLength[targetInode->blockIndex - 1] * fcbSize, SEEK_SET);
		fwrite(newFcb, fcbSize, 1, fp);
		targetInode->blockNumLength[targetInode->blockIndex - 1]++;
		targetInode->length++;
		fseek(fp, superBlockSize + targetInode->ino * inodeSize, SEEK_SET);
		fwrite(targetInode, inodeSize, 1, fp);
		rewind(fp);
		fwrite(&superBlock, superBlockSize, 1, fp);
		fseek(fp, superBlockSize + newInode->ino * inodeSize, SEEK_SET);
		fwrite(newInode, inodeSize, 1, fp);
	}
	else {
		printf("�ռ䲻��\n");
		FLAG = 0;
	}
	free(targetInode);
	free(newFcb);
	free(fileInode);
	fflush(fp);
	//���´������ļ���д������
	//�ҵ���һ��д���λ��
	int blockIndex = 0;
	int byte_pos = 0;
	if (newInode->blockIndex != 0) blockIndex = newInode->blockIndex - 1;
	byte_pos = newInode->blockNumLength[blockIndex];
	int original_blockindex = blockIndex;
	int original_byte_pos = byte_pos;
	int increaseblocknum = 0;
	//if (blockIndex != 0)fseek(fp, superBlockSize + INODENUM * inodeSize + fileInode->blockNum[blockIndex] * BLOCKSIZE + byte_pos, SEEK_SET);
	for (int i = 0; i < cnt;i++) {
		//д��һ�����̿���߸�inode��㻹û�б�������̿�
		if (blockIndex == 0 || BLOCKSIZE - byte_pos < 2) {
			//��Ϊһ�����������ֽ����Ե����̿�ʣ��1��0ʱ������һ�����̿�
			//cout << byte_pos % BLOCKSIZE << endl;
			//byte_pos = byte_pos % BLOCKSIZE;
			//cout << BLOCKSIZE << endl;
			blockIndex = -1;
			byte_pos = 0;
			for (int j = 0; j < BLOCKNUM; ++j) {
				if (superBlock.blockBitmap[j] == 0) {
					superBlock.blockfree--;
					superBlock.blockBitmap[j] = 1;
					blockIndex = j;
					break;
				}
			}
			if (blockIndex != -1) {
				increaseblocknum++;
				newInode->blockNum[newInode->blockIndex] = blockIndex;
				newInode->blockNumLength[newInode->blockIndex] = 0;
				fseek(fp, superBlockSize + INODENUM * inodeSize + (newInode->blockNum[newInode->blockIndex]) * BLOCKSIZE,SEEK_SET);
				newInode->blockIndex++;
			}
		}
		if (blockIndex != -1 && newInode->blockIndex <= INODE_BLOCKNUM)
		{
			//cout << c << endl;
			fputc(content[i], fp);
			//cout << content[i] << endl;
			byte_pos++;
			newInode->blockNumLength[newInode->blockIndex - 1]++;
		}
	}
	for (int i = original_blockindex; i < newInode->blockIndex; i++)
	{
		if (i == original_blockindex) newInode->length += newInode->blockNumLength[i] - original_blockindex;
		else newInode->length += newInode->blockNumLength[i];
	}
	if (blockIndex == -1) {
		printf("�ռ䲻��,��Щδд�룡\n");
	}
	else if (newInode->blockIndex > INODE_BLOCKNUM)
	{
		printf("�����ļ��ռ�ﵽ����,��Щδд�룡\n");
	}
	else
	{
		printf("�ļ����Ƴɹ���\n");
	}
	rewind(fp);
	fwrite(&superBlock, superBlockSize, 1, fp);
	//����inode
	fseek(fp, superBlockSize + newInode->ino * inodeSize, SEEK_SET);
	fwrite(newInode, inodeSize, 1, fp);
	free(newInode);
}
bool judgeParent(int father_ino, int son_ino)
{
	
	if (father_ino == son_ino)
	{
		return true;
	}
	else
	{
		INodePtr dadinode = (INodePtr)malloc(inodeSize);
		fseek(fp, superBlockSize + father_ino * inodeSize, SEEK_SET);
		fread(dadinode, inodeSize, 1, fp);
		INodePtr soninode = (INodePtr)malloc(inodeSize);
		fseek(fp, superBlockSize + son_ino * inodeSize, SEEK_SET);
		fread(soninode, inodeSize, 1, fp);
		for (int i = 0; i < INODE_BLOCKNUM; i++)
		{
			for (int j = 1; j <=dadinode->blockNumLength[i]; j++)
			{
				FcbPtr fcb = (FcbPtr)malloc(fcbSize);
				fseek(fp, superBlockSize + INODENUM * inodeSize + dadinode->blockNum[i] * BLOCKSIZE + (j - 1) * fcbSize, SEEK_SET);
				fread(fcb, fcbSize, 1, fp);
				int next_dir_num = fcb->ino;
				int isdir = fcb->isdir;
				free(fcb);
				if (isdir == 0)continue;
				if (judgeParent(next_dir_num, son_ino))
				{
					free(dadinode);
					free(soninode);
					return true;
				}
				
			}
			
		}
		free(dadinode);
		free(soninode);
		return false;
	}
}
bool DoesithaveOpenfile(int dir_num)
{
	INodePtr fileinode = (INodePtr)malloc(inodeSize);
	fseek(fp, superBlockSize + dir_num * inodeSize, SEEK_SET);
	fread(fileinode, inodeSize, 1, fp);
	if (fileinode->isdir == 1)
	{
		for (int i = 0; i < INODE_BLOCKNUM; i++)
		{
			for (int j = 1; j < fileinode->blockNumLength[i]; j++)
			{
				FcbPtr fcb = (FcbPtr)malloc(fcbSize);
				fseek(fp, superBlockSize + INODENUM * inodeSize + fileinode->blockNum[i] * BLOCKSIZE + (j - 1) * fcbSize, SEEK_SET);
				fread(fcb, fcbSize, 1, fp);
				int next_dir_num = fcb->ino;
				free(fcb);
				if (DoesithaveOpenfile(next_dir_num))
				{
					free(fileinode);
					return true;
				}
			}
		}
		free(fileinode);
		return false;
	}
	else
	{
		for (int i = 0; i < openfilenum; i++)
		{
			if (openDir[i] == fileinode->parent&&strcmp(openFile[i], fileinode->filename) == 0)
			{
				free(fileinode);
				return true;
			}
		}
		free(fileinode);
		return false;
	}
}
void xcopy(int source_ino, int target_ino)
{
	FLAG = 1;
	//rootpath��ǰ�û�����Ŀ¼ path��ǰλ��
	if (strlen(rootpath) > strlen(path)) {
		FLAG = 0;
	}
	//�����ڵ�ǰ�û�Ŀ¼�´����ļ�
	for (int i = 0; i < strlen(rootpath); ++i) {
		if (rootpath[i] != path[i]) {
			FLAG = 0;
			break;
		}
	}
	if (!FLAG) {
		printf("��Ȩ��\n");
		return;
	}
	INodePtr sourceInode = (INodePtr)malloc(inodeSize);
	fseek(fp, superBlockSize + source_ino * inodeSize, SEEK_SET);
	fread(sourceInode, inodeSize, 1, fp);	INodePtr targetInode = (INodePtr)malloc(inodeSize);
	fseek(fp, superBlockSize + target_ino * inodeSize, SEEK_SET);
	fread(targetInode, inodeSize, 1, fp);
	//cout << fileInode->filename << endl;
	if (sourceInode->isdir == 1)
	{
		//������Ŀ���ļ����´����ļ���Ŀ���ļ�ָ������
		//����inode��Ϣ,����1��inode
		int inodeIndex = -1;
		//����λͼ
		for (int i = 0; i < INODENUM; ++i) {
			if (superBlock.inodeBitmap[i] == 0) {
				inodeIndex = i;
				superBlock.inodefree--;
				superBlock.inodeBitmap[i] = 1;
				break;
			}
		}
		//����д���ļ�
		rewind(fp);
		fwrite(&superBlock, superBlockSize, 1, fp);
		FcbPtr newFcb = (FcbPtr)malloc(fcbSize);
		INodePtr newInode = (INodePtr)malloc(inodeSize);
		strcpy(newInode->filename, sourceInode->filename);//�����ļ�/�ļ�����
		newInode->isdir = sourceInode->isdir;//�����ļ����ļ�������
		//�ļ���ŵĴ��̿���
		memset(newInode->blockNum, 0, sizeof(newInode->blockNum));
		//�ļ���ŵ�ÿ�����̿����ļ���������Ϊ0
		memset(newInode->blockNumLength, 0, sizeof(newInode->blockNumLength));
		newInode->parent =targetInode->ino;//���׽ڵ�Ϊ��ǰ�ļ���
		newInode->blockIndex = 0;//blockNum������±�
		newInode->ino = inodeIndex;//���ǵڼ���inode��
		newInode->length = 0;//�ļ����к���0���ļ��������ļ�����Ϊ0
		if (strcmp(FLAGROOT, "") != 0) {
			strcpy(newInode->owner, FLAGROOT);
			strcpy(newFcb->owner, FLAGROOT);
		}
		else {
			strcpy(newInode->owner, rootname);
			strcpy(newFcb->owner, rootname);
		}
		//ͬ������FCB
		strcpy(newFcb->filename, sourceInode->filename);
		newFcb->isdir = sourceInode->isdir;
		newFcb->ino = inodeIndex;
		int f = 1;
		//�����ǰ���̿�Ϊ0�飬����ǰһ�����̿��Ѿ�ռ��������Ҫ�ٴӿ��еĴ��̿�����һ��
		if (targetInode->blockIndex == 0 || (targetInode->blockNumLength[targetInode->blockIndex - 1] + 1) * fcbSize > BLOCKSIZE) {
			int block_Index = -1;
			for (int i = 0; i < BLOCKNUM; ++i) {
				if (superBlock.blockBitmap[i] == 0) {
					if (superBlock.blockfree == 0) {
						f = 0;
						break;
					}
					superBlock.blockfree--;
					block_Index = i;
					superBlock.blockBitmap[i] = 1;
					break;
				}
			}
			if (f) {
				targetInode->blockNum[targetInode->blockIndex] = block_Index;
				targetInode->blockNumLength[targetInode->blockIndex] = 0;
				targetInode->blockIndex++;
			}
		}
		if (f) {
			fseek(fp, superBlockSize + INODENUM * inodeSize + targetInode->blockNum[targetInode->blockIndex - 1] * BLOCKSIZE + targetInode->blockNumLength[targetInode->blockIndex - 1] * fcbSize, SEEK_SET);
			fwrite(newFcb, fcbSize, 1, fp);
			targetInode->blockNumLength[targetInode->blockIndex - 1]++;
			targetInode->length++;
			fseek(fp, superBlockSize + targetInode->ino * inodeSize, SEEK_SET);
			fwrite(targetInode, inodeSize, 1, fp);
			rewind(fp);
			fwrite(&superBlock, superBlockSize, 1, fp);
			fseek(fp, superBlockSize + newInode->ino * inodeSize, SEEK_SET);
			fwrite(newInode, inodeSize, 1, fp);
		}
		else {
			printf("�ռ䲻��\n");
			FLAG = 0;
		}
		free(newInode);
		free(newFcb);
		
		fflush(fp);
		//����Դ�ļ���Inode��FCB��ȡ�µ�Դ�ļ��е�Inode,���еݹ�
		//���ȱ���Դ�ļ���Inode�����12������
		for (int i = 0; i <INODE_BLOCKNUM; i++)
		{
			//����ÿ�����̿��е�fcb
			for (int j = 1; j <= sourceInode->blockNumLength[i]; j++)
			{
				FcbPtr ptrFcb = (FcbPtr)malloc(fcbSize);
				fseek(fp, superBlockSize + INODENUM * inodeSize + sourceInode->blockNum[i] * BLOCKSIZE + (j-1) * fcbSize, SEEK_SET);
				fread(ptrFcb, fcbSize, 1, fp);
				xcopy(ptrFcb->ino, inodeIndex);
				free(ptrFcb);
			}
			
		}
	}
	else//������ļ�
	{
		int temp_dir = currentDir;
		currentDir = sourceInode->parent;
		cp(sourceInode->filename, target_ino);
		currentDir = temp_dir;
	}
	/*free(fileInode);*/
}
void Import(int targetIno)
{
	FILE* ffp; 
	if ((ffp = fopen(argv[1], "rb")) == NULL)
	{
		printf("Windows·���µ��ļ�������\n");
		return;
	}
	char name[30];
	char *subName;
	subName = strtok(argv[1], "\\");
	while (subName != NULL)
	{
		strcpy(name, subName);
		subName = strtok(NULL, "\\");
	}
	char content[1000008] = "";
	fseek(ffp, 0, SEEK_END);	//��λ���ļ�β��ƫ����Ϊ0
	int cnt = ftell(ffp);	//���ص�ǰ��λ���ļ�λ�ã�Ҳ�����ļ��ܳ��ȣ�
	fseek(ffp, 0, SEEK_SET);
	fread(content, cnt, 1, ffp);
	INodePtr targetInode = (INodePtr)malloc(inodeSize);//Ŀ���ļ��е�inode
	fseek(fp, superBlockSize + targetIno * inodeSize, SEEK_SET);
	fread(targetInode, inodeSize, 1, fp);
	//����Ҫ�ж�Ŀ���ļ����Ƿ��ܷ������fcb
	//�����inode���䵽�����һ�����̿鲻���Է���һ��fcb,�򷵻ص����ļ�Ŀ¼���̿���������
	bool isSpaceEnough = false;
	for (int i = 0; i < INODE_BLOCKNUM; i++)
	{
		if (BLOCKSIZE - targetInode->blockNumLength[i] * fcbSize >= fcbSize)
		{
			isSpaceEnough = true;
			break;
		}
	}
	if (!isSpaceEnough)
	{
		printf("�����ļ�Ŀ¼���̿���������\n");
		return;
	}
	//�жϴ���ʣ��ռ��Ƿ��㹻
	int block_need = cnt / BLOCKSIZE;
	if (superBlock.blockfree < block_need || superBlock.inodefree == 0) {
		printf("���̿ռ䲻��\n");
		cpFlag = 0;
		return;
	}
	//����inode��Ϣ,����1��inode
	int inodeIndex = -1;
	//����λͼ
	for (int i = 0; i < INODENUM; ++i) {
		if (superBlock.inodeBitmap[i] == 0) {
			inodeIndex = i;
			superBlock.inodefree--;
			superBlock.inodeBitmap[i] = 1;
			break;
		}
	}
	//����д���ļ�
	rewind(fp);
	fwrite(&superBlock, superBlockSize, 1, fp);
	content[cnt] = '\0';
	FcbPtr newFcb = (FcbPtr)malloc(fcbSize);
	INodePtr newInode = (INodePtr)malloc(inodeSize);
	strcpy(newInode->filename, name);//�����ļ�/�ļ�����
	newInode->isdir = 0;
	//�ļ���ŵĴ��̿���
	memset(newInode->blockNum, 0, sizeof(newInode->blockNum));
	//�ļ���ŵ�ÿ�����̿����ļ����������ļ���������Ϊ0
	memset(newInode->blockNumLength, 0, sizeof(newInode->blockNumLength));
	newInode->parent = targetIno;//���׽ڵ�Ϊ��ǰ�ļ���
	newInode->blockIndex = 0;//blockNum������±�
	newInode->ino = inodeIndex;//���ǵڼ���inode��
	newInode->length = 0;//�ļ����к���0���ļ��������ļ�����Ϊ0
	if (strcmp(FLAGROOT, "") != 0) {
		strcpy(newInode->owner, FLAGROOT);
		strcpy(newFcb->owner, FLAGROOT);
	}
	else {
		strcpy(newInode->owner, rootname);
		strcpy(newFcb->owner, rootname);
	}
	//ͬ������FCB
	strcpy(newFcb->filename, name);
	newFcb->isdir = 0;
	newFcb->ino = inodeIndex;
	int f = 1;
	//�����ǰ���̿�Ϊ0�飬����ǰһ�����̿��Ѿ�ռ��������Ҫ�ٴӿ��еĴ��̿�����һ��
	if (targetInode->blockIndex == 0 || (targetInode->blockNumLength[targetInode->blockIndex - 1] + 1) * fcbSize > BLOCKSIZE) {
		int block_Index = -1;
		for (int i = 0; i < BLOCKNUM; ++i) {
			if (superBlock.blockBitmap[i] == 0) {
				if (superBlock.blockfree == 0) {
					f = 0;
					break;
				}
				superBlock.blockfree--;
				block_Index = i;
				superBlock.blockBitmap[i] = 1;
				break;
			}
		}
		if (f) {
			targetInode->blockNum[targetInode->blockIndex] = block_Index;
			targetInode->blockNumLength[targetInode->blockIndex] = 0;
			targetInode->blockIndex++;
		}
	}
	if (f) {
		//cout << newFcb->filename << endl;
		fseek(fp, superBlockSize + INODENUM * inodeSize + targetInode->blockNum[targetInode->blockIndex - 1] * BLOCKSIZE + targetInode->blockNumLength[targetInode->blockIndex - 1] * fcbSize, SEEK_SET);
		fwrite(newFcb, fcbSize, 1, fp);
		targetInode->blockNumLength[targetInode->blockIndex - 1]++;
		targetInode->length++;
		fseek(fp, superBlockSize + targetInode->ino * inodeSize, SEEK_SET);
		fwrite(targetInode, inodeSize, 1, fp);
		rewind(fp);
		fwrite(&superBlock, superBlockSize, 1, fp);
		fseek(fp, superBlockSize + newInode->ino * inodeSize, SEEK_SET);
		fwrite(newInode, inodeSize, 1, fp);
	}
	else {
		printf("�ռ䲻��\n");
		FLAG = 0;
	}
	free(targetInode);
	free(newFcb);
	fflush(fp);
	//���´������ļ���д������
	//�ҵ���һ��д���λ��
	int blockIndex = 0;
	int byte_pos = 0;
	if (newInode->blockIndex != 0) blockIndex = newInode->blockIndex - 1;
	byte_pos = newInode->blockNumLength[blockIndex];
	int original_blockindex = blockIndex;
	int original_byte_pos = byte_pos;
	int increaseblocknum = 0;
	//if (blockIndex != 0)fseek(fp, superBlockSize + INODENUM * inodeSize + fileInode->blockNum[blockIndex] * BLOCKSIZE + byte_pos, SEEK_SET);
	for (int i = 0; i < cnt; i++) {
		//д��һ�����̿���߸�inode��㻹û�б�������̿�
		if (blockIndex == 0 || BLOCKSIZE - byte_pos < 2) {
			//��Ϊһ�����������ֽ����Ե����̿�ʣ��1��0ʱ������һ�����̿�
			//cout << byte_pos % BLOCKSIZE << endl;
			//byte_pos = byte_pos % BLOCKSIZE;
			//cout << BLOCKSIZE << endl;
			blockIndex = -1;
			byte_pos = 0;
			for (int j = 0; j < BLOCKNUM; ++j) {
				if (superBlock.blockBitmap[j] == 0) {
					superBlock.blockfree--;
					superBlock.blockBitmap[j] = 1;
					blockIndex = j;
					break;
				}
			}
			if (blockIndex != -1) {
				increaseblocknum++;
				newInode->blockNum[newInode->blockIndex] = blockIndex;
				newInode->blockNumLength[newInode->blockIndex] = 0;
				fseek(fp, superBlockSize + INODENUM * inodeSize + (newInode->blockNum[newInode->blockIndex]) * BLOCKSIZE, SEEK_SET);
				newInode->blockIndex++;
			}
		}
		if (blockIndex != -1 && newInode->blockIndex <= INODE_BLOCKNUM)
		{
			//cout << c << endl;
			fputc(content[i], fp);
			//cout << content[i] << endl;
			byte_pos++;
			newInode->blockNumLength[newInode->blockIndex - 1]++;
		}
	}
	for (int i = original_blockindex; i < newInode->blockIndex; i++)
	{
		if (i == original_blockindex) newInode->length += newInode->blockNumLength[i] - original_blockindex;
		else newInode->length += newInode->blockNumLength[i];
	}
	if (blockIndex == -1) {
		printf("�ռ䲻��,��Щδд�룡\n");
	}
	else if (newInode->blockIndex > INODE_BLOCKNUM)
	{
		printf("�����ļ��ռ�ﵽ����,��Щδд�룡\n");
	}
	else
	{
		printf("�ļ����Ƴɹ���\n");
	}
	rewind(fp);
	fwrite(&superBlock, superBlockSize, 1, fp);
	//����inode
	fseek(fp, superBlockSize + newInode->ino * inodeSize, SEEK_SET);
	fwrite(newInode, inodeSize, 1, fp);
	free(newInode);
	
}
void Export(char*name,int dir_num)
{
	FILE* ffp;
	if (argc == 3)
	{
		//strcat(argv[2], "\\");
		//strcat(argv[2], name);
		ffp = fopen(argv[2], "w+");
	}
	else
	{
		//strcat(argv[3], "\\");
		//strcat(argv[3], name);
		ffp = fopen(argv[3], "w+");
	}
	INodePtr fileInode = (INodePtr)malloc(inodeSize);
	INodePtr parentInode = (INodePtr)malloc(inodeSize);
	int fileInodeNum = findInodeNum(name, 0);
	if (fileInodeNum == -1) {
		printf("%s�ļ�������\n", name);
	}
	else {
		fseek(fp, superBlockSize + fileInodeNum * inodeSize, SEEK_SET);
		fread(fileInode, inodeSize, 1, fp);
		for (int i = 0; i < fileInode->blockIndex; ++i) {
			fseek(fp, superBlockSize + INODENUM * inodeSize + fileInode->blockNum[i] * BLOCKSIZE, SEEK_SET);
			/*for (int j = 0; j < fileInode->blockNumLength[i]; ++j) {
				c = fgetc(fp);
				putchar(c);
			}*/
			char content[BLOCKSIZE];
			fread(content, fileInode->blockNumLength[i], 1, fp);
			content[fileInode->blockNumLength[i]] = '\0';
			fwrite(content, fileInode->blockNumLength[i], 1,ffp);
			fflush(ffp);
		}
		//cout << endl;
	}
	free(fileInode);
	free(parentInode);
	fclose(ffp);
}
void order()
{
	char cmd[MAX_NAME];
	while (true)
	{
		FLAG = 1;
		setPath();
		cin.getline(cmd, 1024);//��ȡ���������
		//���������ǻ��з�����ִ�к�������
		if (strcmp(cmd, "\n")==0)
		{
			continue;
		}
		for (int i = 0; i < 5; ++i) {
			argv[i] = (char*)malloc(1024);
			argv[i][0] = '\0';
		}
		switch (splitOrder(cmd)) {
			case 0://cd
			{
				cd(argv[1]);
				break;
			}
			case 1://dir//
			{
				int ls_number;
				ls_number = getDirNumber(argv[1]);
				//printf("%d\n",ls_number);
				if (ls_number == -1)
				{
					cout << "�ļ�Ŀ¼�����ڣ�" << endl;
				}
				else
				{
					ls(ls_number);
				}
				break;
			}
			case 2://treedir
			{
				int tree_number;
				tree_number = getDirNumber(argv[1]);
				if (tree_number == -1)
				{
					cout << "�ļ�Ŀ¼�����ڣ�" << endl;
				}
				else
				{
					treedir(tree_number, 0);
				}
				break;
			}
			case 3://deldir
			{
				int dir_number;
				dir_number = getDirNumber(argv[1]);
				if (dir_number == -1)
				{
					cout << "�ļ�Ŀ¼�����ڣ�" << endl;
				}
				else
				{
					DelBlankDir(dir_number);
				}
				break;
			}
			case 4://mkdir
			{
				mkdir(argv[1]);
				break;
			}	
			case 5://create 
			{
				//create
				int dir_number2;
				if (argc < 2 || argc>3)
				{
					printf("���������������\n");
				}
				else
				{
					if (argc == 2) dir_number2 = currentDir;
					else dir_number2 = getDirNumber(argv[1]);
					if (dir_number2 == -1)
					{
						cout << "�ļ�Ŀ¼�����ڣ�" << endl;
					}
					else
					{
						int tempDir = currentDir;
						currentDir = dir_number2;
						if (argc == 3) createFile(argv[2], 0);
						else createFile(argv[1], 0);//��ָ��·�������ڵ�ǰ�ļ����´����ļ�
						currentDir = tempDir;
					}
				}
				break;
			}
			case 6://open
			{
				int fileDir_number;
				if (argc < 2 || argc>3)
				{
					printf("���������������\n");
				}
				else
				{
					if (argc == 2) fileDir_number = currentDir;
					else fileDir_number = getDirNumber(argv[1]);
					if (fileDir_number == -1)
					{
						cout << "�ļ�Ŀ¼�����ڣ�" << endl;
					}
					else
					{
						INodePtr filenode = (INodePtr)malloc(inodeSize);
						fseek(fp, superBlockSize + fileDir_number * inodeSize, SEEK_SET);
						fread(filenode, inodeSize, 1, fp);
						int tempDir = currentDir;
						currentDir = fileDir_number;
						if (argc == 3)open(argv[2]);
						else open(argv[1]);//��ָ��·�������ڵ�ǰ�ļ����´��ļ�
						currentDir = tempDir;
						free(filenode);
					}
				}
				break;
			}
			case 7://close
			{
				int fileDir_number2;
				if (argc < 2 || argc>3)
				{
					printf("���������������\n");
				}
				else
				{
					if (argc == 2) fileDir_number2 = currentDir;
					else fileDir_number2 = getDirNumber(argv[1]);
					if (fileDir_number2 == -1)
					{
						cout << "�ļ�Ŀ¼�����ڣ�" << endl;
					}
					else
					{
						INodePtr filenode = (INodePtr)malloc(inodeSize);
						fseek(fp, superBlockSize + fileDir_number2 * inodeSize, SEEK_SET);
						fread(filenode, inodeSize, 1, fp);
						int tempDir = currentDir;
						currentDir = fileDir_number2;
						if (argc == 3)close(argv[2], fileDir_number2);
						else close(argv[1], fileDir_number2);//��ָ��·�������ڵ�ǰ�ļ����¹ر��ļ�
						currentDir = tempDir;
						free(filenode);
					}
				}
				break;
			}
			case 8://help
			{
				help();
				break;
			}
			case 9://showpath
			{
				showOpenFilePath();
				break;
			}
			case 10://write
			{
				int fileDir_number3;
				if (argc < 2 || argc>3)
				{
					printf("���������������\n");
				}
				else
				{
					if (argc == 2) fileDir_number3 = currentDir;
					else fileDir_number3 = getDirNumber(argv[1]);
					if (fileDir_number3 == -1)
					{
						cout << "�ļ�Ŀ¼�����ڣ�" << endl;
					}
					else
					{
						INodePtr filenode = (INodePtr)malloc(inodeSize);
						fseek(fp, superBlockSize + fileDir_number3 * inodeSize, SEEK_SET);
						fread(filenode, inodeSize, 1, fp);
						int tempDir = currentDir;
						currentDir = fileDir_number3;
						if (argc == 3)write(argv[2], fileDir_number3);
						else write(argv[1], fileDir_number3);//��ָ��·�������ڵ�ǰ�ļ�����д�ļ�
						currentDir = tempDir;
						free(filenode);
					}
				}
				break;
			}
			case 11://read
			{
				int fileDir_number4;
				if (argc < 2 || argc>3)
				{
					printf("���������������\n");
				}
				else
				{
					if (argc == 2) fileDir_number4 = currentDir;
					else fileDir_number4 = getDirNumber(argv[1]);
					if (fileDir_number4 == -1)
					{
						cout << "�ļ�Ŀ¼�����ڣ�" << endl;
					}
					else
					{
						INodePtr filenode = (INodePtr)malloc(inodeSize);
						fseek(fp, superBlockSize + fileDir_number4 * inodeSize, SEEK_SET);
						fread(filenode, inodeSize, 1, fp);
						int tempDir = currentDir;
						currentDir = fileDir_number4;
						if (argc == 3)read(argv[2], fileDir_number4);
						else read(argv[1], fileDir_number4);//��ָ��·�������ڵ�ǰ�ļ����¶��ļ�
						currentDir = tempDir;
						free(filenode);
					}
				}
				break;
			}
			case 12://copy
			{
				int fileDir_number5,targetDir;
				if (argc < 3 || argc>4)
				{
					printf("���������������\n");
				}
				else
				{
					if (argc == 3)
					{
						fileDir_number5 = currentDir;
						targetDir = getDirNumber(argv[2]);
					}
					else
					{
						fileDir_number5 = getDirNumber(argv[1]);
						targetDir = getDirNumber(argv[3]);
					}
					if (fileDir_number5 == -1)
					{
						cout << "Դ�ļ�Ŀ¼�����ڣ�" << endl;
					}
					else if(targetDir==-1)
					{
						cout << "Ŀ���ļ�Ŀ¼�����ڣ�" << endl;
					}
					else
					{
						INodePtr filenode = (INodePtr)malloc(inodeSize);
						fseek(fp, superBlockSize + fileDir_number5 * inodeSize, SEEK_SET);
						fread(filenode, inodeSize, 1, fp);
						int tempDir = currentDir;
						currentDir = fileDir_number5;
						if (argc == 4)cp(argv[2],targetDir);
						else cp(argv[1],targetDir);//��ָ��·�������ڵ�ǰ�ļ����¶��ļ�
						currentDir = tempDir;
						free(filenode);
					}
				}
				break;
			}
			case 13://exit
			{
				exit(0);
				break;
			}
			case 14://import windows·�� �������·��
			{
				int fileDir_number7;
				if (argc < 2 || argc>3)
				{
					printf("���������������\n");
				}
				else
				{
					
					if (argc == 2) fileDir_number7 = currentDir;
					else fileDir_number7 = getDirNumber(argv[2]);
					if (fileDir_number7 == -1)
					{
						cout << "�ļ�Ŀ¼�����ڣ�" << endl;
					}
					else
					{
						INodePtr filenode = (INodePtr)malloc(inodeSize);
						fseek(fp, superBlockSize + fileDir_number7 * inodeSize, SEEK_SET);
						fread(filenode, inodeSize, 1, fp);
						int tempDir = currentDir;
						currentDir = fileDir_number7;
						Import(fileDir_number7);
						currentDir = tempDir;
						free(filenode);
					}
					
				}
				
				break;
			}
			case 15:
				int fileDir_number8;
				if (argc < 3 || argc>4)
				{
					printf("���������������\n");
				}
				else
				{

					if (argc == 3) fileDir_number8 = currentDir;
					else fileDir_number8 = getDirNumber(argv[1]);
					if (fileDir_number8 == -1)
					{
						cout << "�ļ�Ŀ¼�����ڣ�" << endl;
					}
					else
					{
						INodePtr filenode = (INodePtr)malloc(inodeSize);
						fseek(fp, superBlockSize + fileDir_number8 * inodeSize, SEEK_SET);
						fread(filenode, inodeSize, 1, fp);
						int tempDir = currentDir;
						currentDir = fileDir_number8;
						if(argc==3) Export(argv[1],fileDir_number8);
						else Export(argv[2], fileDir_number8);
						currentDir = tempDir;
						free(filenode);
					}

				}

				break;
			case 16://xcopy
			{
				int fileDir_number6, targetDir;
				if (argc < 2 || argc>3)
				{
					printf("���������������\n");
				}
				else
				{
					if (argc == 2)
					{
						fileDir_number6 = currentDir;
						targetDir = getDirNumber(argv[1]);
					}
					else
					{
						fileDir_number6 = getDirNumber(argv[1]);
						targetDir = getDirNumber(argv[2]);
					}
					if (fileDir_number6 == -1)
					{
						cout << "Դ�ļ�Ŀ¼�����ڣ�" << endl;
					}
					else if (targetDir == -1)
					{
						cout << "Ŀ���ļ�Ŀ¼�����ڣ�" << endl;
					}
					else
					{
						if (DoesithaveOpenfile(fileDir_number6))
						{
							printf("Դ�ļ��������ļ��Ѿ����򿪣����ȹر��ļ���\n");
						}
						else
						{
							INodePtr filenode = (INodePtr)malloc(inodeSize);
							fseek(fp, superBlockSize + fileDir_number6 * inodeSize, SEEK_SET);
							fread(filenode, inodeSize, 1, fp);
							INodePtr targetnode= (INodePtr)malloc(inodeSize);
							fseek(fp, superBlockSize + targetDir * inodeSize, SEEK_SET);
							fread(targetnode, inodeSize, 1, fp);
							if (judgeParent(filenode->ino,targetnode->ino))
							{
								printf("��ֹ�����ļ��з������ļ�����\n");
							}
							else
							{
								int tempDir = currentDir;
								currentDir = fileDir_number6;
								xcopy(fileDir_number6, targetDir);
								currentDir = tempDir;
								
							}
							free(filenode);
							free(targetnode);
						}
					}
				}
				break;
			}
			case 17:
				system("cls");
				break;
			default:
				printf("�������\n");
		}
	}
}
void createFileSystem()
{
	INodePtr ptrInode;
	if ((fp = fopen(FILENAME, "wb+")) == NULL)
	{
		printf("createFileSystem���ļ�����!\n");
		exit(1);
	}
	//��ʼ�����д��̿�
	for (int i = 0; i < BLOCKNUM; i++)
	{
		superBlock.blockBitmap[i] = 0;
	}
	//��ʼ������inode
	for (int i = 0; i < INODENUM; i++)
	{
		superBlock.inodeBitmap[i] = 0;
	}
	//����ռλ��
	for (int i = 0; i < (superBlockSize + inodeSize * INODENUM + BLOCKSIZE * BLOCKNUM + userSize * USERS); i++)
	{
		fputc(0, fp);
	}
	rewind(fp);
	superBlock.blockfree = BLOCKNUM;//��ǰ���д��̿������
	superBlock.inodefree = INODENUM-1;//��ǰ���нڵ�����
	superBlock.blocksize = BLOCKSIZE;
	superBlock.inodenum = INODENUM;
	superBlock.blocknum = BLOCKNUM;
	//�������ڵ㣬0��inode�Ǹ��ڵ�
	ptrInode = (INodePtr)malloc(inodeSize);
	ptrInode->ino = 0;//�ڵ��
	strcpy(ptrInode->filename, "/");//�ļ���
	ptrInode->isdir = 1;//�ļ��������ļ���
	ptrInode->parent = 0;//���ļ������Լ�
	ptrInode->length = 0;//�ļ�������0
	memset(ptrInode->blockNum, 0, sizeof(ptrInode->blockNum));//��ʼ����������ļ��Ĵ��̿�
	memset(ptrInode->blockNumLength, 0, sizeof(ptrInode->blockNumLength));//��ʼ��ÿ�����̿���ռ�õĳ���


	//root�ļ���ռ�õ�һ��inode�͵�һ�����̿�
	//superBlock.blockBitmap[0] = 1;
	superBlock.inodeBitmap[0] = 1;
	ptrInode->blockIndex = 0;

	//��������д���ļ�disk.img
	//����д�볬������Ϣ
	rewind(fp);
	fwrite(&superBlock, superBlockSize, 1, fp);
	//����inode��Ϣ
	fseek(fp, superBlockSize, SEEK_SET);
	fwrite(ptrInode, inodeSize, 1, fp);


	//��ǰ�ļ���inode��
	currentDir = 0;
	//��ǰ·��
	strcpy(path, "/");
	//�û�������1
	superBlock.length = 1;

	//��ʼ��root�˻���������
	User user;
	strcpy(user.username, "root");
	strcpy(user.password, "root");

	//���û���Ϣд�����
	fseek(fp, Offset, SEEK_SET);
	fwrite(&user, sizeof(User), 1, fp);
	rewind(fp);

	//�����º�ĳ����飬д�����
	fwrite(&superBlock, superBlockSize, 1, fp);
	fflush(fp);
}