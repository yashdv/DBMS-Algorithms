#include<iostream>
#include<cstdio>
#include<cstdlib>
#include<cstring>
#include<cmath>
#include<climits>
#include<sstream>

#include<algorithm>
#include<string>
#include<vector>
#include<set>
#include<map>
#include<utility>
#include<stack>
#include<queue>
#include<deque>
#include<list>
#include<bitset>

using namespace std;

typedef vector<int> vi; 
typedef vector<vi> vvi; 
typedef vector<string> vs; 
typedef pair<int,int> ii;
typedef long long int LLI;
typedef unsigned long long int ULLI;

#define sz(a)                        int((a).size()) 
#define pb                           push_back 
#define mp                           make_pair
#define F                            first
#define S                            second
#define present(c,x)                 ((c).find(x) != (c).end()) 
#define cpresent(c,x)                (find(all(c),x) != (c).end())
#define tr(c,i)                      for(typeof((c).begin()) i = (c).begin(); i != (c).end(); i++)
#define all(c)                       (c).begin(),(c).end()
#define si(n)                        scanf("%d",&n)
#define sl(n)                        scanf("%lld",&n)
#define sf(n)                        scanf("%f",&n)
#define sd(n)                        scanf("%lf",&n)
#define ss(n)                        scanf("%s",n)

#define abs(x)                       ((x)<0?-(x):(x))
#define fill(a,v)                    memset((a),(v),sizeof (a))
#define INF                          INT_MAX
#define LINF                         (long long)1e18
#define EPS                          1e-9
#define MAX

int n = 1 << 23;
char s[128][20];

int main(int argc, char *argv[])
{
	int c = 0;
	int r = atoi(argv[2]);
	FILE *fp = fopen(argv[1],"r");

	while(fread(s[c], 16, 1, fp) != 0)
	{
		printf("%s",s[c++]);
		n--;
	}
	
	for(int i=0; i<n;)
		for(int j=0; j<r && i<n; j++, i++)
			printf("%s",s[j]);
	return 0;
}
