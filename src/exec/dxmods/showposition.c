/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * $Header: /src/master/dx/src/exec/dxmods/showposition.c,v 1.6 2000/08/24 20:04:49 davidt Exp $
 */

#include <dxconfig.h>


#include <stdlib.h>
#include <string.h>
#include <dx/dx.h>
#include "_helper_jea.h"
#include "_getfield.h"
#include "showboundary.h"

#define  DEFAULT_COLOR  DXRGB(1.0,1.0,1.0)


#define  NEWBIT(n)      ((Pointer)DXAllocateLocalZero((((n)+31)/32)*4))
#define  SETBIT(a,i,v)  (((unsigned int*)(a))[((i)/32)]|=((v)<<((i)&31)))
#define  GETBIT(a,i)    (1&(((unsigned int*)(a))[((i)/32)]>>((i)&31)))


static
Field show_positions_every ( Field input, float every )
{
    int         i, j;
    float       float_items;
    int         validcount, newitems;
    Array       array      = NULL;
    int         *pull      = NULL;
    int         *aux       = NULL;
    array_info  p_info     = NULL;
    field_info  input_info = NULL;
    component_info    comp = NULL;
    InvalidComponentHandle
                  i_handle = NULL;

    if ( ERROR == ( i_handle = DXCreateInvalidComponentHandle
                                   ( (Object)input, NULL, "positions" ) ) )
         goto error;

    validcount = DXGetValidCount ( i_handle );

    if ( ( float_items = (float)validcount / every ) < 1.0 )
        float_items = 1.0;

    newitems = (int)float_items;

    if ( ( ERROR == ( pull = (int*) DXAllocate ( newitems * sizeof(int) ) ) ) ||
         ( ERROR == ( aux =  (int*) NEWBIT     ( validcount )             ) ) )
        goto error;

    if ( 0 != DXGetInvalidCount ( i_handle ) )
        for ( i=0; i<newitems; )
        {
            j = random() % validcount;

            if ( DXIsElementValidSequential ( i_handle, j ) &&
                 ( 0 == GETBIT ( aux, j ) ) )
                { pull[i++] = j; SETBIT ( aux, j, 1 ); }
        }
    else
        for ( i=0; i<newitems; )
        {
            j = random() % validcount;

            if ( 0 == GETBIT ( aux, j ) )
                { pull[i++] = j; SETBIT ( aux, j, 1 ); }
        }

    DXFree  ( (Pointer) aux );
    DXFreeInvalidComponentHandle ( i_handle );
    i_handle = NULL;  
    aux      = NULL;  

    if ( !DXSetComponentValue ( input, "invalid positions", NULL )
         ||
         ( ERROR == ( input_info = _dxf_InMemory ( input ) ) ) )
        goto error;

    p_info = (array_info) &(input_info->std_comps[(int)POSITIONS]->array);

    for ( i=0, comp=input_info->comp_list;
          i<input_info->comp_count;
          i++, comp++ )
        if ( ( NULL  != comp->std_attribs[(int)DEP]        ) &&
             ( ERROR != comp->std_attribs[(int)DEP]->value ) &&
             ( 0 == strcmp ( comp->std_attribs[(int)DEP]->value, "positions" )))
        {
            if ( ( ERROR == ( array = _dxf_resample_array
                                         ( comp, 1, p_info->items, newitems,
                                           NULL, pull ) ) )
                 ||
                 !DXSetComponentValue
                      ( input_info->field, comp->name, (Object)array ) )
                goto error;
        }

    if ( !_dxf_FreeInMemory ( input_info ) )
        goto error;

    return input;

    error:
        DXFreeInvalidComponentHandle ( i_handle );
        DXASSERT ( DXGetError() != ERROR_NONE );
        return ERROR; 
}



static
Field show_positions ( Field input, char *data, int size )
{
    int    i, j;
    Field  output = NULL;
    Object t  = NULL;
    char   *r = NULL;

    static char *relations[]  = { "dep", "ref", "der", NULL };
    static char *removables[] = { "connections", "normals", "faces", "loops",
                                  "invalid connections", "edges", NULL };

    DXASSERTGOTO ( input != NULL );
    DXASSERTGOTO ( ( data != NULL ) && ( size == sizeof(float) ) );
    DXASSERTGOTO ( DXGetObjectClass ( (Object)input ) == CLASS_FIELD );

    if ( DXEmptyField ( input ) ) return NULL;

    /*
     * 1/ Remove "connections".
     *        Renderability will default to a by points method.
     * 2/ (New) Remove "normals" component.
     * 3/ Remove any components that reference these components.
     * 4/ Ensure that "colors" exist.
     */

    if ( !DXExists ( (Object) input, "positions" ) )
        DXErrorGoto
            ( ERROR_DATA_INVALID, "No \"positions\" component in field" )

    if ( ERROR == ( output = (Field) DXCopy ( (Object)input, COPY_STRUCTURE ) ))
        goto error;


    /* Special scan to keep ChangedComponentStructure from deleting positions */

    for ( i=0; relations[i] != NULL; i++ )
    {
        if ( ( NULL != ( t = DXGetComponentAttribute
                                 ( output, "positions", relations[i] ) ) )
             &&
             ( ERROR == ( r = DXGetString ( (String) t ) ) ) )
            goto error;

        if ( t != NULL ) 
            for ( j=0; removables[j] != NULL; j++ )
                if ( 0 == strcmp ( r, removables[j] ) ) {
                    if ( !DXSetComponentAttribute
                              ( output, "positions", relations[i], NULL ) )
                        goto error;
                    else
                        DXWarning
                          ( "#4415", "positions", relations[i], removables[j] );
                /* `%s' component has unusual `%s' attribute: `%s'.  removing */
		}
    }


    for ( i=0; removables[i] != NULL; i++ )

        if ( DXDeleteComponent ( output, removables[i] ) )
        {
            if ( !DXChangedComponentStructure ( output, removables[i] ) )
                goto error;
        }
        else if ( DXGetError() != ERROR_NONE )
            goto error;

    if ( *((float *) data) != 1.0 )
        if ( !show_positions_every ( output, *((float *) data) ) )
            goto error;

    if ( !_dxf_SetDefaultColor  ( output, DEFAULT_COLOR )
         ||
         !_dxf_SetFuzzAttribute ( output, 2 ) ) 
        goto error;

    return output;

    error:
        DXDelete ( (Object) output );

        DXASSERT ( DXGetError() != ERROR_NONE );
        return ERROR; 
}



int
m_ShowPositions ( Object *in, Object *out )
{
    float every;

#define I_input  in[0]
#define I_every  in[1]
#define O_output out[0]

    if ( I_every == NULL ) every = 1.0;
    else
        if ( !DXExtractFloat ( I_every, &every ) || ( every < 1.0 ) )
            DXErrorGoto3
                ( ERROR_DATA_INVALID,
                  "#10121",
                  /* %s must be a scalar value greater than or equal to %g */
                  "parameter 'every'", 1.0 );

    if ( ( ERROR == ( O_output = DXProcessParts
                                    ( I_input, show_positions,
                                      /* args, size, copy, preserve */
                                      (Pointer)&every, sizeof(float), 1, 1 ) ) )
         ||
         ( DXGetError() != ERROR_NONE ) )
        goto error;

    return OK;

    error:
        if ( O_output != NULL ) DXDelete ( O_output );

        O_output = NULL;

        DXASSERT ( DXGetError() != ERROR_NONE );
        return ERROR;
}
