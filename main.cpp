#include "pin.H"

#include <stdint.h>

#include <algorithm>
#include <stdexcept>
#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <string>
#include <cstring>
#include <set>
#include <map>

#include "db.h"
#include "mysql.h"

/* ===================================================================== */
/* Names of malloc and free */
/* ===================================================================== */
#define STR_CALLOC "calloc"
#define STR_MALLOC "malloc"
#define STR_FREE "free"
#define STR_REALLOC "realloc"

VOID *range_start = (VOID*)0x00400000;
VOID *range_end = (VOID*)0x0082e000;

std::map<void *, unsigned char> mem_queue;
MySQL *db;

PIN_LOCK lock;

void flush_queue(THREADID tid)
{
    PIN_GetLock(&lock, tid+1);
    if (mem_queue.size() == 0) {
        PIN_ReleaseLock(&lock);
        return;
    }

    std::map<void*, unsigned char>::iterator it = mem_queue.begin();
    for (; it != mem_queue.end(); it++) {
        it->second = *(unsigned char*)it->first;
    }

    snapshot_t id = db->addSnapshot(snapshot_reason::MEM_WRITE);
    db->addMemoryWrites(id, mem_queue);

    mem_queue.clear();
    PIN_ReleaseLock(&lock);
}

VOID RecordMemWrite(VOID *ip, VOID *addr, ADDRINT size, THREADID tid)
{
    if (!range_start|| !range_end)
        return;

    if ((ADDRINT)addr < (ADDRINT)range_start ||
            (ADDRINT)addr > (ADDRINT)range_end)
        return;

    PIN_GetLock(&lock, tid+1);

    for (ADDRINT i=0; i<size; i++) {
        if (mem_queue.find((char*)addr+i) != mem_queue.end()) {
            PIN_ReleaseLock(&lock);
            flush_queue(tid);
            PIN_GetLock(&lock, tid+1);
        }

        mem_queue[((char*)addr+i)] = 0;
	printf("writing: %p <- 0x%02x\n", (char*)addr+i, *((unsigned char*)addr+i));
    }

    PIN_ReleaseLock(&lock);
}

VOID* WrapperMalloc(CONTEXT *ctxt, AFUNPTR pf_malloc, size_t size, THREADID tid)
{
    void *res;

    CALL_APPLICATION_FUNCTION_PARAM param;
    memset((void*)&param, 0x00, sizeof(param));

    flush_queue(tid);

    PIN_CallApplicationFunction(ctxt, PIN_ThreadId(),
            CALLINGSTD_DEFAULT, pf_malloc, &param,
            PIN_PARG(void *), &res, PIN_PARG(size_t), size,
            PIN_PARG_END());

    PIN_GetLock(&lock, tid+1);
    snapshot_t id = db->addSnapshot(snapshot_reason::MALLOC);
    db->addReasonMalloc(id, res, size);
    PIN_ReleaseLock(&lock);

    flush_queue(tid);

    printf("malloc(%p) = %p\n", (void*)size, res);

    return res;
}

VOID WrapperFree(CONTEXT *ctxt, AFUNPTR pf_free, void *ptr, THREADID tid)
{
    CALL_APPLICATION_FUNCTION_PARAM param;
    memset((void*)&param, 0x00, sizeof(param));

    flush_queue(tid);

    PIN_CallApplicationFunction(ctxt, PIN_ThreadId(),
            CALLINGSTD_DEFAULT, pf_free, &param, PIN_PARG(void),
            PIN_PARG(void *), ptr,
            PIN_PARG_END());

    PIN_GetLock(&lock, tid+1);
    snapshot_t id = db->addSnapshot(snapshot_reason::FREE);
    db->addReasonFree(id, ptr);
    PIN_ReleaseLock(&lock);

    flush_queue(tid);

    printf("free(%p)\n", ptr);
}

VOID* WrapperCalloc(CONTEXT *ctxt, AFUNPTR pf_calloc, size_t nmemb, size_t size,
        THREADID tid)
{
    void *res;

    CALL_APPLICATION_FUNCTION_PARAM param;
    memset((void*)&param, 0x00, sizeof(param));

    flush_queue(tid);

    PIN_CallApplicationFunction(ctxt, PIN_ThreadId(),
            CALLINGSTD_DEFAULT, pf_calloc, &param,
            PIN_PARG(void *), &res, PIN_PARG(size_t), nmemb,
            PIN_PARG(size_t), size,
            PIN_PARG_END());

    PIN_GetLock(&lock, tid+1);
    snapshot_t id = db->addSnapshot(snapshot_reason::CALLOC);
    db->addReasonCalloc(id, res, nmemb, size);
    PIN_ReleaseLock(&lock);

    flush_queue(tid);

    printf("calloc(%p, %p) = %p\n", (void*)nmemb, (void*)size, res);

    return res;
}

