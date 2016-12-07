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

#include <dx/dx.h>
#include <ctype.h>
#include <stdlib.h>
#include <math.h>


/* 
 * output = Inquire(input, what, value ...);
 */

/* misc worker routines */

static char *article(char first)
{
    if (first == 'a' ||	first == 'e' ||	first == 'i' ||	
	first == 'o' ||	first == 'u')
	return "an";
    
    return "a";
}

#define CHECKIF(o, class, name) \
    if (DXGetObjectClass(o) != class) { \
	DXSetError(ERROR_DATA_INVALID, "#10371", \
		   "input", article(name[0]), name); \
	return ERROR; \
    }

#define CHECKIF2(o, class, name, class2, name2) \
    if (DXGetObjectClass(o) != class && DXGetObjectClass(o) != class2) { \
	DXSetError(ERROR_DATA_INVALID, "#10372", \
		   "input", article(name[0]), name, article(name2[0]), name2);\
	return NULL; \
    }

#define CHECKARRAYIF(o, class, name) \
    if (DXGetArrayClass((Array)o) != class) { \
	DXSetError(ERROR_DATA_INVALID, "#10371", \
		   "input", article(name[0]), name); \
	return NULL; \
    }

#define CHECKGROUPIF(o, class, name) \
    if (DXGetGroupClass((Group)o) != class) { \
	DXSetError(ERROR_DATA_INVALID, "#10371", \
		   "input", article(name[0]), name); \
	return NULL; \
    }


/* return all fields which aren't empty.
 */
static Field getpartplus(Object o, int *i, int *count)
{
    int again = 1;
    Field f;

    while (again) {
	f = DXGetPart(o, *i);
	if (!f)
	    return NULL;
	
	(*i)++;

	if (DXEmptyField(f))
	    continue;

	(*count)++;
	return f;
    }

    /* not reached */
    return NULL;
}

#if 0
/* should be libdx routine */
static Error DXGetComponentCount(Field f, int *i)
{
    *i = 0;

    while (DXGetEnumeratedComponentValue(f, *i, NULL))
	*i++;

    return OK;
}
#endif


static Object 
packit(int n, Type t, Category c, int rank, int *shape, Pointer p)
{
    Array a = NULL;

    a = DXNewArrayV(t, c, rank, shape);
    if (!a)
	return NULL;

    if (!DXAddArrayData(a, 0, n, p)) {
	DXDelete((Object)a);
	return NULL;
    }

    return (Object)a;
}

static Object packint(int i)
{
    return packit(1, TYPE_INT, CATEGORY_REAL, 0, NULL, (Pointer)&i);
}

static Object packfloat(float f)
{
    return packit(1, TYPE_FLOAT, CATEGORY_REAL, 0, NULL, (Pointer)&f);
}

#if 0
static Object packintlist(int n, int *i)
{
    return packit(n, TYPE_INT, CATEGORY_REAL, 0, NULL, (Pointer)i);
}
#endif

static Object packfloatlist(int n, float *f)
{
    return packit(n, TYPE_FLOAT, CATEGORY_REAL, 0, NULL, (Pointer)f);
}

static Object packintvect(int len, int *i)
{
    return packit(1, TYPE_INT, CATEGORY_REAL, 1, &len, (Pointer)i);
}

static Object packfloatvect(int len, float *f)
{
    return packit(1, TYPE_FLOAT, CATEGORY_REAL, 1, &len, (Pointer)f);
}

#if 0
static Object packintvectlist(int n, int len, int *i)
{
    return packit(n, TYPE_INT, CATEGORY_REAL, 1, &len, (Pointer)i);
}
#endif

static Object packfloatvectlist(int n, int len, float *f)
{
    return packit(n, TYPE_FLOAT, CATEGORY_REAL, 1, &len, (Pointer)f);
}

static Object packfloatmat(int x, int y, float *f)
{
    int shape[2];

    shape[0] = x;
    shape[1] = y;

    return packit(1, TYPE_FLOAT, CATEGORY_REAL, 2, shape, (Pointer)f);
}

static Object packstring(char *cp)
{
    return (Object)DXNewString(cp);
}






/* yes/no question routines */

static int qnull(Object o) 
{ 
    return (o ? 0 : 1); 
}

static int qregp(Object o) 
{
    int i, j;
    Field f;

    for (i=0, j=0; (f=getpartplus(o, &i, &j)); ) {

	if (!DXQueryGridPositions ((Array)
				   DXGetComponentValue(f, "positions"),
				   NULL, NULL, NULL, NULL))
	    return 0;
    }

    return (j > 0) ? 1 : 0;
}

static int qregc(Object o) 
{
    int i, j;
    Field f;

    for (i=0, j=0; (f=getpartplus(o, &i, &j)); ) {

	if (!DXQueryGridConnections ((Array)
				     DXGetComponentValue(f, "connections"),
				     NULL, NULL))
	    return 0;
    }
	
    return (j > 0) ? 1 : 0;
}

static int qreg(Object o, Object compname)
{ 
    int i, j;
    char *on;
    Field f;
    Array a;

    if (!compname)
	return (qregp(o) && qregc(o)); 
	
    if (!DXExtractString(compname, &on))
	return 0;
	
    for (i=0, j=0; (f=getpartplus(o, &i, &j)); ) {
	
	if (!(a = (Array)DXGetComponentValue(f, on)))
	    return 0;
	
	switch (DXGetArrayClass(a)) {
	    
	  case CLASS_PRODUCTARRAY:
	    if (DXQueryGridPositions(a, NULL, NULL, NULL, NULL) == NULL)
		return 0;
	    
	    break;
	    
	  case CLASS_MESHARRAY:
	    if (DXQueryGridConnections(a, NULL, NULL) == NULL)
		return 0;

	    break;

	  default:
	    return 0;
	}

    }

    return (j > 0) ? 1 : 0;
}

static int qNdgp(Object o, char *cn)
{
    int i, j;
    Field f;
    int n;
    int rank;

    n = atoi(cn);
    for (i=0, j=0; (f=getpartplus(o, &i, &j)); ) {
    
	if (!DXQueryGridPositions ((Array)
				   DXGetComponentValue(f, "positions"),
				   &rank, NULL, NULL, NULL))
	    return 0;
    
	if (rank != n)
	    return 0;
    }

    return (j > 0) ? 1 : 0;
}

static int qNdgc(Object o, char *cn)
{
    int i, j;
    Field f;
    int n;
    int rank;

    n = atoi(cn);
    for (i=0, j=0; (f=getpartplus(o, &i, &j)); ) {
    
	if (!DXQueryGridConnections ((Array)
				     DXGetComponentValue(f, "connections"),
				     &rank, NULL))
	    return 0;
    
	if (rank != n)
	    return 0;
    }

    return (j > 0) ? 1 : 0;
}

static int qNdp(Object o, char *cn)
{
    int i, j;
    Field f;
    int n;
    int rank, shape;
    Object pos;

    n = atoi(cn);
    for (i=0, j=0; (f=getpartplus(o, &i, &j)); ) {
    
	pos = DXGetComponentValue(f, "positions");
	if (!pos)
	    return 0;
	
	if (DXGetObjectClass(pos) != CLASS_ARRAY)
	    return 0;
	
	if (!DXGetArrayInfo((Array)pos, NULL, NULL, NULL, &rank, NULL))
	    return 0;
	
	if (rank != 1)
	    return 0;
	
	if (!DXGetArrayInfo((Array)pos, NULL, NULL, NULL, &rank, &shape))
	    return 0;
	
	if (n != shape)
	    return 0;
    }

    return (j > 0) ? 1 : 0;
}

static int qNdc(Object o, char *cn)
{
    int i, j;
    Field f;
    int n;
    char *cp;
    Object con;

    n = atoi(cn);
    for (i=0, j=0; (f=getpartplus(o, &i, &j)); ) {

	if (!(con = DXGetComponentValue(f, "connections")))
	    return 0;

	if (DXGetObjectClass(con) != CLASS_ARRAY)
	    return 0;

	if (!DXGetStringAttribute(con, "element type", &cp))
	    return 0;
	
	if (!strcmp(cp, "lines") && (n != 1))
	    return 0;
	
	if ((!strcmp(cp, "triangles") || !strcmp(cp, "quads")) && (n != 2))
	    return 0;
	
	if ((!strcmp(cp, "tetrahedra") || !strcmp(cp, "cubes")) && (n != 3))
	    return 0;
    }

    return (j > 0) ? 1 : 0;
}

static int qNdd(Object o, char *cn)
{
    int i, j, n;
    int rank;
    Field f;
    Object data;

    n = atoi(cn);

    if (DXGetObjectClass(o) == CLASS_ARRAY) {
	
	if (!DXGetArrayInfo((Array)o, NULL, NULL, NULL, &rank, NULL))
	    return 0;
	
	if (n != rank)
	    return 0;

	return 1;
    }

    for (i=0, j=0; (f=getpartplus(o, &i, &j)); ) {

	if (!(data = DXGetComponentValue(f, "data")))
	    return 0;
	
	if (DXGetObjectClass(data) != CLASS_ARRAY)
	    return 0;
	
	if (!DXGetArrayInfo((Array)data, NULL, NULL, NULL, &rank, NULL))
	    return 0;

	if (n != rank)
	    return 0;
    }

    return (j > 0) ? 1 : 0;
}

