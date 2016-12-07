/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>

#if defined(HAVE_STRING_H)
#include <string.h>
#endif

#if defined(HAVE_STRINGS_H)
#include <strings.h>
#endif

#include "hwDeclarations.h"
#include "hwXfield.h"
#include "hwMatrix.h"
#include "hwMemory.h"
#include "hwPortLayer.h"
#include "hwWindow.h"
#include "hwObjectHash.h"

#define String dxString
#define Object dxObject
#define Angle dxAngle
#define Matrix dxMatrix
#define Screen dxScreen
#define Boolean dxBoolean
#include "../libdx/internals.h"
#undef String
#undef Object
#undef Angle
#undef Matrix
#undef Screen
#undef Boolean

#include "hwDebug.h"

static Error _gammaCorrectColors(xfieldP xf, double gamma, int isLit);

#define CAT(x,y) x##y

#define CHECK_CONST_ARRAY(name) 					\
  if(DXQueryConstantArray(xf->CAT(name,_array),NULL,NULL))		\
    xf->CAT(name,Dep) = dep_field;

/*
 * get a component array
 */

#define array(array, name, required) {					    \
    xf->array = (Array) DXGetComponentValue(f, name);			    \
    if (!xf->array) {							    \
	if (required) {							    \
	    DXSetError(ERROR_MISSING_DATA, "#10240", name);  		    \
	    EXIT(("ERROR: missing data"));				    \
	    return ERROR;						    \
	} 								    \
        EXIT(("OK"));				    			    \
	return OK;							    \
    }									    \
    else 								    \
      DXReference((dxObject)xf->array);				    	    \
}


/*
 * check the type of a component
 */

#define check(comp, name, t, dim) {					     \
    if (dim==0 || dim==1) {						     \
	if (!DXTypeCheck(xf->CAT(comp,_array), t, CATEGORY_REAL, 0) &&	     \
	  !DXTypeCheck(xf->CAT(comp,_array), t, CATEGORY_REAL, 1, 1)) {	     \
	    DXSetError(ERROR_DATA_INVALID, "#11829", name);		     \
	    EXIT(("ERROR: invalid data"));				     \
	    return ERROR;						     \
	}								     \
    } else {								     \
	if (!DXTypeCheck(xf->CAT(comp,_array), t, CATEGORY_REAL, 1, dim))    \
	{								     \
	    DXSetError(ERROR_DATA_INVALID, "#11829", name); 		     \
	    EXIT(("ERROR: invalid data"));				     \
	    return ERROR;						     \
	}								     \
    }									     \
}


/*
 * get the component data and number of items
 */

#define get(comp) {							     \
 xf->comp = DXCreateArrayHandle(xf->CAT(comp,_array));                       \
 if (!xf->comp) {						             \
   EXIT(("returning NULL"));			     			     \
   return ERROR; 						             \
 }								             \
 DXGetArrayInfo(xf->CAT(comp,_array),&xf->CAT(n,comp), NULL,NULL,NULL,NULL); \
}

/*
 * Get the component data, and check its number of items
 * against another component.  Note - we only consider it a mismatch
 * if xf->count is non-zero; this for example allows _XOpacities to
 * work even if we haven't done _XColors (as for example in _Survey).
 */

#define compare(comp, name, count) {				\
    int n;							\
    get(comp);							\
    n = xf->CAT(n,comp);					\
    if (n!=count && count!=0) {					\
	DXSetError(ERROR_DATA_INVALID,"#13050",			\
		 name, n, count);				\
        EXIT(("ERROR: invalid data"));			     	\
	return ERROR;						\
    }								\
}

/*=====================================================================*\
  Xfield component functions
\*=====================================================================*/
extern Error _dxfTriangulateField(Field);
extern Error _dxf_XNeighbors(Field f, xfieldT *xf, enum xr required, enum xd xd);
extern Error _dxf_linesToPlines (xfieldT *xf);
extern Error _dxf_linesToPlines (xfieldT *xf);
extern Error _dxf_trisToTmesh (xfieldT *xf, tdmChildGlobalP globals);
extern Error _dxf_quadsToQmesh (xfieldT *xf, void *globals);

/*
 * box, points, connections
 */

static Error
_XBox(Field f, xfieldT* xf, enum xr required)
{
  ENTRY(("_XBox(0x%x, 0x%x, %d)", f, xf, required));

  if(DXBoundingBox((dxObject)f,(Point *) xf->box)) 
  {
    float xMin = DXD_MAX_FLOAT, xMax = -DXD_MAX_FLOAT;
    float yMin = DXD_MAX_FLOAT, yMax = -DXD_MAX_FLOAT;
    float zMin = DXD_MAX_FLOAT, zMax = -DXD_MAX_FLOAT;
    int i;
    Point *p = (Point *)xf->box;

    for (i = 0; i < 8; i++, p++)
    {
	if (xMin > p->x) xMin = p->x;
	if (yMin > p->y) yMin = p->y;
	if (zMin > p->z) zMin = p->z;
	if (xMax < p->x) xMax = p->x;
	if (yMax < p->y) yMax = p->y;
	if (zMax < p->z) zMax = p->z;
    }

    p = (Point *)xf->box;

    p->x = xMin; p->y = yMin; p->z = zMin; p++;
    p->x = xMin; p->y = yMin; p->z = zMax; p++;
    p->x = xMin; p->y = yMax; p->z = zMin; p++;
    p->x = xMin; p->y = yMax; p->z = zMax; p++;
    p->x = xMax; p->y = yMin; p->z = zMin; p++;
    p->x = xMax; p->y = yMin; p->z = zMax; p++;
    p->x = xMax; p->y = yMax; p->z = zMin; p++;
    p->x = xMax; p->y = yMax; p->z = zMax;

    EXIT(("OK"));
    return OK;
  } else {
    EXIT(("ERROR"));
    return ERROR;
  }
}

static Error
_XPositions(Field f, xfieldT* xf, enum xr required)
{
  ENTRY(("_XPositions(0x%x, 0x%x, %d)", f, xf, required));

  array(positions_array, POSITIONS, required);
  get(positions);
  DXGetArrayInfo(xf->positions_array,NULL,NULL,NULL,NULL,&xf->shape);

  if (DXGetComponentValue(f, INVALID_POSITIONS)) {
    /* This also creates DXReference() */
    if((xf->invPositions = DXCreateInvalidComponentHandle((dxObject)f,
							  NULL,
							  POSITIONS))) {
    } else {
      EXIT(("ERROR"));
      return ERROR;
    }
  }

  EXIT(("OK"));
  return OK;
}

static Error
_XPolylines(Field f, xfieldT *xf, enum xr required)
{
    array(polylines_array, POLYLINES, required);
    check(polylines, "polylines", TYPE_INT, 0);

    xf->polylines = (int *)DXGetArrayData(xf->polylines_array);
    DXGetArrayInfo(xf->polylines_array,
		&xf->npolylines, NULL, NULL, NULL, NULL);

    /* edges required if polylines present */
    array(edges_array, "edges", required);
    check(edges, "edges", TYPE_INT, 0);

    xf->edges = (int *)DXGetArrayData(xf->edges_array);
    DXGetArrayInfo(xf->edges_array,
		&xf->nedges, NULL, NULL, NULL, NULL);

    xf->connectionType = ct_polylines;
    xf->nconnections = xf->npolylines;

    if (DXGetComponentValue(f, "invalid polylines"))
    {
      if (NULL == (xf->invCntns = DXCreateInvalidComponentHandle((dxObject)f,
								 NULL, "polylines")))
	  return ERROR;
    }
						    
    return OK;
}

