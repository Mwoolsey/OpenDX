/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"


#ifndef _TMP_LIC_
#define _TMP_LIC_

#if !defined(DXD_OS_NON_UNIX)

#include "License.h"

#define KEY1 "a9"
#define KEY2 "Pp"
#define XOR_KEY 0x12345678

#define PROGVER  "3.1"   /* version of expire program */

#define ANYWHERE_HOSTID "00000000"

enum Arkh { UNKNOWN, ANY, UNIX, IBM6000, HP700, SUN4, SOLARIS, SGI, AVIION, ALPHA, WINTEL, WINNT, WIN95 };
enum DxType { UP, SMP };
enum LicType { TRIAL, BETA, NODELOCKED, FLOATING };


class TemporaryLicense {
  private:
    char* dxui_args;
    boolean limited_use;

    void setDxuiArgs(const char* );

  protected:
    void determineUILicense (LicenseTypeEnum*,LicenseTypeEnum*);
    void initialize();
    TemporaryLicense(){
	this->dxui_args = NUL(char*);
	this->limited_use = FALSE;
    };
    ~TemporaryLicense(){
	if (this->dxui_args) delete this->dxui_args;
    };
    virtual const char* getRootDir() = 0;

    const char* getLimitedArgs(); 

  public:
    static const char* MakeKey(enum Arkh , char *, int , int , time_t *, enum DxType );
    const char* getDxuiArgs() { return this->dxui_args; }
    boolean isLimitedUse() { return this->limited_use; }
};

#endif
#endif