static int q1dNd(Object o, char *cn)
{
    int i, j, n;
    int rank, shape;
    Field f;
    Object data;

    n = atoi(cn);
    
    if (DXGetObjectClass(o) == CLASS_ARRAY) {
	if (!DXGetArrayInfo((Array)o, NULL, NULL, NULL, &rank, NULL))
	    return 0;
	
	if (rank != 1)
	    return 0;
	
	if (!DXGetArrayInfo((Array)o, NULL, NULL, NULL, &rank, &shape))
	    return 0;
	
	if (n != shape)
	    return 0;
	
	return 1;
    }

    for (i=0, j=0; (f=getpartplus(o, &i, &j)); ) {

	if (!(data = DXGetComponentValue(f, "data")))
	    return 0;
	
	if (DXGetObjectClass(data) != CLASS_ARRAY)
	    return 0;
	
	if (!DXGetArrayInfo((Array)data, NULL, NULL, NULL, &rank, NULL))
	    return 0;
	
	if (rank != 1)
	    return 0;
	
	if (!DXGetArrayInfo((Array)data, NULL, NULL, NULL, &rank, &shape))
	    return 0;
	
	if (n != shape)
	    return 0;
    }

    return (j > 0) ? 1 : 0;
}

static int qdatadep(Object o, Object on)
{
    int i, j;
    Field f;
    Object data;
    char *dep = NULL;
    char *attrvalue = NULL;

    for (i=0, j=0; (f=getpartplus(o, &i, &j)); ) {

	if (!(data = DXGetComponentValue(f, "data")))
	    return 0;
	
	if (!on)
	    return 0;
	
	/* extract string from 'on' here */
	if (!DXExtractString(on, &dep))
	    return 0;
	
	if (!DXGetStringAttribute(data, "dep", &attrvalue))
	    return 0;
	
	if (strcmp(attrvalue, dep))
	    return 0;

    }

    return (j > 0) ? 1 : 0;
}

static int qdatatype(Object o, Type t)
{
    int i, j;
    Field f;
    Type dt;

    if (DXGetObjectClass(o) == CLASS_ARRAY) {
	if (!DXGetType(o, &dt, NULL, NULL, NULL))
	    return 0;

	if (dt != t)
	    return 0;
	
	return 1;
    }

    for (i=0, j=0; (f=getpartplus(o, &i, &j)); ) {
    
	if (!DXGetType((Object)f, &dt, NULL, NULL, NULL))
	    return 0;

	if (dt != t)
	    return 0;
    }

    return (j > 0) ? 1 : 0;
}

static int qbyte(Object o)
{
    return qdatatype(o, TYPE_UBYTE);    
}

static int qshort(Object o)
{
    return qdatatype(o, TYPE_SHORT);    
}

static int qint(Object o)
{
    return qdatatype(o, TYPE_INT);    
}

static int qfloat(Object o)
{
    return qdatatype(o, TYPE_FLOAT);    
}

static int qdouble(Object o)
{
    return qdatatype(o, TYPE_DOUBLE);    
}

static int qstring(Object o)
{
    return qdatatype(o, TYPE_STRING);    
}


static int qobjtype(Object o, char *match)
{
    switch (DXGetObjectClass(o)) {    
      case CLASS_OBJECT:         return !strcmp(match, "object");
      case CLASS_PRIVATE:        return !strcmp(match, "private");
      case CLASS_STRING:	 return !strcmp(match, "string");
      case CLASS_FIELD:	       	 return !strcmp(match, "field");
      case CLASS_GROUP:	       	 return !strcmp(match, "group");
      case CLASS_ARRAY:	       	 return !strcmp(match, "array");
      case CLASS_XFORM:	       	 return !strcmp(match, "xform");
      case CLASS_SCREEN:	 return !strcmp(match, "screen");
      case CLASS_CLIPPED:	 return !strcmp(match, "clipped");
      case CLASS_CAMERA:	 return !strcmp(match, "camera");
      case CLASS_LIGHT:	       	 return !strcmp(match, "light");
      default: 			 return 0;
    }
    /* notreached */
}

static int qobjgtype(Object o, char *match)
{
    switch (DXGetObjectClass(o)) {    
      case CLASS_GROUP:
	switch (DXGetGroupClass((Group)o)) {
	  case CLASS_SERIES:         return !strcmp(match, "series");
	  case CLASS_COMPOSITEFIELD: return !strcmp(match, "compositefield");
	  case CLASS_MULTIGRID:	     return !strcmp(match, "multigrid");     
	  case CLASS_GROUP:	     return !strcmp(match, "group");
	  default:                   return 0;
	}
      default: 			     return 0;
    }
    /* notreached */
}

static int qobjatype(Object o, char *match)
{
    switch (DXGetObjectClass(o)) {    
      case CLASS_ARRAY:
	switch (DXGetArrayClass((Array)o)) {
	  case CLASS_ARRAY:	     return !strcmp(match, "array");
	  case CLASS_REGULARARRAY:   return !strcmp(match, "regulararray");
	  case CLASS_PATHARRAY:      return !strcmp(match, "patharray");
	  case CLASS_PRODUCTARRAY:   return !strcmp(match, "productarray");
	  case CLASS_MESHARRAY:      return !strcmp(match, "mesharray");
	  case CLASS_CONSTANTARRAY:  return !strcmp(match, "constantarray");
	  default: 		     return 0;
	}
      default: 			     return 0;
    }
    /* notreached */
}

/* not called -- don't know what the inquire string should be yet */
static int qhasdata(Object o)
{
    Object child = NULL;
    int i;

    switch (DXGetObjectClass(o)) {

      case CLASS_FIELD:
	return DXEmptyField((Field)o);
	
      case CLASS_GROUP:
	for (i=0; (child = DXGetEnumeratedMember((Group)o, i, NULL)); i++) {
	    if (!qhasdata(child))
		return 0;
	}
	return 1;

      case CLASS_SCREEN:
	if (!DXGetScreenInfo((Screen)o, &child, NULL, NULL))
	    return 1;

	return qhasdata(o);

      case CLASS_CLIPPED:
	if (!DXGetClippedInfo((Clipped)o, &child, NULL))
	    return 1;

	return qhasdata(o);

      case CLASS_XFORM:
	if (!DXGetXformInfo((Xform)o, &child, NULL))
	    return 1;

	return qhasdata(o);


      default:
	return 1;

    }
    /* notreached */
}

static int qobjempty(Object o, char *what)
{
    Object child = NULL;
    int i;

    switch (DXGetObjectClass(o)) {

      case CLASS_FIELD:
	if (what && what[0] && what[0] != 'f')
	    return 0;

	return DXEmptyField ((Field)o) ? 1 : 0;

      case CLASS_GROUP:
	if (what && what[0] && what[0] != 'g')
	    return 0;

	if (!DXGetMemberCount ((Group)o, &i))
	    return 0;

	if (i > 0)
	    return 0;

	return 1;

      case CLASS_ARRAY:
	if (what && what[0] && what[0] != 'a')
	    return 0;

	if (!DXGetArrayInfo ((Array)o, &i, NULL, NULL, NULL, NULL))
	    return 0;

	if (i > 0)
	    return 0;

	return 1;

      case CLASS_SCREEN:
	if (!DXGetScreenInfo ((Screen)o, &child, NULL, NULL))
	    return 1;

	return qobjempty(o, what);

      case CLASS_CLIPPED:
	if (!DXGetClippedInfo ((Clipped)o, &child, NULL))
	    return 1;

	return qobjempty(o, what);

      case CLASS_XFORM:
	if (!DXGetXformInfo ((Xform)o, &child, NULL))
	    return 1;

	return qobjempty(o, what);

      default:
	return 0;

    }
    /* notreached */
}

static int qhascomp(Object o, Object name)
{
    char *str = NULL;

    CHECKIF(o, CLASS_FIELD, "field");

    /* component name you are checking for */
    if (!DXExtractString(name, &str))
	return 0;

    if (!DXGetComponentValue((Field)o, str))
	return 0;

    return 1;
}

static int qhasattr(Object o, Object name)
{
    char *str = NULL;

    /* component name you are checking for */
    if (!DXExtractString(name, &str))
	return 0;

    if (!DXGetAttribute(o, str))
	return 0;

    return 1;
}

static int qhasmember(Object o, Object name)
{
    char *str = NULL;

    CHECKIF(o, CLASS_GROUP, "group");

    /* member name you are checking for */
    if (!DXExtractString(name, &str))
	return 0;

    if (!DXGetMember((Group)o, str))
	return 0;

    return 1;
}

