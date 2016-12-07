/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>



/*
 * Process Parts
 * John E. Allain
 * Apply a user specified function to each part of a group.
 * assemble groups with the same structure as the input.
 */

#include <string.h>
#include <dx/dx.h>


#define  PartsDebugChar         "P"
#define  ErrorGotoPlus1(e,s,a)  {DXSetError(e,s,a); goto error;}
#define  MessageGotoPlus1(s,a)  {DXAddMessage(s,a); goto error;}

/*
 * Running debate:
 *    The structure of _recurse is convolved because the convention 
 *    'copy' passes fields uncopied to the process_part worker.
 *    If it were copied first, and the worker modified components in
 *    place, then it could be cleaner.  This would however require the 
 *    worker to clean out beaucoup components when it really wants to 
 *    build a new field.
 *
 * Note significant changes:
 *    DXEmptyField check is no longer performed.
 *    The DXEndField call is now expected of the worker routine.
 *
 * Function naming convention:
 *
 *    Mixed case lettering, leading underscore:
 *            public (extern) routine, undocumented.
 *    Mixed case lettering, no underscores:
 *            public (extern) routine, documented.
 *    Lower case lettering, with underscores:
 *            private (static) routine, undocumented.
 * 
 * Cleanup convention:
 *   out_object is to be considered deletable at all times.
 */



/*-----------------------------------*\
| Repackage of the input for AddTask
\*-----------------------------------*/
typedef struct 
{
    /* User supplied items */
    Field       (*process_part) ( Field, char *, int );
    Object      self;
    Pointer     args;
    int         size;

    /* internal states */
    int         parallel;
    int         copy;
    int         preserve;

    Object      parent;
    char        *member_name;
    int         group_member_position;
    float       series_FP_value;
}
part_arg_type;

static part_arg_type part_arg_initializer
    = { NULL, NULL, NULL, 0,   1, 1, 0,   NULL, NULL, 0, 0.0 };


static
/*
 * Set a child for an existing parent as found in arg->parent.
 *   Specifically, arg->parent will refer to child, using the
 *     referencing mechanism appropriate to that object.
 *   Note that on first call with a given parent/child combination
 *     that the child is is not so much replaced as placed.
 *
 * The caller is expected to set 'child' to null following this call,
 *   for the purposes of not deleting it twice during error cleanup.
 */
Error _replace_child ( part_arg_type *arg, Object child )
{
    DXASSERTGOTO ( arg         != NULL );
    DXASSERTGOTO ( child       != NULL );
    DXASSERTGOTO ( arg->copy   == 1    );
    DXASSERTGOTO ( arg->parent != NULL );

    /* Trace */
    if ( DXQueryDebug ( PartsDebugChar ) ) {
        if ( ( DXGetObjectClass ( child ) == CLASS_FIELD )
             &&
             DXEmptyField ( (Field) child ) )
            DXDebug ( PartsDebugChar, "_replace_child with DXEmptyField" );
        else
            DXDebug ( PartsDebugChar, "_replace_child" );
     }

    switch ( DXGetObjectClass ( arg->parent ) )
    {
        case CLASS_GROUP:
            switch ( DXGetGroupClass ( (Group)arg->parent ) )
            {
                case CLASS_SERIES:
                    if ( !DXSetSeriesMember
                              ( (Series)arg->parent,
                                arg->group_member_position,
                                arg->series_FP_value,
                                child ) )
                        goto error;
                    break;

                default:
                    /* member name association can preserve order */
                    if ( arg->preserve )
                    {
                        if ( arg->member_name == NULL )
                        {
                            if ( !DXSetEnumeratedMember
                                     ( (Group)arg->parent,
                                        arg->group_member_position,
                                        child ) )
                                goto error;
                        }
                        else
                        {
                            if ( !DXSetMember
                                     ( (Group)arg->parent,
                                        arg->member_name,
                                        child ) )
                                goto error;
                        }
                    }
                    else
                        if ( !DXSetMember
                                 ( (Group)arg->parent,
                                    arg->member_name,
                                    child ) )
                            goto error;
            }
            break;

        case CLASS_XFORM:
            if ( !DXSetXformObject ( (Xform)arg->parent, child ) )
                goto error;
            break;

        case CLASS_SCREEN:
            if ( !DXSetScreenObject ( (Screen)arg->parent, child ) )
                goto error;
            break;

        case CLASS_CLIPPED:
            if ( !DXSetClippedObjects ( (Clipped)arg->parent, child, NULL ) )
                goto error;
            break;

        default:
           DXErrorGoto
               ( ERROR_ASSERTION,
                 "#11530" /* ProcessParts: Impossible parent */ );
    }

    return OK;

    error:
        ASSERT ( DXGetError() != ERROR_NONE );

        return ERROR;
}


