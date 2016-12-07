/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/


/*
 * $Header: /src/master/dx/src/uipp/java/server/DXLink.c,v 1.11 2006/07/21 17:56:29 davidt Exp $
 */
#if defined(hp700) 
#define _UINT64_T
#define _INT64_T
#define _UINT32_T
#define _INT32_T
#endif /* defined hp700 */
#include "dxconfig.h"
#include <string.h>
#include <stdlib.h>
/* 
 * This used to say <dx/dxl.h> but I changed it for ease of building within
 * the dx-4.0.x tree
 */
#include "dxl.h"
#include "server_DXServerThread.h"
#include "server_DXServer.h"

/*
 * The ibm6000 version of DXServer leaves defunct processes around.
 * using wait3 seems to take care of the problem.  I havent' tested
 * the other platforms.
 */
#if defined(ibm6000) || defined(sgi)
#if defined(ibm6000) || defined(hp700) || defined(alphax) || defined(sun4)
#define USE_WAIT3 1
#endif

#if USE_WAIT3
#if defined(alphax)
#include <sys/time.h>
#define _BSD
#include <sys/wait.h>
#else
#include <sys/wait.h>
#include <sys/time.h>
#endif
#else
#include <wait.h>
#endif
#endif


typedef struct _JavaEnv {
    void* java_env;
    void* java_obj;
} *JavaEnvironment;





/*
 * remove trailing whitespace from the message
 */
void DXLink_InfoMsgCB (DXLConnection * u1, const char *msg, const void *data)
{
int i,j;
jstring jstr;
jvalue jv[1];
char* dupmsg;

    JavaEnvironment je = (JavaEnvironment)data;
    JNIEnv* env = (JNIEnv*)je->java_env;
    jobject obj = (jobject)je->java_obj;
    jclass cls = (*env)->GetObjectClass(env, obj);
    jmethodID mid = (*env)->GetMethodID(env, cls, "infoNotify", "(Ljava/lang/String;)V");
    if (mid == 0) {
	fprintf (stderr, "%s[%d] couldn't look up java method\n", __FILE__,__LINE__);
	return ;
    }

    i = 1+strlen(msg);
    dupmsg = (char*)malloc(i);
    strcpy (dupmsg, msg);
    for (j = i-1; j>=0; j--) {
	if ((dupmsg[j] == ' ') || (dupmsg[j] == '\t') || (dupmsg[j] == '\n'))
	    dupmsg[j] = '\0';
	else
	    break;
    }
    jstr = (*env)->NewStringUTF(env, dupmsg);
    jv[0].l = jstr;
    (*env)->CallVoidMethodA (env, obj, mid, jv);
    free(dupmsg);
}


JNIEXPORT jlong JNICALL Java_server_DXServer_DXLStartDX
    (JNIEnv *env, jclass u1, jstring jcmdstr, jstring jhost)
{
char cmdstr[256];
DXLConnection* dxl;
	jclass newExcCls;
        const char *host;
        const char *cmd;
        const char *dxargs;
	
	(*env)->ExceptionDescribe(env);
	(*env)->ExceptionClear(env);
	newExcCls = (*env)->FindClass(env, "java/lang/IllegalArgumentException");

    host = (*env)->GetStringUTFChars(env, jhost,0);
    cmd = (*env)->GetStringUTFChars(env, jcmdstr,0);
    dxargs = (const char*)getenv("DXARGS");

    if ((dxargs) && (dxargs[0]))
	sprintf (cmdstr, "dx %s", dxargs);
    else
	strcpy (cmdstr, cmd);
    dxl = DXLStartDX(cmdstr, host);
        
    /* if (dxl) DXLSetMessageDebugging(dxl, 1); */

    (*env)->ReleaseStringUTFChars(env, jcmdstr, cmd);
    (*env)->ReleaseStringUTFChars(env, jhost, host);

    /* Should now look at dxl and the errno to see what happened. */
    /* If it returns null, then throw a java exception. */
    if(dxl == NULL) {
    	if(newExcCls == NULL)
    		return 0;
    	else
    		(*env)->ThrowNew(env, newExcCls, "err starting dx server--check dxserver.hosts file.");
    }


    return (long)dxl;
}