static int qequal(Object o, Object name)
{
    char *input = NULL;
    char *target = NULL;

    /* target string you want to match */
    if (!DXExtractString(name, &target))
	return 0;

    /* input object string */
    if (!DXExtractString(o, &input))
	return 0;

    if (strcmp(input, target))
	return 0;

    return 1;
}

static int qsame(Object o, Object same)
{
    return (o == same) ? 1 : 0;
}

static int qimage(Object o)
{
    int n;
    Array a;
    char *s;
    
    /* an image has got to be a field or a composite field */
    if (DXGetObjectClass(o) != CLASS_FIELD) {
	if (DXGetObjectClass(o) != CLASS_GROUP)
	    return 0;
	if (DXGetGroupClass((Group)o) != CLASS_COMPOSITEFIELD)
	    return 0;
	o = DXGetEnumeratedMember((Group)o, 0, NULL);
    }

    /* check to see if it's the right shape, etc */

    /* check positions */
    a = (Array) DXGetComponentValue((Field)o, "positions");
    if (!a || !DXQueryGridPositions(a, &n, NULL, NULL, NULL) || n!=2)
	return 0;
    
    /* check connections */
    a = (Array) DXGetComponentValue((Field)o, "connections");
    if (!a || !DXQueryGridConnections(a, &n, NULL) || n!=2)
	return 0;

    /* is there a colors component */
    a = (Array) DXGetComponentValue((Field)o, "colors");
    if (!a)
	return 0;

    /* check dep */
    s = DXGetString((String)DXGetAttribute((Object)a, "dep"));
    if (s && strcmp(s, "positions"))
	return 0;
    
    return 1;
}

static int qiscontype(Object o, Object name)
{
    int i, j;
    Field f;
    char *etype, *match;
    Object con;

    /* target string you want to match */
    if (!DXExtractString(name, &match))
	return 0;

    for (i=0, j=0; (f=getpartplus(o, &i, &j)); ) {

	if (!(con = DXGetComponentValue(f, "connections")))
	    return 0;

	if (!DXGetStringAttribute(con, "element type", &etype))
	    return 0;

	return !strcmp(etype, match);
    }

    return 0;
}



/* information routines */

static Object qcount(Object o)
{
    int nitems;

    CHECKIF(o, CLASS_ARRAY, "array");

    if (!DXGetArrayInfo((Array)o, &nitems, NULL, NULL, NULL, NULL))
	return NULL;

    return packint(nitems);
}

static Object qgcount(Object o)
{
    int rank;
    int *counts = NULL;
    Array a = (Array)o;
    Object answer = NULL;

    CHECKIF(o, CLASS_ARRAY, "array");

    switch (DXGetArrayClass(a)) {
      case CLASS_PRODUCTARRAY:
	if (!DXQueryGridPositions(a, &rank, NULL, NULL, NULL))
	    goto notreg;
	
	counts = (int *)DXAllocate(rank * sizeof(int));
	if (!counts)
	    return NULL;
	
	DXQueryGridPositions(a, &rank, counts, NULL, NULL);
	
	answer = packintvect(rank, counts);
	if (!answer)
	    goto notreg;
	
	DXFree((Pointer)counts);
	return answer;
	
      case CLASS_MESHARRAY:
	if (!DXQueryGridConnections(a, &rank, NULL))
	    goto notreg;
	
	counts = (int *)DXAllocate(rank * sizeof(int));
	if (!counts)
	    return NULL;
	
	DXQueryGridConnections(a, &rank, counts);
	
	answer = packintvect(rank, counts);
	if (!answer)
	    goto notreg;
	
	DXFree((Pointer)counts);
	return answer;
      default:
        break;
    }

  notreg:	
    DXSetError(ERROR_DATA_INVALID, "not regular grid");
    return NULL;
}

static Object qtype(Object o)
{
    Type t;
    char *cp = "unknown";

    CHECKIF(o, CLASS_ARRAY, "array");

    if (!DXGetArrayInfo((Array)o, NULL, &t, NULL, NULL, NULL))
	return NULL;

    switch (t) {
      case TYPE_BYTE:   cp = "signed byte";      break;
      case TYPE_UBYTE:  cp = "unsigned byte";    break;
      case TYPE_SHORT:  cp = "short";            break;
      case TYPE_USHORT: cp = "unsigned short";   break;
      case TYPE_INT:    cp = "integer";          break;
      case TYPE_UINT:   cp = "unsigned integer"; break;
      case TYPE_FLOAT:  cp = "float";            break;
      case TYPE_DOUBLE: cp = "double";           break;
      case TYPE_STRING: cp = "string";           break;
      case TYPE_HYPER:  cp = "hyper";		 break;
    }

    return packstring(cp);
}

static Object qcat(Object o)
{
    Category cat;
    char *cp = "unknown";

    CHECKIF(o, CLASS_ARRAY, "array");

    if (!DXGetArrayInfo((Array)o, NULL, NULL, &cat, NULL, NULL))
	return NULL;

    switch (cat) {
      case CATEGORY_REAL:       cp = "real";       break;
      case CATEGORY_COMPLEX:    cp = "complex";    break;
      case CATEGORY_QUATERNION: cp = "quaternion"; break;
    }

    return packstring(cp);
}

static Object qrank(Object o)
{
    int rank;

    CHECKIF(o, CLASS_ARRAY, "array");

    if (!DXGetArrayInfo((Array)o, NULL, NULL, NULL, &rank, NULL))
	return NULL;

    return packint(rank);
}

static Object qshape(Object o)
{
    int rank;
    int *shape = NULL;
    Object answer = NULL;

    CHECKIF(o, CLASS_ARRAY, "array");

    if (!DXGetArrayInfo((Array)o, NULL, NULL, NULL, &rank, NULL))
	return NULL;

    shape = (int *)DXAllocate(rank * sizeof(int));
    if (!shape)
	return NULL;

    if (!DXGetArrayInfo((Array)o, NULL, NULL, NULL, &rank, shape))
	return NULL;

    answer = packintvect(rank, shape);
    if (!answer)
	goto error;
    
    DXFree((Pointer)shape);
    return answer;

  error:
    DXDelete(answer);
    DXFree((Pointer)shape);
    return NULL;
}

static Object qitems(Object o, char *component)
{
    Object comp;

    CHECKIF(o, CLASS_FIELD, "field");

    if (!(comp = DXGetComponentValue((Field)o, component))) {
	DXSetError(ERROR_DATA_INVALID, "#10258", "input", component);
	return NULL;
    }

    return qcount(comp);
}

static Object qpos(Object o)
{
    return qitems(o, "positions");
}
	
static Object qconnect(Object o)
{
    return qitems(o, "connections");
}

static Object qdata(Object o)
{
    return qitems(o, "data");
}

static Object qcompsize(Object o, Object compname)
{
    char *on;

    if (!compname) {
	if (DXGetObjectClass(o) == CLASS_FIELD)
	    return qitems(o, "data");
	else
	    return qcount(o);
    }

    if (!DXExtractString(compname, &on)) {
	DXSetError(ERROR_BAD_PARAMETER, "#10200", "component name");
	return NULL;
    }

    return qitems(o, on);
}

static Object qgriditems(Object o, char *component)
{
    Object comp;

    CHECKIF(o, CLASS_FIELD, "field");

    if (!(comp = DXGetComponentValue((Field)o, component))) {
	DXSetError(ERROR_DATA_INVALID, "#10258", "input", component);
	return NULL;
    }

    return qgcount(comp);
}

static Object qcompgridsize(Object o, Object compname)
{
    char *on;

    if (!compname)
	return qgcount(o);

    if (!DXExtractString(compname, &on)) {
	DXSetError(ERROR_BAD_PARAMETER, "#10200", "component name");
	return NULL;
    }

    return qgriditems(o, on);
}

static Object qcgridsize(Object o)
{
    int rank;
    int *counts = NULL;
    Array a;
    Object answer = NULL;

    CHECKIF(o, CLASS_FIELD, "field");

    if (!(a = (Array)DXGetComponentValue((Field)o, "connections"))) {
	DXSetError(ERROR_DATA_INVALID, "#10258", "input", "connections");
	return NULL;
    }

    if (!DXQueryGridConnections(a, &rank, NULL)) {
	DXSetError(ERROR_DATA_INVALID, 
		   "input does not have regular connections");
	return NULL;
    }

    counts = (int *)DXAllocate(rank * sizeof(int));
    if (!counts)
	return NULL;

    DXQueryGridConnections(a, &rank, counts);
    
    answer = packintvect(rank, counts);
    if (!answer)
	goto error;

    DXFree((Pointer)counts);
    return answer;

  error:
    DXDelete(answer);
    DXFree((Pointer)counts);
    return NULL;
}
	