/*-----------------------------------------*\
| The Leaf Processor called by AddTask
\*-----------------------------------------*/
static
/* 
 * Call the process_part 'worker'.
 *     Do so in a way acceptable to DXAddTask.
 *     Check the return states.
 */
Error _call_process_part ( Pointer p )
{
    part_arg_type  *arg       = (part_arg_type *) p;
    Object         out_object = NULL;

    DXASSERTGOTO ( arg != NULL );

    out_object = (Object)
                 arg->process_part
                     ( (Field)arg->self, (char *)arg->args, arg->size );

    /*
     * Check error states and trivial return cases.
     */
    if ( out_object == NULL )
    {
        if ( DXGetError() != ERROR_NONE )
            goto error;

        else 
        {
            /* 
             * If preserve case or series groups, DXEmptyField's have already 
             * been put in as placeholders.
             * Else, no need to use result.  Either way OK
             */ 
            arg->self = NULL;

            /* 
             * But...
             * Ensure that DXEmptyField is returned if a single-level hierarchy.
             * (if parent is not NULL then a placeholder is there if need be)
             */
            if ( ( arg->copy ) && ( arg->parent == NULL ) )
            {
                if ( NULL == ( arg->self = (Object) DXNewField() ) )
                    goto error;
            }

            return OK;
        }
    }
    else
        if ( DXGetError() != ERROR_NONE )
            MessageGotoPlus1
                ( "#8000",
                  /* %s set the error code, but did not return error */
                  "'process_part' worker" )

    /*
     * Let's see if a valid object.
     *     And if so, use it.
     */
    if ( arg->copy )
    {
        if ( arg->self == out_object )
            DXErrorGoto
                ( ERROR_ASSERTION, "#11500"
       /* ProcessParts: worker output can't equal input when called with copy */
                );

        if ( arg->parent == NULL )
            arg->self = out_object;
        else
        {
            if ( !_replace_child ( arg, out_object ) )
                goto error;

            arg->self  = out_object;
            out_object = NULL;
        }
    }
    else
        if ( arg->self != out_object )
           DXErrorGoto
               ( ERROR_ASSERTION, "#11510"
     /* ProcessParts: worker output can't equal input when called with nocopy */
               );

    return OK;

    error:
        if ( ( out_object != NULL ) && ( out_object != arg->self ) )
            DXDelete ( out_object );

        ASSERT ( DXGetError() != ERROR_NONE );

        return ERROR;
}


