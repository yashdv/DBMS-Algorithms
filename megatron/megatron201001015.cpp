#include<cstdio>
#include<cstdlib>
#include<cstring>
#include<cctype>

#include<iostream>
#include<vector>
#include<string>
#include<algorithm>
#include<utility>
#include<map>

#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<unistd.h>

using namespace std;

#define MAX_LINE_SZ 2048
#define MAX_QSZ 512
#define OP_SZ 16

#define X_LLI 0
#define X_STR 1

#define Z_E 0
#define Z_L 1
#define Z_G 2
#define Z_LE 3
#define Z_GE 4

union T
{
	long long int lli;
	char *s;
};

class I_O
{
	public:
		int openf(const char *, const char *);
		int closef(const int);
		int readf(const int,char *);
		int writef(const int, const char *);
};

class dataT
{
	public:
		T data;
		int type;

		dataT();
		dataT(long long int);
		dataT(char *);
		void print(int);
};

class operand
{
	public:
		dataT v;
		pair<int,int> col;
		bool isv;

		dataT get_val();
};

class conD
{
	public:
		int op;
		operand left;
		operand right;

		bool cmp();
};

class table
{
	public:
		string name;
		string csvname;
		int col_cnt;
		vector<string> col_name;
		vector<int> col_type;
};

class database
{
	public:
		vector<table> tables;
		vector< vector< vector<dataT> > > records;
		map<string,int> table_map;
		map<string,pair<int,int> > col_map;

		void load_schema(char *);
		void print_schema();
		void load_data();
		void save_data();
};

class query
{
	public:
		vector<string> table_names;
		vector<string> col_names;
		bool distinct_flag;
		bool condition_flag;
		bool redirect_flag;
		string condition;
		string out_file;
		vector<int> table_index;
		vector< pair<int,int> > col_index;
		conD cond;
		vector<int> table_row;
		vector< vector<dataT> > output;

		void initialize();
		bool syntax_check(char *);
		bool valid_table();
		pair<int,int> valid_col(string &);
		bool valid();
		bool checkv(char *,operand &);
		bool parse_condition();
		void run(int);
		void process_query();
		dataT get_val();
		void print_debug();
};

char inp_query[MAX_QSZ];
database D;
query Q;
I_O io;

int get_datatype(char *s)
{
	if(strcmp(s,"longlongint") == 0)
		return X_LLI;
	if(strcmp(s,"string") == 0)
		return X_STR;
	return -1;
}

int get_datatype(operand &o)
{
	if(o.isv)
		return o.v.type;
	return D.tables[o.col.first].col_type[o.col.second];
}

void strip_white_space(char *s)
{
	char *p = s;
	while(isblank(*s))
		s++;
	while(*s)
	{
		*p = *s;
		p++;
		s++;
	}
	while(isblank(*(p-1)))
		p--;
	*p = '\0';
}

void cleaning(char *s)
{
	strip_white_space(s);
	char *p = s;
	while(*s)
	{
		if(*s!='"' && *s != '\'')
			*p = *s, p++;
		s++;
	}
	*p = '\0';
}

bool check_distinct(char *s)
{
	char *p1 = strchr(s,'(');
	char *p2 = strchr(s,')');
	if(p1==NULL || p2==NULL)
		return false;
	p1++;
	while(p1 != p2)
	{
		*s = *p1;
		p1++;
		s++;
	}
	*s = '\0';
	return true;
}

int get_op(char *s)
{
	if(strcmp(s,"<") == 0)
		return Z_L;
	if(strcmp(s,">") == 0)
		return Z_G;
	if(strcmp(s,"<=") == 0)
		return Z_LE;
	if(strcmp(s,">=") == 0)
		return Z_GE;
	if(strcmp(s,"=") == 0)
		return Z_E;
	return -1;
}

int I_O::openf(const char *name, const char *amode)
{
	if(*amode == 'r')
		return open(name, O_RDONLY);
	if(*amode == 'w')
		return creat(name, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH);
	return -1;
}

int I_O::closef(const int f)
{
	return close(f);
}

int I_O::readf(const int f, char *buf)
{
	char *p = buf;
	while(1)
	{
		if(read(f,p,1) == 0)
			return EOF;
		if(*p == '\n')
			break;
		p++;
	}
	*p = '\0';
	return (p-buf);
}

