#pragma once
#include <stdint.h>
#include <cstdlib>
#include <map>

typedef unsigned int snapshot_t;

namespace snapshot_reason{
enum type_t{
    MALLOC, FREE, CALLOC, REALLOC, MEM_WRITE
};
};

class Database{
    public:
        Database() {};
        ~Database() {};

        // Write snapshot, returns the id of the new snapshot
        virtual snapshot_t add_snapshot(snapshot_reason::type_t type) = 0;

        // Adds the reason to a snapshot
        virtual void add_reason_malloc(
            snapshot_t snapshot_id, void *mem, size_t size) = 0;
        virtual void add_reason_free(
            snapshot_t snapshot_id, void *mem) = 0;
        virtual void add_reason_calloc(
            snapshot_t snapshot_id, void *mem, size_t nmemb, size_t size) = 0;
        virtual void add_reason_realloc(
            snapshot_t snapshot_id, void *old_mem, void *new_mem,
            size_t new_size) = 0;

        // Add memory write operations
        virtual void add_memory_writes(
            snapshot_t snapshot_id,
            std::map<void *, unsigned char>& writes) = 0;
};
