#include<cstdio>
#include<cstring>
#include<cstdlib>
#include<cmath>

#include<iostream>
#include<algorithm>
#include<vector>
#include<string>
#include<set>

using namespace std;

#define rpb 100
#define noc 2
#define NAMESZ 512
#define LINESZ 1024
#define UNQVAL 1000

int FLAG;               // 1=sort, 2=hash
int M;                  // #buffers
int nr;                 // #sublist of R
int ns;                 // #sublist of S
int joinAttr_idx;       // used to know which column to use during sorting
int **buf;              // buffer / Maing memory

vector<int> bst;        // starting index of buffer 'i'
vector<int> bend;       // ending index of buffer 'i'
vector<int> bp;         // bp[i] is a pointer denoting how much 'i'th buffer is read

char Rfname[NAMESZ];    // file name for relation R
char Sfname[NAMESZ];    // file name for relation S
char pathr[] = "tempr"; // temp folder for R to keep temp files 
char paths[] = "temps"; // temp filder for S to keep temp files

inline void debugx()
{
	puts("________________________");
	printf("FLAG = %d\n",FLAG);
	printf("M = %d\n",M);
	printf("ns = %d\n",ns);
	printf("nr = %d\n",nr);
	puts("________________________");

}

inline int get_num_rec(FILE *fp)
{
	char temp[LINESZ];
	int recsz;
	int fsize;

	fscanf(fp,"%[^\n]",temp);
	recsz = ftell(fp) + 1;
	fseek(fp,0,SEEK_END);
	fsize = ftell(fp);
	fclose(fp);
	return fsize/recsz;
}

inline void initx(int argc, char *argv[])
{
	strcpy(Rfname, argv[1]);
	strcpy(Sfname, argv[2]);
	
	if(strcmp(argv[3], "sort") == 0)
		FLAG = 1;
	else if(strcmp(argv[3], "hash") == 0)
		FLAG = 2;
	else
	{
		printf("Error: nothing like %s\nEnter 'sort' or 'hash'\n",argv[3]);
		exit(-1);
	}
	
	M = atoi(argv[4]);

	if(FLAG == 1)
	{
		int br = ceil(get_num_rec(fopen(Rfname, "r")) / (float)rpb);
		int bs = ceil(get_num_rec(fopen(Sfname, "r")) / (float)rpb);
		nr = ceil(br/(float)(M));
		ns = ceil(bs/(float)(M));
		if(nr + ns >= M)
		{
			puts("Error: B(R) + B(S) <= M^2 violated. Not enought memory");
			printf("nr = %d, ns = %d, M = %d\n",nr,ns,M);
			exit(-1);
		}
		if(nr + ns > 1000)
		{
			puts("Error: will not be able to open so many files in phase2");
			printf("nr = %d, ns = %d\n",nr,ns);
			exit(-1);
		}
	}
	else if(FLAG == 2)
	{
		if(M > 1000)
		{
			puts("Error: will not be able to open so many files in phase2");
			printf("M = %d\n",M);
			exit(-1);
		}
		if(M < 2)
		{
			puts("Error: Too few Buffers. Keep M > 1");
			printf("M = %d\n",M);
			exit(-1);
		}
	}
}

bool cmp1(int *a, int *b)
{
	return a[joinAttr_idx] < b[joinAttr_idx];
}

inline FILE *filemaker(const int c, const char *folder)
{
	char t[16];
	sprintf(t,"%s/%d.txt",folder,c);
	return (fopen(t, "w"));
}

inline FILE *fileopener(const int c, const char *folder)
{
	char t[16];
	sprintf(t,"%s/%d.txt",folder,c);
	return (fopen(t, "r"));
}

inline int **allocx(int n)
{
	n *= rpb;
	n++;
	int **a = (int **)malloc(n * sizeof(int *));
	for(int i=0; i<n; ++i)
		a[i] = (int *)malloc(noc * sizeof(int));
	return a;
}

inline void freex(int **a, int n)
{
	n *= rpb;
	n++;
	for(int i=0; i<n; ++i)
		free(a[i]);
	free(a);
}

inline bool fillBuffer(int bno, FILE *fp)
{
	bool ret = false;
	int i = bp[bno] = bst[bno];

	for(; i<bend[bno] && fscanf(fp,"%d%d",&buf[i][0],&buf[i][1])!=EOF; ++i);
	if(i < bend[bno])
	{
		ret = true;
		bend[bno] = i;
	}
	return ret;
}

inline void empty_buffer(int bno)
{
	for(int i=bst[bno]; i<bp[bno]; i+=2)
		printf("%03d %03d %03d %03d\n",buf[i][0],buf[i][1],buf[i+1][0],buf[i+1][1]);
	bp[bno] = bst[bno];
}
		
inline void write2disk(int bno, FILE *fp)
{
	for(int i=bst[bno]; i<bp[bno]; ++i)
		fprintf(fp,"%d %d\n",buf[i][0],buf[i][1]);
	bp[bno] = bst[bno];
}

