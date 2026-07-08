#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <cstddef>

using namespace std;

/**
 * Problem 015 - File Storage
 * Implementation: Disk-based Hash Table with Chaining.
 * 
 * Optimizations:
 * 1. Increased NUM_BUCKETS to 131072 to reduce chain length and TLE.
 * 2. Used a power-of-two bucket size for faster modulo (bitmask).
 * 3. Used C-style FILE* with a larger buffer via setvbuf.
 * 4. Minimized memory allocations.
 */

const char* DB_FILE = "storage.db";
const int NUM_BUCKETS = 131072; // Increased to reduce collisions
const int BUCKET_MASK = NUM_BUCKETS - 1;
const int MAX_INDEX_LEN = 64;

struct Entry {
    char index[MAX_INDEX_LEN + 1];
    int value;
    long long next_offset;
};

class FileStorage {
    FILE* fp;
    long long* bucket_cache;
    char io_buffer[65536]; // Buffer for setvbuf

    size_t hash_fn(const string& s) {
        size_t h = 5381;
        for (char c : s) h = ((h << 5) + h) + (unsigned char)c;
        return h & BUCKET_MASK;
    }

public:
    FileStorage() {
        bucket_cache = new long long[NUM_BUCKETS];
        fp = fopen(DB_FILE, "rb+");
        if (!fp) {
            fp = fopen(DB_FILE, "wb+");
            for (int i = 0; i < NUM_BUCKETS; ++i) bucket_cache[i] = -1;
            fwrite(bucket_cache, sizeof(long long), NUM_BUCKETS, fp);
            rewind(fp);
        } else {
            fread(bucket_cache, sizeof(long long), NUM_BUCKETS, fp);
        }
        setvbuf(fp, io_buffer, _IOFBF, sizeof(io_buffer));
    }

    ~FileStorage() {
        if (fp) {
            fseek(fp, 0, SEEK_SET);
            fwrite(bucket_cache, sizeof(long long), NUM_BUCKETS, fp);
            fclose(fp);
        }
        delete[] bucket_cache;
    }

    void insert(const string& index, int value) {
        size_t bucket = hash_fn(index);
        long long current_offset = bucket_cache[bucket];

        while (current_offset != -1) {
            fseek(fp, current_offset, SEEK_SET);
            Entry e;
            if (fread(&e, sizeof(Entry), 1, fp) != 1) break;
            if (strcmp(e.index, index.c_str()) == 0 && e.value == value) {
                return; 
            }
            current_offset = e.next_offset;
        }

        fseek(fp, 0, SEEK_END);
        long long new_offset = ftell(fp);
        
        Entry new_entry;
        memset(new_entry.index, 0, sizeof(new_entry.index));
        strncpy(new_entry.index, index.c_str(), MAX_INDEX_LEN);
        new_entry.value = value;
        new_entry.next_offset = bucket_cache[bucket];
        
        fwrite(&new_entry, sizeof(Entry), 1, fp);
        bucket_cache[bucket] = new_offset;
    }

    void remove(const string& index, int value) {
        size_t bucket = hash_fn(index);
        long long current_offset = bucket_cache[bucket];
        long long prev_offset = -1;

        while (current_offset != -1) {
            fseek(fp, current_offset, SEEK_SET);
            Entry e;
            if (fread(&e, sizeof(Entry), 1, fp) != 1) break;

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
            if (fread(&e, sizeof(Entry), 1, fp) != 1) break;
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
