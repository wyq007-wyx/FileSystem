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
char FILENAME[30];			//当前文件名
char rootname[30];			//当前用户的用户名
char rootpath[30];			//当前用户的主目录
char FLAGROOT[30];
#define INODE_BLOCKNUM 12 //每个文件最多占据的磁盘块大小
#define BLOCKSIZE 1024	  //每个磁盘块大小为1024字节
#define BLOCKNUM 65536	  //磁盘块的数量为65536个
#define INODENUM 1024	  //inode一共有1024个
#define MAX_NAME 30		  //name长度最大为30个字符
#define USERS 100		  //最多有100个用户
#define MAX_OPENFILE 5      //最多同时打开5个文件
//超级块 存储基本信息
typedef struct SuperBlock {
	int blocksize;
	int blocknum;
	int inodenum;
	int blockfree;               //剩余block数
	int inodefree;               //剩余inode数
	int blockBitmap[BLOCKNUM];  //block数组
	int inodeBitmap[INODENUM];  //inode数组
	int length;                  //用户数量
}SuperBlock;   

SuperBlock superBlock;          //文件系统块信息

typedef struct INode {
	char filename[10];//文件名
	int ino;//inode编号,与FCB建立关系
	int isdir;//是否为文件目录
	int parent;//该inode的父目录
	int length;//文件则表示其大小，目录则表示其文件数量
	int blockNum[INODE_BLOCKNUM];//INODE的block编号
	int blockIndex;//blockNum数组的下标
	int blockNumLength[INODE_BLOCKNUM];//每个block已经用掉的空间，对于文件(非目录) 表示的是写入的字节数,对于文件目录来说记录的是写入的fcb条数
	char owner[30];//文件所属用户		
}INode,*INodePtr;


typedef struct FCB {
	int ino;//文件编号
	char filename[10];//文件名
	int isdir;//0为文件，1为文件目录
	char owner[30];//文件所属用户
}FCB,*FcbPtr;

//用户信息 储存在虚拟磁盘最后面
typedef struct User {
	char username[30];
	char password[30];
}User, *UserPtr;

int currentDir;//当前记录目录的inode号

