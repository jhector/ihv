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
        void create_new_block(
            snapshot_t snapshot_id, void *address, size_t size);
        void free_block(
            snapshot_t snapshot_id, void *address);

    public:
        MySQL(const char *host, const char *user,
            const char *password, const char *db);
        ~MySQL();

        // Write snapshot, returns the id of the new snapshot
        snapshot_t add_snapshot(snapshot_reason::type_t type);

        // Adds the reason to a snapshot
        void add_reason_malloc(snapshot_t snapshot_id, void *mem, size_t size);
        void add_reason_free(snapshot_t snapshot_id, void *mem);
        void add_reason_calloc(snapshot_t snapshot_id, void *mem,
            size_t nmemb, size_t size);
        void add_reason_realloc(snapshot_t snapshot_id, void *old_mem,
            void *new_mem, size_t new_size);

        // Add memory write operations
        void add_memory_writes(snapshot_t snapshot_id, const std::map<void *,
            unsigned char>& writes);
};
