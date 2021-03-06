#include "moarvm.h"

/* This file implements the various steps involved in getting 6model
 * bootstrapped from the ground up - that is, getting to having a
 * KnowHOW meta-object type so that userland can start building up
 * more interesting meta-objects. Mostly it just has to make objects
 * with some "holes", and later go back and fill them out. This is
 * due to the circular nature of things.
 */

/* Can do something better than statics later... */
static MVMString *str_repr       = NULL;
static MVMString *str_name       = NULL;
static MVMString *str_anon       = NULL;
static MVMString *str_P6opaque   = NULL;
static MVMString *str_type       = NULL;
static MVMString *str_box_target = NULL;
static MVMString *str_attribute  = NULL;
static MVMString *str_array      = NULL;

/* Creates a stub VMString. Note we didn't initialize the
 * representation yet, so have to do this somewhat pokily. */
static void create_stub_VMString(MVMThreadContext *tc) {
    /* Need to create the REPR function table "in advance"; the
     * MVMString REPR specially knows not to duplicately create
     * this. */
    MVMREPROps *repr = MVMString_initialize(tc);

    /* Now we can create a type object; note we have no HOW yet,
     * though. */
    MVMSTable *st  = MVM_gc_allocate_stable(tc, repr, NULL);
    MVMROOT(tc, st, {
        /* We can now go for the type object. */
        MVMObject *obj = MVM_gc_allocate_type_object(tc, st);

        /* Set the WHAT in the STable we just made to point to the type
        * object (this is completely normal). */
        MVM_ASSIGN_REF(tc, st, st->WHAT, obj);

        /* REPR normally sets up size, but we'll have to do that manually
        * here also. */
        st->size = sizeof(MVMString);

        tc->instance->VMString = obj;
    });
}

/* KnowHOW.new_type method. Creates a new type with this HOW as its meta-object. */
static void new_type(MVMThreadContext *tc, MVMCallsite *callsite, MVMRegister *args) {
    MVMObject   *self, *HOW, *type_object, *BOOTHash, *stash;
    MVMArgInfo   repr_arg, name_arg;
    MVMString   *repr_name, *name;
    MVMREPROps  *repr_to_use;

    /* Get arguments. */
    MVMArgProcContext arg_ctx; arg_ctx.named_used = NULL;
    MVM_args_proc_init(tc, &arg_ctx, callsite, args);
    self     = MVM_args_get_pos_obj(tc, &arg_ctx, 0, MVM_ARG_REQUIRED).arg.o;
    repr_arg = MVM_args_get_named_str(tc, &arg_ctx, str_repr, MVM_ARG_OPTIONAL);
    name_arg = MVM_args_get_named_str(tc, &arg_ctx, str_name, MVM_ARG_OPTIONAL);
    MVM_args_proc_cleanup(tc, &arg_ctx);
    if (REPR(self)->ID != MVM_REPR_ID_KnowHOWREPR)
        MVM_exception_throw_adhoc(tc, "KnowHOW methods must be called on object with REPR KnowHOWREPR");
    MVM_gc_root_temp_push(tc, (MVMCollectable **)&self);
    MVM_gc_root_temp_push(tc, (MVMCollectable **)&repr_arg);
    MVM_gc_root_temp_push(tc, (MVMCollectable **)&name_arg);

    /* We first create a new HOW instance. */
    HOW  = REPR(self)->allocate(tc, STABLE(self));
    MVM_gc_root_temp_push(tc, (MVMCollectable **)&HOW);

    /* See if we have a representation name; if not default to P6opaque. */
    repr_name = repr_arg.exists ? repr_arg.arg.s : str_P6opaque;

    /* Create a new type object of the desired REPR. (Note that we can't
     * default to KnowHOWREPR here, since it doesn't know how to actually
     * store attributes, it's just for bootstrapping knowhow's. */
    repr_to_use = MVM_repr_get_by_name(tc, repr_name);
    type_object = repr_to_use->type_object_for(tc, HOW);
    MVM_gc_root_temp_push(tc, (MVMCollectable **)&type_object);

    /* See if we were given a name; put it into the meta-object if so. */
    name = name_arg.exists ? name_arg.arg.s : str_anon;
    REPR(HOW)->initialize(tc, STABLE(HOW), HOW, OBJECT_BODY(HOW));
    MVM_ASSIGN_REF(tc, HOW, ((MVMKnowHOWREPR *)HOW)->body.name, name);

    /* Set .WHO to an empty hash. */
    BOOTHash = tc->instance->boot_types->BOOTHash;
    stash = REPR(BOOTHash)->allocate(tc, STABLE(BOOTHash));
    REPR(stash)->initialize(tc, STABLE(stash), stash, OBJECT_BODY(stash));
    MVM_ASSIGN_REF(tc, STABLE(type_object), STABLE(type_object)->WHO, stash);

    /* Return the type object. */
    MVM_args_set_result_obj(tc, type_object, MVM_RETURN_CURRENT_FRAME);

    MVM_gc_root_temp_pop_n(tc, 5);
}

