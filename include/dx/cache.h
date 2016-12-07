/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#ifndef _DXI_CACHE_H_
#define _DXI_CACHE_H_

/* TeX starts here. Do not remove this comment. */

/*
\section{Cache}

The lookaside cache service provides various parts of the system with
a means to store the results of computations for later re-use.  Each
cache entry is uniquely identified by a string function name, an
integer key (used for example by the executive to store multiple
outputs for a single module), the number of input parameters, and the
set of input parameter values.

The input parameters and the output value must be objects.  The cache
entry constitutes a reference to its output parameter for as long as
the entry remains in the cache.  If you need to associate user data
that is not in the form of an object with a cache entry, you can use a
private object (described in section \ref{privatesec}) to encapsulate
your data in an object.  If you wish to associate more than one object
with a cache entry, you can use a group to contain the objects.

Cache entries are subject to deletion without notice, for example to
reclaim memory.  The relative cost of creating the cache entry, as
specified when creating the cache entry, may be taken into account
when deleting objects from the cache.  If you do not have an estimate
readily available, simply specify 0.  Specifying {\tt
CACHE\_PERMANENT} as the cost prevents the cache entry from being
deleted during storage reclamation.

There are some important subtleties regarding object reference counts
to be aware of when using the cache.  For module inputs, once you have
a pointer to the input, you can be sure it will not be deleted while
you are processing it; and if it is a group, if you extract a member,
the member will not be deleted while you are using it because the
group will not be deleted.  On the other hand, objects put into or
retrieved from the cache behaves differently: once something has been
put into the cache, it is subject to deletion at any time to reclaim
memory.

This fact has two consequences.  First, {\tt DXGetCacheEntry()} returns
an object that has been referenced so that it will not be deleted
until you are finished with it.  Thus, when you are finished using the
object, you must delete it:
\begin{program}
    o = DXGetCacheEntry(...);
    ... use o ...
    DXDelete(o);
\end{program}
If you don't delete it, the result will be a memory leak because the
object will always have an extra reference.  Second, if you put
something in the cache and plan to continue using it, you must
reference it before putting it in the cache, and then delete it when
you are done using it:
\begin{program}
    o = New...;
    DXReference(o);
    DXSetCacheEntry(..., o, ...);
    ... use o ...
    DXDelete(o);
\end{program}
On the other hand, if putting the object in the cache is the last
thing you do before returning, and if {\tt o} is not visible outside
the scope of your routine, it is not necessary to reference it:
\begin{program}
    o = New...;
    ... use o ...
    DXSetCacheEntry(..., o, ...);
    return ...;
\end{program}
where of course the return statement does not return {\tt o}.
*/

#define CACHE_PERMANENT 1e32

Error DXSetCacheEntry(Object out, double cost,
		    char *function, int key, int n, ...);
Error DXSetCacheEntryV(Object out, double cost,
		     char *function, int key, int n, Object *in);
/**
\index{DXSetCacheEntry}\index{DXSetCacheEntryV}
Sets the cache entry for the given {\tt function}, key {\tt key},
number of input parameters {\tt n}, and input objects specified by the
array {\tt in} (or by the last {\tt n} arguments in the case of {\tt
DXSetCacheEntry()}.  Stores with the cache entry the output object {\tt
out}.  The cache references the output object as long as it remains in
the cache.  Setting the value of a cache entry to null deletes any
cached value that may be already stored for this entry.  Returns {\tt
OK} to indicate success, or returns null and sets the error code to
indicate an error.
**/

Object DXGetCacheEntry(char *function, int key, int n, ...);
Object DXGetCacheEntryV(char *function, int key, int n, Object *in);
/**
\index{DXGetCacheEntry}\index{DXGetCacheEntryV}
Retrieves the cache entry for the given {\tt function}, output number
{\tt key}, number of input parameters {\tt n}, and input objects
specified by the array {\tt in} (or by the last {\tt n} arguments in
the case of {\tt DXSetCacheEntry()}.  The reference count of the
returned object has been incremented, so it is your responsibility to
delete the object when you are finished with it.  Returns the cached
output object, or returns null but does not set the error code if no
such cache entry exists.
**/

#endif /* _DXI_CACHE_H_ */

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
