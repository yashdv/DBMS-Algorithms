#include<iostream>
#include<cstdlib>
#include<ctime>
#include<fstream>

using namespace std;

ofstream out;
void getnum()
{
	int num=rand()%1000;
	if(num<10)
		out << "00";
	else if(num<100)
		out<<"0";
	out<<num;
}

int main(int argc, char **argv)
{
	srand(time(NULL));
	if(argv[1]==NULL)
	{
		cout<<"Specify no. of tuples"<<endl;
		return 0;
	}
	if(argv[2]==NULL)
	{
		cout<<"Specify file name"<<endl;
		return 0;
	}
	out.open(argv[2],ios::out);

	int n=atoi(argv[1]),num;

	for(int i=0;i<n;i++)
	{
		getnum();
		out<<" ";
		getnum();
		out<<endl;
	}
	out.close();
	return 0;
}
