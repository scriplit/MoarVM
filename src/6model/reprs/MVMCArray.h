#ifndef CARRAY_H_GUARD
#define CARRAY_H_GUARD

/* Body of a CArray. */
typedef struct {
    /* The storage of C-land elements. */
    void *storage;

    /* The storage of Perl-land elements */
    MVMObject *child_objs;

    /* Are we managing the memory for this array ourselves, or does it come
     * from C? */
    MVMint32 managed;

    /* The number of elements we've allocated. If we do not know,
     * because the array was returned to us from elsewhere and we
     * are not managing its memory, this is 0. */
    MVMuint64 allocated;

    /* The number of elements we have, if known. Invalid if we
     * are not managing the array. */
    MVMuint64 elems;
} MVMCArrayBody;

/* This is how an instance with the CArray representation looks. */
typedef struct {
    MVMObject common;
    MVMCArrayBody body;
} MVMCArray;

/* What kind of element do we have? */
#define MVMCARRAY_ELEM_KIND_NUMERIC    1
#define MVMCARRAY_ELEM_KIND_STRING     2
#define MVMCARRAY_ELEM_KIND_CPOINTER   3
#define MVMCARRAY_ELEM_KIND_CARRAY     4
#define MVMCARRAY_ELEM_KIND_CSTRUCT    5

/* The CArray REPR data contains a little info about the type of array
 * that we have. */
typedef struct {
    /* The number of bytes in size that an element is. */
    MVMuint32 elem_size;

    /* What kind of element is it (lets us quickly know how to handle access
     * to it). */
    MVMint64 elem_kind;

    /* The type of an element. */
    MVMObject *elem_type;
} MVMCArrayREPRData;

/* Initializes the CArray REPR. */
MVMREPROps * MVMCArray_initialize(MVMThreadContext *tc,
        MVMObject * (* wrap_object_func_ptr) (MVMThreadContext *tc, void *obj),
        MVMObject * (* create_stable_func_ptr) (MVMThreadContext *tc, MVMREPROps *REPR, MVMObject *HOW));

#endif
