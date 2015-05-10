#pragma once
#include <stdint.h>
#include <cstdlib>
#include <map>

typedef unsigned int snapshot_t;

namespace snapshot_reason {
enum type_t{
    MALLOC = 1, FREE, CALLOC, REALLOC, MEM_WRITE
};
};

class Database{
    public:
        Database() {};
        ~Database() {};

        // Write snapshot, returns the id of the new snapshot
        virtual snapshot_t addSnapshot(snapshot_reason::type_t type) = 0;

        // Adds the reason to a snapshot
        virtual void addReasonMalloc(
            snapshot_t snapshot_id, void *mem, size_t size) = 0;
        virtual void addReasonFree(
            snapshot_t snapshot_id, void *mem) = 0;
        virtual void addReasonCalloc(
            snapshot_t snapshot_id, void *mem, size_t nmemb, size_t size) = 0;
        virtual void addReasonRealloc(
            snapshot_t snapshot_id, void *old_mem, void *new_mem,
            size_t new_size) = 0;

        // Add memory write operations
        virtual void addMemoryWrites(
            snapshot_t snapshot_id,
            const std::map<void *, unsigned char>& writes) = 0;
};
