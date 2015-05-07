#include "mysql.h"

#include <mysql/mysql.h>
#include <cstring>
#include <iostream>
#include <stdexcept>


// Prepared statement templates
const char *query_add_snapshot =
    "INSERT INTO snapshot(reason) VALUES (?);";

const char *query_add_reason_malloc =
    "INSERT INTO reason_malloc(snapshot_id, address, size)"
    " VALUES(?, ?, ?);";

const char *query_add_reason_free =
    "INSERT INTO reason_free(snapshot_id, address) VALUES (?, ?);";

const char *query_add_reason_calloc =
    "INSERT INTO reason_calloc(snapshot_id, address, member, size)"
    " VALUES (?, ?, ?, ?);";

const char *query_add_reason_realloc =
    "INSERT INTO reason_realloc(snapshot_id, old_address, new_address, size);";

const char *query_add_memory_write =
    "INSERT INTO memory_write(snapshot_id, address, value) VALUES (?, ?, ?);";

const char *query_create_new_block =
    "INSERT INTO block(alloc_snapshot_id, address, size) VALUES (?, ?, ?);";

const char *query_free_block =
    "UPDATE block SET free_snapshot_id = ? WHERE address = ?"
    " AND free_snapshot_id = NULL";


// Raise an exception in the case of an error
void fatal_error(MYSQL *db)
{
    std::cerr << "[FATAL] MySQL error: " << mysql_error(db) << std::endl;
    throw std::runtime_error(mysql_error(db));
}

MySQL::MySQL(
    const char *host, const char *user, const char *password, const char *db)
{
    if (!mysql_real_connect(db_, host, user, password, db, 0, NULL, 0))
            fatal_error(db_);

    // Initialize prepared statements
    add_snapshot_ = mysql_stmt_init(db_);
    add_reason_malloc_ = mysql_stmt_init(db_);
    add_reason_free_ = mysql_stmt_init(db_);
    add_reason_calloc_ = mysql_stmt_init(db_);
    add_reason_realloc_ = mysql_stmt_init(db_);
    add_memory_write_ = mysql_stmt_init(db_);
    create_new_block_ = mysql_stmt_init(db_);
    free_block_ = mysql_stmt_init(db_);

    if(!add_snapshot_ || !mysql_stmt_prepare(
            add_snapshot_, query_add_snapshot, strlen(query_add_snapshot)))
        fatal_error(db_);

    if(!add_reason_malloc_ || !mysql_stmt_prepare(
            add_reason_malloc_, query_add_reason_malloc,
            strlen(query_add_reason_malloc)))
        fatal_error(db_);

    if(!add_reason_free_ || !mysql_stmt_prepare(
            add_reason_free_, query_add_reason_free,
            strlen(query_add_reason_free)))
        fatal_error(db_);

    if(!add_reason_calloc_ || !mysql_stmt_prepare(
            add_reason_calloc_, query_add_reason_calloc,
            strlen(query_add_reason_calloc)))
        fatal_error(db_);

    if(!add_reason_realloc_ || !mysql_stmt_prepare(
            add_reason_realloc_, query_add_reason_realloc,
            strlen(query_add_reason_realloc)))
        fatal_error(db_);

    if(!add_memory_write_ || !mysql_stmt_prepare(
            add_memory_write_, query_add_memory_write,
            strlen(query_add_memory_write)))
        fatal_error(db_);

    if(!create_new_block_ || !mysql_stmt_prepare(
        create_new_block_, query_create_new_block,
            strlen(query_create_new_block)))
        fatal_error(db_);

    if(!free_block_ || !mysql_stmt_prepare(
        free_block_, query_free_block,
            strlen(query_free_block)))
        fatal_error(db_);
}

MySQL::~MySQL()
{
    mysql_close(db_);
    mysql_stmt_close(add_snapshot_);
    mysql_stmt_close(add_reason_malloc_);
    mysql_stmt_close(add_reason_free_);
    mysql_stmt_close(add_reason_calloc_);
    mysql_stmt_close(add_reason_realloc_);
    mysql_stmt_close(add_memory_write_);
}

