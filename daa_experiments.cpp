// DAA Assignment (CIC-311): Experimental analysis of five algorithms in C++
// Algorithms: Merge sort, Quick sort (sorting), Naive and KMP (string matching), LCS (dynamic programming)
// Build : g++ -O2 -std=c++17 daa_experiments.cpp -o daa
// Run   : ./daa > results.csv
#include <bits/stdc++.h>
using namespace std;
using Clock = chrono::steady_clock;

mt19937_64 rng(12345);
volatile long long sink = 0;  // stops the compiler from deleting the work

// ---------------- 1. Merge sort ----------------
void mergeSort(vector<int>& a, vector<int>& tmp, int lo, int hi) {   // sorts a[lo..hi)
    if (hi - lo < 2) return;
    int mid = (lo + hi) / 2;
    mergeSort(a, tmp, lo, mid);
    mergeSort(a, tmp, mid, hi);
    int i = lo, j = mid, k = lo;
    while (i < mid && j < hi) tmp[k++] = (a[i] <= a[j]) ? a[i++] : a[j++];
    while (i < mid) tmp[k++] = a[i++];
    while (j < hi)  tmp[k++] = a[j++];
    for (k = lo; k < hi; k++) a[k] = tmp[k];
}
void mergeSort(vector<int>& a) { vector<int> tmp(a.size()); mergeSort(a, tmp, 0, (int)a.size()); }

// ---------------- 2. Quick sort (Lomuto, last element as pivot) ----------------
int partitionLomuto(vector<int>& a, int lo, int hi) {
    int pivot = a[hi], i = lo - 1;
    for (int j = lo; j < hi; j++)
        if (a[j] <= pivot) swap(a[++i], a[j]);
    swap(a[i + 1], a[hi]);
    return i + 1;
}
void quickSort(vector<int>& a, int lo, int hi) {
    if (lo >= hi) return;
    int p = partitionLomuto(a, lo, hi);
    quickSort(a, lo, p - 1);
    quickSort(a, p + 1, hi);
}
void quickSort(vector<int>& a) { quickSort(a, 0, (int)a.size() - 1); }

// ---------------- 3. Naive string matching ----------------
long long naiveSearch(const string& t, const string& p) {   // counts all occurrences
    long long cnt = 0; size_t n = t.size(), m = p.size();
    for (size_t i = 0; i + m <= n; i++) {
        size_t j = 0;
        while (j < m && t[i + j] == p[j]) j++;
        if (j == m) cnt++;
    }
    return cnt;
}

// ---------------- 4. KMP ----------------
vector<int> buildLPS(const string& p) {
    vector<int> lps(p.size(), 0);
    for (size_t i = 1, len = 0; i < p.size();) {
        if (p[i] == p[len]) lps[i++] = ++len;
        else if (len) len = lps[len - 1];
        else lps[i++] = 0;
    }
    return lps;
}
long long kmpSearch(const string& t, const string& p) {
    vector<int> lps = buildLPS(p);
    long long cnt = 0; size_t j = 0;
    for (size_t i = 0; i < t.size(); i++) {
        while (j && t[i] != p[j]) j = lps[j - 1];
        if (t[i] == p[j]) j++;
        if (j == p.size()) { cnt++; j = lps[j - 1]; }
    }
    return cnt;
}

// ---------------- 5. LCS (dynamic programming) ----------------
int lcsTable(const string& a, const string& b) {            // full table: used for the correctness check
    vector<vector<int>> L(a.size() + 1, vector<int>(b.size() + 1, 0));
    for (size_t i = 1; i <= a.size(); i++)
        for (size_t j = 1; j <= b.size(); j++)
            L[i][j] = (a[i-1] == b[j-1]) ? L[i-1][j-1] + 1 : max(L[i-1][j], L[i][j-1]);
    return L[a.size()][b.size()];
}
int lcsFast(const string& a, const string& b) {             // two rows only: same O(mn) time, O(n) space
    vector<int> prev(b.size() + 1, 0), cur(b.size() + 1, 0);
    for (size_t i = 1; i <= a.size(); i++) {
        for (size_t j = 1; j <= b.size(); j++)
            cur[j] = (a[i-1] == b[j-1]) ? prev[j-1] + 1 : max(prev[j], cur[j-1]);
        swap(prev, cur);
    }
    return prev[b.size()];
}

// ---------------- helpers ----------------
vector<int> randomArray(int n) { vector<int> a(n); for (int& x : a) x = (int)(rng() % 1000000000); return a; }
string randomString(size_t n, int alpha) { string s(n, 'a'); for (char& c : s) c = 'a' + rng() % alpha; return s; }
double ms(Clock::time_point s, Clock::time_point e) { return chrono::duration<double, milli>(e - s).count(); }
void row(const string& alg, const string& cs, long long n, double r1, double r2, double r3) {
    printf("%s,%s,%lld,%.3f,%.3f,%.3f\n", alg.c_str(), cs.c_str(), n, r1, r2, r3);
    fflush(stdout);
}