static Error
_XConnections(Field f, xfieldT* xf, enum xr required)
{
    static struct info {
	char *name;	/* element type attribute */
	connectionTypeT ct;	/* corresponding ct */
	int n;		/* ints per item */
	int volume;	/* whether these are volume connections */
	int posPerConn;	/* positions per connection */
    } info[] = {
	{ "lines",	ct_lines,	2,	0,	2 },
	{ "triangles",	ct_triangles,	3,	0,	3 },
	{ "quads",	ct_quads,	4,	0,	4 },
	{ "tetrahedra",	ct_tetrahedra,	4,	1,	3 },
	{ "cubes",	ct_cubes,	8,	1,	4} ,
	{ NULL }
    };
    struct info *i;
    dxObject cto;
    char *s;

    ENTRY(("_XConnections(0x%x, 0x%x, %d)", f, xf, required));
    
    xf->connectionType = ct_none;
    array(connections_array, CONNECTIONS, required);
    cto = DXGetAttribute((dxObject)xf->connections_array, ELEMENT_TYPE);
    s = DXGetString((dxString)cto);

    if (!s) {
      EXIT(("ERROR"));
      DXErrorReturn(ERROR_BAD_PARAMETER, "#13070");
    }

    /* try to find it */
    for (i=info; i->name; i++) {
	if (strcmp(s, i->name)==0) {
	    check(connections, "connections", TYPE_INT, i->n);
	    get(connections);
	    DXGetArrayInfo(xf->connections_array, &xf->nconnections,
			 NULL,NULL,NULL,NULL);
	    xf->connectionType = i->ct;
	    xf->posPerConn = i->posPerConn;
	    if(i->volume)
	      _dxf_setFlags(_dxf_attributeFlags(_dxf_xfieldAttributes(xf)),
			    CONTAINS_VOLUME);
	    if (DXGetComponentValue(f, INVALID_CONNECTIONS)) {
	      /* This also creates DXReference() */
	      if((xf->invCntns = DXCreateInvalidComponentHandle((dxObject)f,
								 NULL,
								 CONNECTIONS))){
	      } else {
		EXIT(("ERROR"));
		return ERROR;
	      }
	    }
	    EXIT(("OK"));
	    return OK;
	}
    }

    DXSetError(ERROR_BAD_PARAMETER, "#10410", i->ct);
    EXIT(("ERROR"));
    return ERROR;
}
		

static Error
_XNeighbors(Field f, xfieldT* xf, enum xr required)
{
    int n;
    
    ENTRY(("_XNeighbors(0x%x, 0x%x, %d)", f, xf, required));
    
    if (xf->neighbors_array) {
      EXIT(("OK: xf->neighbors_array != NULL"));
      return OK;
    }

    /* If a regular connections_array */
    if (DXQueryGridConnections(xf->connections_array, &n, xf->k)) {
      if( xf->connectionType==ct_cubes && n != 3) {
	DXSetError(ERROR_DATA_INVALID,
		   "#13155","cubes",3);
	goto error;
      } else if( xf->connectionType==ct_quads && n != 2) {
	DXSetError(ERROR_DATA_INVALID,
		   "#13155","quads",2);
	goto error;
      } else if( xf->connectionType==ct_lines && n != 1) {
	DXSetError(ERROR_DATA_INVALID,
		   "#13155","lines",1);
	goto error;
      }
      EXIT(("OK"));
      return OK;
    } else {
      xf->neighbors_array = (Array) DXGetComponentValue(f, NEIGHBORS);
      if(!xf->neighbors_array) {
	xf->neighbors_array = DXNeighbors(f);
	if (!xf->neighbors_array) {
	  if (required) {
	    DXSetError(ERROR_MISSING_DATA, "#10240", NEIGHBORS);
	    EXIT(("ERROR"));
	    return ERROR;
	  } else {
	    EXIT(("OK"));
	    return OK;
	  }
	}
      }
      DXReference((dxObject)xf->neighbors_array);
      check(neighbors, "neighbors", TYPE_INT, 
	    DXGetItemSize((Array)xf->connections_array)/DXTypeSize(TYPE_INT));
      compare(neighbors, "neighbors", xf->nconnections);
    }

    EXIT(("OK"));
    return OK;

  error:

    EXIT(("ERROR"));
    return ERROR;
}


/*
 *
 */

static Error
_XColors(Field f, xfieldT* xf,
	     enum xr required)
{
  Array colors_array;
  char *fs, *bs, *s = NULL;
  dependencyT color_dep;
  int		ncolors;
  
  ENTRY(("_XColors(0x%x, 0x%x, %d)", f, xf, required));

  color_dep = dep_none;
  
  /* get front/back colors arrays */
#if 0 /* we currently don't support back colors in HW */
  colors_array = (Array) DXGetComponentValue(f, COLORS);
  xf->fcolors_array = (Array) DXGetComponentValue(f, FRONT_COLORS);
  xf->bcolors_array = (Array) DXGetComponentValue(f, BACK_COLORS);
  if (!xf->fcolors_array) xf->fcolors_array = colors_array;
  if (!xf->bcolors_array) xf->bcolors_array = colors_array;
#else
  xf->fcolors_array =  (Array) DXGetComponentValue(f, COLORS);
  if(!xf->fcolors_array)
    xf->fcolors_array =  (Array) DXGetComponentValue(f, FRONT_COLORS);
  if(!xf->fcolors_array)
    xf->fcolors_array =  (Array) DXGetComponentValue(f, BACK_COLORS);
  colors_array = xf->bcolors_array = xf->fcolors_array;
#endif
  if (!xf->fcolors_array && !xf->bcolors_array) {
    if (required==XR_REQUIRED) {
      DXSetError(ERROR_MISSING_DATA, "#13060","colors");
      EXIT(("ERROR"));
      return ERROR;
    } else {
      EXIT(("OK"));
      return OK;
    }
  } else {
    DXReference((dxObject)xf->fcolors_array);
    DXReference((dxObject)xf->bcolors_array);
  }
  
  /* find dependency */
  if(color_dep != dep_field) {
    fs =DXGetString((dxString)DXGetAttribute((dxObject)xf->fcolors_array, DEP));
    bs =DXGetString((dxString)DXGetAttribute((dxObject)xf->bcolors_array, DEP));
    if (!fs || !bs)
      s = DXGetString((dxString)DXGetAttribute((dxObject)colors_array, DEP));
    if (!fs) fs = s;
    if (!bs) bs = s;
    if (fs && bs && strcmp(fs,bs)!=0) {
      DXSetError(ERROR_BAD_PARAMETER, 
		 "#10256","colors, front colors or back colors","dep");
      EXIT(("ERROR"));
      return ERROR;
    }

    s = fs? fs : bs;
    if (!s) {
      DXSetError(ERROR_MISSING_DATA, "#10255","colors","dep");
      EXIT(("ERROR"));
      return ERROR;
    }
    if (strcmp(s,"positions")==0) color_dep = dep_positions;
    else if (strcmp(s,"connections")==0) color_dep = dep_connections;
    else if (strcmp(s,"polylines")==0) color_dep = dep_polylines;
    else {
      DXSetError(ERROR_MISSING_DATA, "#10256",
		 "colors, front colors or back colors","dep");
      EXIT(("ERROR"));
      return ERROR;
    }
    xf->fcolorsDep = color_dep;
  }

  /* number of colors */
  DXGetArrayInfo(xf->fcolors_array? xf->fcolors_array : xf->bcolors_array,
		 &ncolors, NULL, NULL, NULL, NULL);

