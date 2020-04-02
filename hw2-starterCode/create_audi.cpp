#include <algorithm>
#include <climits>
#include <cmath>
#include <numeric>
#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <bitset>
/*  Data Structures */
#include <vector>
#include <string>
#include <stack>
#include <queue>
#include <list>
#include <unordered_map>
#include <unordered_set>
#include <map>
#include <set>

using namespace std;

typedef long long ll;
typedef long double ld;
typedef vector<int> vi;
typedef vector<ll> vl;
typedef vector< vector<int> > vvi;
typedef vector< vector<ll> > vvl;
typedef set<int> si;
typedef set<ll> sl;
typedef map<int, int> mii;
typedef pair<int, int> pii;
typedef pair<ll, ll> pll;

#define REP(i, x) for (int i=0; i<x; ++i)
#define FOR(i, x, y) for (int i=x; i<y; ++i)
#define srt(x) sort(x.begin(), x.end())
#define rev(x) reverse(x.begin(), x.end())
#define inf 0x3f3f3f3f
#define INF 0x3f3f3f3f3f3f3f3fLL
#define DEBUG cerr << "======= GOT_HERE ======\n"



int main(int argc, char* argv[]) {
    ios::sync_with_stdio(false);
    ifstream input_file("hw2_core_code/splines/circle.sp");
    ofstream output_file("hw2_core_code/splines/audi.sp");
    int n, a; input_file >> n >> a;
    n -= 3;
    output_file << n * 4 << " " << a << endl;
    vector<vector<double> > vec;
    vec.resize(n);
    REP(i, n) {
        double x,y,z; input_file >> x >> y >> z;
        vec[i].push_back(x);
        vec[i].push_back(y);
        vec[i].push_back(z);
    }
    REP(i, 4) {
        REP(j, n) {
            output_file << fixed << setprecision(6) << vec[j][0] + i * 2 << " " << vec[j][1] << " " << vec[j][2] << endl;
        }
    }
    return 0;
}