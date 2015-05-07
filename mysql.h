#pragma once
#include "db.h"
#include <mysql/mysql.h>

class MySQL: public Database{
    private:
        MYSQL *db_;
        MYSQL_STMT *add_snapshot_;
        MYSQL_STMT *add_reason_malloc_;
        MYSQL_STMT *add_reason_free_;
        MYSQL_STMT *add_reason_calloc_;
        MYSQL_STMT *add_reason_realloc_;
        MYSQL_STMT *add_memory_write_;
        MYSQL_STMT *create_new_block_;
        MYSQL_STMT *free_block_;

        // Helper functions
        void createNewBlock(
            snapshot_t snapshot_id, void *address, size_t size);
        void freeBlock(
            snapshot_t snapshot_id, void *address);

    public:
        MySQL(const char *host, const char *user,
            const char *password, const char *db);
        ~MySQL();

        // Write snapshot, returns the id of the new snapshot
        snapshot_t addSnapshot(snapshot_reason::type_t type);

        // Adds the reason to a snapshot
        void addReasonMalloc(snapshot_t snapshot_id, void *mem, size_t size);
        void addReasonFree(snapshot_t snapshot_id, void *mem);
        void addReasonCalloc(snapshot_t snapshot_id, void *mem,
            size_t nmemb, size_t size);
        void addReasonRealloc(snapshot_t snapshot_id, void *old_mem,
            void *new_mem, size_t new_size);

        // Add memory write operations
        void addMemoryWrites(snapshot_t snapshot_id, const std::map<void *,
            unsigned char>& writes);
};