  if (xf->fcolors_array) {
    Type type;
    DXGetArrayInfo(xf->fcolors_array, NULL, &type, NULL, NULL, NULL);
    if (type==TYPE_UBYTE) {
      /*  FIXME:  Software rendering knows how to deal with ubyte[3] colors  */
      /*      but hardware rendering doesn't.  See also back colors below.   */
      check(fcolors, "colors", TYPE_UBYTE, 1);
      array(cmap_array, "color map", 1);
      check(cmap, "color map", TYPE_FLOAT, 3);
      compare(cmap, "color map", 256);
    } else {
      check(fcolors, "colors", TYPE_FLOAT, 3);
    }
    compare(fcolors, "colors", ncolors);
  }

  if (xf->bcolors_array) {
    Type type;
    DXGetArrayInfo(xf->bcolors_array, NULL, &type, NULL, NULL, NULL);
    if (type==TYPE_UBYTE) {
      check(bcolors, "colors", TYPE_UBYTE, 1);
      if (!xf->cmap_array) {
	if (xf->fcolors_array) {
	  EXIT(("ERROR"));
	  DXErrorReturn(ERROR_BAD_PARAMETER,"#11812");
	}
	array(cmap_array, "color map", 1);
	compare(cmap, "color map", 256);
      }
    } else {
      if (xf->cmap_array) {
	EXIT(("ERROR"));
	DXErrorReturn(ERROR_BAD_PARAMETER,"#11812");
      }
      check(bcolors, "colors", TYPE_FLOAT, 3);
    }
    compare(bcolors, "colors", ncolors);
  }

  CHECK_CONST_ARRAY(fcolors);
  xf->colorsDep = xf->fcolorsDep;

  EXIT(("OK"));
  return OK;
}


/*
 * Normals
 */
static Error
_XNormals(Field f, xfieldT* xf,
	      enum xr required)
{
    char *s;
    dependencyT normal_dep;

    ENTRY(("_XNormals(0x%x, 0x%x, %d)", f, xf, required));
    
    normal_dep = dep_none;
    array(normals_array, NORMALS, required);
    if (xf->normals_array && DXGetComponentValue(f, "binormals")) {
	/* not surface normals */
	xf->normals_array = NULL;
	EXIT(("not surface normals"));
	return OK;
    }
    check(normals, "normals", TYPE_FLOAT, 3);
    if(normal_dep != dep_field) {
      s = DXGetString((dxString)DXGetAttribute((dxObject)xf->normals_array, DEP));
      if (strcmp(s,"positions")==0) normal_dep = dep_positions;
      else if (strcmp(s,"connections")==0) normal_dep = dep_connections;
      else if (strcmp(s,"polylines")==0) normal_dep = dep_polylines;
#if 0
      else if (strcmp(s,"faces")==0) normal_dep = dep_faces;
#endif
      else {
	DXSetError(ERROR_MISSING_DATA, "#10256","normals","dep");
	EXIT(("ERROR"));
	return ERROR;
      }
      if (normal_dep==dep_positions) {
	compare(normals, "normals", xf->npositions);
      } else if (normal_dep==dep_connections) {
	compare(normals, "normals", xf->nconnections);
      } else if (normal_dep==dep_polylines) {
	compare(normals, "normals", xf->npolylines);
      } else {
	DXSetError(ERROR_DATA_INVALID, "#10256","normals","dep");
	EXIT(("ERROR"));
	return ERROR;
      }
    }
    _dxf_setFlags(_dxf_attributeFlags(_dxf_xfieldAttributes(xf)),
		  CONTAINS_NORMALS);
    xf->normalsDep = normal_dep;
    CHECK_CONST_ARRAY(normals);

    EXIT(("OK"));
    return OK;
}


static Error
_XOpacities(Field f, xfieldT* xf,
		enum xr required)
{
    Type type;
    dependencyT opacity_dep;

    ENTRY(("_XOpacities(0x%x, 0x%x, %d)", f, xf, required));
    
    opacity_dep = dep_none; /* Should Be Replaced ... (:Q= */

    array(opacities_array, OPACITIES, required);
    if (xf->opacities_array)
    {
	if (xf->fcolors_array)
	{
	    dxObject o;
	    char *s;

	    if(opacity_dep != dep_field)
	    {
	        o = DXGetComponentAttribute(f, OPACITIES, DEP);
	        if (! o)
		{
		    DXSetError(ERROR_DATA_INVALID, "#10255","opacities","dep");
		    EXIT(("ERROR"));
		    return ERROR;
		}
	    }
	
	    s = DXGetString((dxString)o);
	    if (! s)
	    {
	        DXSetError(ERROR_DATA_INVALID, "#10256","opacities","dep");
	        EXIT(("ERROR"));
	        return ERROR;
	    }
	
	    if (strcmp(s,"positions")==0) opacity_dep = dep_positions;
	    else if (strcmp(s,"connections")==0) opacity_dep = dep_connections;
            else if (strcmp(s,"polylines")==0) opacity_dep = dep_polylines;
	    else
	    {
	        DXSetError(ERROR_MISSING_DATA,"#10256","opacities","dep");
	        EXIT(("ERROR"));
	        return ERROR;
	    }

	    if (opacity_dep==dep_positions)
	    {
	        compare(opacities, "opacities", xf->npositions);
	    }
	    else if (opacity_dep==dep_connections)
	    {
	        compare(opacities, "opacities", xf->nconnections);
	    }
	    else if (opacity_dep==dep_polylines)
	    {
	        compare(opacities, "opacities", xf->npolylines);
	    }
	    xf->opacitiesDep= opacity_dep;
	}

	DXGetArrayInfo(xf->opacities_array, NULL, &type, NULL, NULL, NULL);

	if (type==TYPE_UBYTE)
	{
	    check(opacities, "opacities", TYPE_UBYTE, 1);
	    array(omap_array, "opacity map", 1);
	    compare(omap, "opacity map", 256);
	}
	else
	{
	    check(opacities, "opacities", TYPE_FLOAT, 1);
	}

	CHECK_CONST_ARRAY(opacities);

	_dxf_setFlags(_dxf_attributeFlags(_dxf_xfieldAttributes(xf)),
		  CONTAINS_TRANSPARENT);
    }
    else
    {
	xf->opacities = NULL;
	xf->nopacities = 0;
	xf->opacitiesDep = dep_none;
    }

    EXIT(("OK"));
    return OK;
}


/*
static Error
_XFreeLocal(xfieldT* xf)
{
  ENTRY(("_XFreeLocal(0x%x)", xf));
  EXIT(("stub function"));
  return OK;
}
*/


/*=====================================================================*\
  Xfield functions
\*=====================================================================*/