/* Adds a method. */
static void add_method(MVMThreadContext *tc, MVMCallsite *callsite, MVMRegister *args) {
    MVMObject *self, *type_obj, *method, *method_table;
    MVMString *name;

    /* Get arguments. */
    MVMArgProcContext arg_ctx; arg_ctx.named_used = NULL;
    MVM_args_proc_init(tc, &arg_ctx, callsite, args);
    self     = MVM_args_get_pos_obj(tc, &arg_ctx, 0, MVM_ARG_REQUIRED).arg.o;
    type_obj = MVM_args_get_pos_obj(tc, &arg_ctx, 1, MVM_ARG_REQUIRED).arg.o;
    name     = MVM_args_get_pos_str(tc, &arg_ctx, 2, MVM_ARG_REQUIRED).arg.s;
    method   = MVM_args_get_pos_obj(tc, &arg_ctx, 3, MVM_ARG_REQUIRED).arg.o;
    MVM_args_proc_cleanup(tc, &arg_ctx);
    if (!self || !IS_CONCRETE(self) || REPR(self)->ID != MVM_REPR_ID_KnowHOWREPR)
        MVM_exception_throw_adhoc(tc, "KnowHOW methods must be called on object instance with REPR KnowHOWREPR");

    /* Add to method table. */
    method_table = ((MVMKnowHOWREPR *)self)->body.methods;
    MVM_repr_bind_key_boxed(tc, method_table, name, method);

    /* Return added method as result. */
    MVM_args_set_result_obj(tc, method, MVM_RETURN_CURRENT_FRAME);
}

/* Adds an method. */
static void add_attribute(MVMThreadContext *tc, MVMCallsite *callsite, MVMRegister *args) {
    MVMObject *self, *type_obj, *attr, *attributes;

    /* Get arguments. */
    MVMArgProcContext arg_ctx; arg_ctx.named_used = NULL;
    MVM_args_proc_init(tc, &arg_ctx, callsite, args);
    self     = MVM_args_get_pos_obj(tc, &arg_ctx, 0, MVM_ARG_REQUIRED).arg.o;
    type_obj = MVM_args_get_pos_obj(tc, &arg_ctx, 1, MVM_ARG_REQUIRED).arg.o;
    attr     = MVM_args_get_pos_obj(tc, &arg_ctx, 2, MVM_ARG_REQUIRED).arg.o;
    MVM_args_proc_cleanup(tc, &arg_ctx);

    /* Ensure we have the required representations. */
    if (!self || !IS_CONCRETE(self) || REPR(self)->ID != MVM_REPR_ID_KnowHOWREPR)
        MVM_exception_throw_adhoc(tc, "KnowHOW methods must be called on object instance with REPR KnowHOWREPR");
    if (REPR(attr)->ID != MVM_REPR_ID_KnowHOWAttributeREPR)
        MVM_exception_throw_adhoc(tc, "KnowHOW attributes must use KnowHOWAttributeREPR");

    /* Add to method table. */
    attributes = ((MVMKnowHOWREPR *)self)->body.attributes;
    MVM_repr_push_o(tc, attributes, attr);

    /* Return added attribute as result. */
    MVM_args_set_result_obj(tc, attr, MVM_RETURN_CURRENT_FRAME);
}

