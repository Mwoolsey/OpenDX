/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999,2002                              */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#ifndef _transferAccelerator_h
#define _transferAccelerator_h


#ifdef __cplusplus
extern "C" {                     
#endif

/* Function to force accelerator from one widget onto another.
   This works well when accelerators on sub dialogs need to still
   activate its parents functions.
 */

Boolean TransferAccelerator(Widget shell, Widget source, String action);

#ifdef __cplusplus  
}  /* Close scope of 'extern "C"' declaration which encloses file. */ 
#endif  


#endif