int I_O::writef(const int f, const char *buf)
{
	return write(f,buf,strlen(buf));
}

dataT::dataT()
{
	type = X_LLI;
	data.lli = 0;
}	

dataT::dataT(long long a)
{
	type = X_LLI;
	data.lli = a;
}

dataT::dataT(char *a)
{
	type = X_STR;
	data.s = strdup(a);
}

bool operator < (const dataT &a, const dataT &b)
{
	if(a.type != b.type)
		return false;
	switch(a.type)
	{
		case X_LLI:
			return(a.data.lli < b.data.lli);
			break;
		case X_STR:
			return(string(a.data.s) < string(b.data.s));
			break;
	}
	return false;
}

bool operator > (const dataT &a, const dataT &b)
{
	return b < a;
}

bool operator == (const dataT &a, const dataT &b)
{
	return !(a<b || b<a);
}

bool operator <= (const dataT &a, const dataT &b)
{
	return (a<b || a==b);
}

bool operator >= (const dataT &a, const dataT &b)
{
	return (a>b || a==b);
}

void dataT::print(int fp)
{
	char p[100];
	switch(type)
	{
		case X_LLI:
			sprintf(p,"%lld",data.lli);
			break;
		case X_STR:
			sprintf(p,"%s",data.s);
			break;
	}
	io.writef(fp, p);
}

dataT operand::get_val()
{
	if(isv)
		return v;
	return D.records[col.first][Q.table_row[col.first]][col.second];
}

bool conD::cmp()
{
	switch(op){
		case Z_L :
			return left.get_val() < right.get_val();
		case Z_G :
			return left.get_val() > right.get_val();
		case Z_LE :
			return left.get_val() <= right.get_val();
		case Z_GE :
			return left.get_val() >= right.get_val();
		case Z_E :
			return left.get_val() == right.get_val();
	}
	return false;
}

void database::load_schema(char *name)
{
	int fp = io.openf(name, "r");
	int n;
	char buf[MAX_LINE_SZ];
	char s1[MAX_LINE_SZ];
	char s2[MAX_LINE_SZ];

	while(io.readf(fp, buf) != EOF)
	{
		sscanf(buf,"%[^#]#%[^#]#%d ",s1,s2,&n);
		table t;
		t.name = string(s1);
		t.csvname = string(s2);
		t.col_cnt = n;
		table_map[t.name] = tables.size();

		for(int i=0; i<n; i++)
		{
			io.readf(fp, buf);
			sscanf(buf,"%[^#]#%s ",s1,s2);
			t.col_name.push_back(string(s1));
			t.col_type.push_back(get_datatype(s2));
			col_map[t.name + "." + string(s1)] = make_pair(table_map[t.name], i);
		}
		tables.push_back(t);
	}
	io.closef(fp);
}

void database::print_schema()
{
	int l1 = tables.size();
	for(int i=0; i<l1; i++)
	{
		cout << tables[i].name << endl;
		cout << tables[i].csvname << endl;
		cout << tables[i].col_cnt << endl;
		for(int j=0; j<tables[i].col_cnt; j++)
		{
			cout << tables[i].col_name[j] << " ";
			cout << tables[i].col_type[j] << endl;
		}
		puts("");
	}
}

void database::load_data()
{
	int l1 = tables.size();
	int fp;
	char buf[MAX_LINE_SZ];
	char *p;
	dataT t;

	records.resize(l1);

	for(int i=0; i<l1; i++)
	{
		fp = io.openf(tables[i].csvname.c_str(), "r");
		io.readf(fp, buf);
		while(io.readf(fp, buf) != EOF)
		{
			vector<dataT> row;
			p = strtok(buf,",");
			for(int j=0; j<tables[i].col_cnt; j++)
			{
				cleaning(p);
				switch(tables[i].col_type[j])
				{
					case X_LLI:
						t = atoll(p);
						break;
					case X_STR:
						t = strdup(p);
						break;
				}
				row.push_back(t);
				p = strtok(NULL,",");
			}
			records[i].push_back(row);
		}
		io.closef(fp);
	}
}

void database::save_data()
{
	int l1 = records.size();
	int l2;
	int l3;
	int fp;

	for(int i=0; i<l1; i++)
	{
		fp = io.openf((tables[i].csvname+".save").c_str(), "w");
		l2 = records[i].size();
		for(int j=0; j<l2; j++)
		{
			l3 = records[i][j].size();
			for(int k=0; k<l3; k++)
			{
				records[i][j][k].print(fp);
				io.writef(fp, ",");
			}
			io.writef(fp, "\n");
		}
		io.closef(fp);
	}
}