Error
_dxf_deleteXfield(xfieldP xf)
{
  ENTRY(("_dxf_deleteXfield(0x%x)", xf));

  if (xf->clipPts)
      DXFree((Pointer)xf->clipPts), xf->clipPts  = NULL;

  if (xf->clipVecs)
      DXFree((Pointer)xf->clipVecs), xf->clipVecs  = NULL;

  if (xf->texture_field)
    { DXDelete((dxObject)xf->texture_field); xf->texture_field = NULL; }

  if (xf->texture)
  {
      if (xf->myTextureData)
	  DXFree((Pointer)xf->texture);
      xf->texture = NULL;
      xf->myTextureData = 0;
  }

  if ((xf->glObject || xf->FlatGridTexture.Address) && xf->deletePrivate) 
  {
     (*xf->deletePrivate)((struct xfieldS *)xf);
     xf->glObject = 0;
  }


  if (xf->uv_array)
    { DXDelete((dxObject)xf->uv_array); xf->uv_array = NULL; }

  if (xf->uv)
    { DXFreeArrayHandle(xf->uv); xf->uv = NULL; }

  if (xf->connections) 
    { DXFreeArrayHandle(xf->connections); xf->connections = NULL; }

  if(xf->connections_array)
    { DXDelete((dxObject)xf->connections_array);xf->connections_array = NULL; }

  if(xf->origConnections_array)
    { DXDelete((dxObject)xf->origConnections_array);xf->origConnections_array = NULL; }

  if(xf->polylines_array)
    { DXDelete((dxObject)xf->polylines_array);xf->polylines_array = NULL; }

  if(xf->edges_array)
    { DXDelete((dxObject)xf->edges_array);xf->edges_array = NULL; }

  if (xf->invCntns) 
    { DXFreeInvalidComponentHandle(xf->invCntns); xf->invCntns = NULL; }

  if (xf->positions){ DXFreeArrayHandle(xf->positions); xf->positions = NULL; }

  if(xf->positions_array)
    { DXDelete((dxObject)xf->positions_array); xf->positions_array = NULL; }

  if (xf->invPositions)
    { DXFreeInvalidComponentHandle(xf->invPositions); xf->invPositions = NULL;}

  if (xf->normals) { DXFreeArrayHandle(xf->normals); xf->normals = NULL; }

  if(xf->normals_array)
    { DXDelete((dxObject)xf->normals_array); xf->normals_array = NULL; }

  if (xf->fcolors) { DXFreeArrayHandle(xf->fcolors); xf->fcolors = NULL; }

  if(xf->fcolors_array)
    { DXDelete((dxObject)xf->fcolors_array); xf->fcolors_array = NULL; }

  if (xf->bcolors) { DXFreeArrayHandle(xf->bcolors); xf->bcolors = NULL; }

  if(xf->bcolors_array)
    { DXDelete((dxObject)xf->bcolors_array); xf->bcolors_array = NULL; }

  if (xf->cmap) { DXFreeArrayHandle(xf->cmap); xf->cmap = NULL; }

  if(xf->cmap_array)
    { DXDelete((dxObject)xf->cmap_array); xf->cmap_array = NULL; }

  if (xf->opacities){ DXFreeArrayHandle(xf->opacities); xf->opacities = NULL; }

  if(xf->opacities_array)
    { DXDelete((dxObject)xf->opacities_array); xf->opacities_array = NULL; }

  if (xf->omap) { DXFreeArrayHandle(xf->omap); xf->omap = NULL; }

  if(xf->omap_array)
    { DXDelete((dxObject)xf->omap_array); xf->omap_array = NULL; }

  if (xf->neighbors){ DXFreeArrayHandle(xf->neighbors); xf->neighbors = NULL; }

  if(xf->neighbors_array)
    { DXDelete((dxObject)xf->neighbors_array); xf->neighbors_array = NULL; }

  if(xf->meshes)
    { DXDelete((dxObject)xf->meshes); xf->meshes = NULL; }

  if(xf->meshObject)
    { DXDelete((dxObject)xf->meshObject); xf->meshObject = NULL; }

  if (xf->field)
    { DXDelete((dxObject)xf->field); xf->field = NULL; }


  tdmFree(xf);

  EXIT((""));
  return OK;
}

   
#define ABS(x) ((x)<0)?-(x):(x) 

extern Error _XTexture(Field, xfieldT*, void *globals);

xfieldP
_dxf_newXfieldP(Field f, attributeP attributes, void *globals)
{
  DEFGLOBALDATA(globals);
  DEFPORT(PORT_HANDLE);
  xfieldT 	*xf;
  hwFlags	attFlags;
  hwFlags	servicesFlags;

  ENTRY(("_dxf_newXfield(0x%x, 0x%x, 0x%x)", f, attributes, globals));
  
  if (!(xf = (xfieldT*)tdmAllocateZero(sizeof(xfieldT)))) {
    EXIT(("alloc failed"));
    return NULL;
  }

  xf->field = (Field)DXReference((dxObject)f);

  /* SGI Specific FlatGrid Texturing */
  xf->FlatGridTexture.Address = NULL;
  xf->FlatGridTexture.Index = 0;

  if(attributes)
    *_dxf_xfieldAttributes(xf) = *attributes;

  attFlags = _dxf_attributeFlags(_dxf_xfieldAttributes(xf));
  servicesFlags = _dxf_SERVICES_FLAGS();

/*
XXX Currently no effort is made to make these arrays local
this means access to them is going to be into global memory,
this could be really slow for the cmap and omap (and perhaps others)
on an MP machine.
*/

  if (DXGetComponentValue (f, "faces") ||
      DXGetComponentValue (f, INVALID_POSITIONS)) {
    /* XXX How do I get rid of this allocated header ? */
    f = (Field)DXCopy((dxObject)f,COPY_HEADER);
    if (DXGetComponentValue (f, "faces"))
      if(!_dxfTriangulateField(f))
	goto error;
    if (DXGetComponentValue (f, INVALID_POSITIONS))
      /*
       *  This is an appropriate place to propagate invalid positions,
       *  if any, to invalid connections.  Don't do this in _dxfDraw()
       *  itself.  That routine is also called by the direct
       *  interactors, which update only the view transform; validity
      *  state should remain constant.
       */
      if (!DXInvalidateConnections((dxObject)f))
	/* unable to propagate invalid positions to connections */
	DXErrorGoto (ERROR_INTERNAL, "#13850") ;
  }

  if(_dxf_isFlagsSet(_dxf_attributeFlags(_dxf_xfieldAttributes(xf)),
		     IN_CLIP_OBJECT)) {

    if (!_XPositions(f, xf, XR_REQUIRED)) goto error;
/*
 XXX we should allow only surface type connections here
*/
    if (!_XConnections(f, xf,XR_REQUIRED)) goto error;
  } else {


    if (!_XPositions(f, xf, XR_REQUIRED)) goto error;
    if (!_XBox(f, xf, XR_REQUIRED)) goto error;
    if (!_XColors(f, xf, XR_REQUIRED)) goto error;
    if (!_XOpacities(f, xf, XR_OPTIONAL)) goto error;
    if (!_XConnections(f, xf, XR_OPTIONAL)) goto error;
    if (!_XPolylines(f, xf,XR_OPTIONAL)) goto error;

    if (!_XTexture(f, xf, globals)) goto error;

    /* sanity check for fcolors = bcolors for volumes */
    /* and for position-dependent */
    /* XXX - xf->volume? */
    if (_dxf_isFlagsSet(attFlags, CONTAINS_VOLUME)) {
      if (!_dxf_XNeighbors(f, xf, XR_REQUIRED, XD_GLOBAL)) {
	/* Warning!!!!!! no ERROR is set here!!!!! */
	EXIT(("ERROR"));
	return ERROR;
      }
      if (xf->fcolors_array != xf->bcolors_array) {
	EXIT(("ERROR"));
	DXErrorReturn(ERROR_DATA_INVALID,
		      "#13165");
      }
    }
  
    /* 
     * If we are not in a screen or clip object and we are supposed to shade
     * get the normals
     */
    if(!(_dxf_isFlagsSet(attFlags,(IN_SCREEN_OBJECT | IN_CLIP_OBJECT)))
       && _dxf_xfieldAttributes(xf)->shade)
      if (!_XNormals(f, xf, XR_OPTIONAL)) goto error;

    if(_dxf_isFlagsSet(servicesFlags,SF_TEXTURE_MAP))
    {
      int	n,counts[3];
      float	origin[3];
      float	deltas[9];
      
      if(xf->connectionType == ct_quads &&
	 xf->normalsDep == dep_none &&
	 xf->colorsDep == dep_positions &&
	 !xf->invCntns &&
	 !xf->invPositions &&
	 DXQueryGridPositions(xf->positions_array,&n,
			      counts, origin, deltas) &&
	 (n == 2)) 
	{

	  xf->image = (Field)f;
	  xf->connectionType = ct_flatGrid;
	} 
    }

    
    if(_dxf_isFlagsSet(servicesFlags,SF_POLYLINES) &&
       xf->connectionType == ct_lines && 
       xf->opacitiesDep == dep_none &&
       !(xf->colorsDep == dep_connections) && 
       !(xf->normalsDep == dep_connections)) {
      TIMER("> _dxf_linesToPlines");
      if(!_dxf_linesToPlines(xf)) goto error;
      TIMER("< _dxf_lineToPlines");
    }

#if 0
    if(_dxf_isFlagsSet(servicesFlags,SF_POLYLINES) &&
       xf->connectionType == ct_polylines && 
       !(xf->colorsDep == dep_polylines) && 
       !(xf->normalsDep == dep_polylines)) {
      TIMER("> _dxf_linesToPlines");
      if(!_dxf_polylinesToPlines(xf)) goto error;
      TIMER("< _dxf_lineToPlines");
    }
#endif

    /*  Don't mesh translucent primitives; they're depth sorted for rendering */
    if(
       (_dxf_isFlagsSet(servicesFlags, SF_DOES_TRANS) &&
        (xf->opacitiesDep == dep_none) && 
	!(xf->texture && xf->textureIsRGBA))
       ||
       (!_dxf_isFlagsSet(servicesFlags, SF_DOES_TRANS))
      )
    {
        if(_dxf_isFlagsSet(servicesFlags,SF_TMESH) &&
           xf->connectionType == ct_triangles && 
           !(xf->colorsDep == dep_connections) && 
           !(xf->normalsDep == dep_connections)) {
          if (!_XNeighbors(f, xf, XR_REQUIRED)) goto error;
          TIMER("> _dxf_trisToTmesh");
          if(!(_dxf_trisToTmesh(xf, globals))) goto error;
          TIMER("< _dxf_trisToTmesh");
        }
 
        if(_dxf_isFlagsSet(servicesFlags,SF_QMESH) &&
           xf->connectionType == ct_quads && 
           !(xf->colorsDep == dep_connections) && 
           !(xf->normalsDep == dep_connections)) {
          if (!_XNeighbors(f, xf, XR_REQUIRED)) goto error;
          TIMER("> _dxf_quadsToQmesh");
          if(!_dxf_quadsToQmesh(xf, globals)) goto error;
          TIMER("< _dxf_quadsToQmesh");
        }
    }

    if(_dxf_isFlagsSet(servicesFlags,SF_GAMMA_CORRECT_COLORS)) {
      float	gamma = 2.0;
      char*	gammaStr;

      
      if ( (gammaStr = (char*)getenv("DXHWGAMMA")) ) {
	  gamma = atof(gammaStr);
	  if (gamma < 0.0) gamma = 0.0;
	}

      TIMER("> gammaCorrectColors");
	if(xf->normalsDep != dep_none)
	  _gammaCorrectColors(xf, gamma, 0);
	else
	  _gammaCorrectColors(xf, gamma, 1);
      TIMER("< gammaCorrectColors");
    }

    xf->glObject = 0;
  }

  EXIT(("xf = 0x%x", xf));
  return xf;

 error:

  if(xf)
    tdmFree(xf);

  EXIT(("xf = NULL"));
  return NULL;
}

