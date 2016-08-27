#include "sock.h"

int main() {
	char username[MAX_PATH];
	char password[MAX_PATH];
	cout << "Username: ";
	cin >> username;
	cout << " Username: " << username << endl;

	string logininfo = "tag_id=" + string(username);
	int loginInfoSize = logininfo.length();
	string header;
	header = "POST /login1.php HTTP/1.1\r\n";
	header += "Host: 192.168.1.72:80\r\n";
	header += "Content-Type: application/x-www-form-urlencoded\r\n";
	header += "Content-Length: " + to_string(loginInfoSize) + "\r\n";
	header += "Accept-Charset: utf-8\r\n";
	header += "\r\n";
	header += logininfo + "\r\n";
	header += "\r\n";

	cout << " ===Send Form===" << endl;
	MySock::Instance().Login(header);

	while (true) {
		Sleep(1);
	}
	return 0;
}