FILE *fp;//当前用户的文件指针
char openFile[MAX_OPENFILE][100];//当前处于open状态的文件名
int openDir[MAX_OPENFILE];//当前处于open状态的文件所在目录的inode号
char openPath[MAX_OPENFILE][100] = { "","","","",""}; //当前处于open状态的文件的绝对路径
const int superBlockSize = sizeof(superBlock);
const int inodeSize = sizeof(INode);
const int fcbSize = sizeof(FCB);
const int userSize = sizeof(User);
const int Offset = superBlockSize + INODENUM * inodeSize + BLOCKNUM * BLOCKSIZE;
char *argv[10];                 //指针数组
int argc;                        //记录指令数
char path[100] = "";             //记录当前目录
int FLAG = 0;                    //标记命令
int cdFlag = 1;                  //cd命令的执行情况
int openfilenum = 0;
char content_temp[65536];
//						  0      1		2			3			4		5		6		7		8		9				10		11		12		13		14	  15		16			17		
const char* syscmd[] = { "cd", "dir", "treedir", "deldir", "mkdir", "create", "open", "close", "help", "showpath", "write", "read", "copy", "exit", "import", "export", "xcopydir", "clear"};
//查找当前目录下文件名为name的文件的inode编号
//flag=0 表示为文件 flag=1 表示为目录
//返回值为inode编号
int findInodeNum(char *name, int flag) {
	int fileInodeNum = -1;
	INodePtr parentInode = (INodePtr)malloc(inodeSize);
	FcbPtr newfcb = (FcbPtr)malloc(fcbSize);
	//设置文件内部指针指向当前文件所对应inode所在位置
	fseek(fp, superBlockSize + currentDir * inodeSize, SEEK_SET);
	//读取数据到设置的parentInode节点
	fread(parentInode, inodeSize, 1, fp);
	//获取该文件或者文件夹所占用的磁盘块
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
//实现目录的切换
void cd(char* name)
{
	if (argc > 2)
	{
		printf("命令参数过多,请检查\n");
		cdFlag = 0;
		return;
	}
	//int fileInodeNum;
	INodePtr fileInode = (INodePtr)malloc(inodeSize);
	if (strcmp(name, "..") == 0) 
	{
		//跳转到当前文件夹的上一级 cd ..
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
		//返回根目录 cd /
		fseek(fp, superBlockSize, SEEK_SET);
		fread(fileInode, inodeSize, 1, fp);
		currentDir = fileInode->parent;
		strcpy(path, "/");
	}
	else if (name[0] == '/') { 
		char* subName;
		//strtok函数用于分解字符串为一组字符串
		//返回值
		//该函数返回被分解的第一个子字符串，如果没有可检索的字符串，则返回一个空指针。
		subName = strtok(name, "/");
		int f = 1;
		char temp[100] = "";
		strcpy(temp, path);//先把当前路径放到temp里暂时存储
		int tempInodenum = currentDir;//记录下当前INode编号
		fseek(fp, superBlockSize, SEEK_SET);//跳过文件系统块
		fread(fileInode, inodeSize, 1, fp);//找到第一个INode对应的文件信息
		currentDir = fileInode->parent;//设置当前文件夹为根目录
		strcpy(path, "/");
		while (subName != NULL) {
			if (strcmp(subName, "") != 0) {
				int fileInodeNum = findInodeNum(subName, 1);//找到子文件夹对应INode编号
				if (fileInodeNum == -1) {
					printf("目录不存在!\n");
					cdFlag = 0;
					f = 0;
					break;
				}
				else {
					currentDir = fileInodeNum;//将当前文件夹编号更新
					if (strlen(path) > 1)strcat(path, "/");//更新路径
					strcat(path, subName);
				}
			}
			subName = strtok(NULL, "/");
		}
		if (!f) {
			//如果没找到 还原最初的当前文件夹、路径
			currentDir = tempInodenum;
			strcpy(path, temp);
		}
	}
	else if (strcmp(name, ".") != 0) {
		//跳转到当前文件夹的子文件夹 cd subName/....
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
					printf("目录不存在!\n");
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
//创建文件或文件目录
//flag=1 创建文件夹 flag=0创建文件
int creatFlag = 1;
void createFile(char* name, int flag)
{
	FLAG = 1;
	if (flag==1&&argc > 2) {
		printf("参数过多\n");
		FLAG = 0;
		return;
	}
	if (flag == 0 && argc > 3) {
		printf("参数过多\n");
		FLAG = 0;
		return;
	}
	if (strcmp(name, "") == 0) {
		printf("命令错误\n"); 
		FLAG = 0;
		return;
	}
	//rootpath当前用户的主目录 path当前位置
	if (strlen(rootpath) > strlen(path)) {
		FLAG = 0;
	}
	//必须在当前用户目录下创建文件
	for (int i = 0; i < strlen(rootpath); ++i) {
		if (rootpath[i] != path[i]) {
			FLAG = 0;
			break;
		}
	}
	if (!FLAG) {
		printf("无权限\n");
		return;
	}
	creatFlag = 1;//表示可以创建
	INodePtr fileInode = (INodePtr)malloc(inodeSize);
	fseek(fp, superBlockSize + currentDir * inodeSize, SEEK_SET);
	//读出当前的文件夹的inode
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
		printf("已存在该文件或目录\n"); 
		FLAG = 0;
		free(ptrFcb);
		free(fileInode);
		return;
	}
	if (superBlock.inodefree == 0) {
		printf("空间不足\n"); 
		FLAG = 0;
		free(ptrFcb);
		free(fileInode);
		return;
	}
	bool isSpaceEnough = false;
	//如果该inode分配到的最后一个磁盘块不足以放下一条fcb,则返回单个文件目录磁盘块容量不足
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
		printf("单个文件目录磁盘块容量不足\n");
		return;
	}
	int inodeIndex = -1;
	//更新位图
	for (int i = 0; i < INODENUM; ++i) {
		if (superBlock.inodeBitmap[i] == 0) {
			inodeIndex = i;
			superBlock.inodefree--;
			superBlock.inodeBitmap[i] = 1;
			break;
		}
	}
	//重新写回文件
	rewind(fp);
	fwrite(&superBlock, superBlockSize, 1, fp);

	FcbPtr newFcb = (FcbPtr)malloc(fcbSize);
	INodePtr newInode = (INodePtr)malloc(inodeSize);
	strcpy(newInode->filename, name);//设置文件/文件夹名
	newInode->isdir = flag;//设置文件或文件夹属性
	//文件存放的磁盘块编号
	memset(newInode->blockNum, 0, sizeof(newInode->blockNum));
	//文件存放的每个磁盘块中文件个数设置为0
	memset(newInode->blockNumLength, 0, sizeof(newInode->blockNumLength));
	newInode->parent = currentDir;//父亲节点为当前文件夹
	newInode->blockIndex = 0;//blockNum数组的下标
	newInode->ino = inodeIndex;//这是第几个inode项
	newInode->length = 0;//文件夹中含有0个文件，或者文件长度为0
	if (strcmp(FLAGROOT, "") != 0) {
		strcpy(newInode->owner, FLAGROOT);
		strcpy(newFcb->owner, FLAGROOT);
	}
	else {
		strcpy(newInode->owner, rootname);
		strcpy(newFcb->owner, rootname);
	}
	//同步更新FCB
	strcpy(newFcb->filename, name);
	newFcb->isdir = flag;
	newFcb->ino = inodeIndex;
	f = 1;
	//如果当前磁盘块为0块，或者前一个磁盘块已经占满，则需要再从空闲的磁盘块中找一个
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
		printf("空间不足\n"); 
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
	//创建文件名为name的文件夹
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
//用户登录
int user_register(char* name, char* password)
{
	rewind(fp);
	fread(&superBlock, superBlockSize, 1, fp);
	User user;
	int user_number = superBlock.length;//获取用户数量
	fseek(fp, Offset, SEEK_SET);//用户信息在磁盘的位置
	for (int i = 1; i <= user_number; i++)
	{
		fread(&user, sizeof(User), 1, fp);
		//用户名重复
		if (strcmp(user.username, name) == 0)
		{
			return 0;
		}
	}
	//更新磁盘用户数据
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
	int cnt = superBlock.length;//superBlock.length为用户数量
	fseek(fp, Offset, SEEK_SET);//offset为偏移量
	for (int i = 0; i < cnt; i++)
	{
		fread(&user, sizeof(User), 1, fp);//每次读出一个用户数据
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
		strcpy(openFile[i], "");//openFile当前处于open状态的文件名
		openDir[i] = -1;//当前处于open状态的文件所在目录的inode号
	}
	if ((fp = fopen(FILENAME, "rb+")) == NULL) {
		//rb+：读写打开一个二进制文件，只允许读写数据。
		printf("打开错误\n");
		exit(1);
	}
	rewind(fp);//使文件重新回到开始的位置
	fread(&superBlock, superBlockSize, 1, fp);//读出文件系统块信息
	strcpy(path, "/");//path是当前目录
	currentDir = 0;//当前文件夹对应的inode
	if (strcmp(rootname, "root") != 0) {//如果当前的用户名不是root
		strcpy(FLAGROOT, rootname);//把当前用户名赋给FLAGROOT
		char temp[100];
		strcpy(temp, rootname);//把当前用户名赋给temp
		strcpy(rootname, "root");//把当前用户名设置为root
		if (findInodeNum(temp, 1) == -1) {//如果找不到用户名为temp的文件夹
			mkdir(temp);//生成一个用户名为temp的文件夹
		}
		cd(temp);//跳转到用户名为temp的文件夹
		strcpy(rootname, temp);//重新设置当前用户的用户名rootname为temp
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

	int length = 0;//cmd长度
	char *ptr_temp;

	argc = 0;//参数个数
	//以空格分割cmd命令
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
		printf("命令参数过多，请检查\n");
		return;
	}
	int ls_dir = ls_number;
	printf("<TYPE>%16s%16s%16s%16s\n", "NAME", "INODENUMBER", "OWNER", "LENGTH");
	int dirCount = 0;
	int fileCount = 0;
	int CurTotalFileSize = 0;
	FcbPtr ptrFcb = (FcbPtr)malloc(fcbSize);
	INodePtr parentInode = (INodePtr)malloc(inodeSize);     //指向当前目录
	//parentInode 指向当前目录的inode
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
	printf("\n%d个文件\n", fileCount);
	printf("%d个目录\n", dirCount);
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
		//说明指令是 dir (空)，则应该展示当前目录下的所有文件信息
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
	cout << "\t\tcd [给定路径]                                   切换给定路径为当前目录" << endl;
	cout << "\t\tdir ([给定文件路径])                            列给定路径的文件目录" << endl;
	cout << "\t\ttreedir ([给定文件路径])                        循环列出给定路径下的子子孙孙目录和文件形式，并以树形显示" << endl;
	cout << "\t\tmkdir [给定名字]                                在当前目录下创建给定名字的目录" << endl;
	cout << "\t\tdeldir [给定文件路径]                           删除给定路径的空目录" << endl;
	cout << "\t\tcreate ([给定文件路径]) [给定文件名]            创建指定路径下给定文件名的文件" << endl;
	cout << "\t\tshowpath                                        显示当前打开的文件路径"   << endl;
	cout << "\t\topen ([给定文件路径]) [给定文件名]              打开指定路径下给定文件名的文件" << endl;
	cout << "\t\tclose ([给定文件路径]) [给定文件名]             关闭指定路径下给定文件名的文件" << endl;
	cout << "\t\tread ([给定文件路径]) [给定文件名]              读取指定路径下给定文件名的文件" << endl;
	cout << "\t\twrite ([给定文件路径]) [给定文件名]             写指定路径下给定文件名的文件" << endl;
	cout << "\t\tcopy ([源文件路径]) [给定文件名] [目标文件路径] 复制源文件路径下给定文件名的文件到目标文件路径"<< endl;
	cout << "\t\txcopydir [给定文件目录路径] [给定路径]          给定某个目录名，将它连同其子子孙孙复制到给定的路径下" << endl;
	cout << "\t\texport  ([源文件路径]) [文件名] [给定路径]      将该系统给定目录下的给定文件导入到Windows下的给定路径下" << endl;
	cout << "\t\timport  [给定文件] [给定路径]					 将Windows下的文件导入到该系统给定路径下" << endl;
	cout << "\t\tclear                                           清空屏幕" << endl;
	cout << "\t\texit                                            退出文件系统" << endl;
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
	//rootpath当前用户的主目录 path当前位置
	if (strlen(rootpath) > strlen(path)) {
		FLAG = 0;
	}
	//必须在当前用户目录下创建文件
	for (int i = 0; i < strlen(rootpath); ++i) {
		if (rootpath[i] != path[i]) {
			FLAG = 0;
			break;
		}
	}
	if (!FLAG) {
		printf("无权限\n");
		return;
	}
	if (dir_number == 0)
	{
		printf("根目录无法被删除\n");
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
		parent_node->length--;//文件数减1
		parent_node->blockNumLength[index]--;//fcb数量-1
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
		printf("目录不为空,无法删除\n");
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
		printf("文件不存在\n");
		return;
	}
	if (fileInodeNum == -2) {
		printf("无权限\n");
		return;
	}
	for (int i = 0; i < MAX_OPENFILE; i++)
	{
		if (strcmp(openFile[i], name) == 0 && openDir[i] == currentDir) {
			printf("已经打开了该文件\n");
			return;
		}
	}
	/*if (strcmp(openFile, "") != 0) {
		printf("有文件已经被打开，请先关闭已经打开的文件\n");
		return;
	}*/
	if (openfilenum >= MAX_OPENFILE)
	{
		printf("打开失败！文件打开数量达到上限！\n");
		return;
	}
	openfilenum++;
	cout << "文件打开成功！" << endl;
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
		if(argv[1][0]=='/')strcpy(openPath[index], argv[1]);//绝对路径
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
	cout <<"文件路径:"<< openPath[index] << endl;
}
void close(char*name,int dirnum)
{
	bool isitopened = false;
	int index = -1;
	//判断当前文件是否打开
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
		printf("该文件没有被打开\n");
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
		printf("请先open文件\n");
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
		printf("%s文件不存在\n", name);
	else {
		//找到文件inode

		fseek(fp, superBlockSize + fileInodeNum * inodeSize, SEEK_SET);

		fread(fileInode, inodeSize, 1, fp);

		//找到上一次写入的位置
		int blockIndex =0;
		int byte_pos = 0;
		if (fileInode->blockIndex != 0) blockIndex = fileInode->blockIndex - 1;
		byte_pos = fileInode->blockNumLength[blockIndex];
		int original_blockindex = blockIndex;
		int original_byte_pos = byte_pos;
		if(blockIndex!=0)fseek(fp, superBlockSize+INODENUM * inodeSize + fileInode->blockNum[blockIndex] * BLOCKSIZE+byte_pos, SEEK_SET);
		printf("请输入文件内容(以$结尾):\n");
		//int i = 0;//本次写入的字符数量
		int increaseblocknum = 0;
		//bool addnewblock = false;
		while ((c = getchar()) != '$') {
			//写满一个磁盘块或者该inode结点还没有被分配磁盘块
			if (blockIndex==0||BLOCKSIZE-byte_pos<2) {
				//因为一个汉字两个字节所以当磁盘块剩余1或0时将开辟一个新盘块
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
			printf("空间不足,有些未写入！\n");
		}
		else if (fileInode->blockIndex > INODE_BLOCKNUM)
		{
			printf("单个文件空间达到上限,有些未写入！\n");
		}
		else
		{
			printf("文件写入成功！\n");
		}
		rewind(fp);
		fwrite(&superBlock, superBlockSize, 1, fp);
		//更新inode
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
		printf("请先open文件\n");
		return;
	}
	//char c;
	INodePtr fileInode = (INodePtr)malloc(inodeSize);
	INodePtr parentInode = (INodePtr)malloc(inodeSize);
	int fileInodeNum = findInodeNum(name, 0);
	if (fileInodeNum == -1) {
		printf("%s文件不存在\n", name);
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
	cout << "当前打开的文件数量为" << openfilenum << endl;
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
			printf("请先close文件\n");
			cpFlag = 0;
			return;
		}
	}
	char content[1000008] = "";
	int fileInodeNum = findInodeNum(name, 0);
	if (fileInodeNum == -1) {
		cpFlag = 0;
		printf("找不到%s文件\n", name);
		return;
	}
	int cnt = 0;
	//源文件的文件夹inode
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
	INodePtr targetInode = (INodePtr)malloc(inodeSize);//目标文件夹的inode
	fseek(fp, superBlockSize + targetIno * inodeSize, SEEK_SET);
	fread(targetInode, inodeSize, 1, fp);
	//首先要判断目标文件夹是否能放下这个fcb
	//如果该inode分配到的最后一个磁盘块不足以放下一条fcb,则返回单个文件目录磁盘块容量不足
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
		printf("单个文件目录磁盘块容量不足\n");
		return;
	}
	//判断磁盘剩余空间是否足够
	int block_need = cnt / BLOCKSIZE;
	if (superBlock.blockfree < block_need||superBlock.inodefree==0) {
		printf("磁盘空间不足\n");
		cpFlag = 0;
		return;
	}
	//更新inode信息,增加1个inode
	int inodeIndex = -1;
	//更新位图
	for (int i = 0; i < INODENUM; ++i) {
		if (superBlock.inodeBitmap[i] == 0) {
			inodeIndex = i;
			superBlock.inodefree--;
			superBlock.inodeBitmap[i] = 1;
			break;
		}
	}
	//重新写回文件
	rewind(fp);
	fwrite(&superBlock, superBlockSize, 1, fp);
	content[cnt] = '\0';
	FcbPtr newFcb = (FcbPtr)malloc(fcbSize);
	INodePtr newInode = (INodePtr)malloc(inodeSize);
	strcpy(newInode->filename, name);//设置文件/文件夹名
	newInode->isdir = 0;
	//文件存放的磁盘块编号
	memset(newInode->blockNum, 0, sizeof(newInode->blockNum));
	//文件存放的每个磁盘块中文件个数或者文件长度设置为0
	memset(newInode->blockNumLength, 0, sizeof(newInode->blockNumLength));
	newInode->parent = targetIno;//父亲节点为当前文件夹
	newInode->blockIndex = 0;//blockNum数组的下标
	newInode->ino = inodeIndex;//这是第几个inode项
	newInode->length = 0;//文件夹中含有0个文件，或者文件长度为0
	if (strcmp(FLAGROOT, "") != 0) {
		strcpy(newInode->owner, FLAGROOT);
		strcpy(newFcb->owner, FLAGROOT);
	}
	else {
		strcpy(newInode->owner, rootname);
		strcpy(newFcb->owner, rootname);
	}
	//同步更新FCB
	strcpy(newFcb->filename, name);
	newFcb->isdir = 0;
	newFcb->ino = inodeIndex;
	int f = 1;
	//如果当前磁盘块为0块，或者前一个磁盘块已经占满，则需要再从空闲的磁盘块中找一个
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
		printf("空间不足\n");
		FLAG = 0;
	}
	free(targetInode);
	free(newFcb);
	free(fileInode);
	fflush(fp);
	//向新创建的文件中写入内容
	//找到上一次写入的位置
	int blockIndex = 0;
	int byte_pos = 0;
	if (newInode->blockIndex != 0) blockIndex = newInode->blockIndex - 1;
	byte_pos = newInode->blockNumLength[blockIndex];
	int original_blockindex = blockIndex;
	int original_byte_pos = byte_pos;
	int increaseblocknum = 0;
	//if (blockIndex != 0)fseek(fp, superBlockSize + INODENUM * inodeSize + fileInode->blockNum[blockIndex] * BLOCKSIZE + byte_pos, SEEK_SET);
	for (int i = 0; i < cnt;i++) {
		//写满一个磁盘块或者该inode结点还没有被分配磁盘块
		if (blockIndex == 0 || BLOCKSIZE - byte_pos < 2) {
			//因为一个汉字两个字节所以当磁盘块剩余1或0时将开辟一个新盘块
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
		printf("空间不足,有些未写入！\n");
	}
	else if (newInode->blockIndex > INODE_BLOCKNUM)
	{
		printf("单个文件空间达到上限,有些未写入！\n");
	}
	else
	{
		printf("文件复制成功！\n");
	}
	rewind(fp);
	fwrite(&superBlock, superBlockSize, 1, fp);
	//更新inode
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
	//rootpath当前用户的主目录 path当前位置
	if (strlen(rootpath) > strlen(path)) {
		FLAG = 0;
	}
	//必须在当前用户目录下创建文件
	for (int i = 0; i < strlen(rootpath); ++i) {
		if (rootpath[i] != path[i]) {
			FLAG = 0;
			break;
		}
	}
	if (!FLAG) {
		printf("无权限\n");
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
		//首先在目标文件夹下创建文件，目标文件指针下移
		//更新inode信息,增加1个inode
		int inodeIndex = -1;
		//更新位图
		for (int i = 0; i < INODENUM; ++i) {
			if (superBlock.inodeBitmap[i] == 0) {
				inodeIndex = i;
				superBlock.inodefree--;
				superBlock.inodeBitmap[i] = 1;
				break;
			}
		}
		//重新写回文件
		rewind(fp);
		fwrite(&superBlock, superBlockSize, 1, fp);
		FcbPtr newFcb = (FcbPtr)malloc(fcbSize);
		INodePtr newInode = (INodePtr)malloc(inodeSize);
		strcpy(newInode->filename, sourceInode->filename);//设置文件/文件夹名
		newInode->isdir = sourceInode->isdir;//设置文件或文件夹属性
		//文件存放的磁盘块编号
		memset(newInode->blockNum, 0, sizeof(newInode->blockNum));
		//文件存放的每个磁盘块中文件个数设置为0
		memset(newInode->blockNumLength, 0, sizeof(newInode->blockNumLength));
		newInode->parent =targetInode->ino;//父亲节点为当前文件夹
		newInode->blockIndex = 0;//blockNum数组的下标
		newInode->ino = inodeIndex;//这是第几个inode项
		newInode->length = 0;//文件夹中含有0个文件，或者文件长度为0
		if (strcmp(FLAGROOT, "") != 0) {
			strcpy(newInode->owner, FLAGROOT);
			strcpy(newFcb->owner, FLAGROOT);
		}
		else {
			strcpy(newInode->owner, rootname);
			strcpy(newFcb->owner, rootname);
		}
		//同步更新FCB
		strcpy(newFcb->filename, sourceInode->filename);
		newFcb->isdir = sourceInode->isdir;
		newFcb->ino = inodeIndex;
		int f = 1;
		//如果当前磁盘块为0块，或者前一个磁盘块已经占满，则需要再从空闲的磁盘块中找一个
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
			printf("空间不足\n");
			FLAG = 0;
		}
		free(newInode);
		free(newFcb);
		
		fflush(fp);
		//遍历源文件夹Inode的FCB获取新的源文件夹的Inode,进行递归
		//首先遍历源文件夹Inode分配的12个磁盘
		for (int i = 0; i <INODE_BLOCKNUM; i++)
		{
			//遍历每个磁盘块中的fcb
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
	else//如果是文件
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
		printf("Windows路径下的文件不存在\n");
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
	fseek(ffp, 0, SEEK_END);	//定位到文件尾，偏移量为0
	int cnt = ftell(ffp);	//返回当前定位的文件位置（也就是文件总长度）
	fseek(ffp, 0, SEEK_SET);
	fread(content, cnt, 1, ffp);
	INodePtr targetInode = (INodePtr)malloc(inodeSize);//目标文件夹的inode
	fseek(fp, superBlockSize + targetIno * inodeSize, SEEK_SET);
	fread(targetInode, inodeSize, 1, fp);
	//首先要判断目标文件夹是否能放下这个fcb
	//如果该inode分配到的最后一个磁盘块不足以放下一条fcb,则返回单个文件目录磁盘块容量不足
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
		printf("单个文件目录磁盘块容量不足\n");
		return;
	}
	//判断磁盘剩余空间是否足够
	int block_need = cnt / BLOCKSIZE;
	if (superBlock.blockfree < block_need || superBlock.inodefree == 0) {
		printf("磁盘空间不足\n");
		cpFlag = 0;
		return;
	}
	//更新inode信息,增加1个inode
	int inodeIndex = -1;
	//更新位图
	for (int i = 0; i < INODENUM; ++i) {
		if (superBlock.inodeBitmap[i] == 0) {
			inodeIndex = i;
			superBlock.inodefree--;
			superBlock.inodeBitmap[i] = 1;
			break;
		}
	}
	//重新写回文件
	rewind(fp);
	fwrite(&superBlock, superBlockSize, 1, fp);
	content[cnt] = '\0';
	FcbPtr newFcb = (FcbPtr)malloc(fcbSize);
	INodePtr newInode = (INodePtr)malloc(inodeSize);
	strcpy(newInode->filename, name);//设置文件/文件夹名
	newInode->isdir = 0;
	//文件存放的磁盘块编号
	memset(newInode->blockNum, 0, sizeof(newInode->blockNum));
	//文件存放的每个磁盘块中文件个数或者文件长度设置为0
	memset(newInode->blockNumLength, 0, sizeof(newInode->blockNumLength));
	newInode->parent = targetIno;//父亲节点为当前文件夹
	newInode->blockIndex = 0;//blockNum数组的下标
	newInode->ino = inodeIndex;//这是第几个inode项
	newInode->length = 0;//文件夹中含有0个文件，或者文件长度为0
	if (strcmp(FLAGROOT, "") != 0) {
		strcpy(newInode->owner, FLAGROOT);
		strcpy(newFcb->owner, FLAGROOT);
	}
	else {
		strcpy(newInode->owner, rootname);
		strcpy(newFcb->owner, rootname);
	}
	//同步更新FCB
	strcpy(newFcb->filename, name);
	newFcb->isdir = 0;
	newFcb->ino = inodeIndex;
	int f = 1;
	//如果当前磁盘块为0块，或者前一个磁盘块已经占满，则需要再从空闲的磁盘块中找一个
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
		printf("空间不足\n");
		FLAG = 0;
	}
	free(targetInode);
	free(newFcb);
	fflush(fp);
	//向新创建的文件中写入内容
	//找到上一次写入的位置
	int blockIndex = 0;
	int byte_pos = 0;
	if (newInode->blockIndex != 0) blockIndex = newInode->blockIndex - 1;
	byte_pos = newInode->blockNumLength[blockIndex];
	int original_blockindex = blockIndex;
	int original_byte_pos = byte_pos;
	int increaseblocknum = 0;
	//if (blockIndex != 0)fseek(fp, superBlockSize + INODENUM * inodeSize + fileInode->blockNum[blockIndex] * BLOCKSIZE + byte_pos, SEEK_SET);
	for (int i = 0; i < cnt; i++) {
		//写满一个磁盘块或者该inode结点还没有被分配磁盘块
		if (blockIndex == 0 || BLOCKSIZE - byte_pos < 2) {
			//因为一个汉字两个字节所以当磁盘块剩余1或0时将开辟一个新盘块
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
		printf("空间不足,有些未写入！\n");
	}
	else if (newInode->blockIndex > INODE_BLOCKNUM)
	{
		printf("单个文件空间达到上限,有些未写入！\n");
	}
	else
	{
		printf("文件复制成功！\n");
	}
	rewind(fp);
	fwrite(&superBlock, superBlockSize, 1, fp);
	//更新inode
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
		printf("%s文件不存在\n", name);
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
		cin.getline(cmd, 1024);//获取输入的命令
		//如果输入的是换行符，不执行后面的语句
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
					cout << "文件目录不存在！" << endl;
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
					cout << "文件目录不存在！" << endl;
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
					cout << "文件目录不存在！" << endl;
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
					printf("命令参数错误，请检查\n");
				}
				else
				{
					if (argc == 2) dir_number2 = currentDir;
					else dir_number2 = getDirNumber(argv[1]);
					if (dir_number2 == -1)
					{
						cout << "文件目录不存在！" << endl;
					}
					else
					{
						int tempDir = currentDir;
						currentDir = dir_number2;
						if (argc == 3) createFile(argv[2], 0);
						else createFile(argv[1], 0);//不指定路径，则在当前文件夹下创建文件
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
					printf("命令参数错误，请检查\n");
				}
				else
				{
					if (argc == 2) fileDir_number = currentDir;
					else fileDir_number = getDirNumber(argv[1]);
					if (fileDir_number == -1)
					{
						cout << "文件目录不存在！" << endl;
					}
					else
					{
						INodePtr filenode = (INodePtr)malloc(inodeSize);
						fseek(fp, superBlockSize + fileDir_number * inodeSize, SEEK_SET);
						fread(filenode, inodeSize, 1, fp);
						int tempDir = currentDir;
						currentDir = fileDir_number;
						if (argc == 3)open(argv[2]);
						else open(argv[1]);//不指定路径，则在当前文件夹下打开文件
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
					printf("命令参数错误，请检查\n");
				}
				else
				{
					if (argc == 2) fileDir_number2 = currentDir;
					else fileDir_number2 = getDirNumber(argv[1]);
					if (fileDir_number2 == -1)
					{
						cout << "文件目录不存在！" << endl;
					}
					else
					{
						INodePtr filenode = (INodePtr)malloc(inodeSize);
						fseek(fp, superBlockSize + fileDir_number2 * inodeSize, SEEK_SET);
						fread(filenode, inodeSize, 1, fp);
						int tempDir = currentDir;
						currentDir = fileDir_number2;
						if (argc == 3)close(argv[2], fileDir_number2);
						else close(argv[1], fileDir_number2);//不指定路径，则在当前文件夹下关闭文件
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
					printf("命令参数错误，请检查\n");
				}
				else
				{
					if (argc == 2) fileDir_number3 = currentDir;
					else fileDir_number3 = getDirNumber(argv[1]);
					if (fileDir_number3 == -1)
					{
						cout << "文件目录不存在！" << endl;
					}
					else
					{
						INodePtr filenode = (INodePtr)malloc(inodeSize);
						fseek(fp, superBlockSize + fileDir_number3 * inodeSize, SEEK_SET);
						fread(filenode, inodeSize, 1, fp);
						int tempDir = currentDir;
						currentDir = fileDir_number3;
						if (argc == 3)write(argv[2], fileDir_number3);
						else write(argv[1], fileDir_number3);//不指定路径，则在当前文件夹下写文件
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
					printf("命令参数错误，请检查\n");
				}
				else
				{
					if (argc == 2) fileDir_number4 = currentDir;
					else fileDir_number4 = getDirNumber(argv[1]);
					if (fileDir_number4 == -1)
					{
						cout << "文件目录不存在！" << endl;
					}
					else
					{
						INodePtr filenode = (INodePtr)malloc(inodeSize);
						fseek(fp, superBlockSize + fileDir_number4 * inodeSize, SEEK_SET);
						fread(filenode, inodeSize, 1, fp);
						int tempDir = currentDir;
						currentDir = fileDir_number4;
						if (argc == 3)read(argv[2], fileDir_number4);
						else read(argv[1], fileDir_number4);//不指定路径，则在当前文件夹下读文件
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
					printf("命令参数错误，请检查\n");
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
						cout << "源文件目录不存在！" << endl;
					}
					else if(targetDir==-1)
					{
						cout << "目标文件目录不存在！" << endl;
					}
					else
					{
						INodePtr filenode = (INodePtr)malloc(inodeSize);
						fseek(fp, superBlockSize + fileDir_number5 * inodeSize, SEEK_SET);
						fread(filenode, inodeSize, 1, fp);
						int tempDir = currentDir;
						currentDir = fileDir_number5;
						if (argc == 4)cp(argv[2],targetDir);
						else cp(argv[1],targetDir);//不指定路径，则在当前文件夹下读文件
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
			case 14://import windows路径 虚拟磁盘路径
			{
				int fileDir_number7;
				if (argc < 2 || argc>3)
				{
					printf("命令参数错误，请检查\n");
				}
				else
				{
					
					if (argc == 2) fileDir_number7 = currentDir;
					else fileDir_number7 = getDirNumber(argv[2]);
					if (fileDir_number7 == -1)
					{
						cout << "文件目录不存在！" << endl;
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
					printf("命令参数错误，请检查\n");
				}
				else
				{

					if (argc == 3) fileDir_number8 = currentDir;
					else fileDir_number8 = getDirNumber(argv[1]);
					if (fileDir_number8 == -1)
					{
						cout << "文件目录不存在！" << endl;
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
					printf("命令参数错误，请检查\n");
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
						cout << "源文件目录不存在！" << endl;
					}
					else if (targetDir == -1)
					{
						cout << "目标文件目录不存在！" << endl;
					}
					else
					{
						if (DoesithaveOpenfile(fileDir_number6))
						{
							printf("源文件下已有文件已经被打开！请先关闭文件！\n");
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
								printf("禁止将父文件夹放入子文件夹中\n");
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
				printf("命令错误\n");
		}
	}
}
void createFileSystem()
{
	INodePtr ptrInode;
	if ((fp = fopen(FILENAME, "wb+")) == NULL)
	{
		printf("createFileSystem打开文件错误!\n");
		exit(1);
	}
	//初始化空闲磁盘块
	for (int i = 0; i < BLOCKNUM; i++)
	{
		superBlock.blockBitmap[i] = 0;
	}
	//初始化空闲inode
	for (int i = 0; i < INODENUM; i++)
	{
		superBlock.inodeBitmap[i] = 0;
	}
	//设置占位符
	for (int i = 0; i < (superBlockSize + inodeSize * INODENUM + BLOCKSIZE * BLOCKNUM + userSize * USERS); i++)
	{
		fputc(0, fp);
	}
	rewind(fp);
	superBlock.blockfree = BLOCKNUM;//当前空闲磁盘块的数量
	superBlock.inodefree = INODENUM-1;//当前空闲节点数量
	superBlock.blocksize = BLOCKSIZE;
	superBlock.inodenum = INODENUM;
	superBlock.blocknum = BLOCKNUM;
	//创建根节点，0号inode是根节点
	ptrInode = (INodePtr)malloc(inodeSize);
	ptrInode->ino = 0;//节点号
	strcpy(ptrInode->filename, "/");//文件名
	ptrInode->isdir = 1;//文件类型是文件夹
	ptrInode->parent = 0;//父文件夹是自己
	ptrInode->length = 0;//文件数量是0
	memset(ptrInode->blockNum, 0, sizeof(ptrInode->blockNum));//初始化属于这个文件的磁盘块
	memset(ptrInode->blockNumLength, 0, sizeof(ptrInode->blockNumLength));//初始化每个磁盘块已占用的长度


	//root文件夹占用第一个inode和第一个磁盘块
	//superBlock.blockBitmap[0] = 1;
	superBlock.inodeBitmap[0] = 1;
	ptrInode->blockIndex = 0;

	//将新数据写入文件disk.img
	//重新写入超级块信息
	rewind(fp);
	fwrite(&superBlock, superBlockSize, 1, fp);
	//更新inode信息
	fseek(fp, superBlockSize, SEEK_SET);
	fwrite(ptrInode, inodeSize, 1, fp);


	//当前文件夹inode号
	currentDir = 0;
	//当前路径
	strcpy(path, "/");
	//用户数量加1
	superBlock.length = 1;

	//初始化root账户名，密码
	User user;
	strcpy(user.username, "root");
	strcpy(user.password, "root");

	//将用户信息写入磁盘
	fseek(fp, Offset, SEEK_SET);
	fwrite(&user, sizeof(User), 1, fp);
	rewind(fp);

	//将更新后的超级块，写入磁盘
	fwrite(&superBlock, superBlockSize, 1, fp);
	fflush(fp);
}