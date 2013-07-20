#include "moarvm.h"
#include "../3rdparty/dyncall/dynload/dynload.h"
#include "../3rdparty/dyncall/dyncall/dyncall.h"
#include "../3rdparty/dyncall/dyncallback/dyncallback.h"

/* This representation's function pointer table. */
static MVMREPROps *this_repr;

/* Some functions we have to get references to. */
static wrap_object_t   wrap_object_func;
static create_stable_t create_stable_func;

/* Creates a new type object of this representation, and associates it with
 * the given HOW. */
static MVMObject * type_object_for(MVMThreadContext *tc, MVMObject *HOW) {
    MVMSTable *st  = MVM_gc_allocate_stable(tc, this_repr, HOW);

    MVMROOT(tc, st, {
        MVMObject *obj = MVM_gc_allocate_type_object(tc, st);
        MVM_ASSIGN_REF(tc, st, st->WHAT, obj);
        st->size = sizeof(MVMCPointer);
    });
    
    return st->WHAT;
}

/* Composes the representation. */
static void compose(MVMThreadContext *tc, MVMSTable *st, MVMObject *info) {
    /* Nothing to do for this REPR. */
}

/* Creates a new instance based on the type object. */
static MVMObject * allocate(MVMThreadContext *tc, MVMSTable *st) {
    return MVM_gc_allocate_object(tc, st);
}

/* Initialize a new instance. */
static void initialize(MVMThreadContext *tc, MVMSTable *st, void *data) {
    /* Nothing to do here. */
}

/* Copies to the body of one object to another. */
static void copy_to(MVMThreadContext *tc, MVMSTable *st, void *src, MVMObject *dest_root, void *dest) {
    MVMCPointerBody *src_body = (MVMCPointerBody *)src;
    MVMCPointerBody *dest_body = (MVMCPointerBody *)dest;
    dest_body->ptr = src_body->ptr;
}

/* This Parrot-specific addition to the API is used to free an object. */
static void gc_free(MVMThreadContext *tc, MVMObject *obj) {
    mem_sys_free(PMC_data(obj));
    PMC_data(obj) = NULL;
}

/* Gets the storage specification for this representation. */
static MVMStorageSpec get_storage_spec(MVMThreadContext *tc, MVMSTable *st) {
    MVMStorageSpec spec;
    spec.inlineable = STORAGE_SPEC_REFERENCE;
    spec.boxed_primitive = STORAGE_SPEC_BP_NONE;
    spec.can_box = 0;
    spec.bits = sizeof(void *) * 8;
    spec.align = ALIGNOF1(void *);
    return spec;
}

/* Initializes the CPointer representation. */
MVMREPROps * CPointer_initialize(MVMThreadContext *tc,
        wrap_object_t wrap_object_func_ptr,
        create_stable_t create_stable_func_ptr) {
    /* Stash away functions passed wrapping functions. */
    wrap_object_func = wrap_object_func_ptr;
    create_stable_func = create_stable_func_ptr;

    /* Allocate and populate the representation function table. */
    this_repr = mem_allocate_zeroed_typed(MVMREPROps);
    this_repr->type_object_for = type_object_for;
    this_repr->compose = compose;
    this_repr->allocate = allocate;
    this_repr->initialize = initialize;
    this_repr->copy_to = copy_to;
    this_repr->gc_free = gc_free;
    this_repr->get_storage_spec = get_storage_spec;
    return this_repr;
}