static Object qpgridsize(Object o)
{
    int rank;
    int *counts = NULL;
    Array a;
    Object answer = NULL;

    CHECKIF(o, CLASS_FIELD, "field");

    if (!(a = (Array)DXGetComponentValue((Field)o, "positions"))) {
	DXSetError(ERROR_DATA_INVALID, "#10258", "input", "positions");
	return NULL;
    }

    if (!DXQueryGridPositions(a, &rank, NULL, NULL, NULL)) {
	DXSetError(ERROR_DATA_INVALID, 
		   "input does not have regular positions");
	return NULL;
    }

    counts = (int *)DXAllocate(rank * sizeof(int));
    if (!counts)
	return NULL;

    DXQueryGridPositions(a, &rank, counts, NULL, NULL);

    answer = packintvect(rank, counts);
    if (!answer)
	goto error;

    DXFree((Pointer)counts);
    return answer;

  error:
    DXDelete(answer);
    DXFree((Pointer)counts);
    return NULL;
}


static Object qorigin(Object o)
{
    int rank;
    float *origin = NULL;
    Pointer anyorigin = NULL;
    Array a;
    Type type;
    Object answer = NULL;

    CHECKIF2(o, CLASS_FIELD, "field", CLASS_ARRAY, "regular array");
    
    switch (DXGetObjectClass(o)) {
      case CLASS_FIELD:
	if (!(a = (Array)DXGetComponentValue((Field)o, "positions"))) {
	    DXSetError(ERROR_DATA_INVALID, "#10258", "input", "positions");
	    return NULL;
	}
	
	if (!DXQueryGridPositions(a, &rank, NULL, NULL, NULL)) {
	    DXSetError(ERROR_DATA_INVALID, 
		       "input does not have regular positions");
	    return NULL;
	}
	
	origin = (float *)DXAllocate(rank * sizeof(float));
	if (!origin)
	    return NULL;
	
	DXQueryGridPositions(a, &rank, NULL, origin, NULL);
	
	answer = packfloatvect(rank, origin);
	if (!answer)
	    goto error;
	
	DXFree((Pointer)origin);
	return answer;

      case CLASS_ARRAY:
	if (DXGetArrayClass((Array)o) != CLASS_REGULARARRAY) {
	    DXSetError(ERROR_DATA_INVALID, "#10372",
		       "input", "a", "field", "a", "regular array");
	    return NULL;
	}

	a = (Array)o;

	if (!DXGetArrayInfo(a, NULL, &type, NULL, NULL, &rank))  /* really shape */
	    return NULL;
	
	anyorigin = DXAllocate(rank * DXTypeSize(type));
	if (!anyorigin)
	    return NULL;
	
	DXGetRegularArrayInfo((RegularArray)a, NULL, anyorigin, NULL);
	
	answer = (Object)DXNewArrayV(type, CATEGORY_REAL, 1, &rank);
	if (!answer)
	    goto error;

	if (!DXAddArrayData((Array)answer, 0, 1, anyorigin))
	    goto error;

	DXFree((Pointer)anyorigin);
	return answer;

      default:
	DXSetError(ERROR_DATA_INVALID, "input must be a field or regular array");
	return NULL;
    }

  error:
    DXDelete(answer);
    DXFree((Pointer)origin);
    DXFree(anyorigin);
    return NULL;
}
	

static Object qdeltas(Object o)
{
    int rank;
    float *deltas = NULL;
    Pointer anydeltas = NULL;
    Array a;
    Type type;
    Object answer = NULL;

    CHECKIF2(o, CLASS_FIELD, "field", CLASS_ARRAY, "regular array");
    
    switch (DXGetObjectClass(o)) {
      case CLASS_FIELD:
	if (!(a = (Array)DXGetComponentValue((Field)o, "positions"))) {
	    DXSetError(ERROR_DATA_INVALID, "#10258", "input", "positions");
	    return NULL;
	}
	
	if (!DXQueryGridPositions(a, &rank, NULL, NULL, NULL)) {
	    DXSetError(ERROR_DATA_INVALID, 
		       "input does not have regular positions");
	    return NULL;
	}
	
	deltas = (float *)DXAllocate(rank * rank * sizeof(float));
	if (!deltas)
	    return NULL;
	
	DXQueryGridPositions(a, &rank, NULL, NULL, deltas);
	
	answer = packfloatvectlist(rank, rank, deltas);
	if (!answer)
	    goto error;
	
	DXFree((Pointer)deltas);
	return answer;
	

      case CLASS_ARRAY:
	if (DXGetArrayClass((Array)o) != CLASS_REGULARARRAY) {
	    DXSetError(ERROR_DATA_INVALID, "#10372",
		       "input", "a", "field", "a", "regular array");
	    return NULL;
	}

	a = (Array)o;

	if (!DXGetArrayInfo(a, NULL, &type, NULL, NULL, &rank))  /* really shape */
	    return NULL;
	
	anydeltas = DXAllocate(rank * DXTypeSize(type));
	if (!anydeltas)
	    return NULL;
	
	DXGetRegularArrayInfo((RegularArray)a, NULL, NULL, anydeltas);
	
	answer = (Object)DXNewArrayV(type, CATEGORY_REAL, 1, &rank);
	if (!answer)
	    goto error;

	if (!DXAddArrayData((Array)answer, 0, 1, anydeltas))
	    goto error;

	DXFree((Pointer)anydeltas);
	return answer;

      default:
	DXSetError(ERROR_DATA_INVALID, "input must be a field or regular array");
	return NULL;
    }

  error:
    DXDelete(answer);
    DXFree((Pointer)deltas);
    DXFree(anydeltas);
    return NULL;
}

static Object qprodterms(Object o)
{
    int i, count;
    Group g = NULL;
    Array *terms = NULL;

    CHECKARRAYIF(o, CLASS_PRODUCTARRAY, "product array");
    
    if (!DXGetProductArrayInfo((ProductArray)o, &count, NULL))
	return NULL;
    
    terms = (Array *)DXAllocate(count * sizeof(Array));
    if (!terms)
	return NULL;
    
    if (!DXGetProductArrayInfo((ProductArray)o, &count, terms))
	goto error;
    
    g = DXNewGroup();
    if (!g)
	goto error;

    for (i=0; i < count; i++)
	if (!DXSetMember(g, NULL, (Object)terms[i]))
	    goto error;

    DXFree((Pointer)terms);
    return (Object)g;

  error:
    DXFree((Pointer)terms);
    DXDelete((Object)g);
    return NULL;
}

static Object qmeshterms(Object o)
{
    int i, count;
    Group g = NULL;
    Array *terms = NULL;

    CHECKARRAYIF(o, CLASS_MESHARRAY, "mesh array");
    
    if (!DXGetMeshArrayInfo((MeshArray)o, &count, NULL))
	return NULL;
    
    terms = (Array *)DXAllocate(count * sizeof(Array));
    if (!terms)
	return NULL;
    
    if (!DXGetMeshArrayInfo((MeshArray)o, &count, terms))
	goto error;
    
    g = DXNewGroup();
    if (!g)
	goto error;

    for (i=0; i < count; i++)
	if (!DXSetMember(g, NULL, (Object)terms[i]))
	    goto error;

    DXFree((Pointer)terms);
    return (Object)g;

  error:
    DXFree((Pointer)terms);
    DXDelete((Object)g);
    return NULL;
}
	
static Object qcompcount(Object o)
{
    int i;

    CHECKIF(o, CLASS_FIELD, "field");

    for (i=0; DXGetEnumeratedComponentValue((Field)o, i, NULL); i++)
	;

    return packint(i);
}

static Object qmembers(Object o)
{
    int i;

    CHECKIF(o, CLASS_GROUP, "group");

    if (!DXGetMemberCount((Group)o, &i))
	return NULL;

    return packint(i);
}

static Object qmembernames(Object o, char *type)
{
    int i = 0; 
    char **cp = NULL;
    Array a;

    if (type[0] == 'c') {
	CHECKIF(o, CLASS_FIELD, "field");
    } 
    else if (type[0] == 'm') {
	CHECKIF(o, CLASS_GROUP, "group");
    }

    switch (DXGetObjectClass(o)) {
      case CLASS_GROUP:
	if (!DXGetMemberCount((Group)o, &i))
	    return NULL;
	cp = (char **)DXAllocate(sizeof(char *) * (i+1));
	if (!cp)
	    return NULL;

	for (i=0; DXGetEnumeratedMember((Group)o, i, cp+i); i++)
	    ;

	break;

      case CLASS_FIELD:
	for (i=0; DXGetEnumeratedComponentValue((Field)o, i, NULL); i++)
	    ;

	cp = (char **)DXAllocate(sizeof(char *) * (i+1));
	if (!cp)
	    return NULL;

	for (i=0; DXGetEnumeratedComponentValue((Field)o, i, cp+i); i++)
	    ;

	break;

      default:
	DXSetError(ERROR_DATA_INVALID, "input is not a field or group");
	return NULL;
    }

    if (i == 0)
	a = DXMakeStringList(1, " ");
    else
	a = DXMakeStringListV(i, cp);
    
    DXFree((Pointer)cp);
    return (Object)a;
}