/* Composes the meta-object. */
static void compose(MVMThreadContext *tc, MVMCallsite *callsite, MVMRegister *args) {
    MVMObject *self, *type_obj, *method_table, *attributes, *BOOTArray, *BOOTHash,
              *repr_info_hash, *repr_info, *type_info, *attr_info_list, *parent_info;
    MVMint64   num_attrs, i;

    /* Get arguments. */
    MVMArgProcContext arg_ctx; arg_ctx.named_used = NULL;
    MVM_args_proc_init(tc, &arg_ctx, callsite, args);
    self     = MVM_args_get_pos_obj(tc, &arg_ctx, 0, MVM_ARG_REQUIRED).arg.o;
    type_obj = MVM_args_get_pos_obj(tc, &arg_ctx, 1, MVM_ARG_REQUIRED).arg.o;
    MVM_args_proc_cleanup(tc, &arg_ctx);
    if (!self || !IS_CONCRETE(self) || REPR(self)->ID != MVM_REPR_ID_KnowHOWREPR)
        MVM_exception_throw_adhoc(tc, "KnowHOW methods must be called on object instance with REPR KnowHOWREPR");

    /* Fill out STable. */
    method_table = ((MVMKnowHOWREPR *)self)->body.methods;
    MVM_ASSIGN_REF(tc, STABLE(type_obj), STABLE(type_obj)->method_cache, method_table);
    STABLE(type_obj)->mode_flags              = MVM_METHOD_CACHE_AUTHORITATIVE;
    STABLE(type_obj)->type_check_cache_length = 1;
    STABLE(type_obj)->type_check_cache        = malloc(sizeof(MVMObject *));
    MVM_ASSIGN_REF(tc, STABLE(type_obj), STABLE(type_obj)->type_check_cache[0], type_obj);

    /* Next steps will allocate, so make sure we keep hold of the type
     * object and ourself. */
    MVM_gc_root_temp_push(tc, (MVMCollectable **)&self);
    MVM_gc_root_temp_push(tc, (MVMCollectable **)&type_obj);

    /* Use any attribute information to produce attribute protocol
     * data. The protocol consists of an array... */
    BOOTArray = tc->instance->boot_types->BOOTArray;
    BOOTHash = tc->instance->boot_types->BOOTHash;
    MVM_gc_root_temp_push(tc, (MVMCollectable **)&BOOTArray);
    MVM_gc_root_temp_push(tc, (MVMCollectable **)&BOOTHash);
    repr_info = REPR(BOOTArray)->allocate(tc, STABLE(BOOTArray));
    MVM_gc_root_temp_push(tc, (MVMCollectable **)&repr_info);
    REPR(repr_info)->initialize(tc, STABLE(repr_info), repr_info, OBJECT_BODY(repr_info));

    /* ...which contains an array per MRO entry (just us)... */
    type_info = REPR(BOOTArray)->allocate(tc, STABLE(BOOTArray));
    MVM_gc_root_temp_push(tc, (MVMCollectable **)&type_info);
    REPR(type_info)->initialize(tc, STABLE(type_info), type_info, OBJECT_BODY(type_info));
    MVM_repr_push_o(tc, repr_info, type_info);

    /* ...which in turn contains this type... */
    MVM_repr_push_o(tc, type_info, type_obj);

    /* ...then an array of hashes per attribute... */
    attr_info_list = REPR(BOOTArray)->allocate(tc, STABLE(BOOTArray));
    MVM_gc_root_temp_push(tc, (MVMCollectable **)&attr_info_list);
    REPR(attr_info_list)->initialize(tc, STABLE(attr_info_list), attr_info_list,
        OBJECT_BODY(attr_info_list));
    MVM_repr_push_o(tc, type_info, attr_info_list);
    attributes = ((MVMKnowHOWREPR *)self)->body.attributes;
    MVM_gc_root_temp_push(tc, (MVMCollectable **)&attributes);
    num_attrs = REPR(attributes)->elems(tc, STABLE(attributes),
        attributes, OBJECT_BODY(attributes));
    for (i = 0; i < num_attrs; i++) {
        MVMObject *attr_info = REPR(BOOTHash)->allocate(tc, STABLE(BOOTHash));
        MVMKnowHOWAttributeREPR *attribute = (MVMKnowHOWAttributeREPR *)
            MVM_repr_at_pos_o(tc, attributes, i);
        MVMROOT(tc, attr_info, {
            MVMROOT(tc, attribute, {
                if (REPR((MVMObject *)attribute)->ID != MVM_REPR_ID_KnowHOWAttributeREPR)
                    MVM_exception_throw_adhoc(tc, "KnowHOW attributes must use KnowHOWAttributeREPR");

                MVM_repr_init(tc, attr_info);
                MVM_repr_bind_key_boxed(tc, attr_info, str_name, (MVMObject *)attribute->body.name);
                MVM_repr_bind_key_boxed(tc, attr_info, str_type, attribute->body.type);
                if (attribute->body.box_target) {
                    /* Merely having the key serves as a "yes". */
                    MVM_repr_bind_key_boxed(tc, attr_info, str_box_target, attr_info);
                }

                MVM_repr_push_o(tc, attr_info_list, attr_info);
            });
        });
    }

    /* ...followed by a list of parents (none). */
    parent_info = REPR(BOOTArray)->allocate(tc, STABLE(BOOTArray));
    MVM_gc_root_temp_push(tc, (MVMCollectable **)&parent_info);
    MVM_repr_init(tc, parent_info);
    MVM_repr_push_o(tc, type_info, parent_info);

    /* Finally, this all goes in a hash under the key 'attribute'. */
    repr_info_hash = REPR(BOOTHash)->allocate(tc, STABLE(BOOTHash));
    MVM_gc_root_temp_push(tc, (MVMCollectable **)&repr_info_hash);
    MVM_repr_init(tc, repr_info_hash);
    MVM_repr_bind_key_boxed(tc, repr_info_hash, str_attribute, repr_info);

    /* Compose the representation using it. */
    MVM_repr_compose(tc, type_obj, repr_info_hash);

    /* Clear temporary roots. */
    MVM_gc_root_temp_pop_n(tc, 10);

    /* Return type object. */
    MVM_args_set_result_obj(tc, type_obj, MVM_RETURN_CURRENT_FRAME);
}