class SortMerge
{
	public:
		vector<bool> isempty;
		vector<FILE *> FP;
		vector< pair<int,int> > vr;
		vector< pair<int,int> > vs;
		set<int> rset;
		set<int> sset;
		int I;
		int J;
		int l1;
		int l2;

		inline void read_sort(FILE *fp, int nx, const char *folder, int idx)
		{
			int i;
			int nrec = M * rpb;
			FILE *fp2;

			joinAttr_idx = idx;
			for(int subl=0; subl<nx; ++subl)
			{
				for(i=0; i<nrec && fscanf(fp,"%d%d",&buf[i][0],&buf[i][1])!=EOF; ++i);
				sort(buf, buf+i, cmp1);

				fp2 = filemaker(subl, folder);
				for(int j=0; j<i; ++j)
					fprintf(fp2,"%d %d\n",buf[j][0],buf[j][1]);
				fclose(fp2);
			}
			fclose(fp);
		}

		
		inline void initialize()
		{
			int n = nr + ns;
			int i;
			
			bp = bst = bend = vector<int>(n+1);
			isempty = vector<bool>(n,false);
			FP = vector<FILE *>(n);

			for(i=0; i<=n; ++i)
			{
				bp[i] = bst[i] = i*rpb;
				bend[i] = bst[i] + rpb;
			}
			for(i=0; i<nr; ++i)
			{
				FP[i] = fileopener(i, pathr);
				isempty[i] = fillBuffer(i, FP[i]);
				rset.insert(buf[bp[i]][1]);
			}
			for(; i<n; ++i)
			{
				FP[i] = fileopener(i-nr, paths);
				isempty[i] = fillBuffer(i, FP[i]);
				sset.insert(buf[bp[i]][0]);
			}
		}

		inline void remove_tuples(int y, int idx, int l, int h, set<int> &s)
		{
			s.erase(y);
			for(int i=l; i<h; ++i)
			{
				while(1)
				{
					if(bp[i] == bend[i])
						if(!isempty[i])
							isempty[i] = fillBuffer(i, FP[i]);
					if(bp[i] == bend[i])
						break;
					if(buf[bp[i]][idx] != y)
					{
						s.insert(buf[bp[i]][idx]);
						break;
					}
					++bp[i];
				}
			}
		}

		inline void remove_tuples(int y, int idx, int l, int h, set<int> &s, vector< pair<int,int> > &v)
		{
			s.erase(y);
			for(int i=l; i<h; ++i)
			{
				while(1)
				{
					if(bp[i] == bend[i])
						if(!isempty[i])
							isempty[i] = fillBuffer(i, FP[i]);
					if(bp[i] == bend[i])
						break;
					if(buf[bp[i]][idx] != y)
					{
						s.insert(buf[bp[i]][idx]);
						break;
					}
					v.push_back(make_pair(buf[bp[i]][0], buf[bp[i]][1]));
					++bp[i];
				}
			}
		}

		inline void openx()
		{
			buf = allocx(M);
			read_sort(fopen(Rfname, "r"), nr, pathr, 1);
			read_sort(fopen(Sfname, "r"), ns, paths, 0);
			freex(buf, M);
			buf = allocx(nr+ns+1);
			initialize();
		}

		inline bool getnext(int **out)
		{

			++J;
			if(J == l2)
			{
				++I;
				if(I == l1)
					l1 = l2 = 0;
				J = 0;
			}

			if(l1 == 0 || l2 == 0)
			{
				int n = nr + ns;
				int y1;
				int y2;
				while(!rset.empty() && !sset.empty())
				{
					y1 = *rset.begin();
					y2 = *sset.begin();
					if(y1 < y2)
						remove_tuples(y1, 1, 0, nr, rset);
					else if(y1 > y2)
						remove_tuples(y2, 0, nr, n, sset);
					else
					{
						vr.clear();
						vs.clear();
						remove_tuples(y1, 1, 0, nr, rset, vr);
						remove_tuples(y2, 0, nr, n, sset, vs);
						l1 = vr.size();
						l2 = vs.size();
						break;
					}
				}
				if(l1 == 0 || l2 == 0)
					return false;
				I = J = 0;
			}
			
			out[0][0] = vr[I].first; 
			out[0][1] = vr[I].second; 
			out[1][0] = vs[J].first;
			out[1][1] = vs[J].second;

			return true;
		}

		inline void closex()
		{
			int n = nr + ns;
			freex(buf, n+1);
			for(int i=0; i<n; ++i)
				fclose(FP[i]);
		}

		inline void do_the_join()
		{
			int n = nr + ns;

			while(getnext(&buf[bp[n]]))
			{
				bp[n] += 2;
				if(bp[n] >= bend[n])
					empty_buffer(n);
			}
			empty_buffer(n);
		}
};

class HashJoin
{
	public:
		vector<FILE *> FP;
		vector<int> szr;
		vector<int> szs;
		bool isempty;
		vector< pair<int,int> >table[UNQVAL];
		int bucket;
		int IDX;
		int I;