void query::initialize()
{
	distinct_flag = false;
	condition_flag = false;
	redirect_flag = false;
	table_names.clear();
	col_names.clear();
	table_index.clear();
	col_index.clear();
	output.clear();
}

bool query::syntax_check(char *s)
{
	this->initialize();

	if(strchr(s,';') == NULL)
	{
		printf("Invalid Query : No semicolon found\n");
		return false;
	}

	s = strtok(s," \t");
	if(strcasecmp(s,"select") != 0)
	{
		printf("Invalid Query : Query must begin with 'select'\n");
		return false;
	}

	s = strtok(NULL," ,\t");
	while(s!=NULL && strcasecmp(s, "from")!=0)
	{
		if(check_distinct(s))
			distinct_flag = true;
		col_names.push_back(string(s));
		s = strtok(NULL," ,\t");
	}
	if(s == NULL)
	{
		printf("Invalid Query : No 'from' keyword found\n");
		return false;
	}
	if(col_names.size() == 0)
	{
		printf("Invalid Query : what to select is unspecified\n");
		return false;
	}

	s = strtok(NULL," ,;\t");
	while(s!=NULL && strcasecmp(s,"where")!=0 && s[0]!='|')
	{
		table_names.push_back(s);
		s = strtok(NULL," ,;\t");
	}
	if(table_names.size() == 0)
	{
		printf("Invalid Query : tables are unspecified\n");
		return false;
	}
	if(s!=NULL && strcasecmp(s,"where")==0)
	{
		s = strtok(NULL,";");
		if(s == NULL)
		{
			printf("Invalid Query : No condition after 'where'\n");
			return false;
		}
		condition_flag = true;
		condition = s;
		s = strtok(NULL,";, \t");
	}
	if(s!=NULL && s[0]=='|')
	{
		s = strtok(NULL,";,");
		cleaning(s);
		if(s != NULL)
			out_file = s, redirect_flag = true;;
	}
	return true;
}

bool query::valid_table()
{
	int l1 = table_names.size();
	for(int i=0; i<l1; i++)
	{
		if(D.table_map.count(table_names[i]))
			table_index.push_back(D.table_map[table_names[i]]);
		else
		{
			cout << "Error : No such table " << table_names[i] << endl;
			return false;
		}
	}
	return true;
}

pair<int,int> query::valid_col(string &s)
{
	if(D.col_map.count(s))
		return D.col_map[s];
	
	pair<int,int> ret(-1,-1);
	string x;
	pair<int,int> id;
	int c = 0;
	int l2 = table_index.size();
	for(int j=0; j<l2; j++)
	{
		x = D.tables[table_index[j]].name + "." + s;
		if(D.col_map.count(x))
		{
			id = D.col_map[x];
			c++;
		}
	}
	if(c == 0)
	{
		cout << "Error : No such column " << s << endl;
		return ret;
	}
	if(c > 1)
	{
		cout << "Error : Ambigous column " << s << endl;
		return ret;
	}
	return id;
}

bool query::valid()
{
	if(!valid_table())
		return false;

	int l1 = table_names.size();
	if(col_names.size()==1 && col_names[0][0]=='*')
	{
		for(int i=0; i<l1; i++)
		{
			int l2 = D.tables[table_index[i]].col_cnt;
			for(int j=0; j<l2; j++)
				col_index.push_back(make_pair(table_index[i], j));
		}
	}
	else
	{
		pair<int,int> t;
		l1 = col_names.size();
		for(int i=0; i<l1; i++)
		{
			t = valid_col(col_names[i]);
			if(t == pair<int,int>(-1,-1))
				return false;
			col_index.push_back(t);
		}
	}
	return true;
}

bool query::checkv(char *s, operand &o)
{
	if(*s=='\'' || *s=='"' || isdigit(*s) || (*s=='-' && isdigit(s[1])))
	{
		o.isv = true;
		if(isdigit(*s) || (*s=='-' && isdigit(s[1])))
			o.v = atoll(s);
		else
			cleaning(s), o.v = strdup(s);
	}
	else
	{
		o.isv = false;
		string ss = string(s);
		o.col = valid_col(ss);
		if(o.col == pair<int,int>(-1,-1))
			return false;
	}
	return true;
}

