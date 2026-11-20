// SMART DAILY TASK MANAGER (DSA project): Max Heap + Hash Table + Sorting + Search/Filter
// Compile: g++ -std=c++17 -o tasks DailyTaskManager.cpp
// Idea: the most URGENT task (highest priority) is always at the top of a Max Heap.
#include <iostream>
#include <vector>
#include <list>
#include <string>
#include <functional>
#include <algorithm>
#include <iomanip>
#include <chrono>
#include <optional>
using namespace std;
struct Task { int id; string title, category; int priority, minutes, days; };  // priority 1-100, days = days left
long long g_cmp = 0, g_swp = 0;   // counters for the operation history
int g_nextId = 1;
// ------------------------- input & output helpers -------------------------
string trim(const string& s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    return a == string::npos ? "" : s.substr(a, s.find_last_not_of(" \t\r\n") - a + 1);
}
string lower(string s) { for (char& c : s) c = (char)tolower((unsigned char)c); return s; }
string cut(const string& s, size_t w) { return s.size() > w ? s.substr(0, w - 1) + "~" : s; }
string readLine(const string& p) {
    cout << p; string s;
    if (!getline(cin, s)) { cout << "\nBye!\n"; exit(0); }
    return trim(s);
}
// Validated whole-number input. skip=true: ENTER keeps the old value (returns nullopt)
optional<int> askNum(const string& p, int lo, int hi, bool skip = false) {
    while (true) {
        string s = readLine(p);
        if (skip && s.empty()) return nullopt;
        try { size_t n; int v = stoi(s, &n); if (n == s.size() && v >= lo && v <= hi) return v; } catch (...) {}
        cout << "  [Invalid] Enter a whole number from " << lo << " to " << hi << (skip ? " (or ENTER to keep)" : "") << ".\n";
    }
}
int askInt(const string& p, int lo, int hi) { return *askNum(p, lo, hi); }
optional<string> askText(const string& p, bool skip = false) {
    while (true) {
        string s = readLine(p);
        if (skip && s.empty()) return nullopt;
        if (!s.empty() && s.size() <= 30) return s;
        cout << "  [Invalid] Text must be 1-30 characters.\n";
    }
}
void printTable(const vector<Task>& v) {
    if (v.empty()) { cout << "  (no tasks)\n"; return; }
    cout << left << setw(5) << "ID" << setw(26) << "Task" << setw(12) << "Category" << right
         << setw(10) << "Priority" << setw(9) << "Minutes" << setw(11) << "Days left" << "\n" << string(73, '-') << "\n";
    for (auto& t : v)
        cout << left << setw(5) << t.id << setw(26) << cut(t.title, 25) << setw(12) << cut(t.category, 11) << right
             << setw(10) << t.priority << setw(9) << t.minutes << setw(11) << t.days << "\n";
    cout << "  " << v.size() << " task(s)\n";
}
// ============================ HASH TABLE ============================
// Separate chaining: task ID -> index in heap array. O(1) average lookup.
class HashTable {
    vector<list<pair<int, int>>> t = vector<list<pair<int, int>>>(17);
    int count = 0;
    int h(int k) const { return k % (int)t.size(); }
public:
    void put(int k, int v) {
        for (auto& kv : t[h(k)]) if (kv.first == k) { kv.second = v; return; }
        t[h(k)].push_back({k, v});
        if (++count > (int)t.size() * 2) {                       // rehash: keep load factor <= 2
            auto old = t; t.assign(old.size() * 2 + 1, {});
            for (auto& c : old) for (auto& kv : c) t[h(kv.first)].push_back(kv);
        }
    }
    int get(int k) const { for (auto& kv : t[h(k)]) if (kv.first == k) return kv.second; return -1; }   // -1 if missing
    void erase(int k) { auto& c = t[h(k)]; for (auto it = c.begin(); it != c.end(); ++it) if (it->first == k) { c.erase(it); count--; return; } }
    void clear() { t.assign(17, {}); count = 0; }
    int buckets() const { return (int)t.size(); }
};
// ============================== MAX HEAP ==============================
class TaskHeap {
    vector<Task> a;           // a[0] = most urgent task
    HashTable pos;            // id -> index
    void place(int i) { pos.put(a[i].id, i); }
    void swapNodes(int i, int j) { swap(a[i], a[j]); place(i); place(j); g_swp++; }
    void siftUp(int i) {                                         // move up while bigger than parent
        while (i > 0) {
            int p = (i - 1) / 2; g_cmp++;
            if (a[i].priority <= a[p].priority) break;
            swapNodes(i, p); i = p;
        }
    }
    void heapify(int i, int n) {                                 // move down until heap property holds
        while (true) {
            int l = 2 * i + 1, r = l + 1, big = i;
            if (l < n) { g_cmp++; if (a[l].priority > a[big].priority) big = l; }
            if (r < n) { g_cmp++; if (a[r].priority > a[big].priority) big = r; }
            if (big == i) return;
            swapNodes(i, big); i = big;
        }
    }
public:
    void build(const vector<Task>& v) {                          // bottom-up buildMaxHeap, O(n)
        a = v; pos.clear();
        for (int i = 0; i < (int)a.size(); i++) place(i);
        for (int i = (int)a.size() / 2 - 1; i >= 0; i--) heapify(i, (int)a.size());
    }
    void clear() { a.clear(); pos.clear(); }
    void insertTask(const Task& t) { a.push_back(t); place((int)a.size() - 1); siftUp((int)a.size() - 1); }   // O(log n)
    Task extractMax() {                                          // "complete the top task", O(log n)
        Task top = a[0];
        swapNodes(0, (int)a.size() - 1); pos.erase(top.id); a.pop_back();
        heapify(0, (int)a.size());
        return top;
    }
    bool updateTask(int id, const Task& nt) {                    // hash gives index in O(1), fix heap in O(log n)
        int i = pos.get(id); if (i < 0) return false;
        int old = a[i].priority; a[i] = nt;
        if (nt.priority > old) siftUp(i); else if (nt.priority < old) heapify(i, (int)a.size());
        return true;
    }
    bool deleteTask(int id) {                                    // replace by last element, then fix, O(log n)
        int i = pos.get(id), last = (int)a.size() - 1; if (i < 0) return false;
        int old = a[i].priority;
        if (i != last) { swapNodes(i, last); pos.erase(id); a.pop_back(); if (a[i].priority > old) siftUp(i); else heapify(i, (int)a.size()); }
        else { pos.erase(id); a.pop_back(); }
        return true;
    }
    vector<Task> topK(int k) const {                             // works on a COPY: K x extractMax, O(n + k log n)
        TaskHeap tmp = *this; vector<Task> res;
        while ((int)res.size() < k && tmp.size()) res.push_back(tmp.extractMax());
        return res;
    }
    const Task* find(int id) const { int i = pos.get(id); return i < 0 ? nullptr : &a[i]; }
    const vector<Task>& data() const { return a; }
    int size() const { return (int)a.size(); }
    int buckets() const { return pos.buckets(); }
    bool valid() const { for (int i = 1; i < (int)a.size(); i++) if (a[i].priority > a[(i - 1) / 2].priority) return false; return true; }
    void showTree(int i = 0, int d = 0) const {                  // tree sideways: root left, right child on top
        if (i >= (int)a.size()) return;
        showTree(2 * i + 2, d + 1);
        cout << "   " << string(d * 8, ' ') << "[" << a[i].priority << "] " << cut(a[i].title, 14) << "\n";
        showTree(2 * i + 1, d + 1);
    }
};
// ============================== SORTING ==============================
using Cmp = function<bool(const Task&, const Task&)>;            // true if x must come BEFORE y
void mergeSort(vector<Task>& v, int l, int r, const Cmp& bf) {
    if (l >= r) return;
    int m = (l + r) / 2, i = l, j = m + 1; vector<Task> tmp;
    mergeSort(v, l, m, bf); mergeSort(v, m + 1, r, bf);
    while (i <= m && j <= r) { g_cmp++; tmp.push_back(bf(v[j], v[i]) ? v[j++] : v[i++]); }
    while (i <= m) tmp.push_back(v[i++]);
    while (j <= r) tmp.push_back(v[j++]);
    copy(tmp.begin(), tmp.end(), v.begin() + l);
}
void quickSort(vector<Task>& v, int lo, int hi, const Cmp& bf) {
    if (lo >= hi) return;
    swap(v[(lo + hi) / 2], v[hi]); g_swp++;
    int i = lo;
    for (int j = lo; j < hi; j++) { g_cmp++; if (bf(v[j], v[hi])) { swap(v[i++], v[j]); g_swp++; } }
    swap(v[i], v[hi]); g_swp++;
    quickSort(v, lo, i - 1, bf); quickSort(v, i + 1, hi, bf);
}
void heapSort(vector<Task>& v, const Cmp& bf) {
    auto down = [&](int i, int n) {
        while (true) {
            int l = 2 * i + 1, r = l + 1, big = i;
            if (l < n) { g_cmp++; if (bf(v[big], v[l])) big = l; }
            if (r < n) { g_cmp++; if (bf(v[big], v[r])) big = r; }
            if (big == i) return;
            swap(v[i], v[big]); g_swp++; i = big;
        }
    };
    int n = (int)v.size();
    for (int i = n / 2 - 1; i >= 0; i--) down(i, n);
    for (int e = n - 1; e > 0; e--) { swap(v[0], v[e]); g_swp++; down(0, e); }
}
struct Op { string name, cx; long long cmp, swp, us; };  vector<Op> history;
template <class F> void timed(const string& name, const string& cx, F f) {
    g_cmp = g_swp = 0; auto t0 = chrono::steady_clock::now(); f();
    long long us = chrono::duration_cast<chrono::microseconds>(chrono::steady_clock::now() - t0).count();
    history.push_back({name, cx, g_cmp, g_swp, us}); cout << "  [Done] comparisons=" << g_cmp << "  swaps=" << g_swp << "  time=" << us << " us\n";
}
// ============================== MENUS ==============================
void loadSample(TaskHeap& h) {
    vector<Task> s = {
        {1, "Pay electricity bill", "Finance", 90, 15, 1}, {2, "Submit assignment", "Study", 95, 120, 1}, {3, "Buy groceries", "Home", 60, 45, 2},
        {4, "Doctor appointment", "Health", 85, 60, 3}, {5, "Gym workout", "Health", 50, 60, 1}, {6, "Call parents", "Family", 70, 20, 1},
        {7, "Clean the room", "Home", 40, 40, 5}, {8, "Revise DSA notes", "Study", 80, 90, 2}, {9, "Book train tickets", "Travel", 75, 20, 4},
        {10, "Watch a movie", "Leisure", 10, 120, 7}, {11, "Recharge phone", "Finance", 65, 5, 1}, {12, "Water the plants", "Home", 30, 10, 1},
        {13, "Plan weekend trip", "Travel", 35, 50, 6}, {14, "Laundry", "Home", 45, 60, 3}};
    g_nextId = (int)s.size() + 1; h.build(s);
}
void addTask(TaskHeap& h) {
    Task t; t.id = g_nextId;
    t.title = *askText("  Task (1-30 chars): "); t.category = *askText("  Category: ");
    t.priority = askInt("  Priority (1-100, 100 = most urgent): ", 1, 100);
    t.minutes = askInt("  Time needed in minutes (1-1440): ", 1, 1440); t.days = askInt("  Days left (0-365): ", 0, 365);
    timed("insertTask", "O(log n)", [&] { h.insertTask(t); });
    cout << "  Added with ID " << g_nextId++ << "\n";
}
void updateMenu(TaskHeap& h) {
    int id = askInt("  Task ID to update: ", 1, 1000000);
    const Task* cur = h.find(id);                                // hash lookup, O(1)
    if (!cur) { cout << "  [Not found] ID " << id << "\n"; return; }
    printTable({*cur}); cout << "  (press ENTER to keep a value)\n";
    Task nt = *cur;
    if (auto v = askText("  New title: ", true)) nt.title = *v;
    if (auto v = askText("  New category: ", true)) nt.category = *v;
    if (auto v = askNum("  New priority (changes rank!): ", 1, 100, true)) nt.priority = *v;
    if (auto v = askNum("  New minutes: ", 1, 1440, true)) nt.minutes = *v;
    if (auto v = askNum("  New days left: ", 0, 365, true)) nt.days = *v;
    timed("updateTask", "O(log n)", [&] { h.updateTask(id, nt); });
}
void deleteMenu(TaskHeap& h) {
    int id = askInt("  Task ID to delete: ", 1, 1000000);
    if (!h.find(id)) { cout << "  [Not found] ID " << id << "\n"; return; }
    if (lower(readLine("  Really delete? (y/n): ")) != "y") { cout << "  Cancelled.\n"; return; }
    timed("deleteTask", "O(log n)", [&] { h.deleteTask(id); });
}
void completeTop(TaskHeap& h) {
    if (!h.size()) { cout << "  No tasks left. Enjoy your free time!\n"; return; }
    Task t; timed("extractMax", "O(log n)", [&] { t = h.extractMax(); });
    cout << "  Completed: \"" << t.title << "\" (priority " << t.priority << ")\n";
    if (h.size()) cout << "  Next up  : \"" << h.data()[0].title << "\"\n";
}
void topKMenu(TaskHeap& h) {
    if (!h.size()) { cout << "  No tasks.\n"; return; }
    int k = askInt("  How many top tasks (1-" + to_string(h.size()) + "): ", 1, h.size());
    vector<Task> res; timed("Top-K (K=" + to_string(k) + ")", "O(n + k log n)", [&] { res = h.topK(k); });
    cout << "\n  ===== TOP " << k << " URGENT TASKS =====\n"; printTable(res);
}
void searchMenu(TaskHeap& h) {
    if (askInt("  1) By ID (hash, O(1))  2) By title (O(n)): ", 1, 2) == 1) {
        int id = askInt("  ID: ", 1, 1000000); const Task* p = nullptr;
        timed("Search by ID", "O(1) avg", [&] { p = h.find(id); });
        if (p) printTable({*p}); else cout << "  Not found.\n";
    } else {
        string key = lower(*askText("  Title contains: ")); vector<Task> res;
        timed("Search by title", "O(n)", [&] { for (auto& t : h.data()) { g_cmp++; if (lower(t.title).find(key) != string::npos) res.push_back(t); } });
        printTable(res);
    }
}
void filterMenu(TaskHeap& h) {
    int c = askInt("  Filter by 1) Category  2) Max minutes  3) Due within N days: ", 1, 3);
    string cat; int lim = 0;
    if (c == 1) cat = lower(*askText("  Category: "));
    if (c == 2) lim = askInt("  Max minutes: ", 1, 1440);
    if (c == 3) lim = askInt("  Within days: ", 0, 365);
    vector<Task> res;
    timed("Filter", "O(n)", [&] {
        for (auto& t : h.data()) {
            g_cmp++;
            if ((c == 1 && lower(t.category) == cat) || (c == 2 && t.minutes <= lim) || (c == 3 && t.days <= lim)) res.push_back(t);
        }
    });
    printTable(res);
}
void sortMenu(TaskHeap& h) {
    int key = askInt("  Sort by 1) Title 2) Priority 3) Minutes 4) Days left: ", 1, 4);
    bool asc = askInt("  Order 1) Ascending 2) Descending: ", 1, 2) == 1;
    int alg = askInt("  Algorithm 1) Merge 2) Quick 3) Heap Sort: ", 1, 3);
    auto num = [=](const Task& t) { return key == 2 ? t.priority : key == 3 ? t.minutes : t.days; };
    Cmp bf = [=](const Task& x, const Task& y) {
        if (key == 1) { string a = lower(x.title), b = lower(y.title); return asc ? a < b : a > b; }
        return asc ? num(x) < num(y) : num(x) > num(y);
    };
    vector<Task> v = h.data(); int n = (int)v.size();
    string nm[] = {"Merge Sort", "Quick Sort", "Heap Sort"};
    timed(nm[alg - 1], alg == 2 ? "O(n log n) avg" : "O(n log n)", [&] {
        if (alg == 1) mergeSort(v, 0, n - 1, bf); else if (alg == 2) quickSort(v, 0, n - 1, bf); else heapSort(v, bf);
    });
    printTable(v);
}
void dashboard(TaskHeap& h) {
    int mins = 0, urgent = 0;
    for (auto& t : h.data()) { mins += t.minutes; if (t.days <= 1) urgent++; }
    cout << "\n================= DASHBOARD =================\n"
         << "  Tasks: " << h.size() << "   Total work: " << mins / 60 << "h " << mins % 60 << "m   Due today/tomorrow: " << urgent << "\n"
         << "  Hash buckets: " << h.buckets() << "   Heap property: " << (h.valid() ? "VALID" : "BROKEN") << "\n"
         << "\n  Max Heap as a tree (root left, right child on top):\n"; h.showTree();
    cout << "\n  --- Operation History (last 10) ---\n";
    if (history.empty()) cout << "   (none yet)\n";
    for (size_t i = history.size() > 10 ? history.size() - 10 : 0; i < history.size(); i++)
        cout << left << "   " << setw(4) << i + 1 << setw(20) << cut(history[i].name, 19) << setw(18) << history[i].cx << right
             << " cmp=" << setw(5) << history[i].cmp << " swaps=" << setw(4) << history[i].swp << " " << history[i].us << "us\n";
    cout << "=============================================\n";
}
int main() {
    TaskHeap heap; loadSample(heap);
    cout << "Loaded " << heap.size() << " sample tasks into the Max Heap.\n";
    while (true) {
        cout << "\n=========== SMART DAILY TASK MANAGER ===========\n"
             << "  1 Add task        5 Top-K urgent     9 Complete top task\n"
             << "  2 Update task     6 Search          10 Dashboard + heap tree\n"
             << "  3 Delete task     7 Filter          11 Load sample data\n"
             << "  4 Display all     8 Sort            12 Reset (clear all)     0 Exit\n";
        int c = askInt("Choose option: ", 0, 12); cout << "\n";
        switch (c) {
            case 1: addTask(heap); break;      case 2: updateMenu(heap); break;
            case 3: deleteMenu(heap); break;   case 4: printTable(heap.data()); break;
            case 5: topKMenu(heap); break;     case 6: searchMenu(heap); break;
            case 7: filterMenu(heap); break;   case 8: sortMenu(heap); break;
            case 9: completeTop(heap); break;  case 10: dashboard(heap); break;
            case 11: timed("Load sample data", "O(n)", [&] { loadSample(heap); }); break;
            case 12: if (lower(readLine("  Delete ALL tasks? (y/n): ")) == "y") { heap.clear(); g_nextId = 1; history.clear(); cout << "  System reset.\n"; } break;
            case 0: cout << "Goodbye!\n"; return 0;
        }
    }
}
