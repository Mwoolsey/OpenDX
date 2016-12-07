/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
#ifndef __OEM_H_
#define __OEM_H_

#include <dxconfig.h>



char *ScrambleAndEncrypt(const char *src, const char *hash, char *cryptbuf);
char *ScrambleString(const char *str, const char *hash);


#endif /* __OEM_H_ */
