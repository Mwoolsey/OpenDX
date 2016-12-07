/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>



#include <string.h>
#include <dx/dx.h>
#include "internals.h"

#define N       1024
#define WRAP(i) ((i)&(N-1))

char * _dxfstring(char *, int);
Object _dxfstringobject(char *, int);

#define DECLARE(x) char * _dxd_##x = NULL; Object _dxd_o_##x = NULL

DECLARE(back_colors);
DECLARE(binormals);
DECLARE(both_colors);
DECLARE(box);
DECLARE(colors);
DECLARE(color_map);
DECLARE(color_multiplier);
DECLARE(connections);
DECLARE(cubes);
DECLARE(data);
DECLARE(dep);
DECLARE(der);
DECLARE(element_type);
DECLARE(faces);
DECLARE(fb_image);
DECLARE(fontwidth);
DECLARE(front_colors);
DECLARE(image);
DECLARE(image_type);
DECLARE(inner);
DECLARE(invalid_positions);
DECLARE(invalid_connections);
DECLARE(lines);
DECLARE(neighbors);
DECLARE(normals);
DECLARE(opacities);
DECLARE(opacity_map);
DECLARE(opacity_multiplier);
DECLARE(points);
DECLARE(polylines);
DECLARE(positions);
DECLARE(quads);
DECLARE(ref);
DECLARE(surface);
DECLARE(tangents);
DECLARE(tetrahedra);
DECLARE(time);
DECLARE(tri);
DECLARE(triangles);
DECLARE(vbox);
DECLARE(vol);
DECLARE(x_image);
DECLARE(x_server);



static
struct table {
    lock_type DXlock;		/* for adding strings */
    struct hash {		/* string hash table */
	char *s;		/* allocated string */
	Object o;		/* String object */
    } hash[N];			/* N entries */
} *table;

#define DO_INIT(l, s) _dxd_##l = _dxfstring(s, 1); \
		      _dxd_o_##l = _dxfstringobject(_dxd_##l, 1);
Error
_dxf_initstringtable(void)
{
    table = (struct table *) DXAllocateZero(sizeof(struct table));
    if (!table)
	return ERROR;
    DXcreate_lock(&table->DXlock, "string table");

    DO_INIT(back_colors, "back colors");
    DO_INIT(binormals, "binormals");
    DO_INIT(both_colors, "both colors");
    DO_INIT(box, "box");
    DO_INIT(colors, "colors");
    DO_INIT(color_map, "color map");
    DO_INIT(color_multiplier, "color multiplier");
    DO_INIT(connections, "connections");
    DO_INIT(cubes, "cubes");
    DO_INIT(data, "data");
    DO_INIT(dep, "dep");
    DO_INIT(der, "der");
    DO_INIT(element_type, "element type");
    DO_INIT(faces, "faces");
    DO_INIT(fb_image, "fb image");
    DO_INIT(fontwidth, "fontwidth");
    DO_INIT(front_colors, "front colors");
    DO_INIT(image, "image");
    DO_INIT(image_type, "image type");
    DO_INIT(inner, "inner");
    DO_INIT(invalid_positions, "invalid positions");
    DO_INIT(invalid_connections, "invalid connections");
    DO_INIT(lines, "lines");
    DO_INIT(neighbors, "neighbors");
    DO_INIT(normals, "normals");
    DO_INIT(opacities, "opacities");
    DO_INIT(opacity_map, "opacity map");
    DO_INIT(opacity_multiplier, "opacity multiplier");
    DO_INIT(points, "points");
    DO_INIT(polylines, "polylines");
    DO_INIT(positions, "positions");
    DO_INIT(quads, "quads");
    DO_INIT(ref, "ref");
    DO_INIT(surface, "surface");
    DO_INIT(tangents, "tangents");
    DO_INIT(tetrahedra, "tetrahedra");
    DO_INIT(time, "time");
    DO_INIT(tri, "tri");
    DO_INIT(triangles, "triangles");
    DO_INIT(vbox, "valid box");
    DO_INIT(vol, "vol");
    DO_INIT(x_image, "x image");
    DO_INIT(x_server, "x server");

    return OK;
}

static
ulong
_string(char *s, int add, int object)
{
    char *t, *ss;
    int h, c;
    struct hash *hash;

    /* initialization */
    if (!table)
	DXinitdx();
    if (!table)
	return 0;
    hash = table->hash;

    /* calculate initial hash value h */
    for (ss=s, h=0; (c=(unsigned char)*ss)!=0; ss++)	/* for each char c in s */
	h = WRAP(17*h + c);			/* update h */

    /* look for it (closed chaining) */
    for (h=h; (t=hash[h].s)!=0; h=WRAP(h+1))		/* start with h; wrap around */
	if (strcmp(t,s)==0)			/* is t the string we want? */
	    break;				/* if so break */

    /* add it if not there and adding was requested */
    if (!t && add) {				/* t is NULL if not there */
	DXlock(&table->DXlock, 0);		/* one person at a time */
	for (h=h; (t=hash[h].s)!=0; h=WRAP(h+1))	/* start where we left off */
	    if (strcmp(t,s)==0)			/* did anyone beat us to it? */
		break;				/* if so break */
	if (!t) {				/* if no one else added it */
	    Object o = (Object) DXNewString(s);	/* make a string object */
	    _dxf_SetPermanent(o);		/* avoid Ref bottlenecks */
	    if (o) {				/* if it succeeded */
		t = DXGetString((String)o);	/* the string */
		hash[h].o = o;			/* put object there first */
		hash[h].s = t;			/* do this last */
	    }
	}
	DXunlock(&table->DXlock, 0);		/* now unlock table */
    }

    return object? (t?(ulong)(hash[h].o):0) : (ulong)t;
}

char *
_dxfstring(char *s, int add)
{
    return (char *)_string(s, add, 0);
}

Object
_dxfstringobject(char *s, int add)
{
    return (Object)_string(s, add, 1);
}