static Object qmemberpos(Object o)
{
    int i = 0; 
    float *pos = NULL;
    Object answer = NULL;

    CHECKGROUPIF(o, CLASS_SERIES, "series group");

    if (!DXGetMemberCount((Group)o, &i) || i <= 0)
	return NULL;

    pos = (float *)DXAllocate(sizeof(float) * (i+1));
    if (!pos)
	return NULL;

    for (i=0; DXGetSeriesMember((Series)o, i, pos+i); i++)
	;
    
    answer = packfloatlist(i, pos);
	
    DXFree((Pointer)pos);
    return answer;
}


static Object qvalid(Object o, char *type)
{
    int icount = 0;
    Array data = NULL;
    char *dep = NULL;
    char *invalid = NULL;
    InvalidComponentHandle ih;

    CHECKIF(o, CLASS_FIELD, "field");

    data = (Array)DXGetComponentValue((Field)o, "data");
    if (!data)
	return packint(icount);

    /* follow data dependency. */
    if (!DXGetStringAttribute((Object)data, "dep", &dep))
	return packint(icount);
    
#define INVLEN 10     /* strlen("invalid "); */    
    
    if (!(invalid = (char *)DXAllocate(strlen(dep) + INVLEN)))
	return ERROR;

    strcpy(invalid, "invalid ");
    strcat(invalid, dep);

    /* if no invalid component, all valid */
    if (!DXGetComponentValue((Field)o, invalid)) {
	DXFree((Pointer)invalid);
	if (type[0] == 'i')
	    return packint(icount);
	else
	    return qcount((Object)data);
    }
    DXFree((Pointer)invalid);

    ih = DXCreateInvalidComponentHandle(o, NULL, dep);
    if (!ih) 
	return packint(icount);

    if (type[0] == 'i')
	icount = DXGetInvalidCount(ih);
    else
	icount = DXGetValidCount(ih);

    DXFreeInvalidComponentHandle(ih);
    return packint(icount);
}

static Object qcontype(Object o)
{
    int i, j;
    Field f;
    Object con;

    for (i=0, j=0; (f=getpartplus(o, &i, &j)); ) {

	if (!(con = DXGetComponentValue(f, "connections")))
	    return NULL;

	return DXGetAttribute(con, "element type");
    }

    return NULL;
}



static Object qnumprocess(Object o)
{
    return packint(DXProcessors(0));
}

/* return a string which contains the number of each type of primitive
 * (connection) in the input object.
 */
static Object qprimitives(Object o)
{
    Object rc = NULL;
    int i, j;
    Field f;
    int items;
    char *elem_type;
    char **ccp = NULL;
    Array a;
    struct pc {
	char con_name[64];
	int con_count;
	char con_total[64];
	struct pc *next;
    } *pcp, *pcp2;

    pcp = NULL;

    /* for each field in the input...
     */
    for (i=0, j=0; (f=getpartplus(o, &i, &j)); ) {

	/* find out how many connection items there are.
	 *  if there is no connections array, then these
	 *  must be unconnected points.
	 */
	a = (Array)DXGetComponentValue(f, "connections");
	if (!a) {
	    elem_type = "points";
	    a = (Array)DXGetComponentValue(f, "positions");
	    if (!a)
		continue;
	} else if (!DXGetStringAttribute((Object)a, "element type", &elem_type))
	    elem_type = "unknown";
	
	if (!DXGetArrayInfo(a, &items, NULL, NULL, NULL, NULL))
	    continue;

	/* now try to find this connection type in the list.  if there
	 * isn't a list yet, or this type doesn't exist, add a new block
	 * to the linked list.
	 */
	if (!pcp) {
	    pcp = (struct pc *)DXAllocate(sizeof(struct pc));
	    if (!pcp)
		return NULL;
	    memset(pcp, '\0', sizeof(struct pc));
	    pcp2 = pcp;
	    strcpy(pcp2->con_name, elem_type);
	} else {
	    for (pcp2 = pcp; pcp2->next; pcp2 = pcp2->next)
		if (!strcmp(pcp2->con_name, elem_type))
		    break;
	    
	    /* if we went thru the whole list & didn't find this type
	     * yet, add a new block.  otherwise we are at the right block.
	     */
	    if (strcmp(pcp2->con_name, elem_type)) {
		pcp2->next = (struct pc *)DXAllocate(sizeof(struct pc));
		if (!pcp2->next)
		    return NULL;
		pcp2 = pcp2->next;
		memset(pcp2, '\0', sizeof(struct pc));
		strcpy(pcp2->con_name, elem_type);
	    }
	}
	    
	/* finally, increment the item count
	 */
	pcp2->con_count += items;
	
    }
    
    /* count the number of different primitive types we found
     * in the input object.
     */
    for (pcp2 = pcp, i = 0; pcp2; pcp2 = pcp2->next)
	i++;
    i++;  /* add one extra for a trailing null */
    
    ccp = (char **)DXAllocate(sizeof(char *) * i);
    if (!ccp)
	return NULL;
    
    for (pcp2 = pcp, i = 0; pcp2; pcp2 = pcp2->next, i++) {
	sprintf(pcp2->con_total, "%d %s", pcp2->con_count, pcp2->con_name);
	ccp[i] = pcp2->con_total;
    }
    ccp[i] = NULL;
    
    rc = (Object)DXMakeStringListV(i, ccp);

    for ( ; pcp; pcp = pcp2) {
	pcp2 = pcp->next;
	DXFree((Pointer)pcp);
    }
    DXFree((Pointer)ccp);
    
    return rc;

}


#define DEG2RAD(x)      (x/180.0*M_PI)   /* degrees to radians */
#define RAD2DEG(x)      (x*180.0/M_PI)   /* visa versa */

static Object qcamera(Object o, char *info)
{
    Object answer = NULL;
    Matrix m;
    int i[3];
    float f[3];

    CHECKIF(o, CLASS_CAMERA, "camera");

    switch (info[0]) {
      case 'i':   /* is perspective or is orthographic (yes/no questions) */
	switch (info[2]) {
	  case 'p':  /* perspective */
	    i[0] = DXGetPerspective((Camera)o, NULL, NULL) == (Camera)o;
	    answer = packint(i[0]);
	    break;
	    
	  case 'o':  /* ortho */
	    i[0] = DXGetOrthographic((Camera)o, NULL, NULL) == (Camera)o;
	    answer = packint(i[0]);
	    break;
	}
	break;

      /* the rest of the cases return int or float values */
      case 't':  /* to or transform */
	switch (info[1]) {
	  case 'o':
	    if (!DXGetView((Camera)o, NULL, (Vector *)f, NULL))
		return NULL;
	    
	    answer = packfloatvect(3, f);
	    break;
	    
	  case 'r':
	    m = DXGetCameraMatrix((Camera)o);

	    answer = packfloatmat(4, 3, (float *)&m);
	    break;
	}
	break;

      case 'f':  /* from or fieldofview */
	switch (info[1]) {
	  case 'r':
	    if (!DXGetView((Camera)o, (Vector *)f, NULL, NULL))
		return NULL;
	    
	    answer = packfloatvect(3, f);
	    break;

	  case 'i':
	    if (!DXGetPerspective((Camera)o, f, NULL)) {
		DXSetError(ERROR_DATA_INVALID, 
		       "field of view only valid for perspective camera");
		return NULL;
	    }
	    
	    answer = packfloat(f[0]);
	    break;
	}
	break;

      case 'w':  /* width */
	if (!DXGetOrthographic((Camera)o, f, NULL)) {
	    DXSetError(ERROR_DATA_INVALID, 
		       "width only valid for orthographic camera");
	    return NULL;
	}
	
	answer = packfloat(f[0]);
	break;

      case 'b':  /* background color */
	if (!DXGetBackgroundColor((Camera)o, (RGBColor *)f))
	    return NULL;

	answer = packfloatvect(3, f);
	break;

      case 'r':  /* resolution */
	if (!DXGetCameraResolution((Camera)o, &i[0], &i[1]))
	    return NULL;
	
	answer = packint(i[0]);
	break;

      case 'a':  /* angle or aspect */
	switch (info[1]) {
	  case 'n':
	    if (!DXGetPerspective((Camera)o, f, NULL)) {
		DXSetError(ERROR_DATA_INVALID, 
			   "angle only valid for perspective camera");
		return NULL;
	    }
	    
	    f[0] = 2 * RAD2DEG(atan(f[0]/2.0));
	    answer = packfloat(f[0]);
	    break;

	  case 's':
	    if (!DXGetCameraResolution((Camera)o, &i[0], &i[1]))
		return NULL;
	
	    f[0] = i[0] ? (float)i[1] / i[0] : 0.0;
	    answer = packfloat(f[0]);
	    break;
	}
	break;

      case 'u':  /* up */
	if (!DXGetView((Camera)o, NULL, NULL, (Vector *)f))
	    return NULL;

	answer = packfloatvect(3, f);
	break;

      case 'p':  /* perspective */
	i[0] = DXGetPerspective((Camera)o, NULL, NULL) == (Camera)o;
	answer = packint(i[0]);
	break;

    }
    
    return answer;
}


