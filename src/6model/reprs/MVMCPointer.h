#ifndef CPOINTER_H_GUARD
#define CPOINTER_H_GUARD

/* Body of a CPointer. */
typedef struct {
    void *ptr;
} CPointerBody;

/* This is how an instance with the CPointer representation looks. */
typedef struct {
    SixModelObjectCommonalities common;
    CPointerBody body;
} CPointerInstance;

/* Initializes the CPointer REPR. */
MVMREPROps * CPointer_initialize(MVMThreadContext *tc,
        MVMObject * (* wrap_object_func_ptr) (MVMThreadContext *tc, void *obj),
        MVMObject * (* create_stable_func_ptr) (MVMThreadContext *tc, MVMREPROps *REPR, MVMObject *HOW));

#endif
