#include<iostream>
#include<fstream>
#include<vector>
using namespace std;
int main() 
{
	vector<string> file_name_list;
	vector<string> animation_name_list;
	ifstream open_file;
	open_file.open("animation_name.txt");
	char line[1024] = { 0 };
	while (open_file.getline(line, sizeof(line)))
	{
		animation_name_list.push_back(line);
	}
	open_file.close();
	open_file.open("file_name.txt");
	while (open_file.getline(line, sizeof(line)))
	{
		file_name_list.push_back(line);
	}
	open_file.close();
	ofstream out_file;
	out_file.open("json_name.txt");
	for (int i = 0; i < animation_name_list.size(); ++i) 
	{
		string out_data = string("{\n") + "\"animation_name\" : " + "\"" + animation_name_list[i]+"\"" +",\n";
		out_data += string("\"animation_file\" : ") + "\"" + "animation\\\\" + file_name_list[i] +"\"" + "\n},\n";
		out_file.write(out_data.c_str(), out_data.size());
	}
	return 0;
}