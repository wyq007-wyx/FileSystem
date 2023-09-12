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
			printf("�������û���:");
			cin >> name;
			getchar();
			printf("����������:");
			cin >> pass;
			getchar();
			if (login(name, pass) == 1) {
				printf("��½�ɹ�..\n");
				Sleep(500);
				system("cls");
				strcpy(rootname, name);
				openFileSystem();
				order();
			}
			else {
				cout << "�û������������.." << endl;
				Sleep(2);
			}
		}
		else if (strcmp(choice, "register") == 0) {
			cout << "�������û���:";
			cin >> name;
			cout << "����������:";
			cin >> pass;
			cout << "��ȷ������:";
			cin >> pass1;
			while (strcmp(pass, pass1) != 0) {
				cout << "          --�������벻һ�£�������--            " << endl;
				cout << "����������: ";
				cin >> pass;
				cout << "��ȷ������: ";
				cin >> pass1;
			}
			if (user_register(name, pass) == 1) {
				cout << "        --ע��ɹ�--        " << endl;
				Sleep(100);
			}
			else {
				cout << "         --ע��ʧ�ܣ�������--      " << endl;
				Sleep(100);
			}
		}
		else if (strcmp(choice, "help") == 0) {
			cout << "   login              ��¼�ļ�ϵͳ\n";
			cout << "   register           ע���û�\n";
			cout << "   exit               �˳��ļ�ϵͳ\n";
		}
		else if (strcmp(choice, "exit") == 0) {
			exit(0);
		}
		else {
			printf("������Ч\n");
		}
	}

}
int main() {
	init();
	return 0;
}