#define introspect_member(member, set_result, result) \
static void member(MVMThreadContext *tc, MVMCallsite *callsite, MVMRegister *args) { \
    MVMObject *self, *type_obj, *member; \
    MVMArgProcContext arg_ctx; arg_ctx.named_used = NULL; \
    MVM_args_proc_init(tc, &arg_ctx, callsite, args); \
    self     = MVM_args_get_pos_obj(tc, &arg_ctx, 0, MVM_ARG_REQUIRED).arg.o; \
    type_obj = MVM_args_get_pos_obj(tc, &arg_ctx, 1, MVM_ARG_REQUIRED).arg.o; \
    MVM_args_proc_cleanup(tc, &arg_ctx); \
    if (!self || !IS_CONCRETE(self) || REPR(self)->ID != MVM_REPR_ID_KnowHOWREPR) \
        MVM_exception_throw_adhoc(tc, "KnowHOW methods must be called on object instance with REPR KnowHOWREPR"); \
    member = (MVMObject *)((MVMKnowHOWREPR *)self)->body.member; \
    set_result(tc, result, MVM_RETURN_CURRENT_FRAME); \
}

/* Introspects the attributes. For now just hand back real list. */
introspect_member(attributes, MVM_args_set_result_obj, attributes)

/* Introspects the methods. */
introspect_member(methods, MVM_args_set_result_obj, methods)

/* Introspects the name. */
introspect_member(name, MVM_args_set_result_str, (MVMString *)name)

/* Adds a method into the KnowHOW.HOW method table. */
static void add_knowhow_how_method(MVMThreadContext *tc, MVMKnowHOWREPR *knowhow_how,
        char *name, void (*func) (MVMThreadContext *, MVMCallsite *, MVMRegister *)) {
    MVMObject *BOOTCCode, *code_obj, *method_table, *name_str;

    /* Create string for name. */
    name_str = (MVMObject *)MVM_string_ascii_decode_nt(tc,
        tc->instance->VMString, name);

    /* Allocate a BOOTCCode and put pointer in. */
    BOOTCCode = tc->instance->boot_types->BOOTCCode;
    code_obj = REPR(BOOTCCode)->allocate(tc, STABLE(BOOTCCode));
    ((MVMCFunction *)code_obj)->body.func = func;

    /* Add into the table. */
    method_table = knowhow_how->body.methods;
    REPR(method_table)->ass_funcs->bind_key_boxed(tc, STABLE(method_table),
        method_table, OBJECT_BODY(method_table), name_str, code_obj);
}

/* Bootstraps the KnowHOW type. */
static void bootstrap_KnowHOW(MVMThreadContext *tc) {
    MVMObject *VMString  = tc->instance->VMString;
    MVMObject *BOOTArray = tc->instance->boot_types->BOOTArray;
    MVMObject *BOOTHash  = tc->instance->boot_types->BOOTHash;

    /* Create our KnowHOW type object. Note we don't have a HOW just yet, so
     * pass in NULL. */
    MVMREPROps *REPR    = MVM_repr_get_by_id(tc, MVM_REPR_ID_KnowHOWREPR);
    MVMObject  *knowhow = REPR->type_object_for(tc, NULL);

    /* We create a KnowHOW instance that can describe itself. This means
     * (once we tie the knot) that .HOW.HOW.HOW.HOW etc will always return
     * that, which closes the model up. Note that the STable for it must
     * be allocated first, since that holds the allocation size. */
    MVMKnowHOWREPR *knowhow_how;
    MVMSTable *st = MVM_gc_allocate_stable(tc, REPR, NULL);
    st->WHAT      = (MVMObject *)knowhow;
    st->size      = sizeof(MVMKnowHOWREPR);
    knowhow_how   = (MVMKnowHOWREPR *)REPR->allocate(tc, st);
    st->HOW       = (MVMObject *)knowhow_how;
    knowhow_how->common.st = st;

    /* Add various methods to the KnowHOW's HOW. */
    REPR->initialize(tc, NULL, (MVMObject *)knowhow_how, &knowhow_how->body);
    add_knowhow_how_method(tc, knowhow_how, "new_type", new_type);
    add_knowhow_how_method(tc, knowhow_how, "add_method", add_method);
    add_knowhow_how_method(tc, knowhow_how, "add_attribute", add_attribute);
    add_knowhow_how_method(tc, knowhow_how, "compose", compose);
    add_knowhow_how_method(tc, knowhow_how, "attributes", attributes);
    add_knowhow_how_method(tc, knowhow_how, "methods", methods);
    add_knowhow_how_method(tc, knowhow_how, "name", name);

    /* Set name KnowHOW for the KnowHOW's HOW. */
    knowhow_how->body.name = MVM_string_ascii_decode_nt(tc, VMString, "KnowHOW");

    /* Set this built up HOW as the KnowHOW's HOW. */
    STABLE(knowhow)->HOW = (MVMObject *)knowhow_how;

    /* Give it an authoritative method cache; this in turn will make the
     * method dispatch bottom out. */
    STABLE(knowhow)->method_cache = knowhow_how->body.methods;
    STABLE(knowhow)->mode_flags   = MVM_METHOD_CACHE_AUTHORITATIVE;
    STABLE(knowhow_how)->method_cache = knowhow_how->body.methods;
    STABLE(knowhow_how)->mode_flags   = MVM_METHOD_CACHE_AUTHORITATIVE;

    /* Associate the created objects with the initial core serialization
     * context. */
    /* XXX TODO */

    /* Stash the created KnowHOW. */
    tc->instance->KnowHOW = (MVMObject *)knowhow;
    MVM_gc_root_add_permanent(tc, (MVMCollectable **)&tc->instance->KnowHOW);
}

