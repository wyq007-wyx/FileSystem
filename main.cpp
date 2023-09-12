#include"fileSystem.h"
//#include <iostream>


void init() {
	strcpy(FILENAME, "disk.img");
	if ((fp = fopen(FILENAME, "rb+")) == NULL) {
		createFileSystem();
	}
	while (true) {
		printf("$>");
		char choice[100];
		cin >> choice;
		char name[30], pass[30], pass1[30];
		if (strcmp(choice, "login") == 0) {
			printf("请输入用户名:");
			cin >> name;
			getchar();
			printf("请输入密码:");
			cin >> pass;
			getchar();
			if (login(name, pass) == 1) {
				printf("登陆成功..\n");
				Sleep(500);
				system("cls");
				strcpy(rootname, name);
				openFileSystem();
				order();
			}
			else {
				cout << "用户名或密码错误.." << endl;
				Sleep(2);
			}
		}
		else if (strcmp(choice, "register") == 0) {
			cout << "请输入用户名:";
			cin >> name;
			cout << "请输入密码:";
			cin >> pass;
			cout << "请确认密码:";
			cin >> pass1;
			while (strcmp(pass, pass1) != 0) {
				cout << "          --两次密码不一致，请重试--            " << endl;
				cout << "请输入密码: ";
				cin >> pass;
				cout << "请确认密码: ";
				cin >> pass1;
			}
			if (user_register(name, pass) == 1) {
				cout << "        --注册成功--        " << endl;
				Sleep(100);
			}
			else {
				cout << "         --注册失败，请重试--      " << endl;
				Sleep(100);
			}
		}
		else if (strcmp(choice, "help") == 0) {
			cout << "   login              登录文件系统\n";
			cout << "   register           注册用户\n";
			cout << "   exit               退出文件系统\n";
		}
		else if (strcmp(choice, "exit") == 0) {
			exit(0);
		}
		else {
			printf("命令无效\n");
		}
	}

}
int main() {
	init();
	return 0;
}