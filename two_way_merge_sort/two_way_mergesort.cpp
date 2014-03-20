#include<cstdio>
#include<cstdlib>
#include<cstring>
#include<cmath>

#include<algorithm>
#include<queue>

using namespace std;

#define MAX_COL 1024
#define MAX_LINE_SZ 10000 
#define MAX_NUM_FILES 1022
#define MAX_NAME_SZ 512
#define EXTRA 2

bool sort_reverse;                    // true if desc
bool file_empty[MAX_NUM_FILES+2];

long long int x;
long long int y;                      // main memory size


char ***table;
char inpFile[MAX_NAME_SZ];

int num_col;
int col_order_cnt;
int col_order[MAX_COL];
int col_sz[MAX_COL];
int rec_sz;
int rec_str_sz;                       // the length excluding \n\r
int rec_in_MM;
int total_num_rec;
int num_files;
int rec_per_buffer;
int bufst[MAX_NUM_FILES+2];
int bufend[MAX_NUM_FILES+2];
int bufp[MAX_NUM_FILES+2];

inline void debugx()
{
	puts("--------------------------------------------");
	printf("x = %lld\n",x);
	printf("y = %lld\n",y);
	printf("num_col = %d\n",num_col);
	printf("rec_sz = %d\n",rec_sz);
	printf("rec_str_sz = %d\n",rec_str_sz);
	printf("total_num_rec = %d\n",total_num_rec);
	printf("rec_in_MM = %d\n",rec_in_MM);
	printf("num_files = %d\n",num_files);
	printf("rec_per_buffer = %d\n",rec_per_buffer);

	puts("");
	for(int i=0; i<=num_files; i++)
	{
		printf("file = %d, ",i);
		printf("st = %d, end = %d, p = %d, empty = %d\n",bufst[i],bufend[i],bufp[i],file_empty[i]);
	}
	puts("--------------------------------------------");
	puts("");
}

inline bool cmp2(char **a, char **b)
{
	int ret;
	for(int i=0; i<col_order_cnt; i++)
	{
		ret = strcmp(a[col_order[i]], b[col_order[i]]);
		if(ret)
			return (ret < 0);
	}
	return false;
}

bool cmp(char **a, char **b)
{
	if(sort_reverse)
		return cmp2(b, a);
	return cmp2(a, b);
}

struct Compare
{
	bool operator () (pair<char **,int> &a, pair<char **,int> &b)
	{
		if(sort_reverse)
			return cmp2(a.first, b.first);
		return cmp2(b.first, a.first);
	}
};

priority_queue< pair<char **,int>, vector< pair<char **,int> >, Compare > heap;

inline void find_all()
{
	FILE *fp;
	char temp[MAX_LINE_SZ];
	int l;
	int prev = 0;

	fp = fopen(inpFile, "r");
	fscanf(fp,"%[^\n\r] ",temp);
	rec_str_sz = strlen(temp);
	rec_sz = ftell(fp);

	fseek(fp , 0 , SEEK_END);
	x = ftell(fp);
	fclose(fp);

	l = strlen(temp) - 1;
	for(int i=0; i<l; ++i)
	{
		if(temp[i]==' ' && temp[i+1]==' ')
		{
			col_sz[num_col++] = i - prev;
			prev = i+2;
			++i;
		}
	}
	col_sz[num_col++] = l + 1 - prev;
}

inline void initx(int argc, char *argv[])
{
	strcpy(inpFile, argv[1]);
	y = atoll(argv[2]) * (1LL<<20);
	find_all();
	sort_reverse = !strcmp(argv[3], "desc");
	
	for(int i=4; i<argc; ++i)
		col_order[col_order_cnt++] = atoi(argv[i])-1;

	if(rec_sz == 0 || rec_str_sz == 0)
	{
		printf("Error: record size = 0\n");
		printf("rec_sz = %d, rec_str_sz = %d\n",rec_sz,rec_str_sz);
		exit(-1);
	}

	total_num_rec = x / rec_sz;
	rec_in_MM = y / rec_sz;
	num_files = max(1, (int)ceil(total_num_rec / (float)rec_in_MM));
	rec_per_buffer = rec_in_MM / (num_files + 1); // #buffers = #files +1(output)

	if(num_files > MAX_NUM_FILES)
	{
		printf("Error: Too many files/fp to create for system\n");
		printf("num_files = %d, MAX_NUM_FILES = %d\n",num_files,MAX_NUM_FILES);
		exit(-1);
	}

	if(rec_in_MM == 0)
	{
		printf("Error: main memory insufficient\n");
		printf("rec_in_MM = %d, y = %lld, rec_sz = %d\n",rec_in_MM,y,rec_sz);
		exit(-1);
	}

	if(rec_per_buffer == 0)
	{
		printf("Error: main memory too small. There are too many buffers/files for MM\n");
		printf("rec_per_buffer = %d, rec_in_MM = %d, #buffers = %d\n",rec_per_buffer,rec_in_MM,num_files+1);
		exit(-1);
	}

	if(col_order_cnt > num_col)
	{
		printf("Error: number of columns in input is more than that of file\n");
		printf("col_order_cnt = %d, num_col = %d\n",col_order_cnt,num_col);
		exit(-1);
	}
}

