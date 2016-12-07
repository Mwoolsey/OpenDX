/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


#include <string.h>
#include "interact.h"
#include "_colormap.h"

#define NUMBINS 20

extern Array DXScalarConvert(Array ); /* from libdx/stats.h */

typedef struct map {
   float level;
   float value;
} Map;
typedef struct hsvo {
   Map *map;
   char *name;
   int count;
   int minmax;
} HSVO;

static Map interp(Map bot, Map top, Map pt);
static int findmin(float a,float b,float c);
static Error fill_end(Array inmap,Map **map,int *num);


/* input three arrays representing each map 
 * an hsv field is created */
Field _dxfeditor_to_hsv(Array huemap,Array satmap,Array valmap)
{
   Map h,s,v;
   Map *hue,*sat,*val;
   Map *huevalue,*satvalue,*valvalue;
   int num_hue,num_sat,num_val;
   float *hsv_pts;
   RGBColor *hsv_data;
   int max_pts,num;
   int i,which,ih,is,iv;
   Field hsv;

   huevalue = NULL; satvalue = NULL; valvalue = NULL; 
   hsv_pts = NULL; hsv_data = NULL;
   /* if max and min are not in maps add them */
   if (!fill_end(huemap,&huevalue,&num))
      goto error;
   if (num == 0){
      if (!DXGetArrayInfo(huemap,&num_hue,NULL,NULL,NULL,NULL))
	 goto error;
      hue = (Map *)DXGetArrayData(huemap);
   }
   else {
      num_hue = num;
      hue = huevalue;
   }
   if (!fill_end(satmap,&satvalue,&num))
      goto error;
   if (num == 0){
      if (!DXGetArrayInfo(satmap,&num_sat,NULL,NULL,NULL,NULL))
	 goto error;
      sat = (Map *)DXGetArrayData(satmap);
   }
   else {
      num_sat = num;
      sat = satvalue;
   }
   if (!fill_end(valmap,&valvalue,&num))
      goto error;
   if (num == 0){
      if (!DXGetArrayInfo(valmap,&num_val,NULL,NULL,NULL,NULL))
	 goto error;
      val = (Map *)DXGetArrayData(valmap);
   }
   else {
      num_val = num;
      val = valvalue;
   }

   max_pts = num_hue + num_sat + num_val - 4;
   hsv_data = (RGBColor *)DXAllocate(sizeof(RGBColor) * max_pts);
   hsv_pts = (float *)DXAllocate(sizeof(float) *max_pts);

   /* convert to hsv arrays */
   /* the first point is always at zero level and  the last at level 1.0
    * there are always at least two points (fill_ends) */
   hsv_pts[0] = hue[0].level;
   hsv_data[0] = DXRGB(hue[0].value,sat[0].value,val[0].value);

   i=1;
   for (ih=1,iv=1,is=1; (ih<num_hue || is<num_sat || iv<num_val);  ){
      if (ih >=num_hue) ih--;
      if (is >=num_sat) is--;
      if (iv >=num_val) iv--;
      which = findmin(hue[ih].level,sat[is].level,val[iv].level);

      switch(which){
	 case(0):
	    if (hue[ih].level == sat[is].level){ 
	       s = sat[is];
	       is++;
	    }
	    else
	       s = interp(sat[is-1],sat[is],hue[ih]);
	    if (hue[ih].level == val[iv].level){
	       v = val[iv];
	       iv++;
	    }
	    else
	       v = interp(val[iv-1],val[iv],hue[ih]);
	    hsv_data[i] = DXRGB(hue[ih].value,s.value,v.value);
	    hsv_pts[i++] = hue[ih].level;
	    ih++;
	    break;
	 case(1):
	    if (sat[is].level == val[iv].level){
	       v = val[iv];
	       iv++;
	    }
	    else
	       v = interp(val[iv-1],val[iv],sat[is]);
	    h = interp(hue[ih-1],hue[ih],sat[is]);
	    hsv_data[i] = DXRGB(h.value,sat[is].value,v.value);
	    hsv_pts[i++] = sat[is].level;
	    is++;
	    break;
	 case(2):
	    h = interp(hue[ih-1],hue[ih],val[iv]);
	    s = interp(sat[is-1],sat[is],val[iv]);
	    hsv_data[i] = DXRGB(h.value,s.value,val[iv].value);
	    hsv_pts[i++] = val[iv].level;
	    iv++;
	    break;
      }
   }

   /* make an HSV field */
   hsv = (Field)_dxfcolorfield(hsv_pts,(float *)hsv_data,i,1);
   if (!hsv)
      goto error;
   /* Free newmaps */
   if (huevalue) DXFree((Pointer)huevalue);
   if (satvalue) DXFree((Pointer)satvalue);
   if (valvalue) DXFree((Pointer)valvalue);
   DXFree((Pointer)hsv_pts);
   DXFree((Pointer)hsv_data);
   return (Field) hsv;
error:
   if (hsv_pts) DXFree((Pointer)hsv_pts);
   if (hsv_data) DXFree((Pointer)hsv_data);
   if (huevalue) DXFree((Pointer)huevalue);
   if (satvalue) DXFree((Pointer)satvalue);
   if (valvalue) DXFree((Pointer)valvalue);
   return NULL;
}