static Object qclipped(Object o, char *which)
{
    Object c, subo = NULL;

    CHECKIF(o, CLASS_CLIPPED, "clipped object");

    if (!DXGetClippedInfo((Clipped)o, &subo, &c))
	return NULL;

    if (which[0] == 'c')
	return c;
    
    return subo;
}

static Object qxform(Object o, char *which)
{
    Object subo;
    Matrix m;

    CHECKIF(o, CLASS_XFORM, "transformed object");

    if (!DXGetXformInfo((Xform)o, &subo, &m))
	return NULL;

    if (which[0] == 'o')
	return subo;
    
    return packfloatmat(4, 3, (float *)&m);
}

static Object doxform(Object o)
{
    Object subo;
    Matrix m;

    CHECKIF(o, CLASS_XFORM, "transformed object");

    if (!DXGetXformInfo((Xform)o, &subo, &m))
	return NULL;

    return (Object)DXApplyTransform(subo, &m);
}

static Object qscreen(Object o, char *which)
{
    Object subo;
    int position, z;

    CHECKIF(o, CLASS_SCREEN, "screen object");

    if (!DXGetScreenInfo((Screen)o, &subo, &position, &z))
	return NULL;

    if (which[0] == 'o')
	return subo;

    if (which[0] == 'p')
	return packint(position);

    return packint(z);
}

Error MakeMany(Object attr, int memcount, int *isstring,
	       Object *answer, char ***strlist)
{
    int i;
    Type t;
    Category c;
    int rank, shape[100];

    if (!attr)
	return ERROR;
    
    if (DXGetObjectClass(attr) == CLASS_STRING) {
	*isstring = 1;

	(*strlist) = DXAllocate(sizeof(char *) * (memcount+1));
	if (!strlist)
	    return ERROR;

	(*strlist)[0] = DXGetString((String)attr);
	*answer = NULL;
	return OK;
    }

    *isstring = 0;
    *strlist = NULL;
    if (DXGetObjectClass(attr) == CLASS_ARRAY) {
	if (!DXGetArrayInfo((Array)attr, &i, &t, &c, &rank, shape))
	    return ERROR;
	
	if (!(*answer = (Object)DXNewArrayV(t, c, rank, shape)))
	    return ERROR;
	
	if (!DXAddArrayData((Array)*answer, 0, 1, 
			    DXGetArrayData((Array)attr))) {
	    DXDelete(*answer);
	    return ERROR;
	}

	return OK;
    }

    /* ? */
    DXSetError(ERROR_DATA_INVALID, "unrecognized attribute object type");
    return ERROR;
}

Error AddStrAttr(Object attr, int attrcount, char **strlist)
{
    if (!attr)
	return ERROR;
    
    (strlist)[attrcount] = DXGetString((String)attr);
    return OK;
}

Error AddOtherAttr(Object attr, int attrcount, Object answer)
{
    if (!attr)
	return ERROR;
    
    if (DXGetObjectClass(attr) != CLASS_ARRAY) {
	DXSetError(ERROR_DATA_INVALID, "unrecognized attribute object type");
	return ERROR;
    }

    if (!DXAddArrayData((Array)answer, attrcount, 
			1, DXGetArrayData((Array)attr))) {
	return ERROR;
    }
    
    return OK;
}



#define A_OBJECT  0
#define A_MEMBERS 1
#define A_ALL     2

/* THIS ONLY DOES GROUPS OF STRING OBJECTS FOR NOW AS A HACK to get
 * data driven selector interactors going in the new ui.
 * attributes can be any type of object and this code should support it.
 * it also only does `object' and `member'.  i'll put `all' in at the
 * same time i put generic object attributes in.
 */
static Object qattr(Object in, Object n, int flag)
{
    char *attrname = NULL;
    char **strlist = NULL;
    Object answer = NULL;
    Object attr = NULL;
    Object member = NULL;
    int memcount, attrcount, firstmember=0, i;
    int isstring;

    if (!DXExtractString(n, &attrname)) {
	DXSetError(ERROR_BAD_PARAMETER, "#10200", "attribute name");
	return NULL;
    }

    /* prevent errors because of unimplemented code */
    if (flag == A_ALL) {
	DXSetError(ERROR_NOT_IMPLEMENTED, "all attributes");
	return NULL;
    }

    /* top level object only */
    if (flag == A_OBJECT) {
	answer = DXGetAttribute(in, attrname);
	if (!answer)
	    DXSetError(ERROR_BAD_PARAMETER, "#10257", "input", attrname);
	return answer;
    }

    /* have to look at members */
    switch (DXGetObjectClass(in)) {

      case CLASS_GROUP:
	if (!DXGetMemberCount((Group)in, &memcount))
	    return NULL;
	if (memcount == 0) {
	    DXSetError(ERROR_DATA_INVALID, "group has no members");
	    return NULL;
	}

	/* find the first group member with this attribute.  all members
         * aren't required to have the attribute - just at least one.
	 */
	attrcount = 0;
	for (i=0; i<memcount; i++) {
	    if (!(member = DXGetEnumeratedMember((Group)in, i, NULL)))
		goto error;
	    if (!DXGetAttribute(member, attrname))
		continue;
	 
	    if (attrcount == 0)
		firstmember = i;
	    attrcount++;
	}
	
	if (attrcount == 0) {
	    DXSetError(ERROR_BAD_PARAMETER, "#10257", "input", attrname);
	    goto error;
	}
	
	/* if the attribute is a string, initialize the strlist pointer.
         * if it's anything else, initialize the answer pointer.
	 */
	if (!(member = DXGetEnumeratedMember((Group)in, firstmember, NULL)))
	    goto error;
	if (!(attr = DXGetAttribute(member, attrname)))
	    goto error;
	if (!MakeMany(attr, attrcount, &isstring, &answer, &strlist))
	    goto error;

	attrcount = 1;
	for (i=firstmember+1; i<memcount; i++) {
	    if (!(member = DXGetEnumeratedMember((Group)in, i, NULL)))
		goto error;
	    if (!(attr = DXGetAttribute(member, attrname)))
		continue;

	    if (isstring)
		AddStrAttr(attr, attrcount, strlist);
	    else
		AddOtherAttr(attr, attrcount, answer);

	    attrcount++;
	}

	/* if attribute isn't string, you have the finished attribute
         * list already.  if attribute is a string, call the makelist
         * routine to allocate a stringlist array
	 */
	if (isstring && 
	    !(answer = (Object)DXMakeStringListV(memcount, strlist)))
	    goto error;

	DXFree((Pointer)strlist);
	return answer;

#if 0
      /* untested, and not sure this is really what people want */
      case CLASS_FIELD:
	if (!DXGetComponentCount((Field)in, &memcount))
	    return NULL;
	if (memcount == 0) {
	    DXSetError(ERROR_DATA_INVALID, "field has no components");
	    return NULL;
	}

	/* find the first component with this attribute.  all components
         * aren't required to have the attribute - just at least one.
	 */
	attrcount = 0;
	for (i=0; i<memcount; i++) {
	    if (!(member = DXGetEnumeratedComponentValue((Field)in, i, NULL)))
		goto error;
	    if (!DXGetAttribute(member, attrname))
		continue;
	    
	    if (attrcount == 0)
		firstmember = i;
	    attrcount++;
	}
	
	if (attrcount == 0) {
	    DXSetError(ERROR_BAD_PARAMETER, "#10257", "input", attrname);
	    goto error;
	}
	
	/* if the attribute is a string, initialize the strlist pointer.
         * if it's anything else, initialize the answer pointer.
	 */
	if (!(member = DXGetEnumeratedComponentValue((field)in, firstmember, NULL)))
	    goto error;
	if (!(attr = DXGetAttribute(member, attrname)))
	    goto error;
	if (!MakeMany(attr, attrcount, &isstring, &answer, &strlist))
	    goto error;

	attrcount = 1;
	for (i=firstmember+1; i<memcount; i++) {
	    if (!(member = DXGetEnumeratedComponentValue((Field)in, i, NULL)))
		goto error;
	    if (!(attr = DXGetAttribute(member, attrname)))
		continue;

	    if (isstring)
		AddStrAttr(attr, attrcount, strlist);
	    else
		AddOtherAttr(attr, attrcount, answer);

	    attrcount++;
	}

	/* if attribute isn't string, you have the finished attribute
         * list already.  if attribute is a string, call the makelist
         * routine to allocate a stringlist array
	 */
	if (isstring && 
	    !(answer = (Object)DXMakeStringListV(memcount, strlist)))
	    goto error;

	DXFree((Pointer)strlist);
	return answer;
#endif

      default:
	answer = DXGetAttribute(in, attrname);
	if (!answer)
	    DXSetError(ERROR_BAD_PARAMETER, "#10257", "input", attrname);
	return answer;
    }

  error:
    DXFree((Pointer)strlist);
    DXDelete(answer);
    return NULL;
}

static Object qoattr(Object in, Object n)
{
    return qattr(in, n, A_OBJECT);
}

static Object qmattr(Object in, Object n)
{
    return qattr(in, n, A_MEMBERS);
}