JNIEXPORT jint JNICALL Java_server_DXServerThread_DXLExecuteOnce
    (JNIEnv *env, jobject jobj, jlong jdxl, jstring jmacro)
{
int retval = 1;
DXLConnection* conn = (DXLConnection*)jdxl;
const char *macro = (*env)->GetStringUTFChars(env, jmacro,0);
char tmpbuf[128];
int size = sizeof(struct _JavaEnv);

    if (DXLGetSocket(conn) > 0) {

	JavaEnvironment je = (JavaEnvironment)malloc(size);
	je->java_env = (void*)env;
	je->java_obj = (void*)jobj;
	DXLSetMessageHandler (conn, PACK_INFO, NULL, 
	    (DXLMessageHandler)DXLink_InfoMsgCB, (const void*)je);
	DXLSetMessageHandler (conn, PACK_LINK, NULL, 
	    (DXLMessageHandler)DXLink_InfoMsgCB, (const void*)je);
	DXLSetMessageHandler (conn, PACK_ERROR, NULL,
	    (DXLMessageHandler)DXLink_InfoMsgCB, (const void*)je);

	strcpy (tmpbuf, macro);
	if (exDXLExecuteOnceNamed(conn, tmpbuf) == ERROR) {
	    retval = 0;
	} else {
	    /*const char* method_name = "hasEndMsg";*/
	    jclass cls = (*env)->GetObjectClass(env, jobj);
	    /* use the javap command to generate the signature. */
	    jmethodID mid = (*env)->GetMethodID(env, cls, "hasEndMsg", "()I");
	    int state = 1;
	    if (mid == 0) 
		fprintf (stderr, 
		    "%s[%d] couldn't look up java method\n", __FILE__,__LINE__);

	    while (state > 0) {
		/*
		 * Check for an end-execution message
		 */
		if (mid != 0) {
		    jint tmp = 0;
		    tmp = (*env)->CallIntMethod (env, jobj, mid);
		    if (tmp == 1) {
			DXLEndExecution (conn);
			break;
		    }
		}

		if (DXLWaitForEvent (conn) == ERROR) {
		    retval = 0;
		    break;
		}
		if (DXLHandlePendingMessages (conn) == ERROR) {
		    retval = 0;
		    break;
		}
		if (DXLGetExecutionStatus(conn, &state) == ERROR) {
		    retval = 0;
		    break;
		}
	    }
	}

	DXLRemoveMessageHandler (conn, PACK_INFO, NULL, 
	    (DXLMessageHandler)DXLink_InfoMsgCB);
	DXLRemoveMessageHandler (conn, PACK_LINK, NULL, 
	    (DXLMessageHandler)DXLink_InfoMsgCB);
	DXLRemoveMessageHandler (conn, PACK_ERROR, NULL,
	    (DXLMessageHandler)DXLink_InfoMsgCB);
    } else {
	retval = 0;
    }
    (*env)->ReleaseStringUTFChars(env, jmacro, macro);
    return retval;
}



JNIEXPORT jint JNICALL Java_server_DXServer_DXLSend
    (JNIEnv *env, jclass u1, jlong jdxl, jstring jval)
{
    int retval = 1;
    DXLConnection* conn = (DXLConnection*)jdxl;
    if (DXLGetSocket(conn) > 0) {
	const char* value = (*env)->GetStringUTFChars(env, jval, 0);
	if (DXLSend (conn, value) == ERROR) {
	    retval = 0;
	} 
	(*env)->ReleaseStringUTFChars(env, jval, value);
    } else {
	retval = 0;
    }

    return retval;
}


JNIEXPORT jint JNICALL Java_server_DXServerThread_DXLLoadVisualProgram
  (JNIEnv *env, jobject jobj, jlong jdxl, jstring jnet)
{
int retval = 0;

#if defined(intelnt) || defined(WIN32)
int i;
#endif

DXLConnection* conn = (DXLConnection*)jdxl;

    if (DXLGetSocket(conn) > 0) {
	char *str;
	const char* net_file = (*env)->GetStringUTFChars(env, jnet, 0); 
	str = (char *) malloc (sizeof(char)* (strlen(net_file) + 1));
	strcpy (str, net_file);

#if defined(intelnt) || defined(WIN32)
	for(i=0; i<strlen(str); i++)
		if(str[i] == '\\') str[i] = '/';
#endif

	retval = DXLLoadVisualProgram (conn, str) != ERROR; 
	(*env)->ReleaseStringUTFChars(env, jnet, net_file);
	free(str);
    }

    return retval;
}

JNIEXPORT jint JNICALL Java_server_DXServer_DXLExitDX
  (JNIEnv* u1, jclass u2, jlong jdxl)
{
    int retval = 1;

    DXLConnection* conn = (DXLConnection*)jdxl;
    if (DXLGetSocket(conn) > 0) {
	DXLSend(conn, "Executive(\"quit\");");
	if (DXLExitDX(conn) == ERROR) {
	    retval = 0;
	}
    } else {
	DXLCloseConnection (conn);
	retval = 0;
    }
#if defined(ibm6000) || defined(sgi)
#if defined(USE_WAIT3)
    wait3 (NULL, WNOHANG, NULL);
#endif

#if !defined(USE_WAIT3) && !defined(DXD_OS_NON_UNIX)
    {
    siginfo_t winfo;
    waitid (P_ALL, 0, &winfo, WNOHANG|WEXITED);
    }
#endif
#endif
    return retval;
}

JNIEXPORT jint JNICALL Java_server_DXServerThread_DXLIsMessagePending
  (JNIEnv *u1, jobject u2, jlong jdxl)
{
    int retval = 1;
    DXLConnection* conn = (DXLConnection*)jdxl;
    if (DXLGetSocket(conn) > 0) {
	retval = DXLIsMessagePending(conn);
    } else {
	retval = 0;
    }

    return retval;
}

JNIEXPORT jint JNICALL Java_server_DXServerThread_DXLHandlePendingMessages
    (JNIEnv *env, jobject jobj, jlong jdxl)
{
    int retval = 1;
    DXLConnection* conn = (DXLConnection*)jdxl;
    if (DXLGetSocket(conn) > 0) {
	if (DXLHandlePendingMessages (conn) == ERROR) {
	    retval = 0;
	}
    } else {
	retval = 0;
    }
    return retval;
}