/* input the opacity positions array and the opacity field 
 * will be returned
 */
Field _dxfeditor_to_opacity(Array opmap)
{
   int i,num_op,new_num,offset=0;
   Field opacity;
   float *op_pts,*op_data;
   Array a,b;
   Map *op;

   opacity = NULL;
   /* if array is NULL create a default opacity field */
   if (!opmap){
      num_op = 2;
      opacity = (Field)_dxfcolorfield(NULL,NULL,num_op,0);
      a = (Array)DXGetComponentValue((Field)opacity,"positions");
      b = (Array)DXGetComponentValue((Field)opacity,"data");
      op_pts = (float *)DXGetArrayData(a);
      op_data = (float *)DXGetArrayData(b);
      op_pts[0] = 0.0;
      op_pts[1] = 1.0;
      op_data[0] = op_data[1] = 1.0;
   }
      
   if (!DXGetArrayInfo(opmap,&num_op,NULL,NULL,NULL,NULL))
      goto error;
   op = (Map *)DXGetArrayData(opmap);
   /* check for missing end points */
   new_num=num_op;
   if (op[num_op-1].level != 1.0)
      new_num++;
   if (op[0].level != 0.0) {
      offset = 1;
      new_num++;
   }
   
   /* build new field */
   opacity = (Field)_dxfcolorfield(NULL,NULL,new_num,0);
   a = (Array)DXGetComponentValue((Field)opacity,"positions");
   b = (Array)DXGetComponentValue((Field)opacity,"data");
   op_pts = (float *)DXGetArrayData(a);
   op_data = (float *)DXGetArrayData(b);
   if (offset == 1){
      op_pts[0] = 0.0;
      op_data[0] = op[0].value;
   }
   if (op[num_op-1].level != 1.0){
      op_pts[new_num-1] = 1.0;
      op_data[new_num-1] = op[num_op-1].value;
   }
   for (i=0; i<num_op; i++){
      op_pts[offset + i] = op[i].level;
      op_data[offset + i] = op[i].value;
   }

   if (!DXEndField(opacity))
      goto error;
   
   return (Field)opacity;

error:
   if (opacity) DXDelete((Object)opacity);
   return NULL;
}


/* if the min and max points are missing copy inmaps to outmaps and
 * fill the end points the control points are normalized 
 * if maps are NULL create a default map */