static Object qlattr(Object in, Object n)
{
    return qattr(in, n, A_ALL);
}

static Object qobjecttag(Object in)
{
    return packint((int)DXGetObjectTag(in));
}

#define MAXQLEN  64
#define MAXVLEN  16
#define DEFAULT_Q  "isnull"

#define IS_YESNO(q)         q, NULL,    "", NULL, NULL, NULL,   "", NULL
#define YESNO_1PARM(q,p) NULL,    q,     p, NULL, NULL, NULL,   "", NULL
#define YESNO_1INPUT(q)  NULL, NULL,    "",    q, NULL, NULL,   "", NULL
#define HAS_NOPARMS(q)   NULL, NULL,    "", NULL,    q, NULL,   "", NULL
#define HAS_1PARM(q,p)   NULL, NULL,    "", NULL, NULL,    q,    p, NULL
#define HAS_1INPUT(q)    NULL, NULL,    "", NULL, NULL, NULL,   "",    q
#define TABLE_END        NULL, NULL,    "", NULL, NULL, NULL,   "", NULL

static struct functable {
    char question[MAXQLEN];
    int (*qyes)(Object);
    int (*qyes1p)(Object, char *);
    char yesvalue[MAXVLEN];
    int (*qyes1i)(Object, Object);
    Object (*qobj)(Object);
    Object (*qobj1p)(Object, char *);
    char objvalue[MAXVLEN];
    Object (*qobj1i)(Object, Object);
} ft[] = 
{
    { "isnull",  		IS_YESNO(qnull)},
    { "isregularpositions",   	IS_YESNO(qregp)},
    { "isregularconnections", 	IS_YESNO(qregc)},
    { "isbyte", 		IS_YESNO(qbyte)},
    { "isshort", 		IS_YESNO(qshort)},
    { "isint", 		    	IS_YESNO(qint)},
    { "isinteger",	    	IS_YESNO(qint)},
    { "isfloat", 		IS_YESNO(qfloat)},
    { "isdouble",		IS_YESNO(qdouble)},
    { "isdatastring",           IS_YESNO(qstring)},
    { "isimage",		IS_YESNO(qimage)},

    { "isobject",             	YESNO_1PARM(qobjtype, "object")},
    { "isprivate",            	YESNO_1PARM(qobjtype, "private")},
    { "isstring",             	YESNO_1PARM(qobjtype, "string")},
    { "isfield",              	YESNO_1PARM(qobjtype, "field")},
    { "isgroup",              	YESNO_1PARM(qobjtype, "group")},
    { "isgenericgroup",       	YESNO_1PARM(qobjgtype,  "group")},
    { "isseries",             	YESNO_1PARM(qobjgtype,  "series")},
    { "iscompositefield",     	YESNO_1PARM(qobjgtype,  "compositefield")},
    { "ismultigrid",          YESNO_1PARM(qobjgtype,  "multigrid")},
    { "isarray",              YESNO_1PARM(qobjtype, "array")},
    { "isgenericarray",       YESNO_1PARM(qobjatype,  "array")},
    { "isirregulararray",     YESNO_1PARM(qobjatype,  "array")},
    { "isregulararray",       YESNO_1PARM(qobjatype,  "regulararray")},
    { "ispatharray",          YESNO_1PARM(qobjatype,  "patharray")},
    { "isproductarray",       YESNO_1PARM(qobjatype,  "productarray")},
    { "ismesharray",          YESNO_1PARM(qobjatype,  "mesharray")},
    { "isconstantarray",      YESNO_1PARM(qobjatype,  "constantarray")},
    { "istransform",          YESNO_1PARM(qobjtype, "xform")},
    { "isxform",              YESNO_1PARM(qobjtype, "xform")},
    { "isscreen",             YESNO_1PARM(qobjtype, "screen")},
    { "isclipped",            YESNO_1PARM(qobjtype, "clipped")},
    { "iscamera",             YESNO_1PARM(qobjtype, "camera")},
    { "islight",              YESNO_1PARM(qobjtype, "light")},
    { "isempty",              YESNO_1PARM(qobjempty, "")},
    { "isemptygroup",         YESNO_1PARM(qobjempty, "group")},
    { "isemptyfield",         YESNO_1PARM(qobjempty, "field")},
    { "isemptyarray",         YESNO_1PARM(qobjempty, "array")},
    { "is1dgridpositions",    YESNO_1PARM(qNdgp, "1")},
    { "is2dgridpositions",    YESNO_1PARM(qNdgp, "2")},
    { "is3dgridpositions",    YESNO_1PARM(qNdgp, "3")},
    { "is4dgridpositions",    YESNO_1PARM(qNdgp, "4")},
    { "is1dgridconnections",  YESNO_1PARM(qNdgc, "1")},
    { "is2dgridconnections",  YESNO_1PARM(qNdgc, "2")},
    { "is3dgridconnections",  YESNO_1PARM(qNdgc, "3")},
    { "is4dgridconnections",  YESNO_1PARM(qNdgc, "4")},
    { "is1dpositions", 	    YESNO_1PARM(qNdp, "1")},
    { "is2dpositions", 	    YESNO_1PARM(qNdp, "2")},
    { "is3dpositions", 	    YESNO_1PARM(qNdp, "3")},
    { "is4dpositions",        YESNO_1PARM(qNdp, "4")},
    { "is1dconnections",	    YESNO_1PARM(qNdc, "1")},
    { "is2dconnections",	    YESNO_1PARM(qNdc, "2")},
    { "is3dconnections", 	    YESNO_1PARM(qNdc, "3")},
    { "isline", 		    YESNO_1PARM(qNdc, "1")},
    { "issurface",	    YESNO_1PARM(qNdc, "2")},
    { "isvolume", 	    YESNO_1PARM(qNdc, "3")},
    { "isscalar",		    YESNO_1PARM(qNdd, "0")},
    { "isvector",		    YESNO_1PARM(qNdd, "1")},
    { "is2vector",	    YESNO_1PARM(q1dNd, "2")},
    { "is3vector",	    YESNO_1PARM(q1dNd, "3")},
    { "ismatrix", 	    YESNO_1PARM(qNdd, "2")},


    { "isdatadependent",      YESNO_1INPUT(qdatadep)},
    { "hascomponent",         YESNO_1INPUT(qhascomp)},
    { "hasattribute",         YESNO_1INPUT(qhasattr)},
    { "hasmember",            YESNO_1INPUT(qhasmember)},
    { "isregular",	    YESNO_1INPUT(qreg)},
    { "stringmatch",	    YESNO_1INPUT(qequal)},
    { "stringsmatch",	    YESNO_1INPUT(qequal)},
    { "issame",	            YESNO_1INPUT(qsame)},
    { "isconnection",	    YESNO_1INPUT(qiscontype)},
    { "isconnections",	    YESNO_1INPUT(qiscontype)},
    { "areconnections",	    YESNO_1INPUT(qiscontype)},


    { "type",       	    HAS_NOPARMS(qtype)},
    { "category",       	    HAS_NOPARMS(qcat)},
    { "rank",       	    HAS_NOPARMS(qrank)}, 
    { "shape",       	    HAS_NOPARMS(qshape)},
    { "origin",       	    HAS_NOPARMS(qorigin)},
    { "deltas",       	    HAS_NOPARMS(qdeltas)},
    { "positioncounts",       HAS_NOPARMS(qpos)},
    { "positionscounts",      HAS_NOPARMS(qpos)},
    { "connectioncounts",     HAS_NOPARMS(qconnect)},
    { "connectionscounts",    HAS_NOPARMS(qconnect)},
    { "datacounts", 	    HAS_NOPARMS(qdata)},
    { "positiongridcounts",   HAS_NOPARMS(qpgridsize)},
    { "positionsgridcounts",  HAS_NOPARMS(qpgridsize)},
    { "connectiongridcounts", HAS_NOPARMS(qcgridsize)},
    { "connectionsgridcounts",HAS_NOPARMS(qcgridsize)},
    { "productterm",          HAS_NOPARMS(qprodterms)},
    { "productterms",         HAS_NOPARMS(qprodterms)},
    { "meshterm",             HAS_NOPARMS(qmeshterms)},
    { "meshterms",            HAS_NOPARMS(qmeshterms)},
    { "membercount",	    HAS_NOPARMS(qmembers)},
    { "componentcount",	    HAS_NOPARMS(qcompcount)},
    { "memberposition",	    HAS_NOPARMS(qmemberpos)},
    { "seriesposition",	    HAS_NOPARMS(qmemberpos)},
    { "memberpositions",	    HAS_NOPARMS(qmemberpos)},
    { "seriespositions",	    HAS_NOPARMS(qmemberpos)},
    { "processors",	    HAS_NOPARMS(qnumprocess)},
    { "primitives",	    HAS_NOPARMS(qprimitives)},
    { "applytransform",	    HAS_NOPARMS(doxform)},
    { "transformapplied",     HAS_NOPARMS(doxform)},
    { "objecttag",	    HAS_NOPARMS(qobjecttag)},
    { "connectiontype",	    HAS_NOPARMS(qcontype)},
			    		     
    { "transformobject",	    HAS_1PARM(qxform, "o")},
    { "transformmatrix",	    HAS_1PARM(qxform, "m")},
    { "screenobject",	    HAS_1PARM(qscreen, "o")},
    { "screenposition",	    HAS_1PARM(qscreen, "p")},
    { "screendepth",	    HAS_1PARM(qscreen, "z")},
    { "clippedobject",	    HAS_1PARM(qclipped, "o")},
    { "clippingobject",	    HAS_1PARM(qclipped, "c")},
    { "camerato",	  	    HAS_1PARM(qcamera, "to")},
    { "camerafrom",	    HAS_1PARM(qcamera, "fr")},
    { "camerawidth",	    HAS_1PARM(qcamera, "w")},
    { "cameraresolution",     HAS_1PARM(qcamera, "r")},
    { "cameraaspect",	    HAS_1PARM(qcamera, "as")},
    { "cameraup",	   	    HAS_1PARM(qcamera, "u")},
    { "cameraperspective",    HAS_1PARM(qcamera, "p")},
    { "cameraangle",	    HAS_1PARM(qcamera, "an")},
    { "camerafieldofview",    HAS_1PARM(qcamera, "fi")},
    { "camerabackground",     HAS_1PARM(qcamera, "b")},
    { "cameratransform",	    HAS_1PARM(qcamera, "tr")},
    { "cameramatrix",	    HAS_1PARM(qcamera, "tr")},
    { "iscameraorthographic", HAS_1PARM(qcamera, "iso")},
    { "iscameraperspective",  HAS_1PARM(qcamera, "isp")},
    { "membernames",	    HAS_1PARM(qmembernames, "m")},
    { "componentnames",	    HAS_1PARM(qmembernames, "c")},
    { "validcount",	    HAS_1PARM(qvalid, "v")},
    { "validcounts",	    HAS_1PARM(qvalid, "v")},
    { "invalidcount",	    HAS_1PARM(qvalid, "i")},
    { "invalidcounts",	    HAS_1PARM(qvalid, "i")},

    { "items",		    HAS_1INPUT(qcompsize)},
    { "count",		    HAS_1INPUT(qcompsize)},
    { "counts",		    HAS_1INPUT(qcompsize)},
    { "gridcounts",	    HAS_1INPUT(qcompgridsize)},
    { "attribute",            HAS_1INPUT(qoattr)},
    { "memberattribute",      HAS_1INPUT(qmattr)},
    { "memberattributes",     HAS_1INPUT(qmattr)},
    { "leafattribute",        HAS_1INPUT(qlattr)},
    { "leafattributes",       HAS_1INPUT(qlattr)},

    { "",		    TABLE_END},
};