xfieldO
_dxf_newXfieldO(Field f, attributeP attributes, void *globals)
{
  xfieldO 	xfo = NULL;
  xfieldP 	xf = _dxf_newXfieldP(f, attributes, globals);

  if (xf)
    xfo = (xfieldO)_dxf_newHwObject(HW_CLASS_XFIELD, (Pointer)xf, _dxf_deleteXfield);

  EXIT(("xfo = 0x%x", xfo));
  return xfo;
}

xfieldP 
_dxf_getXfieldData(xfieldO obj)
{
    return (xfieldP)_dxf_getHwObjectData((dxObject)obj);
}

/*=====================================================================*\
  Attribute handling stuff
\*=====================================================================*/

/*
 * Parameters
 */
#define PARAMETER(parm, name, type, get) {				    \
    type p;								    \
    dxObject a;								    \
    a = DXGetAttribute(o, name);					    \
    if (a) {								    \
        if (!get(a, &p)) {						    \
	    DXSetError(ERROR_BAD_PARAMETER, "#10020", name " attribute");   \
	    EXIT(("ERROR"));						    \
	    return ERROR;						    \
	}								    \
        new->parm = p;							    \
    }									    \
}

#define PARAMETER1(parm, name, type, get, rhs, msg) {			    \
    type p;								    \
    dxObject a;								    \
    a = DXGetAttribute(o, name);					    \
    if (a) {								    \
	if (!get(a, &p)) {						    \
	    DXSetError(ERROR_BAD_PARAMETER, msg, name " attribute");	    \
	    EXIT(("ERROR"));						    \
	    return ERROR;						    \
	}								    \
        new->parm = rhs;						    \
    }									    \
}


#define PARAMETER2(parm, name, type, get, msg) {			    \
    type p;								    \
    dxObject a;								    \
    a = DXGetAttribute(o, name);					    \
    if (a) {								    \
	if (!get(DXGetAttribute(o, name), &p)) {			    \
	    DXSetError(ERROR_BAD_PARAMETER, msg, name " attribute");	    \
            EXIT(("ERROR"));						    \
	    return ERROR;						    \
	}								    \
        new->front.parm = new->back.parm = p;				    \
    }									    \
    a = DXGetAttribute(o, "front " name);				    \
    if (a && !get(a, &(new->front.parm))) {				    \
	    DXSetError(ERROR_BAD_PARAMETER,				    \
		msg,"front " name " attribute");			    \
            EXIT(("ERROR"));						    \
	    return ERROR;						    \
    }									    \
    a = DXGetAttribute(o, "back " name);				    \
    if (a && !get(a, &(new->back.parm))) {				    \
	    DXSetError(ERROR_BAD_PARAMETER,				    \
		msg, "back " name " attribute");			    \
            EXIT(("ERROR"));						    \
	    return ERROR;						    \
    }									    \
}


#define MULTIPLIER(parm, name, type, get) {				    \
    type p;								    \
    dxObject a;								    \
    a = DXGetAttribute(o, name);					    \
    if (a) {								    \
        if (!get(a, &p)) {						    \
	    DXSetError(ERROR_BAD_PARAMETER, "#10080", name " attribute");   \
            EXIT(("ERROR"));						    \
	    return ERROR;						    \
	}								    \
        new->parm *= p;							    \
    }									    \
}