bool query::parse_condition()
{
	if(!condition_flag)
		return true;

	char s1[MAX_LINE_SZ];
	char s2[MAX_LINE_SZ];
	char op[OP_SZ];
	int t1;
	int t2;

	sscanf(condition.c_str()," %[^<>=! \t] %[<>=!] %[^<>=!]",s1,op,s2);
	strip_white_space(s2);

	cond.op = get_op(op);
	if(!checkv(s1,cond.left) || !checkv(s2,cond.right))
		return false;
	t1 = get_datatype(cond.left);
	t2 = get_datatype(cond.right);
	if(t1 != t2)
	{
		cout << "Error : type mismatch in condition " << t1 << " " << t2 << endl;
		return false;
	}
	return true;
}

void query::run(int index)
{
	int l1;
	if(index == (int)table_index.size())
	{
		if(condition_flag)
		{
			if(!cond.cmp())
				return ;
		}

		vector<dataT> tup;
		l1 = col_index.size();
		int tableno;
		int colno;
		for(int i=0; i<l1; i++)
		{
			tableno = col_index[i].first;
			colno = col_index[i].second;
			tup.push_back(D.records[tableno][table_row[tableno]][colno]);
		}
		output.push_back(tup);
		return;
	}

	l1 = D.records[table_index[index]].size();
	for(int i=0; i<l1; i++)
	{
		table_row[table_index[index]] = i;
		run(index+1);
	}
}

void query::process_query()
{
	int l1;
	int l2;
	int fp = 2;

	if(redirect_flag)
		fp = io.openf(out_file.c_str(), "w");

	table_row.resize(D.tables.size());
	run(0);

	if(distinct_flag)
	{
		sort(output.begin(), output.end());
		output.resize(unique(output.begin(), output.end()) - output.begin());
	}

	l1 = output.size();
	for(int i=0; i<l1; i++)
	{
		l2 = output[i].size();
		for(int j=0; j<l2; j++)
		{
			output[i][j].print(fp);
			io.writef(fp, ",");
		}
		io.writef(fp, "\n");
	}
	if(redirect_flag)
		io.closef(fp);
}

void query::print_debug()
{
	for(size_t i=0; i<col_names.size(); i++)
		cout << col_names[i] << endl;
	for(size_t i=0; i<table_names.size(); i++)
		cout << table_names[i] << endl;
	cout << condition << endl << out_file << endl;

	for(size_t i=0; i<col_index.size(); i++)
		cout << col_index[i].first << " " << col_index[i].second << endl;
	for(size_t i=0; i<table_index.size(); i++)
		cout << table_index[i] << endl;

	cout << "op = " << cond.op << endl;
	if(cond.left.isv)
	{
		cout << "is value = ";
		switch(cond.left.v.type)
		{
			case X_LLI:
				cout << cond.left.v.data.lli;
				break;
			case X_STR:
				
				cout << cond.left.v.data.s;
				break;
		}
		puts("");
	}
	else
		cout << "is col = " << cond.left.col.first << " " << cond.left.col.second << endl;

	if(cond.right.isv)
	{
		cout << "is value = ";
		switch(cond.right.v.type)
		{
			case X_LLI:
				cout << cond.right.v.data.lli;
				break;
			case X_STR:
				cout << cond.right.v.data.s;
				break;
		}
		puts("");

	}
	else
		cout << "is col = " << cond.right.col.first << " " << cond.right.col.second << endl;
}

int main(int argc, char *argv[])
{
	if(argc < 2)
	{
		puts("Usage : ./megatron201001015 schema.txt");
		return 0;
	}

	D.load_schema(argv[1]);
	//D.print_schema();
	D.load_data();
	//D.save_data();

	int b1,b2,b3;
	while(printf("\n$"))
	{
		cin.getline(inp_query, MAX_QSZ);
		if(strcasecmp(inp_query, "quit") == 0)
			break;

		if((b1=Q.syntax_check(inp_query)) && (b2=Q.valid()) && (b3=Q.parse_condition()))
			Q.process_query();
		else
		{
			puts("Query not processed");
			printf("%d%d%d\n",b1,b2,b3);
			//Q.print_debug();
		}
	}
	return 0;
}
