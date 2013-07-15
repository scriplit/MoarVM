#ifndef NATIVECALL_H_GUARD
#define NATIVECALL_H_GUARD

#include "dyncall.h"
#include "dynload.h"
#include "dyncall_callback.h"

/* Body of a NativeCall. */
typedef struct {
    char *lib_name;
    DLLib *lib_handle;
    void *entry_point;
    MVMint64 convention;
    MVMint64 num_args;
    MVMint64 *arg_types;
    MVMint64 ret_type;
    MVMObject **arg_info;
} NativeCallBody;

/* This is how an instance with the NativeCall representation looks. */
typedef struct {
    SixModelObjectCommonalities common;
    NativeCallBody body;
} NativeCallInstance;

/* Initializes the NativeCall REPR. */
MVMREPROps * NativeCall_initialize(MVMThreadContext *tc,
        MVMObject * (* wrap_object_func_ptr) (MVMThreadContext *tc, void *obj),
        MVMObject * (* create_stable_func_ptr) (MVMThreadContext *tc, MVMREPROps *REPR, MVMObject *HOW));

#endif
