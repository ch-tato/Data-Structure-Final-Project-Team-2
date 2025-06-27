// compile with: g++ compare_bptree_hashmap.cpp -o compare.exe -std=c++17 -lpsapi

#include <bits/stdc++.h>
#include <windows.h>
#include <psapi.h>
using namespace std;
using ll = long long;
#define QUERY_RANGE 100 // set query range
#define ORDER 4 // set bpt order

size_t getCurrentRSS() {
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
        return pmc.WorkingSetSize;
    }
    return 0;
}

class BPlusTree {
    struct Node {
        bool leaf;
        vector<int> keys;
        vector<Node*> children;
        vector<int> values;
        Node* next;
        Node(bool leaf): leaf(leaf), next(nullptr) {}
    };
    Node* root;
    int order;
public:
    BPlusTree(int order = ORDER): root(nullptr), order(order) {} // order
    ~BPlusTree() { clear(root); }

    void clear(Node* node) {
        if (!node) return;
        if (!node->leaf) {
            for (auto c: node->children) clear(c);
        }
        delete node;
    }

    int search(int key) {
        Node* node = root;
        while (node && !node->leaf) {
            int i = upper_bound(node->keys.begin(), node->keys.end(), key) - node->keys.begin();
            node = node->children[i];
        }
        if (!node) return INT_MIN;
        auto it = lower_bound(node->keys.begin(), node->keys.end(), key);
        if (it != node->keys.end() && *it == key) {
            int idx = it - node->keys.begin();
            return node->values[idx];
        }
        return INT_MIN;
    }

    vector<pair<int,int>> rangeSearch(int low, int high) {
        vector<pair<int,int>> res;
        Node* node = root;
        if (!node) return res;
        while (!node->leaf) {
            int i = upper_bound(node->keys.begin(), node->keys.end(), low) - node->keys.begin();
            node = node->children[i];
        }
        while (node) {
            for (int i = 0; i < node->keys.size(); i++) {
                if (node->keys[i] >= low && node->keys[i] <= high) {
                    res.emplace_back(node->keys[i], node->values[i]);
                }
            }
            if (node->keys.empty() || node->keys.back() > high) break;
            node = node->next;
        }
        return res;
    }

    void insert(int key, int value) {
        if (!root) {
            root = new Node(true);
            root->keys.push_back(key);
            root->values.push_back(value);
            return;
        }
        auto splitInfo = insertInternal(root, key, value);
        if (splitInfo.first) {
            Node* newRoot = new Node(false);
            newRoot->keys.push_back(splitInfo.second.first);
            newRoot->children.push_back(root);
            newRoot->children.push_back(splitInfo.second.second);
            root = newRoot;
        }
    }

    using InsertResult = std::pair<bool, std::pair<int, Node*>>;
    InsertResult insertInternal(Node* node, int key, int value) {
        if (node->leaf) {

            auto insert_pos = std::lower_bound(node->keys.begin(), node->keys.end(), key);
            int idx = insert_pos - node->keys.begin();

            if (insert_pos != node->keys.end() && *insert_pos == key) {
                node->values[idx] = value;
                return {false, {0, nullptr}};
            }

            node->keys.insert(insert_pos, key);
            node->values.insert(node->values.begin() + idx, value);

            if (node->keys.size() < order) {
                return {false, {0, nullptr}};
            }

            int split_point = node->keys.size() / 2;
            Node* new_sibling = new Node(true);
            
            new_sibling->keys.assign(node->keys.begin() + split_point, node->keys.end());
            new_sibling->values.assign(node->values.begin() + split_point, node->values.end());
            
            node->keys.resize(split_point);
            node->values.resize(split_point);
            
            new_sibling->next = node->next;
            node->next = new_sibling;

            int promoted_key = new_sibling->keys.front();
            return {true, {promoted_key, new_sibling}};
        }

        auto child_pos = std::upper_bound(node->keys.begin(), node->keys.end(), key);
        int child_idx = child_pos - node->keys.begin();

        InsertResult child_result = insertInternal(node->children[child_idx], key, value);
        if (!child_result.first) {
            return child_result; 
        }

        int promoted_key = child_result.second.first;
        Node* new_child = child_result.second.second;

        auto key_pos = std::upper_bound(node->keys.begin(), node->keys.end(), promoted_key);
        int insert_idx = key_pos - node->keys.begin();

        node->keys.insert(key_pos, promoted_key);
        node->children.insert(node->children.begin() + insert_idx + 1, new_child);

        if (node->keys.size() < order) {
            return {false, {0, nullptr}};
        }

        int split_point = node->keys.size() / 2;
        Node* new_sibling = new Node(false);

        new_sibling->keys.assign(node->keys.begin() + split_point + 1, node->keys.end());
        new_sibling->children.assign(node->children.begin() + split_point + 1, node->children.end());

        int parent_key = node->keys[split_point];
        node->keys.resize(split_point);
        node->children.resize(split_point + 1);
        
        return {true, {parent_key, new_sibling}};
    }

    bool update(int key, int newValue) {
        Node* node = root;
        while (node && !node->leaf) {
            int i = upper_bound(node->keys.begin(), node->keys.end(), key) - node->keys.begin();
            node = node->children[i];
        }
        if (!node) return false;
        auto it = lower_bound(node->keys.begin(), node->keys.end(), key);
        if (it != node->keys.end() && *it == key) {
            int idx = it - node->keys.begin();
            node->values[idx] = newValue;
            return true;
        }
        return false;
    }