snapshot_t MySQL::add_snapshot(snapshot_reason::type_t type)
{
    MYSQL_BIND bind;
    memset(&bind, 0, sizeof(MYSQL_BIND));

    bind.buffer_type = MYSQL_TYPE_TINY;
    bind.buffer = (char*) &type;
    bind.buffer_length = sizeof(type);

    if (mysql_stmt_bind_param(add_snapshot_, &bind) ||
            mysql_stmt_execute(add_snapshot_))
        fatal_error(db_);

    return mysql_insert_id(db_);
}

void MySQL::add_reason_malloc(snapshot_t snapshot_id, void *mem, size_t size)
{
    MYSQL_BIND bind[3];
    memset(&bind, 0, sizeof(MYSQL_BIND)*3);

    // snapshot ID
    bind[0].buffer_type = MYSQL_TYPE_LONG;
    bind[0].buffer = &snapshot_id;
    bind[0].buffer_length = sizeof(snapshot_t);

    // memory pointer
    bind[1].buffer_type = MYSQL_TYPE_LONGLONG;
    bind[1].buffer = &mem;
    bind[1].buffer_length = sizeof(void*);

    // size
    bind[2].buffer_type = MYSQL_TYPE_LONG;
    bind[2].buffer = &size;
    bind[2].buffer_length = sizeof(size_t);

    if (mysql_stmt_bind_param(add_reason_malloc_, bind) ||
            mysql_stmt_execute(add_reason_malloc_))
        fatal_error(db_);

    // Create new block
    create_new_block(snapshot_id, mem, size);
}

void MySQL::add_reason_free(snapshot_t snapshot_id, void *mem)
{
    MYSQL_BIND bind[2];

    memset(&bind, 0, sizeof(MYSQL_BIND)*2);

    // snapshot ID
    bind[0].buffer_type = MYSQL_TYPE_LONG;
    bind[0].buffer = &snapshot_id;
    bind[0].buffer_length = sizeof(snapshot_t);

    // memory pointer
    bind[1].buffer_type = MYSQL_TYPE_LONGLONG;
    bind[1].buffer = &mem;
    bind[1].buffer_length = sizeof(void*);

    if (mysql_stmt_bind_param(add_reason_free_, bind) ||
            mysql_stmt_execute(add_reason_free_))
        fatal_error(db_);

    // Free block
    if(mem)
        free_block(snapshot_id, mem);
}

void MySQL::add_reason_calloc(
    snapshot_t snapshot_id, void *mem, size_t nmemb, size_t size)
{
    MYSQL_BIND bind[4];
    memset(&bind, 0, sizeof(MYSQL_BIND)*4);

    // snapshot ID
    bind[0].buffer_type = MYSQL_TYPE_LONG;
    bind[0].buffer = &snapshot_id;
    bind[0].buffer_length = sizeof(snapshot_t);

    // memory pointer
    bind[1].buffer_type = MYSQL_TYPE_LONGLONG;
    bind[1].buffer = &mem;
    bind[1].buffer_length = sizeof(void*);

    // nmemb
    bind[2].buffer_type = MYSQL_TYPE_LONG;
    bind[2].buffer = &nmemb;
    bind[2].buffer_length = sizeof(size_t);

    // size
    bind[3].buffer_type = MYSQL_TYPE_LONG;
    bind[3].buffer = &size;
    bind[3].buffer_length = sizeof(size_t);

    if (mysql_stmt_bind_param(add_reason_malloc_, bind) ||
            mysql_stmt_execute(add_reason_malloc_))
        fatal_error(db_);

    // Create new block
    create_new_block(snapshot_id, mem, nmemb * size);
}