inline FILE *filemaker(const int c)
{
	char t[16];
	sprintf(t,"temp/%d.txt",c);
	return (fopen(t, "w"));
}

inline FILE *fileopener(const int c)
{
	char t[16];
	sprintf(t,"temp/%d.txt",c);
	return (fopen(t, "r"));
}

inline void alloc_table(int num_rec)
{
	table = (char ***)malloc(num_rec * sizeof(char **));
	for(int i=0; i<num_rec; ++i)
		table[i] = (char **)malloc(num_col * sizeof(char *));
	for(int i=0; i<num_rec; ++i)
		for(int j=0; j<num_col; ++j)
			table[i][j] = (char *)malloc((col_sz[j]+EXTRA) * sizeof(char));
}

inline void clear_table(int num_rec)
{
	for(int i=0; i<num_rec; ++i)
		for(int j=0; j<num_col; ++j)
			free(table[i][j]);
	for(int i=0; i<num_rec; ++i)
		free(table[i]);
	free(table);
}

inline void sort_files()
{
	FILE *fp2;
	FILE *fp;
	int i;
	char temp[MAX_COL];
	char newl;

	fp = fopen(inpFile, "r");
	for(int file=0; file<num_files; ++file)
	{
		for(i=0; i<rec_in_MM && fscanf(fp,"%[^\n]%c",temp,&newl)!=EOF; ++i)
		{
			for(int j=0, k=0; j<num_col; ++j)
			{
				strncpy(table[i][j], temp+k, col_sz[j]);
				table[i][j][col_sz[j]] = '\0';
				k += col_sz[j] + 2;
			}
		}
		sort(table, table+i, cmp);

		fp2 = filemaker(file);
		for(int k=0; k<i; ++k)
		{
			fprintf(fp2,"%s",table[k][0]);
			for(int j=1; j<num_col; ++j)
					fprintf(fp2,"  %s",table[k][j]);
			fprintf(fp2,"\r\n");
		}
		fclose(fp2);
	}
	fclose(fp);
}

inline void phase2init()
{	
	int i;
	for(i=0; i<=num_files; ++i)
		bufp[i] = bufst[i] = i*rec_per_buffer;
	bufst[i] = rec_in_MM;
	
	for(i=0; i<=num_files; ++i)
		bufend[i] = bufst[i+1];

	memset(file_empty,false,sizeof(file_empty));
}

inline void fillBuffer(int bno, FILE *fp)
{
	char newl;
	char temp[MAX_COL];
	int i;

	bufp[bno] = bufst[bno];
	for(i=bufst[bno]; i<bufend[bno] && fscanf(fp,"%[^\n]%c",temp,&newl)!=EOF; ++i)
	{
		for(int j=0, k=0; j<num_col; ++j)
		{
			strncpy(table[i][j], temp+k, col_sz[j]);
			table[i][j][col_sz[j]] = '\0';
			k += col_sz[j] + 2;
		}
	}

	if(i < bufend[bno])
	{
		file_empty[bno] = true;
		bufend[bno] = i;
	}
}

inline void empty_buffer(int bno)
{
	for(int i=bufst[bno]; i<bufp[bno]; ++i)
	{
		printf("%s",table[i][0]);
		for(int j=1; j<num_col; ++j)
			printf("  %s",table[i][j]);
		printf("\r\n");
	}
	bufp[bno] = bufst[bno];
}

inline void merge()
{
	FILE *fp[MAX_NUM_FILES+2];
	char **rec;
	int top;
	
	for(int i=0; i<num_files; ++i)
	{
		fp[i] = fileopener(i);
		fillBuffer(i, fp[i]);
		heap.push(make_pair(table[ bufp[i]++ ], i));
	}

	while(!heap.empty())
	{
		rec = heap.top().first;
		top = heap.top().second;
		heap.pop();

		for(int j=0; j<num_col; ++j)
			strcpy(table[bufp[num_files]][j], rec[j]);
		++bufp[num_files];
		
		if(bufp[num_files] == bufend[num_files])
			empty_buffer(num_files);

		if(bufp[top] == bufend[top] && !file_empty[top])
			fillBuffer(top, fp[top]);
		if(bufp[top] != bufend[top])
			heap.push(make_pair(table[ bufp[top]++ ], top));
	}
	empty_buffer(num_files);

	for(int i=0; i<num_files; ++i)
		fclose(fp[i]);
}

int main(int argc, char *argv[])
{
	if(argc < 5)
	{
		puts("Usage: ./sort <input file> <main memory size> <asc/desc> <c0>");
		return 0;
	}
	// the first record is clean and good with double space anywhere means delimiter
	initx(argc, argv);
	alloc_table(rec_in_MM);
	sort_files();
	phase2init();
	merge();
	clear_table(rec_in_MM);
	//debugx();
	return 0;
}