/* Takes a stub object that existed before we had bootstrapped things and
 * gives it a meta-object. */
static void add_meta_object(MVMThreadContext *tc, MVMObject *type_obj, char *name) {
    MVMObject *meta_obj;
    MVMString *name_str;

    /* Create meta-object. */
    meta_obj = MVM_repr_alloc_init(tc, STABLE(tc->instance->KnowHOW)->HOW);
    MVMROOT(tc, meta_obj, {
        /* Put it in place. */
        MVM_ASSIGN_REF(tc, STABLE(type_obj), STABLE(type_obj)->HOW, meta_obj);

        /* Set name. */
        name_str = MVM_string_ascii_decode_nt(tc, tc->instance->VMString, name);
        MVM_ASSIGN_REF(tc, meta_obj, ((MVMKnowHOWREPR *)meta_obj)->body.name, name_str);
    });
}

/* Creates a new attribute meta-object. */
static void attr_new(MVMThreadContext *tc, MVMCallsite *callsite, MVMRegister *args) {
    MVMObject   *self, *obj;
    MVMArgInfo   type_arg, name_arg, bt_arg;
    MVMREPROps  *repr;

    /* Process arguments. */
    MVMArgProcContext arg_ctx; arg_ctx.named_used = NULL;
    MVM_args_proc_init(tc, &arg_ctx, callsite, args);
    self     = MVM_args_get_pos_obj(tc, &arg_ctx, 0, MVM_ARG_REQUIRED).arg.o;
    name_arg = MVM_args_get_named_str(tc, &arg_ctx, str_name, MVM_ARG_REQUIRED);
    type_arg = MVM_args_get_named_obj(tc, &arg_ctx, str_type, MVM_ARG_OPTIONAL);
    bt_arg   = MVM_args_get_named_int(tc, &arg_ctx, str_box_target, MVM_ARG_OPTIONAL);
    MVM_args_proc_cleanup(tc, &arg_ctx);

    /* Anchor all the things. */
    MVM_gc_root_temp_push(tc, (MVMCollectable **)&self);
    MVM_gc_root_temp_push(tc, (MVMCollectable **)&name_arg);
    MVM_gc_root_temp_push(tc, (MVMCollectable **)&type_arg);

    /* Allocate attribute object. */
    repr = MVM_repr_get_by_id(tc, MVM_REPR_ID_KnowHOWAttributeREPR);
    obj = repr->allocate(tc, STABLE(self));

    /* Populate it. */
    MVM_ASSIGN_REF(tc, obj, ((MVMKnowHOWAttributeREPR *)obj)->body.name, name_arg.arg.s);
    MVM_ASSIGN_REF(tc, obj, ((MVMKnowHOWAttributeREPR *)obj)->body.type, type_arg.exists ? type_arg.arg.o : tc->instance->KnowHOW);
    ((MVMKnowHOWAttributeREPR *)obj)->body.box_target = bt_arg.exists ? bt_arg.arg.i64 : 0;

    /* Return produced object. */
    MVM_gc_root_temp_pop_n(tc, 3);
    MVM_args_set_result_obj(tc, obj, MVM_RETURN_CURRENT_FRAME);
}

/* Composes the attribute; actually, nothing to do really. */
static void attr_compose(MVMThreadContext *tc, MVMCallsite *callsite, MVMRegister *args) {
    MVMObject *self;
    MVMArgProcContext arg_ctx; arg_ctx.named_used = NULL;
    MVM_args_proc_init(tc, &arg_ctx, callsite, args);
    self = MVM_args_get_pos_obj(tc, &arg_ctx, 0, MVM_ARG_REQUIRED).arg.o;
    MVM_args_proc_cleanup(tc, &arg_ctx);
    MVM_args_set_result_obj(tc, self, MVM_RETURN_CURRENT_FRAME);
}