static 
Error fill_end(Array in,Map **outmap,int *num)
{
   int i,offset=0,item,new;
   float min=0.0,max=1.0;
   Map *inmap,*map;

   map = NULL;
   *num = 0;
   /* does the array exist */
   if (!in){			/* create default map */
      map = (Map *)DXAllocate(sizeof(Map) * 2);
      map[0].level = min;
      map[1].level = max;
      map[0].value = map[1].value = 1.0;
      *num = 2;
      goto done;
   }

   /* does the array have end points at the min and max */
   if (!DXGetArrayInfo(in,&item,NULL,NULL,NULL,NULL))
      goto error;
   new = item;
   inmap = (Map *)DXGetArrayData(in);
   if (inmap[item-1].level != 1.0)
      new++;
   if (inmap[0].level != 0.0){
      new++;
      offset=1;
   }
   if (new > item){
      map = (Map *)DXAllocate(sizeof(Map) *new);
      if (offset==1){
         map[0].level = 0.0;
         map[0].value = inmap[0].value;
      }
      if (inmap[item-1].level != 1.0){
         map[new-1].level = 1.0;
         map[new-1].value = inmap[item-1].value;
      }
      for (i=0; i<item; i++){
         map[i+offset].level = inmap[i].level;
         map[i+offset].value = inmap[i].value;
      }
      *num = new;
   }

done:
   *outmap = map;
   
   /* 
   for (n=0; n<3; n++){
      if(inmaps[n].count == 0){
	 outmaps[n].count = 2;
	 outmaps[n].map = (Map *)DXAllocate(sizeof(Map) *outmaps[n].count);
	 outmaps[n].map[0].level = min;
	 outmaps[n].map[1].level = max;
	 outmaps[n].map[0].value = outmaps[n].map[1].value = 1.0;
      }
      else{
       offset=0;
       if (inmaps[n].minmax > 2){
         outmaps[n].count = inmaps[n].count+2;
         outmaps[n].map = (Map *)DXAllocate(sizeof(Map) *outmaps[n].count);
         outmaps[n].map[0].level = min;
         outmaps[n].map[0].value = inmaps[n].map[0].value;
         outmaps[n].map[outmaps[n].count-1].level = max;
         outmaps[n].map[outmaps[n].count-1].value =
                     inmaps[n].map[inmaps[n].count-1].value;
         offset = 1;
       }
       else if (inmaps[n].minmax > 0){
         outmaps[n].count = inmaps[n].count +1;
         outmaps[n].map = (Map *)DXAllocate(sizeof(Map) *outmaps[n].count);
         if (inmaps[n].minmax == 1){
	    outmaps[n].map[0].level = min;
            outmaps[n].map[0].value = inmaps[n].map[0].value;
	    offset = 1;
         }
         else{
            outmaps[n].map[outmaps[n].count-1].level = max;
            outmaps[n].map[outmaps[n].count-1].value =
	    inmaps[n].map[inmaps[n].count-1].value;
         }
       }
       else{
         outmaps[n].count = inmaps[n].count;
         outmaps[n].map = (Map *)DXAllocate(sizeof(Map) *outmaps[n].count);
       }
       for (i=0; i<inmaps[n].count; i++){
         outmaps[n].map[offset+i].level= inmaps[n].map[i].level;
         outmaps[n].map[offset+i].value = inmaps[n].map[i].value;
       }
      }
   }
   */
   return OK;
error:
   return ERROR;
}

Field _dxfcolorfield(float *pts, float *data,int items,int isvector)
{
   Field f;
   int rank=0,dim=0;
   Array pa,da,ca;

   pa = NULL; ca = NULL; da = NULL;
   f = NULL;
   /* make the pts a 1-vector Array, they become the positions */
   pa = DXNewArray(TYPE_FLOAT,CATEGORY_REAL,1,1);
   if (!pa)
      goto error;
   if (!DXAddArrayData(pa, 0, items, (Pointer)pts))
      goto error; 

   /* make data array */ 
   if (isvector==1) {rank=1; dim=3;
      da = DXNewArray(TYPE_FLOAT,CATEGORY_REAL,rank,dim);
   }
   else 
      da = DXNewArray(TYPE_FLOAT,CATEGORY_REAL,rank);
      
   if (!da)
      goto error;
   if (!DXAddArrayData(da, 0, items, (Pointer)data))
      goto error;

   /* create field */ 
   f = DXNewField();
   if (!f)
      goto error;

   if (!DXSetComponentValue(f, "positions",(Object)pa))
      goto error;
   if (!DXSetComponentValue(f, "data",(Object)da))
      goto error;
   da = NULL;
   pa = NULL;

   if (! DXSetComponentAttribute(f, "data", "dep",
                                (Object)DXNewString("positions")))
       goto error;

   /* create regular connections array */
   if (items > 1)
   {
      ca = DXMakeGridConnectionsV(1,&items);
      if (! ca)
          goto error;

      if (! DXSetComponentValue(f, "connections", (Object)ca))
          goto error;
      ca = NULL;

      if (! DXSetComponentAttribute(f, "connections", "element type",
                                          (Object)DXNewString("lines")))
          goto error;
   }

   if (!DXEndField(f))
      goto error;

   return (Field)f;

error:
   if (pa) DXDelete((Object)pa);
   if (da) DXDelete((Object)da);
   if (ca) DXDelete((Object)ca);
   if (f) DXDelete((Object)f);
   return NULL;
}


/* interpolate to find points between bot and top at pt */
static
Map interp(Map bot, Map top, Map pt)
{
   Map new;

   new.level = pt.level;
   new.value = bot.value + (top.value-bot.value)*(bot.level-pt.level)/
	       (bot.level-top.level);

   return new;
}

/* find the min value */
static
int findmin(float a,float b,float c)
{
   if (a <= b && a <= c) return 0;
   if (b <= a && b <= c) return 1;
   return 2;
}