attributeP
_dxf_parameters(dxObject o, attributeP old)
{
  static struct 
    { char *str; textureFilterE val; }
  filter_to_str[] = {
     { "nearest",               tf_nearest                },
     { "linear",                tf_linear                 },
     { "nearest_mipmap_nearest",tf_nearest_mipmap_nearest },
     { "nearest_mipmap_linear", tf_nearest_mipmap_linear  },
     { "linear_mipmap_nearest", tf_linear_mipmap_nearest  },
     { "linear_mipmap_linear",  tf_linear_mipmap_linear   },
     { 0,                       (textureFilterE) 0        } };

  dxObject 	options;
  int	 	density;
  char		*densityString;
  char		optionsString[201];
  attributeP	new;
  dxObject 	obj;
  Class class;

  ENTRY(("_dxf_parameters(0x%x, 0x%x)", o, old));

  new = _dxf_newAttribute(old);

  if (!(new->flags & IGNORE_PARAMS)) {
    PARAMETER2(ambient,   "ambient",   float, DXExtractFloat,   "#10080");
    PARAMETER2(specular,  "specular",  float, DXExtractFloat,   "#10080");
    PARAMETER2(diffuse,   "diffuse",   float, DXExtractFloat,   "#10080");
    PARAMETER2(shininess, "shininess", float, DXExtractFloat, "#10020");
    PARAMETER1(fuzz, "fuzz", float, DXExtractFloat, new->fuzz+p, "#10080");
    PARAMETER(shade, 	    "shade",        int,  DXExtractInteger);
    PARAMETER(flat_z,       "flat z",       int,  DXExtractInteger);
    PARAMETER(skip,         "skip",         int,  DXExtractInteger);
    PARAMETER(linewidth,    "line width",    float, DXExtractFloat);
    MULTIPLIER(color_multiplier  ,"color multiplier"  ,float,DXExtractFloat);
    MULTIPLIER(opacity_multiplier,"opacity multiplier",float,DXExtractFloat);
  }

  /*
   * get texture map if there is one
   */
   class = DXGetObjectClass(o);
  if (class == CLASS_FIELD && (obj = DXGetAttribute(o, "texture")) != NULL)
  {
      if (new->texture)
	  DXDelete(new->texture);
      new->texture = DXReference(obj);

      /* get texture wrap options */
      if ((options = DXGetAttribute(o, "texture wrap s"))) {
	char *wrap_s;
	if(!(DXExtractString(options, &wrap_s))) {
	  DXSetError(ERROR_BAD_PARAMETER,"#13384");
	  EXIT(("ERROR"));
	  return ERROR;
	}
	if     (!strcmp(wrap_s,"clamp" )) new->texture_wrap_s = tw_clamp;
	else if(!strcmp(wrap_s,"repeat")) new->texture_wrap_s = tw_repeat;
	else if(!strcmp(wrap_s,"clamp to edge")) 
    new->texture_wrap_s = tw_clamp_to_edge;
	else if(!strcmp(wrap_s,"clamp to border")) 
    new->texture_wrap_s = tw_clamp_to_border;
	else {
	  DXSetError(ERROR_BAD_PARAMETER,"#13384");
	  EXIT(("ERROR"));
	  return ERROR;
	}
      }
      if ((options = DXGetAttribute(o, "texture wrap t"))) {
	char *wrap_t;
	if(!(DXExtractString(options, &wrap_t))) {
	  DXSetError(ERROR_BAD_PARAMETER,"#13384");
	  EXIT(("ERROR"));
	  return ERROR;
	}
	if     (!strcmp(wrap_t,"clamp" )) new->texture_wrap_t = tw_clamp;
	else if(!strcmp(wrap_t,"repeat")) new->texture_wrap_t = tw_repeat;
	else if(!strcmp(wrap_t,"clamp to edge")) 
    new->texture_wrap_t = tw_clamp_to_edge;
	else if(!strcmp(wrap_t,"clamp to border")) 
    new->texture_wrap_t = tw_clamp_to_border;
	else {
	  DXSetError(ERROR_BAD_PARAMETER,"#13384");
	  EXIT(("ERROR"));
	  return ERROR;
	}
      }

      /* get texture filter options */
      if ((options = DXGetAttribute(o, "texture min filter"))) {
	char *str;
	int i;
	if(!(DXExtractString(options, &str))) {
	  DXSetError(ERROR_BAD_PARAMETER,"#13386");
	  EXIT(("ERROR"));
	  return ERROR;
	}
	for ( i = 0; filter_to_str[i].str != 0; i++ )
	  if (!strcmp(str,filter_to_str[i].str))
	    break;
        if ( filter_to_str[i].str == 0 ) {
	  DXSetError(ERROR_BAD_PARAMETER,"#13386");
	  EXIT(("ERROR"));
	  return ERROR;
	}
	new->texture_min_filter = filter_to_str[i].val;
      }
      if ((options = DXGetAttribute(o, "texture mag filter"))) {
	char *str;
	int i;
	if(!(DXExtractString(options, &str))) {
	  DXSetError(ERROR_BAD_PARAMETER,"#13386");
	  EXIT(("ERROR"));
	  return ERROR;
	}
	for ( i = 0; i < 2; i++ )
	  if (!strcmp(str,filter_to_str[i].str))
	    break;
        if ( i >= 2 ) {
	  DXSetError(ERROR_BAD_PARAMETER,"#13386");
	  EXIT(("ERROR"));
	  return ERROR;
	}
	new->texture_mag_filter = filter_to_str[i].val;
      }

      /* get texture function options */
      if ((options = DXGetAttribute(o, "texture function"))) {
	char *str;
	if(!(DXExtractString(options, &str))) {
	  DXSetError(ERROR_BAD_PARAMETER,"#13388");
	  EXIT(("ERROR"));
	  return ERROR;
	}
	if     (!strcmp(str,"decal"))    new->texture_function = tfn_decal;
	else if(!strcmp(str,"replace"))  new->texture_function = tfn_replace;
	else if(!strcmp(str,"modulate")) new->texture_function = tfn_modulate;
	else if(!strcmp(str,"blend"))    new->texture_function = tfn_blend;
        else {
	  DXSetError(ERROR_BAD_PARAMETER,"#13388");
	  EXIT(("ERROR"));
	  return ERROR;
	}
      }
  }

  /* get culling options */
  if ((options = DXGetAttribute(o, "cull face"))) {
    char *cull;
    if(!(DXExtractString(options, &cull))) {
      DXSetError(ERROR_BAD_PARAMETER,"#13389");
      EXIT(("ERROR"));
      return ERROR;
    }
    if     (!strcmp(cull,"off"           )) new->cull_face = cf_off;
    else if(!strcmp(cull,"front"         )) new->cull_face = cf_front;
    else if(!strcmp(cull,"back"          )) new->cull_face = cf_back;
    else if(!strcmp(cull,"front and back")) new->cull_face = cf_front_and_back;
    else {
      DXSetError(ERROR_BAD_PARAMETER,"#13389");
      EXIT(("ERROR"));
      return ERROR;
    }
  }

  /* get lighting model options */
  if ((options = DXGetAttribute(o, "light model"))) {
    char *lmodel;
    if(!(DXExtractString(options, &lmodel))) {
      DXSetError(ERROR_BAD_PARAMETER,"#13387");
      EXIT(("ERROR"));
      return ERROR;
    }
    if      (!strcmp(lmodel,"one side" )) new->light_model = lm_one_side;
    else if (!strcmp(lmodel,"two side" )) new->light_model = lm_two_side;
    else {
      DXSetError(ERROR_BAD_PARAMETER,"#13387");
      EXIT(("ERROR"));
      return ERROR;
    }
  }

  /* get anti-aliasing options */
  if ((options = DXGetAttribute(o, "antialias"))) {
    char *aaoptions;
    if(!(DXExtractString(options, &aaoptions))) {
      DXSetError(ERROR_BAD_PARAMETER,"#13382");
      EXIT(("ERROR"));
      return ERROR;
    }
    if(!strcmp(aaoptions,"lines")) new->aalines = 1;
  }

  /* get rendering approximation options */
  if ((options = DXGetAttribute(o, "rendering approximation"))) {
    char *down,*up;

    if(!(DXExtractString(options, &down))) {
      DXSetError(ERROR_BAD_PARAMETER,"#13380",down);
      EXIT(("ERROR"));
      return ERROR;
    }
    strncpy(optionsString,down,200);
    down = up = optionsString;
    while(*up && *up != ',') up++;
    if (*up == ',') *up++ = '\0';
    if(!strlen(up)) up = down;
    if(!strcmp(down,"none")) new->buttonDown.approx = approx_none;
    else if(!strcmp(down,"dots")) new->buttonDown.approx = approx_dots;
    else if(!strcmp(down,"box")) new->buttonDown.approx = approx_box;
    else if(!strcmp(down,"wireframe")) new->buttonDown.approx = approx_lines;
    else {
      DXSetError(ERROR_BAD_PARAMETER,"#13380",down);
      EXIT(("ERROR"));
      return ERROR;
    }

    if(!strcmp(up,"none")) new->buttonUp.approx = approx_none;
    else if(!strcmp(up,"dots")) new->buttonUp.approx = approx_dots;
    else if(!strcmp(up,"box")) new->buttonUp.approx = approx_box;
    else if(!strcmp(up,"wireframe")) new->buttonUp.approx = approx_lines;
    else {
      DXSetError(ERROR_BAD_PARAMETER,"#13380",up);
      EXIT(("ERROR"));
      return ERROR;
    }

  }

  /* get approximation density */
  if ((options = DXGetAttribute(o, "render every"))) {
    if (DXExtractInteger (options, &density)) {
      new->buttonDown.density = density;
      new->buttonUp.density = density;
    } else if(DXExtractString (options, &densityString)) {
      new->buttonDown.density = new->buttonUp.density = -1;
      sscanf(densityString,"%d,%d",
	     &new->buttonDown.density,&new->buttonUp.density);
      if(new->buttonUp.density == -1) 
	new->buttonUp.density = new->buttonDown.density;
    } else {
      EXIT(("ERROR"));
      DXErrorReturn(ERROR_BAD_PARAMETER,"#13360");
    }
  }
  
  EXIT(("new = 0x%x", new));
  return new;
}