/* Introspects the attribute's name. */
static void attr_name(MVMThreadContext *tc, MVMCallsite *callsite, MVMRegister *args) {
    MVMObject *self;
    MVMString *name;
    MVMArgProcContext arg_ctx; arg_ctx.named_used = NULL;
    MVM_args_proc_init(tc, &arg_ctx, callsite, args);
    self = MVM_args_get_pos_obj(tc, &arg_ctx, 0, MVM_ARG_REQUIRED).arg.o;
    MVM_args_proc_cleanup(tc, &arg_ctx);
    name = ((MVMKnowHOWAttributeREPR *)self)->body.name;
    MVM_args_set_result_str(tc, name, MVM_RETURN_CURRENT_FRAME);
}

/* Introspects the attribute's type. */
static void attr_type(MVMThreadContext *tc, MVMCallsite *callsite, MVMRegister *args) {
    MVMObject *self, *type;
    MVMArgProcContext arg_ctx; arg_ctx.named_used = NULL;
    MVM_args_proc_init(tc, &arg_ctx, callsite, args);
    self = MVM_args_get_pos_obj(tc, &arg_ctx, 0, MVM_ARG_REQUIRED).arg.o;
    MVM_args_proc_cleanup(tc, &arg_ctx);
    type = ((MVMKnowHOWAttributeREPR *)self)->body.type;
    MVM_args_set_result_obj(tc, type, MVM_RETURN_CURRENT_FRAME);
}

/* Introspects the attribute's box target flag. */
static void attr_box_target(MVMThreadContext *tc, MVMCallsite *callsite, MVMRegister *args) {
    MVMObject *self;
    MVMint64   box_target;
    MVMArgProcContext arg_ctx; arg_ctx.named_used = NULL;
    MVM_args_proc_init(tc, &arg_ctx, callsite, args);
    self = MVM_args_get_pos_obj(tc, &arg_ctx, 0, MVM_ARG_REQUIRED).arg.o;
    MVM_args_proc_cleanup(tc, &arg_ctx);
    box_target = ((MVMKnowHOWAttributeREPR *)self)->body.box_target;
    MVM_args_set_result_int(tc, box_target, MVM_RETURN_CURRENT_FRAME);
}

/* Creates and installs the KnowHOWAttribute type. */
static void create_KnowHOWAttribute(MVMThreadContext *tc) {
    MVMObject      *knowhow_how, *meta_obj, *type_obj;
    MVMString      *name_str;
    MVMREPROps     *repr;

    /* Create meta-object. */
    meta_obj = MVM_repr_alloc_init(tc, STABLE(tc->instance->KnowHOW)->HOW);
    MVMROOT(tc, meta_obj, {
        /* Add methods. */
        add_knowhow_how_method(tc, (MVMKnowHOWREPR *)meta_obj, "new", attr_new);
        add_knowhow_how_method(tc, (MVMKnowHOWREPR *)meta_obj, "compose", attr_compose);
        add_knowhow_how_method(tc, (MVMKnowHOWREPR *)meta_obj, "name", attr_name);
        add_knowhow_how_method(tc, (MVMKnowHOWREPR *)meta_obj, "type", attr_type);
        add_knowhow_how_method(tc, (MVMKnowHOWREPR *)meta_obj, "box_target", attr_box_target);

        /* Set name. */
        name_str = MVM_string_ascii_decode_nt(tc, tc->instance->VMString, "KnowHOWAttribute");
        MVM_ASSIGN_REF(tc, meta_obj, ((MVMKnowHOWREPR *)meta_obj)->body.name, name_str);

        /* Create a new type object with the correct REPR. */
        repr = MVM_repr_get_by_id(tc, MVM_REPR_ID_KnowHOWAttributeREPR);
        type_obj = repr->type_object_for(tc, meta_obj);

        /* Set up method dispatch cache. */
        STABLE(type_obj)->method_cache = ((MVMKnowHOWREPR *)meta_obj)->body.methods;
        STABLE(type_obj)->mode_flags   = MVM_METHOD_CACHE_AUTHORITATIVE;

        /* Stash the created type object. */
        tc->instance->KnowHOWAttribute = (MVMObject *)type_obj;
        MVM_gc_root_add_permanent(tc, (MVMCollectable **)&tc->instance->KnowHOWAttribute);
    });
}

/* Bootstraps a typed array. */
static MVMObject * boot_typed_array(MVMThreadContext *tc, char *name, MVMObject *type) {
    MVMBoolificationSpec *bs;
    MVMObject  *repr_info;
    MVMREPROps *repr  = MVM_repr_get_by_id(tc, MVM_REPR_ID_MVMArray);
    MVMObject  *array = repr->type_object_for(tc, NULL);
    MVMROOT(tc, array, {
        /* Give it a meta-object. */
        add_meta_object(tc, array, name);

        /* Now need to compose it with the specified type. */
        repr_info = MVM_repr_alloc_init(tc, tc->instance->boot_types->BOOTHash);
        MVMROOT(tc, repr_info, {
            MVMObject *arr_info = MVM_repr_alloc_init(tc, tc->instance->boot_types->BOOTHash);
            MVMROOT(tc, arr_info, {
                MVM_repr_bind_key_boxed(tc, repr_info, str_array, arr_info);
                MVM_repr_bind_key_boxed(tc, arr_info, str_type, type);
                REPR(array)->compose(tc, STABLE(array), repr_info);
            });
        });

        /* Also give it a boolification spec. */
        bs = malloc(sizeof(MVMBoolificationSpec));
        bs->mode = MVM_BOOL_MODE_HAS_ELEMS;
        bs->method = NULL;
        array->st->boolification_spec = bs;
    });
    return array;
}