/*--------------------------------*\
| The Group Processor [Recursive] 
\*--------------------------------*/
static
Error _recurse ( part_arg_type *arg, Field placeholder  )
{
    part_arg_type  call_arg;
    Object         out_object = NULL;
    Class          class;

    /* 
     * Determine if a Field or Non-Field, and act accordingly.
     */
    if ( ( class = DXGetObjectClass ( arg->self ) ) == CLASS_FIELD )
    {
        if ( arg->parallel == 0 )
        {
            if ( !_call_process_part ( (Pointer)arg ) ) goto error;

            out_object = arg->self;
        }
        else
        {
            /*
             * (insure that there is a place to put result during parallelism) 
             */
            DXASSERTGOTO ( arg->parent != NULL );

            /*
             * Recall that DXAddTask will copy its input when
             * it is passed in as non-zero length.
             */
            if ( !DXAddTask ( _call_process_part,
                            (Pointer)arg,
                            sizeof( part_arg_type ),
                            (double)1.0 ) )
                goto error;

            /*
             * Install a place-holder if called for; And that is:
             *     copy and preserve mode
             *     or copy mode with a series group parent
             *   The actual output will be placed after ExectueTaskGroup() 
             *     is called.
             */
            if ( arg->copy )
                if ( arg->preserve 
                     ||
                     ( ( DXGetObjectClass ( arg->parent )
                             == CLASS_GROUP )
                       &&
                       ( DXGetGroupClass ( (Group)arg->parent )
                             == CLASS_SERIES ) ) )
                    if ( !_replace_child ( arg, (Object)placeholder ) )
                        goto error;
        }
    }
    else /* class != CLASS_FIELD */
    {
        call_arg              = part_arg_initializer;
        call_arg.size         = arg->size;
        call_arg.args         = arg->args;
        call_arg.process_part = arg->process_part;
        call_arg.parallel     = arg->parallel;
        call_arg.copy         = arg->copy;
        call_arg.preserve     = arg->preserve;
    
        if ( arg->copy == 0 ) 
            call_arg.parent = arg->self;
        else
        {
            /*
             * XXX
             * It is not entirely known how this DXCopy will behave across
             *   the different object types.
             */
            if ( !( out_object = DXCopy ( arg->self, COPY_ATTRIBUTES ) ) )
                goto error;

            call_arg.parent = out_object;
        }

        switch ( class )
        {
            case CLASS_GROUP:

                switch ( class = DXGetGroupClass ( (Group) arg->self ) )
                {
                    case CLASS_SERIES:
                        call_arg.member_name = NULL;
                        for(call_arg.group_member_position = 0;
                            (call_arg.self=DXGetSeriesMember
                                      ( (Series)arg->self,
                                         call_arg.group_member_position,
                                         &call_arg.series_FP_value ));
                             call_arg.group_member_position++ )
                            if ( !_recurse ( &call_arg, placeholder ) )
                                goto error;

                        break;

                    default:
                        call_arg.series_FP_value = 0.0;
                        for(call_arg.group_member_position = 0;
                            (call_arg.self=DXGetEnumeratedMember
                                        ( (Group)arg->self,
                                          call_arg.group_member_position,
                                          &call_arg.member_name ));
                             call_arg.group_member_position++ )
                            if ( ! _recurse ( &call_arg, placeholder ) )
                                goto error;

                        /* XXX above */
                }
                break;

            case CLASS_XFORM:
                if ( !DXGetXformInfo ( (Xform)arg->self, &call_arg.self, NULL )
                     ||
                     !_recurse ( &call_arg, placeholder ) )
                    goto error;
                break;

            case CLASS_SCREEN:
                if ( !DXGetScreenInfo
                         ( (Screen)arg->self, &call_arg.self, NULL,NULL )
                     ||
                     !_recurse ( &call_arg, placeholder ) )
                    goto error;
                break;

            case CLASS_CLIPPED:
                if ( !DXGetClippedInfo
                          ( (Clipped)arg->self, &call_arg.self, NULL )
                     ||
                     !_recurse ( &call_arg, placeholder ) )
                    goto error;
                break;

            default:
                DXASSERTGOTO ( class != CLASS_FIELD );
                DXDebug ( PartsDebugChar, "funny class: %d", class );
        }

        /* 
         * Place the computed output in the new hierarchy;
         */
        if ( arg->copy )
        {
            if ( arg->parent == NULL )
                arg->self = out_object;
            else
            {
                if ( !_replace_child ( arg, out_object ) )
                    goto error;

                arg->self  = out_object;
                out_object = NULL;
            }
        }
    }

    return OK;

    error:
        if ( ( out_object != NULL ) && ( out_object != arg->self ) )
            DXDelete ( out_object );

        ASSERT ( DXGetError() != ERROR_NONE );

        return ERROR;
}


static int _protected_parallel_switch = 1;

extern
Object _dxfProcessPartsNP ( Object  object,
                        Field   (*process_part) ( Field, char *, int ),
                        Pointer args,
                        int     size,
                        int     copy,
                        int     preserve )
{
    Object out_object;

    _protected_parallel_switch = 0;

    out_object = DXProcessParts
                     ( object, process_part, args, size, copy, preserve );

    _protected_parallel_switch = 1;

    return out_object;

}