/*=====================================================================*\
  Xfield attribute functions
\*=====================================================================*/

static attributeT defAttribute = {
  /* flags */			0,
  /* flat_z */  		0,
  /* skip  */   		1,
  /* color_multiplier */	1.0,
  /* opacity_multiplier */	1.0,
  /* shade */ 			1,
  /* buttonDown */ 		{approx_none, 1},
  /* buttonUp */                {approx_none, 1},
  /* fuzz */ 			0.0,
  /* ff */			1.0,
  /* linewidth */		1.0,
  /* aalines */			0,
  /* front */ 			{1.0, 0.7, 0.5, 10},
  /* back  */ 			{1.0, 0.7, 0.5, 10},
  /* mm */			{
				 {1.0, 0.0, 0.0, 0.0},
				 {0.0, 1.0, 0.0, 0.0},
				 {0.0, 0.0, 1.0, 0.0},
				 {0.0, 0.0, 0.0, 1.0}
				},
  /* vm */			{
				 {1.0, 0.0, 0.0, 0.0},
				 {0.0, 1.0, 0.0, 0.0},
				 {0.0, 0.0, 1.0, 0.0},
				 {0.0, 0.0, 0.0, 1.0}
				},
  /* texture */			NULL,

  /* texture_wrap_s */          tw_clamp,
  /* texture_wrap_t */          tw_clamp,
  /* texture_min_filter */      tf_nearest,
  /* texture_mag_filter */      tf_nearest,
  /* texture_function */        tfn_modulate,

  /* cull_face */               cf_off,
  /* light_model */             lm_one_side
};



float *
_dxf_attributeFuzz(attributeP att, double *ff)
{
  ENTRY(("_dxf_attributeFuzz(0x%x, 0x%x)", att, ff));

  if (ff)
    *ff = att->ff;

  EXIT((""));
  return &att->fuzz;
}

hwFlags
_dxf_attributeFlags(attributeP att)
{
  ENTRY(("_dxf_attributeFlags(0x%x)", att));

  EXIT((""));
  return &att->flags;
}

materialAttrP
_dxf_attributeFrontMaterial(attributeP att)
{
  ENTRY(("_dxf_attributeFrontMaterial(0x%x)", att));

  EXIT((""));
  return &att->front;
}

void
_dxf_attributeMatrix(attributeP att, float matrix[4][4])
{
  ENTRY(("_dxf_attributeMatrix(0x%x, 0x%x)", att, matrix));

  COPYMATRIX(matrix,att->mm);

  EXIT((""));
}

void
_dxf_setAttributeMatrix(attributeP att, float matrix[4][4])
{
  ENTRY(("_dxf_setAttributeMatrix(0x%x, 0x%x)", att, matrix));
  
  COPYMATRIX(att->mm,matrix);

  EXIT((""));
}

 Error
_dxf_deleteAttribute(attributeP att)
{
  ENTRY(("_dxf_deleteAttribute(0x%x)", att));

  if (att->texture) {
      DXDelete(att->texture);
      att->texture = NULL;
  }

  /* !!!!! should check for non-null */
  tdmFree(att);

  EXIT((""));
  return OK;
}

attributeP
_dxf_newAttribute(attributeP from)
{
  attributeP	ret;

  ENTRY(("_dxf_newAttribute(0x%x)", from));
  
  if(!(ret = (attributeP)tdmAllocate(sizeof(attributeT)))) {
    EXIT(("alloc errror"));
    DXErrorReturn(ERROR_NO_MEMORY,"");
  }

  if(from)
    *ret = *from;
  else {
    *ret = defAttribute;
    COPYMATRIX(ret->mm,identity);
  }

  if (ret->texture)
      DXReference(ret->texture);

  EXIT(("ret = 0x%x", ret));
  return ret;
}


int
_dxf_xfieldNconnections(xfieldP xf)
{
  ENTRY(("_dxf_xfieldNconnections(0x%x)", xf));

  EXIT((""));
  return xf->nconnections;
}

attributeP
_dxf_xfieldAttributes(xfieldP xf)
{
  ENTRY(("_dxf_xfieldAttributes(0x%x)", xf));

  EXIT((""));
  return &xf->attributes;
}

int
_dxf_xfieldSidesPerConnection(xfieldP xf)
{
  ENTRY(("_dxf_xfieldSidesPerConnection(0x%x)", xf));
  
  switch (xf->connectionType) {
  case ct_cubes:
    EXIT(("6"));
    return 6;
    break;
  case ct_tetrahedra:
    EXIT(("4"));
    return 4;
    break;
  default:
    EXIT(("1"));
    return 1;
    break;
  }
}



/*
 * XXX For gl 3.2 we can't do back colors so we always set front colors
 * equal to back colors in newXfield. This allows us to optimize and use
 * a single ambient-diffuse color for the surface by multiplying the
 * diffuse into the surface color and using kamb/kdiff*AmbientLightColor
 * for the ambient light. 
 *
 * This won't work if the front and back material properties are different.
 * We'll revisit this when we have back colors
 *
 * Each port needs to apply the surface properties in one of three places
 * A global surface property. (If the port allows, this is how xgl,starbase
 * are done).
 * Apply the inverse of the coefs to the lights before each field.
 * Apply the coefs to the colors once (in this function) before coloring 
 *  surface.
 */