template <class Sorter>
void benchSort(const string& name, const string& cs, vector<int> sizes, int kind, Sorter sorter) {
    for (int n : sizes) {
        vector<int> base = randomArray(n);
        if (kind == 1) sort(base.begin(), base.end());                      // already sorted
        double t[3];
        for (int r = 0; r < 3; r++) {
            vector<int> a = base;
            auto s = Clock::now(); sorter(a); auto e = Clock::now();
            t[r] = ms(s, e);
            if (!is_sorted(a.begin(), a.end())) { fprintf(stderr, "SORT WRONG\n"); exit(1); }
        }
        row(name, cs, n, t[0], t[1], t[2]);
    }
}

template <class Matcher>
void benchMatch(const string& name, const string& cs, vector<long long> sizes, bool worst, Matcher f) {
    const size_t m = 50;
    for (long long n : sizes) {
        string text, pat;
        if (worst) { text = string(n, 'a'); pat = string(m - 1, 'a') + 'b'; }   // naive worst case
        else       { text = randomString(n, 26); pat = randomString(m, 26); }
        double t[3];
        for (int r = 0; r < 3; r++) {
            auto s = Clock::now(); long long c = f(text, pat); auto e = Clock::now();
            t[r] = ms(s, e); sink = c;
        }
        row(name, cs, n, t[0], t[1], t[2]);
    }
}

int main() {
    // ---------- correctness checks (printed to stderr) ----------
    for (int n : {0, 1, 2, 10, 1000, 50000}) {
        vector<int> a = randomArray(n), b = a, c = a, ref = a;
        sort(ref.begin(), ref.end());
        mergeSort(b); quickSort(c);
        if (b != ref || c != ref) { fprintf(stderr, "sort check FAILED n=%d\n", n); return 1; }
    }
    fprintf(stderr, "[OK] Merge sort and Quick sort match std::sort on 6 test inputs\n");
    for (int it = 0; it < 200; it++) {
        string t = randomString(2000, 3), p = randomString(1 + rng() % 4, 3);
        long long ref = 0;
        for (size_t pos = t.find(p); pos != string::npos; pos = t.find(p, pos + 1)) ref++;
        if (naiveSearch(t, p) != ref || kmpSearch(t, p) != ref) { fprintf(stderr, "match check FAILED\n"); return 1; }
    }
    fprintf(stderr, "[OK] Naive and KMP agree with std::string::find on 200 random tests\n");
    for (int it = 0; it < 50; it++) {
        string a = randomString(300, 4), b = randomString(250, 4);
        if (lcsTable(a, b) != lcsFast(a, b)) { fprintf(stderr, "lcs check FAILED\n"); return 1; }
    }
    if (lcsFast("AGGTAB", "GXTXAYB") != 4) { fprintf(stderr, "lcs sample FAILED\n"); return 1; }
    fprintf(stderr, "[OK] LCS: two-row version equals full-table version; LCS(AGGTAB,GXTXAYB)=4\n");

    printf("algorithm,case,n,run1_ms,run2_ms,run3_ms\n");
    vector<int> big = {100000, 200000, 400000, 800000, 1600000};
    vector<int> small = {4000, 8000, 16000, 32000, 64000};
    // Merge sort
    benchSort("Merge", "random", big, 0, [](vector<int>& a){ mergeSort(a); });
    benchSort("Merge", "sorted", big, 1, [](vector<int>& a){ mergeSort(a); });
    // Quick sort
    benchSort("Quick", "random", big, 0, [](vector<int>& a){ quickSort(a); });
    benchSort("Quick", "sorted", small, 1, [](vector<int>& a){ quickSort(a); });
    // String matching
    vector<long long> tsz = {1000000, 2000000, 4000000, 8000000, 16000000};
    benchMatch("Naive", "random", tsz, false, naiveSearch);
    benchMatch("KMP",   "random", tsz, false, kmpSearch);
    benchMatch("Naive", "worst",  tsz, true,  naiveSearch);
    benchMatch("KMP",   "worst",  tsz, true,  kmpSearch);
    // LCS
    for (int n : {1000, 2000, 4000, 8000, 16000}) {
        string a = randomString(n, 26), b = randomString(n, 26);
        double t[3];
        for (int r = 0; r < 3; r++) {
            auto s = Clock::now(); int L = lcsFast(a, b); auto e = Clock::now();
            t[r] = ms(s, e); sink = L;
        }
        row("LCS", "random", n, t[0], t[1], t[2]);
    }
    return 0;
}