VOID* WrapperRealloc(CONTEXT *ctxt, AFUNPTR pf_realloc, void *ptr, size_t size,
        THREADID tid)
{
    void *res;

    CALL_APPLICATION_FUNCTION_PARAM param;
    memset((void*)&param, 0x00, sizeof(param));

    flush_queue(tid);

    PIN_CallApplicationFunction(ctxt, PIN_ThreadId(),
            CALLINGSTD_DEFAULT, pf_realloc,
            &param, PIN_PARG(void *), &res,
            PIN_PARG(void *), ptr, PIN_PARG(size_t), size,
            PIN_PARG_END());

    PIN_GetLock(&lock, tid+1);
    snapshot_t id = db->addSnapshot(snapshot_reason::REALLOC);
    db->addReasonRealloc(id, ptr, res, size);
    PIN_ReleaseLock(&lock);

    flush_queue(tid);

    printf("realloc(%p, %p) = %p\n", ptr, (void*)size, res);

    return res;
}

/* ===================================================================== */
/* Instrumentation routines                                              */
/* ===================================================================== */

VOID Instruction(INS ins, VOID *v)
{
    if (INS_IsMemoryWrite(ins)) {
        INS_InsertPredicatedCall(ins, IPOINT_BEFORE,
                (AFUNPTR) RecordMemWrite,
                IARG_INST_PTR, IARG_MEMORYWRITE_EA,
                IARG_MEMORYWRITE_SIZE,
                IARG_THREAD_ID,
                IARG_END);
    }
}

VOID ImageLoad(IMG img, VOID *v)
{
    // replace malloc singature
    RTN rtn_malloc = RTN_FindByName(img, STR_MALLOC);
    if (RTN_Valid(rtn_malloc)) {
        PROTO proto_malloc = PROTO_Allocate(PIN_PARG(void*),
                CALLINGSTD_DEFAULT, STR_MALLOC, PIN_PARG(size_t),
                PIN_PARG_END());

        RTN_ReplaceSignature(rtn_malloc, AFUNPTR(WrapperMalloc),
                IARG_PROTOTYPE, proto_malloc, IARG_CONTEXT,
                IARG_ORIG_FUNCPTR,
                IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
                IARG_THREAD_ID,
                IARG_END);
    }

    // replace free singature
    RTN rtn_free = RTN_FindByName(img, STR_FREE);
    if (RTN_Valid(rtn_free)) {
        PROTO proto_free = PROTO_Allocate(PIN_PARG(void),
                CALLINGSTD_DEFAULT, STR_FREE, PIN_PARG(void *),
                PIN_PARG_END());

        RTN_ReplaceSignature(rtn_free, AFUNPTR(WrapperFree),
                IARG_PROTOTYPE, proto_free, IARG_CONTEXT,
                IARG_ORIG_FUNCPTR,
                IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
                IARG_THREAD_ID,
                IARG_END);
    }
    // replace calloc singature
    RTN rtn_calloc = RTN_FindByName(img, STR_CALLOC);
    if (RTN_Valid(rtn_calloc)) {
        PROTO proto_calloc = PROTO_Allocate(PIN_PARG(void*),
                CALLINGSTD_DEFAULT, STR_CALLOC, PIN_PARG(size_t),
                PIN_PARG(size_t),
                PIN_PARG_END());

        RTN_ReplaceSignature(rtn_calloc, AFUNPTR(WrapperCalloc),
                IARG_PROTOTYPE, proto_calloc, IARG_CONTEXT,
                IARG_ORIG_FUNCPTR,
                IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
                IARG_FUNCARG_ENTRYPOINT_VALUE, 1,
                IARG_THREAD_ID,
                IARG_END);
    }

    // replace realloc singature
    RTN rtn_realloc = RTN_FindByName(img, STR_REALLOC);
    if (RTN_Valid(rtn_realloc)) {
        PROTO proto_realloc = PROTO_Allocate(PIN_PARG(void*),
                CALLINGSTD_DEFAULT, STR_REALLOC, PIN_PARG(void *),
                PIN_PARG(size_t),
                PIN_PARG_END());

        RTN_ReplaceSignature(rtn_realloc, AFUNPTR(WrapperRealloc),
                IARG_PROTOTYPE, proto_realloc,
                IARG_CONTEXT, IARG_ORIG_FUNCPTR,
                IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
                IARG_FUNCARG_ENTRYPOINT_VALUE, 1,
                IARG_THREAD_ID,
                IARG_END);
    }
}

/* ===================================================================== */

VOID Fini(INT32 code, VOID *v)
{
}

/* ===================================================================== */
/* Print Help Message                                                    */
/* ===================================================================== */

INT32 Usage()
{
    PIN_ERROR("This Pintool traces heap changes\n"
            + KNOB_BASE::StringKnobSummary() + "\n");
    return -1;
}

/* ===================================================================== */
/* Main                                                                  */
/* ===================================================================== */

int main(int argc, char *argv[])
{
    PIN_InitLock(&lock);

    // Initialize pin & symbol manager
    PIN_InitSymbols();
    if (PIN_Init(argc,argv)) {
        return Usage();
    }

    db = new MySQL("localhost", "root", "root", "ihv");

    // Register Image to be called to instrument functions.
    IMG_AddInstrumentFunction(ImageLoad, 0);
    INS_AddInstrumentFunction(Instruction, 0);
    PIN_AddFiniFunction(Fini, 0);

    // Never returns
    PIN_StartProgram();

    return 0;
}

/* ===================================================================== */
/* eof */
/* ===================================================================== */
