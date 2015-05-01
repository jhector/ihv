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

/* ===================================================================== */
/* Names of malloc and free */
/* ===================================================================== */
#define CALLOC "calloc"
#define MALLOC "malloc"
#define FREE "free"
#define REALLOC "realloc"

VOID *track_start = (VOID*)0x060d000;
VOID *track_end = (VOID*)0x0062e000;

VOID RecordMemWrite(VOID *ip, VOID *addr, ADDRINT size)
{
    if (!track_start || !track_end)
        return;

    if ((ADDRINT)addr < (ADDRINT)track_start ||
            (ADDRINT)addr > (ADDRINT)track_end)
        return;

    // TODO: implement
}

VOID* WrapperMalloc(CONTEXT *ctxt, AFUNPTR pf_malloc, size_t size)
{
    void *res;

    CALL_APPLICATION_FUNCTION_PARAM param;
    memset((void*)&param, 0x00, sizeof(param));

    PIN_CallApplicationFunction(ctxt, PIN_ThreadId(),
            CALLINGSTD_DEFAULT, pf_malloc, &param,
            PIN_PARG(void *), &res, PIN_PARG(size_t), size,
            PIN_PARG_END());

    printf("malloc(%p) = %p\n", (void*)size, res);

    return res;
}

VOID WrapperFree(CONTEXT *ctxt, AFUNPTR pf_free, void *ptr)
{
    CALL_APPLICATION_FUNCTION_PARAM param;
    memset((void*)&param, 0x00, sizeof(param));

    PIN_CallApplicationFunction(ctxt, PIN_ThreadId(),
            CALLINGSTD_DEFAULT, pf_free, &param, PIN_PARG(void),
            PIN_PARG(void *), ptr,
            PIN_PARG_END());

    printf("free(%p)\n", ptr);
}

VOID* WrapperCalloc(CONTEXT *ctxt, AFUNPTR pf_calloc, size_t nmemb, size_t size)
{
    void *res;

    CALL_APPLICATION_FUNCTION_PARAM param;
    memset((void*)&param, 0x00, sizeof(param));

    PIN_CallApplicationFunction(ctxt, PIN_ThreadId(),
            CALLINGSTD_DEFAULT, pf_calloc, &param,
            PIN_PARG(void *), &res, PIN_PARG(size_t), nmemb,
            PIN_PARG(size_t), size,
            PIN_PARG_END());

    printf("calloc(%p, %p) = %p\n", (void*)nmemb, (void*)size, res);

    return res;
}

VOID* WrapperRealloc(CONTEXT *ctxt, AFUNPTR pf_realloc, void *ptr, size_t size)
{
    void *res;

    CALL_APPLICATION_FUNCTION_PARAM param;
    memset((void*)&param, 0x00, sizeof(param));

    PIN_CallApplicationFunction(ctxt, PIN_ThreadId(),
            CALLINGSTD_DEFAULT, pf_realloc,
            &param, PIN_PARG(void *), &res,
            PIN_PARG(void *), ptr, PIN_PARG(size_t), size,
            PIN_PARG_END());

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
                IARG_END);
    }
}

VOID ImageLoad(IMG img, VOID *v)
{
    // replace malloc singature
    RTN rtn_malloc = RTN_FindByName(img, MALLOC);
    if (RTN_Valid(rtn_malloc)) {
        PROTO proto_malloc = PROTO_Allocate(PIN_PARG(void*),
                CALLINGSTD_DEFAULT, MALLOC, PIN_PARG(size_t),
                PIN_PARG_END());

        RTN_ReplaceSignature(rtn_malloc, AFUNPTR(WrapperMalloc),
                IARG_PROTOTYPE, proto_malloc, IARG_CONTEXT,
                IARG_ORIG_FUNCPTR,
                IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
                IARG_END);
    }

    // replace free singature
    RTN rtn_free = RTN_FindByName(img, FREE);
    if (RTN_Valid(rtn_free)) {
        PROTO proto_free = PROTO_Allocate(PIN_PARG(void),
                CALLINGSTD_DEFAULT, FREE, PIN_PARG(void *),
                PIN_PARG_END());

        RTN_ReplaceSignature(rtn_free, AFUNPTR(WrapperFree),
                IARG_PROTOTYPE, proto_free, IARG_CONTEXT,
                IARG_ORIG_FUNCPTR,
                IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
                IARG_END);
    }
    // replace calloc singature
    RTN rtn_calloc = RTN_FindByName(img, CALLOC);
    if (RTN_Valid(rtn_calloc)) {
        PROTO proto_calloc = PROTO_Allocate(PIN_PARG(void*),
                CALLINGSTD_DEFAULT, CALLOC, PIN_PARG(size_t),
                PIN_PARG(size_t),
                PIN_PARG_END());

        RTN_ReplaceSignature(rtn_calloc, AFUNPTR(WrapperCalloc),
                IARG_PROTOTYPE, proto_calloc, IARG_CONTEXT,
                IARG_ORIG_FUNCPTR,
                IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
                IARG_FUNCARG_ENTRYPOINT_VALUE, 1,
                IARG_END);
    }

    // replace realloc singature
    RTN rtn_realloc = RTN_FindByName(img, REALLOC);
    if (RTN_Valid(rtn_realloc)) {
        PROTO proto_realloc = PROTO_Allocate(PIN_PARG(void*),
                CALLINGSTD_DEFAULT, REALLOC, PIN_PARG(void *),
                PIN_PARG(size_t),
                PIN_PARG_END());

        RTN_ReplaceSignature(rtn_realloc, AFUNPTR(WrapperRealloc),
                IARG_PROTOTYPE, proto_realloc,
                IARG_CONTEXT, IARG_ORIG_FUNCPTR,
                IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
                IARG_FUNCARG_ENTRYPOINT_VALUE, 1,
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
    // Initialize pin & symbol manager
    PIN_InitSymbols();
    if (PIN_Init(argc,argv)) {
        return Usage();
    }

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