static Object SayYesNo(int say)
{
    if (say != 0 && say != 1) {
	DXSetError(ERROR_INTERNAL, "yes/no question routine returned %d", say);
	return NULL;
    }

    return packint(say);
}

/* lowercase the question string, squeeze out blanks, and check
 * for optional trailing +/- 1 and remove it if found
 */
static Error squish(char *in, char *out, int *invert, int *offset)
{
    char *cp;

    /* save start location of output buffer.
     */
    cp = out;

    /* convert to lowercase and squeeze out all blanks and tabs.
     */
    while (*in) {
	if (!isspace(*in)) 
	    *out++ = isupper(*in) ? tolower(*in) : *in;
	in++;
    }
    *out = '\0';

    /* now look for leading "isnot" and copy the question string down over
     * the "not" so it's just plain "is" and set the invert flag.
     */
    if (cp[0] == 'i' && cp[1] == 's' && 
	cp[2] == 'n' && cp[3] == 'o' && cp[4] == 't') {
	*invert = 1;
	
	cp += 2;
       /* i had to break the while body into two statements, because
        * the order of evaluation (basically, when does cp get incremented)
        * is documented to be up to the compiler writer.
        */
        while (*(cp+3)) {
           *cp = *(cp+3);
           cp++;
       }
	*cp = '\0';
    } else
	cp = out;


    /* now look for a trailing -1 or +1, set the corresponding
     * offset value and truncate the string so the offset doesn't show.
     */
    if (cp[-2] == '-' || cp[-2] == '+' || cp[-1] == '1') {
	if (cp[-1] != '1') {
	    DXSetError(ERROR_BAD_PARAMETER, 
		       "questions can only be followed by +1 or -1");
	    return ERROR;
	}
	if (cp[-2] == '-') {
	    *offset = -1;
	    cp[-2] = '\0';
	    return OK;
	}
	if (cp[-2] == '+') {
	    *offset = 1;
	    cp[-2] = '\0';
	    return OK;
	}
	DXSetError(ERROR_BAD_PARAMETER, 
		   "questions can only be followed by +1 or -1");
	return ERROR;
    }

    return OK;
}

static Object fixinvert(Object o)
{
    int oldvalue;
    int items;
    Type t;
    Category c;
    int rank;

    if (DXGetObjectClass(o) != CLASS_ARRAY)
	return NULL;

    if (!DXGetArrayInfo((Array)o, &items, &t, &c, &rank, NULL))
	return NULL;

    if (items != 1 || t != TYPE_INT || c != CATEGORY_REAL || rank != 0)
	return NULL;

    oldvalue = *(int *)DXGetArrayData((Array)o);
    if (oldvalue != 0 && oldvalue != 1)
	return NULL;

    oldvalue = !oldvalue;

    return packint(oldvalue);
}


static Object fixoffset(Object o, int offset)
{
    int oldvalue;
    int items;
    Type t;
    Category c;
    int rank;

    if (DXGetObjectClass(o) != CLASS_ARRAY)
	return NULL;

    if (!DXGetArrayInfo((Array)o, &items, &t, &c, &rank, NULL))
	return NULL;

    if (items != 1 || t != TYPE_INT || c != CATEGORY_REAL || rank != 0)
	return NULL;

    oldvalue = *(int *)DXGetArrayData((Array)o);
    oldvalue += offset;

    return packint(oldvalue);
}


Error
m_Inquire(Object *in, Object *out)
{
    int offset = 0, invert = 0;
    char userquestion[MAXQLEN], *qp;
    struct functable *fp;
    Object op = NULL;

    out[0] = NULL;

    if (!in[1])
	qp = DEFAULT_Q;
    
    else if (!DXExtractString(in[1], &qp)) {
	DXSetError(ERROR_BAD_PARAMETER, "#10200", "inquiry");
	return ERROR;
    }

    if (!squish(qp, userquestion, &invert, &offset))
	return ERROR;

    for (fp = ft; fp->question[0] != '\0'; fp++) {
	
	if (strcmp(fp->question, userquestion))
	    continue;
	
	if (fp->qyes)
	    out[0] = SayYesNo((fp->qyes)(in[0]));
	else if (fp->qyes1p)
	    out[0] = SayYesNo((fp->qyes1p)(in[0], fp->yesvalue));
	else if (fp->qyes1i)
	    out[0] = SayYesNo((fp->qyes1i)(in[0], in[2]));
	else if (fp->qobj)
	    out[0] = (Object)(fp->qobj)(in[0]);
	else if (fp->qobj1p)
	    out[0] = (Object)(fp->qobj1p)(in[0], fp->objvalue);
	else if (fp->qobj1i)
	    out[0] = (Object)(fp->qobj1i)(in[0], in[2]);
	else
	    DXSetError(ERROR_INTERNAL, "inconsistant inquiry table");
	
	/* if a yes/no routine returned a value but someone set an
         *  error along the way, delete the return value and really
         *  return error.
	 */
	if ((DXGetError() != ERROR_NONE) && out[0]) {
	    DXDelete(out[0]);
	    out[0] = NULL;
	}
	
	if (!out[0] && (DXGetError() == ERROR_NONE))
	    DXSetError(ERROR_BAD_PARAMETER, "inquiry failed");

	
	if (invert != 0) {
	    if ((op = fixinvert(out[0])) == NULL) {
		DXSetError(ERROR_BAD_PARAMETER, 
			   "cannot use 'not' with a value other than 0 or 1");
		DXDelete(out[0]);
		out[0] = NULL;
		return ERROR;
	    }
	    DXDelete(out[0]);
	    out[0] = op;
	}
	    
	if (offset != 0) {
	    if ((op = fixoffset(out[0], offset)) == NULL) {
		DXSetError(ERROR_BAD_PARAMETER, 
			   "cannot add or subtract 1 from this type of data");
		DXDelete(out[0]);
		out[0] = NULL;
		return ERROR;
	    }
	    DXDelete(out[0]);
	    out[0] = op;
	}
	    
	return out[0] ? OK : ERROR;

    }

    DXSetError(ERROR_BAD_PARAMETER, "#10480", "inquiry");
    return ERROR;
}


/* 
 * new requests:
 *  that you can query max memory and currently available mem
 *
 *  that things which work on fields also work on composite fields
 * 
 *  that you can compare strings somehow.
 */