		inline int Hash(int y)
		{
			return y%(M-1);
		}

		inline void initialize(const char *folder, vector<int> &sz)
		{
			sz = bp = bst = bend = vector<int>(M);
			FP = vector<FILE *>(M-1);
			isempty = false;

			for(int i=0; i<M-1; ++i)
				FP[i] = filemaker(i, folder);
			for(int i=0; i<M; ++i)
			{
				bp[i] = bst[i] = i*rpb;
				bend[i] = bst[i] + rpb;
			}
		}

		inline void read_hash(FILE *fp, const char *folder, int idx, vector<int> &sz)
		{
			int h;

			initialize(folder, sz);
			isempty = fillBuffer(M-1, fp);
			while(1)
			{
				if(bp[M-1] == bend[M-1])
				{
					if(!isempty)
						isempty = fillBuffer(M-1, fp);
					else break;
				}
				if(bp[M-1] == bend[M-1])
					break;
				h = Hash(buf[bp[M-1]][idx]);

				buf[bp[h]][0] = buf[bp[M-1]][0];
				buf[bp[h]][1] = buf[bp[M-1]][1];
				++bp[h];
				++sz[h];
				if(bp[h] == bend[h])
					write2disk(h, FP[h]);

				++bp[M-1];
			}
			for(int i=0; i<M-1; ++i)
			{
				write2disk(i, FP[i]);
				fclose(FP[i]);
			}
			fclose(fp);
		}

		inline void check()
		{
			for(int i=0; i<M-1; ++i)
				if(ceil(min(szr[i], szs[i])/(float)rpb) > M-2)
				{
					puts("Error: violation of min(B(Ri),B(Si)) < M-2");
					printf("i = %d\n",i);
					printf("min(B(ri),B(Si)) = %f\n",ceil(min(szr[i], szs[i])/(float)rpb));
					exit(-1);
				}
		}

		inline void make_search_str(FILE *fp, int idx)
		{
			int tup[2];

			while(fscanf(fp,"%d%d",tup,tup+1) != EOF)
				table[tup[idx]].push_back(make_pair(tup[0],tup[1]));
		}

		inline void openx()
		{
			buf = allocx(M);
			read_hash(fopen(Rfname, "r"), pathr, 1, szr);
			read_hash(fopen(Sfname, "r"), paths, 0, szs);
			freex(buf, M);
			check();

			buf = allocx(2);
			IDX = bucket = -1;
			isempty = true;
			bend[0] = bp[0] = bst[0] + rpb;
			bend[1] = (bp[1] = bst[1]) + rpb;
			FP = vector<FILE *>(2);
		}

		inline bool getnext(int **out)
		{
			while(1)
			{
				if(bp[0] == bend[0])
					if(!isempty)
						isempty = fillBuffer(0, FP[1-IDX]);
				while(bp[0] == bend[0])
				{
					if(++bucket == M-1)
						return false;

					if(szr[bucket] == 0 || szs[bucket] == 0)
						continue;

					for(int j=0; j<UNQVAL; ++j)
						table[j].clear();

					for(int i=0; i<2; ++i)
						if(FP[i] != NULL)
							fclose(FP[i]);
					FP[0] = fileopener(bucket, pathr);
					FP[1] = fileopener(bucket, paths);

					isempty = false;
					bend[0] = (bp[0] = bst[0]) + rpb;

					IDX = (szr[bucket] < szs[bucket] ? 0 : 1);
					make_search_str(FP[IDX], 1-IDX);
					isempty = fillBuffer(0, FP[1-IDX]);
					I = 0;
				}

				if(table[ buf[bp[0]][IDX] ].size() == 0)
				{
					++bp[0];
					continue;
				}

				out[0][0] = buf[bp[0]][0];
				out[0][1] = buf[bp[0]][1];
				out[1][0] = table[ buf[bp[0]][IDX] ][I].first;
				out[1][1] = table[ buf[bp[0]][IDX] ][I].second;
				if(IDX == 0)
					swap(out[0], out[1]);

				if(++I == (int)table[ buf[bp[0]][IDX] ].size())
				{
					++bp[0];
					I = 0;
				}
				return true;
			}
		}

		inline void closex()
		{
			freex(buf, 2);
		}

		inline void do_the_join()
		{
			int n = 1;

			while(getnext(&buf[bp[n]]))
			{
				bp[n] += 2;
				if(bp[n] >= bend[n])
					empty_buffer(n);
			}
			empty_buffer(n);
		}
};

int main(int argc, char *argv[])
{
	if(argc != 5)
	{
		puts("./a.out <R> <S> <sort/hash> <M>");
		return 0;
	}

	initx(argc, argv);
	//debugx();
	if(FLAG == 1)
	{
		SortMerge sm;
		sm.openx();
		sm.do_the_join();
		sm.closex();
	}
	else if(FLAG == 2)
	{
		HashJoin hj;
		hj.openx();
		hj.do_the_join();
		hj.closex();
	}

	return 0;
}