static Error
_computeOnArray(double diffuse, double invertGamma,
		Array array, int items, 
		Array *retArray, ArrayHandle *retHandle)
{
  float	*data = NULL, *dataPtr = NULL, *retData = NULL;
  RGBColor	scratch;
  int	i;

  ENTRY(("_computeOnArray(%f, %f, 0x%x, %d, 0x%x, 0x%x)",
	 diffuse, invertGamma, array, items, retArray, retHandle));
  
  *retArray = NULL;
  *retHandle = NULL;

  if(DXGetArrayClass(array) == CLASS_CONSTANTARRAY) {
    if (!(*retArray = (Array)DXNewConstantArray(items, (Pointer)&scratch,
						TYPE_FLOAT, CATEGORY_REAL,
						1, 3)))
      goto error;


    if(!(data = (float *)DXGetConstantArrayData(array))) goto error;
    if(!(retData = (float *)DXGetConstantArrayData(*retArray))) goto error;

    for(i=0 ; i < 3 ; i++) {
      retData[i] = ((*data <= 0.0 || diffuse <= 0.0) ? 0.0 
	           : pow(*data * diffuse, invertGamma));
      data++;
    }
  } else {
    if (!(*retArray = DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 1, 3)))
      goto error;
    if (!DXAddArrayData(*retArray, 0, items, NULL))
      goto error;

    if(!(retData = (float *) DXGetArrayData(*retArray))) goto error;
    if(diffuse <= 0)
      bzero(retData,sizeof(float) * 3 * items);
    else {
      if(!(dataPtr = data = (float *) DXGetArrayData(array))) goto error;
 
/* XXX
 * since all attempts at correcting GAMMA by using lighting is flawed
 * (incorrect if multiple lights, low light colors, intense colors for 
 * different correction factors) we will only do the expensive pow function
 * for the constant color case, because this is both cheap and corrects
 * the more noticeable problems when a low intensity single color is picked.
 */
#ifndef APPLY_GAMMA_TO_COLORS
  if(!diffuse)
    diffuse = pow(1./10000.,invertGamma);
  else
    diffuse = pow(diffuse,invertGamma);

  invertGamma = 1.0;
#endif

      /* if the gamma is aprox 1.0, avoid the pow() function */
      if(invertGamma > .9 && invertGamma < 1.1) {
	for(i=0 ; i < 3 * items; i++) {
	  *retData++ = ((*data <= 0.) ? 0. 
	               : (*data * diffuse));
	  data++;
	}
      } else {
	for(i=0 ; i < 3 * items; i++) {
	  *retData++ = ((*data <= 0.) ? 0. 
	                : pow(*data * diffuse, invertGamma));
	  data++;
	}
      }
      if(DXGetArrayClass(array) != CLASS_ARRAY) {
	DXFree(dataPtr);
	dataPtr = NULL;
      }
    }
  }
  if(!(*retHandle = DXCreateArrayHandle(*retArray))) goto error;
  DXReference((dxObject)*retArray);

  EXIT(("OK"));
  return OK;

 error:
  if(DXGetArrayClass(array) != CLASS_ARRAY &&
     DXGetArrayClass(array) != CLASS_CONSTANTARRAY){
    if(dataPtr) DXFree(dataPtr);
    if(retData) DXFree(retData);
  }
  if(*retArray) DXDelete((dxObject)*retArray);
  if(*retHandle) DXFreeArrayHandle(*retHandle);
  *retArray = NULL;
  *retHandle = NULL;

  EXIT(("ERROR"));
  return ERROR;
}

static Error
_gammaCorrectColors(xfieldP xf, double gamma, int isLit)
{
  hwFlags 	flags = _dxf_attributeFlags(_dxf_xfieldAttributes(xf));
  materialAttrP material = _dxf_attributeFrontMaterial(
					_dxf_xfieldAttributes(xf));
  float		diffuse = material->diffuse;
  Array		array;
  ArrayHandle	arrayHandle;
  
  ENTRY(("_gammaCorrectColors(0x%x, %f, %d)", xf, gamma, isLit));
  
  TIMER("> _gammaCorrectColors ");

  if(_dxf_isFlagsSet(flags,CORRECTED_COLORS)) {
    EXIT(("OK: CORRECTED_COLORS flag set"));
    return OK;
  }

  if(_dxf_isFlagsSet(flags,SF_CONST_COEF_HELP))
     diffuse = 1.0;

  if(gamma > .9 && gamma < 1.1 && diffuse > .9 && diffuse < 1.1) {
    EXIT(("gamma > .9 etc."));
    return OK;
  }

  material->specular = ((material->specular <= 0.0) ? 0.0
                       : pow(material->specular,1./gamma));

  /* These have already been type checked in hwXfield.c:_XColors */
  if(xf->cmap_array) {
    if(! isLit)
      _computeOnArray(1.0,1./gamma,xf->cmap_array,xf->ncmap,
		      &array,&arrayHandle);
    else if(diffuse)
      _computeOnArray(diffuse,1./gamma,xf->cmap_array,xf->ncmap,
		      &array,&arrayHandle);
    else
      _computeOnArray(1./10000.,1./gamma,xf->cmap_array,xf->ncmap,
		      &array,&arrayHandle);
    DXDelete((dxObject)xf->cmap_array);
    DXFreeArrayHandle(xf->cmap);
    xf->cmap_array = array;
    xf->cmap = arrayHandle;
  } else {
    if(! isLit)
      _computeOnArray(1.0,1./gamma,xf->fcolors_array,xf->nfcolors,
		      &array,&arrayHandle);
    if(diffuse)
      _computeOnArray(diffuse,1./gamma,xf->fcolors_array,xf->nfcolors,
		      &array,&arrayHandle);
    else
      _computeOnArray(1./10000.,1./gamma,xf->fcolors_array,xf->nfcolors,
		      &array,&arrayHandle);
    /* 
     * assume front and back colors are equal, 
     * (see comment before this function)
     */
    DXDelete((dxObject)xf->fcolors_array);
    DXDelete((dxObject)xf->bcolors_array);
    DXFreeArrayHandle(xf->fcolors);
    DXFreeArrayHandle(xf->bcolors);
    xf->fcolors_array = array;
    xf->bcolors_array = array;
    xf->fcolors = arrayHandle;
    if(!(xf->bcolors = DXCreateArrayHandle(xf->bcolors_array))) goto error;
    DXReference((dxObject)xf->fcolors_array);
    DXReference((dxObject)xf->bcolors_array);
  }

  _dxf_setFlags(flags,CORRECTED_COLORS);

  TIMER("< _gammaCorrectColors ");

  EXIT(("OK"));
  return OK;
 error:

  TIMER("< _gammaCorrectColors ");

  EXIT(("ERROR"));
  return ERROR;
}

Error
_dxf_getHwXfieldClipPlanes(xfieldO xo, int *nClips, Point **pts, Vector **vecs)
{
    xfieldP xf = (xfieldP)_dxf_getHwObjectData((dxObject)xo);
    if (! xf)
        return ERROR;
    
    *nClips = xf->nClips;
    *pts = xf->clipPts;
    *vecs = xf->clipVecs;

    return OK;
}

Error
_dxf_setHwXfieldClipPlanes(xfieldO xo, int nClips, Point *pts, Vector *vecs)
{
    int i;
    xfieldP xf = (xfieldP)_dxf_getHwObjectData((dxObject)xo);
    if (! xf)
        return ERROR;
    
    if (! nClips)
    {
	xf->nClips   = 0;
	xf->clipPts  = NULL;
	xf->clipVecs = NULL;
    }

    xf->clipPts  = (Point *)DXAllocate(nClips * sizeof(Point));
    xf->clipVecs = (Vector *)DXAllocate(nClips * sizeof(Vector));
    if (! xf->clipPts || ! xf->clipVecs)
	return ERROR;
    
    xf->nClips = nClips;
    
    for (i = 0; i < nClips; i++)
    {
	xf->clipPts[i] = pts[i];
	xf->clipVecs[i] = vecs[i];
    }

    return OK;
}