void MySQL::add_reason_realloc(
    snapshot_t snapshot_id, void *old_mem, void *new_mem, size_t new_size)
{
    MYSQL_BIND bind[4];
    memset(&bind, 0, sizeof(MYSQL_BIND)*4);

    // snapshot ID
    bind[0].buffer_type = MYSQL_TYPE_LONG;
    bind[0].buffer = &snapshot_id;
    bind[0].buffer_length = sizeof(snapshot_t);

    // memory pointer
    bind[1].buffer_type = MYSQL_TYPE_LONGLONG;
    bind[1].buffer = &old_mem;
    bind[1].buffer_length = sizeof(void*);

    // nmemb
    bind[2].buffer_type = MYSQL_TYPE_LONGLONG;
    bind[2].buffer = &new_mem;
    bind[2].buffer_length = sizeof(void*);

    // size
    bind[3].buffer_type = MYSQL_TYPE_LONG;
    bind[3].buffer = &new_size;
    bind[3].buffer_length = sizeof(size_t);

    if (mysql_stmt_bind_param(add_reason_malloc_, bind) ||
            mysql_stmt_execute(add_reason_malloc_))
        fatal_error(db_);

    // Realloc has a special logic. If new_size is equal to zero, a free
    // is performed.
    // If old_mem is equal to zero, a malloc is performed.
    if (!new_size && old_mem) {
        free_block(snapshot_id, old_mem);
    } else if (new_size && !old_mem) {
        create_new_block(snapshot_id, new_mem, new_size);
    } else {
        free_block(snapshot_id, old_mem);
        create_new_block(snapshot_id, new_mem, new_size);
    }
}

void MySQL::add_memory_writes(
    snapshot_t snapshot_id, const std::map<void *, unsigned char>& writes)
{
    MYSQL_BIND bind[3];
    char current_char;
    void* current_addr = NULL;
    memset(&bind, 0, sizeof(MYSQL_BIND)*3);

    // snapshot ID
    bind[0].buffer_type = MYSQL_TYPE_LONG;
    bind[0].buffer = &snapshot_id;
    bind[0].buffer_length = sizeof(snapshot_t);

    // address
    bind[1].buffer_type = MYSQL_TYPE_LONGLONG;
    bind[1].buffer = &current_addr;
    bind[1].buffer_length = sizeof(void*);

    // value
    bind[2].buffer_type = MYSQL_TYPE_TINY;
    bind[2].buffer = &current_char;
    bind[2].buffer_length = 1;

    for (std::map<void *, unsigned char>::const_iterator it = writes.begin();
            it != writes.end(); it++) {
        current_addr = it -> first;
        current_char = it -> second;
        if (mysql_stmt_bind_param(add_reason_malloc_, bind) ||
                    mysql_stmt_execute(add_reason_malloc_))
                fatal_error(db_);
    }
}

void MySQL::create_new_block(snapshot_t snapshot_id, void *address, size_t size)
{
    MYSQL_BIND bind[3];
    memset(&bind, 0, sizeof(MYSQL_BIND)*3);

    // snapshot ID
    bind[0].buffer_type = MYSQL_TYPE_LONG;
    bind[0].buffer = &snapshot_id;
    bind[0].buffer_length = sizeof(snapshot_t);

    // memory pointer
    bind[1].buffer_type = MYSQL_TYPE_LONGLONG;
    bind[1].buffer = &address;
    bind[1].buffer_length = sizeof(void*);

    // size
    bind[2].buffer_type = MYSQL_TYPE_LONG;
    bind[2].buffer = &size;
    bind[2].buffer_length = sizeof(size_t);

    if (mysql_stmt_bind_param(add_reason_malloc_, bind) ||
            mysql_stmt_execute(add_reason_malloc_))
        fatal_error(db_);
}

void MySQL::free_block(snapshot_t snapshot_id, void *address)
{
    MYSQL_BIND bind[2];
    memset(&bind, 0, sizeof(MYSQL_BIND)*2);

    // snapshot ID
    bind[0].buffer_type = MYSQL_TYPE_LONG;
    bind[0].buffer = &snapshot_id;
    bind[0].buffer_length = sizeof(snapshot_t);

    // memory pointer
    bind[1].buffer_type = MYSQL_TYPE_LONGLONG;
    bind[1].buffer = &address;
    bind[1].buffer_length = sizeof(void*);

    if (mysql_stmt_bind_param(add_reason_malloc_, bind) ||
            mysql_stmt_execute(add_reason_malloc_))
        fatal_error(db_);
}