/* Sets up the core serialization context. It is marked as the SC of various
 * rooted objects, which means in turn it will never be collected. */
static void setup_core_sc(MVMThreadContext *tc) {
    MVMString *handle;
    MVMSerializationContext *sc;

    handle = MVM_string_ascii_decode_nt(tc, tc->instance->VMString, "__6MODEL_CORE__");
    sc = (MVMSerializationContext *)MVM_sc_create(tc, handle);

#define knowhow_init(tc, sc, index, variable) do { \
    MVM_sc_set_object(tc, sc, index, variable); \
    MVM_sc_set_obj_sc(tc, variable, sc); \
    MVM_sc_set_stable(tc, sc, index, STABLE(variable)); \
    MVM_sc_set_stable_sc(tc, STABLE(variable), sc); \
} while (0)

    MVMROOT(tc, sc, {
        /* KnowHOW */
        knowhow_init(tc, sc, 0, tc->instance->KnowHOW);

        /* KnowHOW.HOW */
        knowhow_init(tc, sc, 1, STABLE(tc->instance->KnowHOW)->HOW);

        /* KnowHOWAttribute */
        knowhow_init(tc, sc, 2, tc->instance->KnowHOWAttribute);
    });
}

/* Drives the overall bootstrap process. */
void MVM_6model_bootstrap(MVMThreadContext *tc) {
    /* First, we have to get the VMString type to exist; this has to
     * come even before REPR registry setup because it relies on
     * being able to create strings. */
    create_stub_VMString(tc);

    /* Now we've enough to actually create the REPR registry. */
    MVM_repr_initialize_registry(tc);

    /* Create stub BOOTInt, BOOTNum, BOOTStr, BOOTArray, BOOTHash, BOOTCCode,
     * BOOTCode, BOOTThread, BOOTIter, BOOTContext, SCRef, Lexotic,
     * CallCapture, BOOTIO and BOOTException types. */
#define create_stub_boot_type(tc, reprid, slot, makeboolspec, boolspec) do { \
    MVMREPROps *repr = MVM_repr_get_by_id(tc, reprid); \
    MVMObject *type = tc->instance->slot = repr->type_object_for(tc, NULL); \
    if (makeboolspec) { \
        MVMBoolificationSpec *bs; \
        bs = malloc(sizeof(MVMBoolificationSpec)); \
        bs->mode = boolspec; \
        bs->method = NULL; \
        type->st->boolification_spec = bs; \
    } \
} while (0)
    create_stub_boot_type(tc, MVM_REPR_ID_P6int, boot_types->BOOTInt, 1, MVM_BOOL_MODE_UNBOX_INT);
    create_stub_boot_type(tc, MVM_REPR_ID_P6num, boot_types->BOOTNum, 1, MVM_BOOL_MODE_UNBOX_NUM);
    create_stub_boot_type(tc, MVM_REPR_ID_P6str, boot_types->BOOTStr, 1, MVM_BOOL_MODE_UNBOX_STR_NOT_EMPTY_OR_ZERO);
    create_stub_boot_type(tc, MVM_REPR_ID_MVMArray, boot_types->BOOTArray, 1, MVM_BOOL_MODE_HAS_ELEMS);
    create_stub_boot_type(tc, MVM_REPR_ID_MVMHash, boot_types->BOOTHash, 0, MVM_BOOL_MODE_HAS_ELEMS);
    create_stub_boot_type(tc, MVM_REPR_ID_MVMCFunction, boot_types->BOOTCCode, 0, MVM_BOOL_MODE_NOT_TYPE_OBJECT);
    create_stub_boot_type(tc, MVM_REPR_ID_MVMCode, boot_types->BOOTCode, 0, MVM_BOOL_MODE_NOT_TYPE_OBJECT);
    create_stub_boot_type(tc, MVM_REPR_ID_MVMThread, boot_types->BOOTThread, 0, MVM_BOOL_MODE_NOT_TYPE_OBJECT);
    create_stub_boot_type(tc, MVM_REPR_ID_MVMIter, boot_types->BOOTIter, 1, MVM_BOOL_MODE_ITER);
    create_stub_boot_type(tc, MVM_REPR_ID_MVMContext, boot_types->BOOTContext, 0, MVM_BOOL_MODE_NOT_TYPE_OBJECT);
    create_stub_boot_type(tc, MVM_REPR_ID_SCRef, SCRef, 0, MVM_BOOL_MODE_NOT_TYPE_OBJECT);
    create_stub_boot_type(tc, MVM_REPR_ID_Lexotic, Lexotic, 0, MVM_BOOL_MODE_NOT_TYPE_OBJECT);
    create_stub_boot_type(tc, MVM_REPR_ID_MVMCallCapture, CallCapture, 0, MVM_BOOL_MODE_NOT_TYPE_OBJECT);
    create_stub_boot_type(tc, MVM_REPR_ID_MVMOSHandle, boot_types->BOOTIO, 0, MVM_BOOL_MODE_NOT_TYPE_OBJECT);
    create_stub_boot_type(tc, MVM_REPR_ID_MVMException, boot_types->BOOTException, 0, MVM_BOOL_MODE_NOT_TYPE_OBJECT);
    create_stub_boot_type(tc, MVM_REPR_ID_MVMStaticFrame, boot_types->BOOTStaticFrame, 0, MVM_BOOL_MODE_NOT_TYPE_OBJECT);
    create_stub_boot_type(tc, MVM_REPR_ID_MVMCompUnit, boot_types->BOOTCompUnit, 0, MVM_BOOL_MODE_NOT_TYPE_OBJECT);

    /* Set up some strings. */
