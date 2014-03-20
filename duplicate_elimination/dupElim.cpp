#include<iostream>
#include<cstdio>
#include<cstdlib>
#include<cstring>
#include<cmath>

#include<algorithm>
#include<vector>
#include<string>

using namespace std;

#define NAMESZ 1024
#define RSZ 16
#define BT_N 7

int FLAG;           // 1=Btree, 2=Hash
int BUFSZ;          // buffer size
int M;              // #buffers
int rpb;            // #records per buffer
int r2r;            // #records to read in M-1 buffers
int rpp;            // #records per page in hash

long long int MMSZ; // Main mem size

char inpFname[NAMESZ];
char outFname[NAMESZ];
char *buf;
char *obuf;
char *obufp;

inline void debugx()
{
	puts("________________________");
	printf("FLAG = %d\n",FLAG);
	printf("BUFSZ = %d\n",BUFSZ);
	printf("M = %d\n",M);
	printf("rpb = %d\n",rpb);
	printf("r2r = %d\n",r2r);
	printf("rpp = %d\n",rpp);
	printf("MMSZ = %lld\n",MMSZ);
	puts("________________________");
}

inline void initx(int argc, char *argv[])
{
	buf = obuf = NULL;
	FLAG = atoi(argv[1]);
	BUFSZ = atoi(argv[2]) * (1<<20);
	M = atoi(argv[3]);
	strcpy(inpFname, argv[4]);

	MMSZ = BUFSZ * M;
	rpb = BUFSZ / RSZ;
	r2r = (MMSZ - BUFSZ) / RSZ;
	rpp = 16;

	if(rpb == 0)
	{
		puts("Error: buffer size too small for a record");
		printf("rpb = %d, BUFSZ = %d, RSZ = %d\n",rpb,BUFSZ,RSZ);
		exit(-1);
	}
	
	if(r2r == 0)
	{
		puts("Error: output buffer size too small for a record");
		printf("r2r = %d, BUFSZ = %d, RSZ = %d\n",r2r,BUFSZ,RSZ);
		exit(-1);
	}
	
	if(rpp == 0)
	{
		puts("Error: page size too small for a record");
		printf("rpp = %d, BUFSZ = %d, RSZ = %d\n",rpp,BUFSZ,RSZ);
		exit(-1);
	}
}

inline void allocx()
{
	buf  = (char *)malloc(MMSZ - BUFSZ + 8);
	obuf = (char *)malloc(BUFSZ + 8);
}

inline void freex()
{
	free(buf);
	free(obuf);
}

inline int readx(FILE *fp)
{
	int c = fread(buf, RSZ, r2r, fp);
	buf[c*RSZ] = '\0';
	return c;
}

inline void flushx(int &c)
{
	obufp = obuf;
	fwrite(obufp, RSZ, c, stdout);
	c = 0;
}

inline void writex(string &x, int &c)
{
	sprintf(obufp, "%s", x.c_str());
	obufp += RSZ;
	c++;
	if(c >= rpb)
		flushx(c);
}

class node
{
	public:
		vector<string> key;
		vector<struct node *> p;
		bool isleaf;
};
node *root;

class BT
{
	public:

		unsigned int BTN;

		BT(int bt_n)
		{
			BTN = bt_n;
		}

		inline FILE *bt_open()
		{
			obufp = obuf;
			root = new node;
			root->isleaf = true;
			return fopen(inpFname, "r");
		}

		inline void bt_close(FILE *fp)
		{
			fclose(fp);
		}

		inline void bt_printx(node *r)
		{
			if(r == NULL)
				return;
			for(size_t i=0; i<r->key.size(); i++)
				cout << r->key[i];
			puts("");
			for(size_t i=0; i<r->p.size(); i++)
				bt_printx(r->p[i]);
		}

		inline void bt_insert_rec(node *r, string &x, node *p)
		{
			int idx = lower_bound(r->key.begin(), r->key.end(), x) - r->key.begin();
			r->key.insert(r->key.begin() + idx, x);

			if(r->p.empty())
				r->p.push_back(NULL);
			r->p.insert(r->p.begin() + idx + 1, p);
		}

		inline void bt_split_leaf(node *r1, node *par)
		{
			node *r2 = new node;
			r2->isleaf = true;

			int n = r1->key.size();
			int lim = ceil((BTN+1)/2.0);

			r2->p.push_back(NULL);
			for(int i=lim; i<n; i++)
			{
				r2->key.push_back(r1->key[i]);
				r2->p.push_back(r1->p[i+1]);
			}

			r1->key.resize(lim);
			r1->p.resize(lim+1);

			if(par == NULL)
			{
				par = new node;
				par->isleaf = false;
				par->p.push_back(r1);
				root = par;
			}
			bt_insert_rec(par, r2->key[0], r2);
		}

		inline void bt_split_nonleaf(node *r1, node *par)
		{
			node *r2 = new node;
			r2->isleaf = false;

			int n = r1->key.size();
			int limf = floor(BTN/2.0);
			int limc = ceil(BTN/2.0);

			for(int i=n-limf; i<n; i++)
			{
				r2->key.push_back(r1->key[i]);
				r2->p.push_back(r1->p[i]);
			}
			r2->p.push_back(r1->p[n]);

			if(par == NULL)
			{
				par = new node;
				par->isleaf = false;
				par->p.push_back(r1);
				root = par;
			}
			bt_insert_rec(par, r1->key[limc], r2);

			r1->key.resize(limc);
			r1->p.resize(limc+1);
		}

