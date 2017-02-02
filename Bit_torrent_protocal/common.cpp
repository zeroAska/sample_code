#include "const_values.h"
#include <iostream>
#include "sha1.h"
#include <dirent.h>
#include <fstream>
#include <string>
#include <fstream>
#include <sstream>
#include "common.h"

using namespace std;

int error_print(string error_msg)
{
	cerr<<error_msg<<endl;
	return -1;
}


 size_t get_file_size(const char* filename)
{
	/*
   FILE* f = fopen(filename, "r");

   // Determine file size
   fseek(f, 0, SEEK_END);
   size_t size =  ftell(f);
   fclose(f);
   */
   struct stat statbuff;
	if (stat(filename, & statbuff) < 0)	// get the file size via OS
	{
		printf("creating file object failed at states check\n");
		return 0;
	}
	size_t size = statbuff.st_size;
   return size;
}


void uint32_to_string(string & out_str, uint32_t to_be_convert[], size_t size = 5)
{
	char text_to_write[  sizeof(uint32_t) ];
	for (int i = 0; i < 5; ++i)
	{
		sprintf(text_to_write, "%07X", to_be_convert[i]);
		//printf("uint32_to_string[part] :: %s\n", text_to_write);
		out_str += string(text_to_write);
	}
	//printf("uint32_to_string :: %s\n", out_str.c_str());
	return;
}


 void char_str_to_hash(char * data, size_t size, uint32_t result_array[])
{
	SHA1 sha1_for_file;
	
	sha1_for_file.Reset();
	sha1_for_file.Input(data,size);
	sha1_for_file.Result(result_array);
	//string str_for_hash = uint32_to_string(hash_file);
	
}

int ls(string & path, string & result)
{
    //list<string> ret_list;
    DIR * d = opendir( path.c_str());
    if (d == NULL) 
    {
    	error_print("[ls] folder: "+path+" doesn't exist!");
    	return EXIT_FAILURE;
    }
    for (dirent * de = NULL; (de = readdir(d)) != NULL;)
    {
    	string file_name(de->d_name);
		if (file_name == string(".") || file_name == string("..")) continue;
        result = result + file_name + "\n";
    }
    return 0; 
}

 string get_first_line(string & file_name)
{
	ifstream infile;
	string line;
	infile.open(file_name);
	getline(infile,line);
	infile.close();
	return line;
}

int string_to_list(string & old_str, list<string> & l)
{
	stringstream ss;
	ss<<old_str;
	string tmp;
	while(getline(ss, tmp, ',')) 
	{
	    l.push_back(tmp);
    }
    return 0;
}

