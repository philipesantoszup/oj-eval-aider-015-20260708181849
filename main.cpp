#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <fstream>
#include <cstring>

using namespace std;

/**
 * Problem 015 - File Storage
 * Implementation: Disk-based Hash Table with Chaining.
 * 
 * Optimization: Cache the bucket array in memory to reduce disk I/O.
 */

const string DB_FILE = "storage.db";
const int NUM_BUCKETS = 100003; 
const int MAX_INDEX_LEN = 64;

struct Entry {
    char index[MAX_INDEX_LEN + 1];
    int value;
    long long next_offset;
};

class FileStorage {
    fstream file;
    vector<long long> bucket_cache;

    size_t hash_fn(const string& s) {
        size_t h = 5381;
        for (char c : s) h = ((h << 5) + h) + c;
        return h % NUM_BUCKETS;
    }

public:
    FileStorage() : bucket_cache(NUM_BUCKETS, -1) {
        file.open(DB_FILE, ios::in | ios::out | ios::binary);
        if (!file) {
            // Initialize new database
            file.open(DB_FILE, ios::out | ios::binary);
            vector<long long> initial_buckets(NUM_BUCKETS, -1);
            file.write(reinterpret_cast<const char*>(initial_buckets.data()), NUM_BUCKETS * sizeof(long long));
            file.close();
            file.open(DB_FILE, ios::in | ios::out | ios::binary);
        } else {
            // Load bucket array into cache
            file.seekg(0, ios::beg);
            file.read(reinterpret_cast<char*>(bucket_cache.data()), NUM_BUCKETS * sizeof(long long));
        }
    }

    ~FileStorage() {
        if (file.is_open()) {
            // Sync cache back to disk
            file.seekp(0, ios::beg);
            file.write(reinterpret_cast<const char*>(bucket_cache.data()), NUM_BUCKETS * sizeof(long long));
            file.close();
        }
    }

    void insert(const string& index, int value) {
        size_t bucket = hash_fn(index);
        long long current_offset = bucket_cache[bucket];

        // Check if (index, value) already exists
        while (current_offset != -1) {
            file.seekg(current_offset);
            Entry e;
            file.read(reinterpret_cast<char*>(&e), sizeof(Entry));
            if (strcmp(e.index, index.c_str()) == 0 && e.value == value) {
                return; // Already exists
            }
            current_offset = e.next_offset;
        }

        // Append new entry to the end of the file
        file.seekp(0, ios::end);
        long long new_offset = file.tellp();
        
        Entry new_entry;
        memset(new_entry.index, 0, sizeof(new_entry.index));
        strncpy(new_entry.index, index.c_str(), MAX_INDEX_LEN);
        new_entry.value = value;
        
        // New entry points to the old head of the bucket
        new_entry.next_offset = bucket_cache[bucket];
        
        file.write(reinterpret_cast<const char*>(&new_entry), sizeof(Entry));
        
        // Update bucket head in cache
        bucket_cache[bucket] = new_offset;
    }

    void remove(const string& index, int value) {
        size_t bucket = hash_fn(index);
        long long current_offset = bucket_cache[bucket];
        long long prev_offset = -1;

        while (current_offset != -1) {
            file.seekg(current_offset);
            Entry e;
            file.read(reinterpret_cast<char*>(&e), sizeof(Entry));

            if (strcmp(e.index, index.c_str()) == 0 && e.value == value) {
                if (prev_offset == -1) {
                    // Removing the head
                    bucket_cache[bucket] = e.next_offset;
                } else {
                    // Removing from middle/end
                    file.seekp(prev_offset + offsetof(Entry, next_offset));
                    file.write(reinterpret_cast<const char*>(&e.next_offset), sizeof(long long));
                }
                return;
            }
            prev_offset = current_offset;
            current_offset = e.next_offset;
        }
    }

    void find(const string& index) {
        size_t bucket = hash_fn(index);
        long long current_offset = bucket_cache[bucket];
        vector<int> results;

        while (current_offset != -1) {
            file.seekg(current_offset);
            Entry e;
            file.read(reinterpret_cast<char*>(&e), sizeof(Entry));
            if (strcmp(e.index, index.c_str()) == 0) {
                results.push_back(e.value);
            }
            current_offset = e.next_offset;
        }

        if (results.empty()) {
            cout << "null" << endl;
        } else {
            sort(results.begin(), results.end());
            for (size_t i = 0; i < results.size(); ++i) {
                cout << results[i] << (i == results.size() - 1 ? "" : " ");
            }
            cout << endl;
        }
    }
};

int main() {
    ios_base::sync_with_stdio(false);
    cin.tie(NULL);

    int n;
    if (!(cin >> n)) return 0;

    FileStorage db;
    string cmd, index;
    int value;

    for (int i = 0; i < n; ++i) {
        if (!(cin >> cmd)) break;
        if (cmd == "insert") {
            cin >> index >> value;
            db.insert(index, value);
        } else if (cmd == "delete") {
            cin >> index >> value;
            db.remove(index, value);
        } else if (cmd == "find") {
            cin >> index;
            db.find(index);
        }
    }

    return 0;
}
