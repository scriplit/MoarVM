#ifndef CSTR_H_GUARD
#define CSTR_H_GUARD

typedef struct {
    char *cstr;
} CStrBody;

/* This is how an instance with the CStr representation looks. */
typedef struct {
    SixModelObjectCommonalities common;
    CStrBody body;
} CStrInstance;

/* Not needed yet.
typedef struct {
} CStrREPRData;*/

MVMREPROps *CStr_initialize(MVMThreadContext *tc,
        MVMObject * (* wrap_object_func_ptr) (MVMThreadContext *tc, void *obj),
        MVMObject * (* create_stable_func_ptr) (MVMThreadContext *tc, MVMREPROps *REPR, MVMObject *HOW));

#endif