extern
Object DXProcessParts ( Object  object,
                      Field   (*process_part) ( Field, char *, int ),
                      Pointer args,
                      int     size,
                      int     copy,
                      int     preserve )
{
    /** prototype new argument **/
    int           parallel         = _protected_parallel_switch;
    /**/

    part_arg_type call_arg;
    Pointer       args_copy_global = NULL;
    Field         placeholder      = NULL;

    call_arg = part_arg_initializer;

    if (object == NULL) {
        DXSetError (ERROR_MISSING_DATA, "#10000", "Object");
	return NULL;
    }

    call_arg.self         = object;
    call_arg.args         = args;
    call_arg.size         = size;
    call_arg.process_part = process_part;
    call_arg.parallel     = parallel;
    call_arg.copy         = copy;
    call_arg.preserve     = preserve;

    /*
     * Note: the assumption currently is made that sizeof(args) = size
     */
    if (size > 0 && args != NULL)
    {
       /*
	* Assume worst case that the argument 'args' is a pointer to
	* local memory.  For this reason, 'args' is copied to global
	* memory to ensure validity across all processors accessed by
	* DXAddTask.
	*/

	if ((args_copy_global = DXAllocate (size)) == ERROR)
	    goto error;

	memcpy (args_copy_global, args, size);
	call_arg.args = args_copy_global;
    }


    /*
     * When not copying, then order is preserved as a byproduct of
     * the modify in place operation.  Preserve setting can be ignored.
     */

    switch (DXGetObjectClass (call_arg.self))
    {
        case CLASS_FIELD:
            call_arg.parallel = 0;

        case CLASS_GROUP:
        case CLASS_XFORM:
        case CLASS_SCREEN:
        case CLASS_CLIPPED:
            if ((call_arg.copy == 1) && (call_arg.parallel == 1))
                /* 
                 * placeholder will be needed in cases of explicit preserve
                 * order, or for series groups, which imply this.
                 */
                if (((placeholder = DXNewField()) == NULL)
                     ||
                     !DXReference ((Object)placeholder))
                    goto error;

            if (call_arg.parallel == 1)
            {
                if (!DXCreateTaskGroup() ||
                    ! _recurse (&call_arg, placeholder) ||
                    !DXExecuteTaskGroup())
                    goto error;
            }
            else
                if (! _recurse (&call_arg, placeholder))
                    goto error;

            DXDelete ((Object)placeholder);
            break;

        default:
            DXSetError (ERROR_BAD_CLASS, "#10190", "Object");
	    goto error;
    }

    if (args_copy_global && !DXFree (args_copy_global))
        goto error;
    else
        args_copy_global = NULL;

    return (call_arg.self);

    error:
        if ( copy && ( call_arg.self != object ) )
            DXDelete ( call_arg.self );

        if ( args_copy_global )
            DXFree ( args_copy_global );

        DXDelete ( (Object)placeholder );

        return ERROR;
}



extern
Object _dxfProcessPartsG ( Object  object,
                           Group   (*process_part) ( Group, char *, int ),
                           Pointer args,
                           int     size,
                           int     copy,
                           int     preserve )
{
    Object child  = NULL;
    Group  result = NULL;
    int    i;

/* Parallel !!! */

#if 0
"#11500"
/* ProcessParts: worker output cannot equal input when called with copy */
"#11520"
/* ProcessParts: args is NULL but size is nonzero */
#endif

    if ( ( copy     != 0 ) ||
         ( preserve != 1 )   )
        DXErrorGoto
            ( ERROR_NOT_IMPLEMENTED,
              "DXProcessPartsG must be called with copy = 0, preserve = 1" );

    switch ( DXGetObjectClass ( object ) )
    {
        case CLASS_FIELD:
        case CLASS_ARRAY:
        default:
#if 0
            DXErrorGoto ( ERROR_BAD_CLASS, "encountered in object traversal" );
#endif
            break;

        case CLASS_GROUP:
            switch ( DXGetGroupClass ( (Group) object ) )
            {
                case CLASS_GROUP:
                default:
                    for (i = 0;
                         ( NULL != ( child = DXGetEnumeratedMember
                                                 ( (Group)object, i, NULL ) ) );
                         i++ )
                        if ( !_dxfProcessPartsG
                                  ( child, process_part,
                                    args, size, copy, preserve ) )
                            goto error;
#if 0
DXSetEnumeratedMember((Group)o, i, n);
#endif

                case CLASS_COMPOSITEFIELD:
                case CLASS_SERIES:
                    if ( ERROR == ( result = process_part
                                                 ( (Group)object,
                                                   (char*)args, size ) ) )
                        goto error;

                    if ( ( copy == 0 )
                         &&
                         ( result != (Group)object ) )
                        DXErrorGoto
                            ( ERROR_ASSERTION, "#11510"
                              /* ProcessParts: worker output must equal
                                 input when called with nocopy */ );
            }
            break;

        case CLASS_XFORM:
            if ( !DXGetXformInfo ( (Xform)object, &child, NULL ) )
                goto error;

            if ( !_dxfProcessPartsG
                      ( child, process_part, args, size, copy, preserve ) )
                goto error;
            break;

#if 0
DXSetXformObject((Xform)o,n);
#endif

        case CLASS_SCREEN:
            if ( !DXGetScreenInfo((Screen)object, &child, NULL, NULL ) )
                goto error;

            if ( !_dxfProcessPartsG
                      ( child, process_part, args, size, copy, preserve ) )
                goto error;
            break;

#if 0
DXSetScreenObject((Screen)o,n);
#endif

        case CLASS_CLIPPED:
            if ( !DXGetClippedInfo((Clipped)object, &child, NULL ) )
                goto error;

            if ( !_dxfProcessPartsG
                      ( child, process_part, args, size, copy, preserve ) )
                goto error;
            break;

#if 0
DXSetClippedObjects((Clipped)o,n,NULL);
#endif
    }

    return object;

    error:
        return ERROR;

}