		bool bt_insert(node *r, node *par, string &x)
		{
			bool ret = false;
			if(r->isleaf)
			{
				if(find(r->key.begin(), r->key.end(), x) != r->key.end())
					return false;
				bt_insert_rec(r, x, NULL);
				ret = true;
			}
			else
			{
				if(x < r->key[0])
					ret |= bt_insert(r->p[0], r, x);
				else
				{
					int i, n = r->key.size();
					for(i=1; i<n; ++i)
						if(r->key[i-1] <= x && x < r->key[i])
						{
							ret |= bt_insert(r->p[i], r, x);
							break;
						}
					if(i == n && r->key[i-1] <= x)
						ret |= bt_insert(r->p[i], r, x);
				}
			}

			if(r->key.size() > BTN)
				(r->isleaf) ? bt_split_leaf(r, par) : bt_split_nonleaf(r, par);
			return ret;
		}

		inline bool bt_getNext(FILE *fp, string &rec2)
		{
			char *rec = NULL;

			do
			{
				rec = strtok(NULL, "\n");
				if(rec == NULL)
				{
					if(readx(fp) == 0)
						return false;
					rec = strtok(buf, "\n");
				}
				rec2 = string(rec) + "\n";
			} while(!bt_insert(root, NULL, rec2));

			return true;
		}

		inline void useBtree()
		{
			FILE *fp;
			string rec;
			int cnt = 0;

			allocx();

			fp = bt_open();
			while(bt_getNext(fp, rec))
				writex(rec, cnt);
			flushx(cnt);
			bt_close(fp);

			//bt_printx(root);
			freex();
		}
};

class Page
{
	public:
		vector<string> m;
		int d;
		
		Page()
		{
			d = 0;
		}

		inline bool fullx()
		{
			return (int)m.size() >= rpp;
		}

		inline void putx(string &x)
		{
			m.push_back(x);
		}

		inline bool findx(string &k)
		{
			int lim = m.size();
			for(int i=0; i<lim; i++)
				if(m[i] == k)
					return true;
			return false;
		}
};

class HT
{
	public:

		int gd;
		vector<Page *> ht;

		HT()
		{
			gd = 0;
			Page *p = new Page;
			ht.push_back(p);
		}
		
		inline FILE *ht_open()
		{
			obufp = obuf;
			return fopen(inpFname, "r");
		}

		inline void ht_close(FILE *fp)
		{
			fclose(fp);
		}

		inline void ht_printx()
		{
			puts("----------HT START---------------------------");
			for(size_t i=0; i<ht.size(); i++)
			{
				Page *p = ht[i];
				for(size_t j=0; j<p->m.size(); j++)
					cout << p->m[j];
				puts("");
			}
			puts("----------HT END----------------------------");
		}

		inline unsigned int Hash( const string &key)
		{
			int r   = 24;
			int len = key.size();
			unsigned int m    = 0x5bd1e995;
			unsigned int seed = 77;
			unsigned int h    = seed ^ len;
			const unsigned char * data = (const unsigned char *)strdup(key.c_str());

			while(len >= 4)
			{
				unsigned int k = *(unsigned int *)data;
				k *= m; 
				k ^= k >> r; 
				k *= m; 

				h *= m; 
				h ^= k;

				data += 4;
				len -= 4;
			}
			
			switch(len)
			{
				case 3: h ^= data[2] << 16;
				case 2: h ^= data[1] << 8;
				case 1: h ^= data[0];
					h *= m;
			};

			h ^= h >> 13;
			h *= m;
			h ^= h >> 15;

			return h;
		}

		inline Page *get_page(string &k)
		{
			unsigned int h = Hash(k);
			return ht[h & ((1<<gd) - 1)];
		}
		
		inline bool ht_insert(string &k)
		{
			Page *p = get_page(k);
			if(p->findx(k))
				return false;

			while(p->fullx())
			{
				if(p->d == gd)
				{
					vector<Page *>ht2 = ht;
					ht.reserve(ht.size() + ht2.size());
					ht.insert(ht.end(), ht2.begin(), ht2.end());
					++gd;
				}

				Page *p1 = new Page;
				Page *p2 = new Page;
				if(p->d < gd)
				{
					int lim = p->m.size();
					unsigned int h;
					for(int i=0; i<lim; i++)
					{
						h = Hash(p->m[i]) & ((1<<gd) - 1);
						(((h>>p->d) & 1) == 1) ? p2->putx(p->m[i]) : p1->putx(p->m[i]);
					}

					lim = ht.size();
					for(int i=0; i<lim; i++)
						if(ht[i] == p)
							ht[i] = ((((i>>p->d) & 1) == 1) ? p2 : p1);

					p1->d = p2->d = p->d + 1;
					delete p;
				}
				p = get_page(k);
			}
			p->putx(k);
			
			return true;
		}

		inline bool ht_getNext(FILE *fp, string &rec2)
		{
			char *rec = NULL;

			do
			{
				rec = strtok(NULL, "\n");
				if(rec == NULL)
				{
					if(readx(fp) == 0)
						return false;
					rec = strtok(buf, "\n");
				}
				rec2 = string(rec) + "\n";
			} while(!ht_insert(rec2));

			return true;
		}

		inline void useHashTable()
		{
			FILE *fp;
			string rec;
			int cnt = 0;

			allocx();

			fp = ht_open();
			while(ht_getNext(fp, rec))
				writex(rec, cnt);
			flushx(cnt);
			ht_close(fp);

			//ht_printx();
			freex();
		}
};

int main(int argc, char *argv[])
{
	if(argc != 5)
	{
		puts("Usage: ./a.out <Flag> <BufferSize(MB)> <M> <input file>");
		return 0;
	}

	initx(argc, argv);
	//debugx();
	if(FLAG == 1)
	{
		BT bt(BT_N);
		bt.useBtree();
	}
	else if(FLAG == 2)
	{
		HT ht;
		ht.useHashTable();
	}

	return 0;
}