    // note: remove key from leaf, no rebalancing (may cause undeflow)
    bool remove(int key) {
        Node* node = root;
        if (!node) return false;
        vector<Node*> path;
        while (!node->leaf) {
            path.push_back(node);
            int i = upper_bound(node->keys.begin(), node->keys.end(), key) - node->keys.begin();
            node = node->children[i];
        }
        auto it = lower_bound(node->keys.begin(), node->keys.end(), key);
        if (it == node->keys.end() || *it != key) return false;
        int idx = it - node->keys.begin();
        node->keys.erase(it);
        node->values.erase(node->values.begin() + idx);
        return true;
    }
};

void testBPlusTree(const vector<int>& data) {
    BPlusTree tree(ORDER); // set order
    size_t mem_before = getCurrentRSS();
    auto t1 = chrono::high_resolution_clock::now();
    // insert
    for (int v: data) tree.insert(v, v);
    auto t2 = chrono::high_resolution_clock::now();
    size_t mem_after_insert = getCurrentRSS();
    // exact search
    auto t3 = chrono::high_resolution_clock::now();
    for (int v: data) tree.search(v);
    auto t4 = chrono::high_resolution_clock::now();
    // range search
    mt19937 rng(123);
    uniform_int_distribution<int> dist(0, data.size()-1);
    vector<pair<int,int>> ranges;
    for (int i = 0; i < QUERY_RANGE; i++) { 
        int a = data[dist(rng)];
        int b = data[dist(rng)];
        if (a > b) swap(a,b);
        ranges.emplace_back(a,b);
    }
    auto t5 = chrono::high_resolution_clock::now();
    for (auto &r: ranges) tree.rangeSearch(r.first, r.second);
    auto t6 = chrono::high_resolution_clock::now();
    // update: change value to value+1
    auto t7 = chrono::high_resolution_clock::now();
    for (int v: data) tree.update(v, v+1);
    auto t8 = chrono::high_resolution_clock::now();
    // delete
    auto t9 = chrono::high_resolution_clock::now();
    for (int v: data) tree.remove(v);
    auto t10 = chrono::high_resolution_clock::now();
    // output
    cout << "B+ Tree with " << data.size() << " items:\n";
    cout << "  Insert time: " << chrono::duration<double, milli>(t2-t1).count() << " ms, Memory delta: " << (mem_after_insert - mem_before)/1024.0 << " KB\n";
    cout << "  Exact search time: " << chrono::duration<double, milli>(t4-t3).count() << " ms\n";
    cout << "  Range search time (" << QUERY_RANGE << " queries): " << chrono::duration<double, milli>(t6-t5).count() << " ms\n";
    cout << "  Update time: " << chrono::duration<double, milli>(t8-t7).count() << " ms\n";
    cout << "  Delete time: " << chrono::duration<double, milli>(t10-t9).count() << " ms\n";
}

void testHashMap(const vector<int>& data) {
    unordered_map<int,int> mp;
    size_t mem_before = getCurrentRSS();
    auto t1 = chrono::high_resolution_clock::now();
    // insert
    for (int v: data) mp[v] = v;
    auto t2 = chrono::high_resolution_clock::now();
    size_t mem_after_insert = getCurrentRSS();
    // exact search
    auto t3 = chrono::high_resolution_clock::now();
    for (int v: data) {
        volatile int x = mp[v]; (void)x;
    }
    auto t4 = chrono::high_resolution_clock::now();
    // range search: scan all keys for each range
    mt19937 rng(123);
    uniform_int_distribution<int> dist(0, data.size()-1);
    vector<pair<int,int>> ranges;
    for (int i = 0; i < QUERY_RANGE; i++) {
        int a = data[dist(rng)];
        int b = data[dist(rng)];
        if (a > b) swap(a,b);
        ranges.emplace_back(a,b);
    }
    auto t5 = chrono::high_resolution_clock::now();
    for (auto &r: ranges) {
        for (auto &p: mp) {
            if (p.first >= r.first && p.first <= r.second) {
                volatile int y = p.second; (void)y;
            }
        }
    }
    auto t6 = chrono::high_resolution_clock::now();
    // update
    auto t7 = chrono::high_resolution_clock::now();
    for (int v: data) mp[v] = v+1;
    auto t8 = chrono::high_resolution_clock::now();
    // delete
    auto t9 = chrono::high_resolution_clock::now();
    for (int v: data) mp.erase(v);
    auto t10 = chrono::high_resolution_clock::now();
    // output
    cout << "Hash Map with " << data.size() << " items:\n";
    cout << "  Insert time: " << chrono::duration<double, milli>(t2-t1).count() << " ms, Memory delta: " << (mem_after_insert - mem_before)/1024.0 << " KB\n";
    cout << "  Exact search time: " << chrono::duration<double, milli>(t4-t3).count() << " ms\n";
    cout << "  Range search time (" << QUERY_RANGE << " queries): " << chrono::duration<double, milli>(t6-t5).count() << " ms\n";
    cout << "  Update time: " << chrono::duration<double, milli>(t8-t7).count() << " ms\n";
    cout << "  Delete time: " << chrono::duration<double, milli>(t10-t9).count() << " ms\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        cout << "Usage: compare.exe [bptree|hashmap]" << endl;
        return 0;
    }
    string mode = argv[1];
    vector<int> sizes = {100, 500, 1000};
    for (int n: sizes) {
        vector<int> data(n);
        iota(data.begin(), data.end(), 1);
        shuffle(data.begin(), data.end(), mt19937(chrono::high_resolution_clock::now().time_since_epoch().count()));
        if (mode == "bptree") {
            testBPlusTree(data);
        } else if (mode == "hashmap") {
            testHashMap(data);
        } else {
            cout << "Invalid mode. Use 'bptree' or 'hashmap'." << endl;
            return 0;
        }
        cout << endl;
    }
    return 0;
}