#define string_creator(tc, variable, name) do { \
    variable = MVM_string_ascii_decode_nt((tc), (tc)->instance->VMString, (name)); \
    MVM_gc_root_add_permanent((tc), (MVMCollectable **)&(variable)); \
} while (0)
    string_creator(tc, str_repr, "repr");
    string_creator(tc, str_name, "name");
    string_creator(tc, str_anon, "<anon>");
    string_creator(tc, str_P6opaque, "P6opaque");
    string_creator(tc, str_type, "type");
    string_creator(tc, str_box_target, "box_target");
    string_creator(tc, str_attribute, "attribute");
    string_creator(tc, str_array, "array");

    /* Bootstrap the KnowHOW type, giving it a meta-object. */
    bootstrap_KnowHOW(tc);

    /* Give stub types meta-objects. */
#define meta_objectifier(tc, slot, name) do { \
    add_meta_object((tc), (tc)->instance->slot, (name)); \
    MVM_gc_root_add_permanent((tc), (MVMCollectable **)&(tc)->instance->slot); \
} while (0)
    meta_objectifier(tc, VMString, "VMString");
    meta_objectifier(tc, boot_types->BOOTInt, "BOOTInt");
    meta_objectifier(tc, boot_types->BOOTNum, "BOOTNum");
    meta_objectifier(tc, boot_types->BOOTStr, "BOOTStr");
    meta_objectifier(tc, boot_types->BOOTArray, "BOOTArray");
    meta_objectifier(tc, boot_types->BOOTHash, "BOOTHash");
    meta_objectifier(tc, boot_types->BOOTCCode, "BOOTCCode");
    meta_objectifier(tc, boot_types->BOOTCode, "BOOTCode");
    meta_objectifier(tc, boot_types->BOOTThread, "BOOTThread");
    meta_objectifier(tc, boot_types->BOOTIter, "BOOTIter");
    meta_objectifier(tc, boot_types->BOOTContext, "BOOTContext");
    meta_objectifier(tc, SCRef, "SCRef");
    meta_objectifier(tc, Lexotic, "Lexotic");
    meta_objectifier(tc, CallCapture, "CallCapture");
    meta_objectifier(tc, boot_types->BOOTIO, "BOOTIO");
    meta_objectifier(tc, boot_types->BOOTException, "BOOTException");
    meta_objectifier(tc, boot_types->BOOTStaticFrame, "BOOTStaticFrame");
    meta_objectifier(tc, boot_types->BOOTCompUnit, "BOOTCompUnit");

    /* Create the KnowHOWAttribute type. */
    create_KnowHOWAttribute(tc);

    /* Bootstrap typed arrays. */
    tc->instance->boot_types->BOOTIntArray = boot_typed_array(tc, "BOOTIntArray",
        tc->instance->boot_types->BOOTInt);
    MVM_gc_root_add_permanent(tc, (MVMCollectable **)&tc->instance->boot_types->BOOTIntArray);
    tc->instance->boot_types->BOOTNumArray = boot_typed_array(tc, "BOOTNumArray",
        tc->instance->boot_types->BOOTNum);
    MVM_gc_root_add_permanent(tc, (MVMCollectable **)&tc->instance->boot_types->BOOTNumArray);
    tc->instance->boot_types->BOOTStrArray = boot_typed_array(tc, "BOOTStrArray",
        tc->instance->boot_types->BOOTStr);
    MVM_gc_root_add_permanent(tc, (MVMCollectable **)&tc->instance->boot_types->BOOTStrArray);

    /* Get initial __6MODEL_CORE__ serialization context set up. */
    setup_core_sc(tc);
    MVM_6model_containers_setup(tc);
}
