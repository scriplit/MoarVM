#include "moarvm.h"

/* This representation's function pointer table. */
static MVMREPROps *this_repr;

/* Invocation protocol handler. */
static void invoke_handler(MVMThreadContext *tc, MVMObject *invokee, MVMCallsite *callsite, MVMRegister *args) {
    MVM_exception_throw_adhoc(tc, "Cannot invoke comp unit object");
}

/* Creates a new type object of this representation, and associates it with
 * the given HOW. Also sets the invocation protocol handler in the STable. */
static MVMObject * type_object_for(MVMThreadContext *tc, MVMObject *HOW) {
    MVMSTable *st;
    MVMObject *obj;

    st = MVM_gc_allocate_stable(tc, this_repr, HOW);
    MVMROOT(tc, st, {
        obj = MVM_gc_allocate_type_object(tc, st);
        MVM_ASSIGN_REF(tc, st, st->WHAT, obj);
        st->invoke = invoke_handler;
        st->size = sizeof(MVMCompUnit);
    });

    return st->WHAT;
}

/* Creates a new instance based on the type object. */
static MVMObject * allocate(MVMThreadContext *tc, MVMSTable *st) {
    return MVM_gc_allocate_object(tc, st);
}

/* Initializes a new instance. */
static void initialize(MVMThreadContext *tc, MVMSTable *st, MVMObject *root, void *data) {
}

/* Copies the body of one object to another. */
static void copy_to(MVMThreadContext *tc, MVMSTable *st, void *src, MVMObject *dest_root, void *dest) {
    MVMCompUnitBody *src_body  = (MVMCompUnitBody *)src;
    MVMCompUnitBody *dest_body = (MVMCompUnitBody *)dest;
    MVM_exception_throw_adhoc(tc, "this representation (CompUnit) cannot be cloned");
}

/* Adds held objects to the GC worklist. */
static void gc_mark(MVMThreadContext *tc, MVMSTable *st, void *data, MVMGCWorklist *worklist) {
    MVMCompUnitBody *body = (MVMCompUnitBody *)data;
    MVMuint32 i;

    /* Add code refs and static frames to the worklists. */
    for (i = 0; i < body->num_frames; i++) {
        MVM_gc_worklist_add(tc, worklist, &body->frames[i]);
        MVM_gc_worklist_add(tc, worklist, &body->coderefs[i]);
    }

    /* Add strings to the worklists. */
    for (i = 0; i < body->num_strings; i++)
        MVM_gc_worklist_add(tc, worklist, &body->strings[i]);

    /* Add serialization contexts to the worklist. */
    for (i = 0; i < body->num_scs; i++) {
        if (body->scs[i]) {
            MVM_gc_worklist_add(tc, worklist, &body->scs[i]);
        }
        /* Unresolved sc bodies' handles are marked by the GC instance root marking. */
    }

    /* Add various other referenced strings, etc. */
    MVM_gc_worklist_add(tc, worklist, &body->hll_name);
    MVM_gc_worklist_add(tc, worklist, &body->filename);
}

/* Called by the VM in order to free memory associated with this object. */
static void gc_free(MVMThreadContext *tc, MVMObject *obj) {
    MVMCompUnitBody *body = &((MVMCompUnit *)obj)->body;
    MVM_checked_free_null(body->frames);
    MVM_checked_free_null(body->coderefs);
    body->main_frame = NULL;
    body->load_frame = NULL;
    body->deserialize_frame = NULL;
    MVM_checked_free_null(body->callsites);
    MVM_checked_free_null(body->strings);
    MVM_checked_free_null(body->scs);
    MVM_checked_free_null(body->scs_to_resolve);
    body->hll_config = NULL;
    body->hll_name = NULL;
    body->filename = NULL;
}

/* Gets the storage specification for this representation. */
static MVMStorageSpec get_storage_spec(MVMThreadContext *tc, MVMSTable *st) {
    /* XXX in the end we'll support inlining of this... */
    MVMStorageSpec spec;
    spec.inlineable      = MVM_STORAGE_SPEC_REFERENCE;
    spec.boxed_primitive = MVM_STORAGE_SPEC_BP_NONE;
    spec.can_box         = 0;
    return spec;
}

/* Compose the representation. */
static void compose(MVMThreadContext *tc, MVMSTable *st, MVMObject *info) {
    /* Nothing to do for this REPR. */
}

/* Initializes the representation. */
MVMREPROps * MVMCompUnit_initialize(MVMThreadContext *tc) {
    /* Allocate and populate the representation function table. */
    this_repr = malloc(sizeof(MVMREPROps));
    memset(this_repr, 0, sizeof(MVMREPROps));
    this_repr->type_object_for = type_object_for;
    this_repr->allocate = allocate;
    this_repr->initialize = initialize;
    this_repr->copy_to = copy_to;
    this_repr->gc_mark = gc_mark;
    this_repr->gc_free = gc_free;
    this_repr->get_storage_spec = get_storage_spec;
    this_repr->compose = compose;
    return this_repr;
}
