#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <cstdio>
#include <cstring>

using namespace std;

/**
 * Problem 015 - File Storage
 * Implementation: Disk-based Hash Table with Chaining.
 * 
 * Optimization: 
 * 1. Switched from fstream to C-style FILE* for faster binary I/O.
 * 2. Cached the bucket array in memory.
 * 3. Minimized seek operations.
 */

const char* DB_FILE = "storage.db";
const int NUM_BUCKETS = 100003; 
const int MAX_INDEX_LEN = 64;

struct Entry {
    char index[MAX_INDEX_LEN + 1];
    int value;
    long long next_offset;
};

class FileStorage {
    FILE* fp;
    vector<long long> bucket_cache;

    size_t hash_fn(const string& s) {
        size_t h = 5381;
        for (char c : s) h = ((h << 5) + h) + (unsigned char)c;
        return h % NUM_BUCKETS;
    }

public:
    FileStorage() : bucket_cache(NUM_BUCKETS, -1) {
        fp = fopen(DB_FILE, "rb+");
        if (!fp) {
            // Initialize new database
            fp = fopen(DB_FILE, "wb+");
            vector<long long> initial_buckets(NUM_BUCKETS, -1);
            fwrite(initial_buckets.data(), sizeof(long long), NUM_BUCKETS, fp);
            rewind(fp);
        } else {
            // Load bucket array into cache
            fread(bucket_cache.data(), sizeof(long long), NUM_BUCKETS, fp);
        }
    }

    ~FileStorage() {
        if (fp) {
            fseek(fp, 0, SEEK_SET);
            fwrite(bucket_cache.data(), sizeof(long long), NUM_BUCKETS, fp);
            fclose(fp);
        }
    }

    void insert(const string& index, int value) {
        size_t bucket = hash_fn(index);
        long long current_offset = bucket_cache[bucket];

        // Check if (index, value) already exists
        while (current_offset != -1) {
            fseek(fp, current_offset, SEEK_SET);
            Entry e;
            fread(&e, sizeof(Entry), 1, fp);
            if (strcmp(e.index, index.c_str()) == 0 && e.value == value) {
                return; // Already exists
            }
            current_offset = e.next_offset;
        }

        // Append new entry to the end of the file
        fseek(fp, 0, SEEK_END);
        long long new_offset = ftell(fp);
        
        Entry new_entry;
        memset(new_entry.index, 0, sizeof(new_entry.index));
        strncpy(new_entry.index, index.c_str(), MAX_INDEX_LEN);
        new_entry.value = value;
        new_entry.next_offset = bucket_cache[bucket];
        
        fwrite(&new_entry, sizeof(Entry), 1, fp);
        
        // Update bucket head in cache
        bucket_cache[bucket] = new_offset;
    }

    void remove(const string& index, int value) {
        size_t bucket = hash_fn(index);
        long long current_offset = bucket_cache[bucket];
        long long prev_offset = -1;

        while (current_offset != -1) {
            fseek(fp, current_offset, SEEK_SET);
            Entry e;
            fread(&e, sizeof(Entry), 1, fp);

            if (strcmp(e.index, index.c_str()) == 0 && e.value == value) {
                if (prev_offset == -1) {
                    bucket_cache[bucket] = e.next_offset;
                } else {
                    fseek(fp, prev_offset + offsetof(Entry, next_offset), SEEK_SET);
                    fwrite(&e.next_offset, sizeof(long long), 1, fp);
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
            fseek(fp, current_offset, SEEK_SET);
            Entry e;
            fread(&e, sizeof(Entry), 1, fp);
            if (strcmp(e.index, index.c_str()) == 0) {
                results.push_back(e.value);
            }
            current_offset = e.next_offset;
        }

        if (results.empty()) {
            printf("null\n");
        } else {
            sort(results.begin(), results.end());
            for (size_t i = 0; i < results.size(); ++i) {
                printf("%d%c", results[i], (i == results.size() - 1 ? '\n' : ' '));
            }
        }
    }
};

int main() {
    int n;
    if (scanf("%d", &n) != 1) return 0;

    FileStorage db;
    char cmd[20], index_buf[MAX_INDEX_LEN + 1];
    int value;

    for (int i = 0; i < n; ++i) {
        if (scanf("%s", cmd) != 1) break;
        if (strcmp(cmd, "insert") == 0) {
            scanf("%s %d", index_buf, &value);
            db.insert(index_buf, value);
        } else if (strcmp(cmd, "delete") == 0) {
            scanf("%s %d", index_buf, &value);
            db.remove(index_buf, value);
        } else if (strcmp(cmd, "find") == 0) {
            scanf("%s", index_buf);
            db.find(index_buf);
        }
    }

    return 0;
}
