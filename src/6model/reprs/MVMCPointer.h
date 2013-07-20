/* Body of a MVMCPointer. */
typedef struct _MVMCPointerBody {
    void *ptr;
} MVMCPointerBody;

/* This is how an instance with the MVMCPointer representation looks. */
typedef struct _MVMCPointer {
    MVMObject common;
    MVMCPointerBody body;
} MVMCPointer;

/* Initializes the MVMCPointer REPR. */
MVMREPROps * MVMCPointer_initialize(MVMThreadContext *tc);
