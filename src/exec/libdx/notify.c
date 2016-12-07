/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


#include <stdio.h>

#if defined(HAVE_STRING_H)
#include <string.h>
#endif

#include <dx/dx.h>

#define NAME	in[0]
#define DEFAULT	in[1]
#define RESULT	out[0]

#define NOTIFY_READYTORUN		1
#define NOTIFY_READYTORUN_NOEXECUTE	2

struct _string
{
    struct _string *next;
    struct _string *link;
    int executeFlag;
    char   string[1];
};
    
struct  _table
{
    struct _string *strings;
    HashTable hash;
    Private owner;
};

typedef struct _entry
{
    char *name;
    struct _string *modids;
} *Entry;
    
static struct _string *
newString(struct _table *table, char *str)
{
    struct _string *_str;

    if (! table)
    {
	DXSetError(ERROR_INTERNAL, "newString: no table");
	return NULL;
    }

    _str = (struct _string *)DXAllocate(strlen(str) + sizeof(struct _string) + 3);
    if (! str)
	return ERROR;
    
    strcpy(_str->string, str);
    _str->next = table->strings;
    table->strings = _str;

    _str->link = NULL;

    return _str;
}

static Error
delete_table(Pointer p)
{
    struct _string *s, *n;
    struct _table *table = (struct _table *)p;
    HashTable hash = table->hash;
    DXDestroyHash(hash);
    for (s = table->strings; s; s = n)
    {
	n = s->next;
	DXFree((Pointer)s);
    }
    return OK;
}

static PseudoKey
hash(Entry key)
{
    char *name = ((struct _entry *) key)->name;
    long l, i, s=0;
    for (l = i = 0; i < strlen(name); i++, l ^= name[i], s+= name[i-1]);
    return l*s;
}

static int
cmp(Entry k0, Entry k1)
{
    char *n0 = ((struct _entry *) k0)->name;
    char *n1 = ((struct _entry *) k1)->name;

    return strcmp(n0, n1);
}

static void
freeTable(struct _table *tab)
{
    if (tab)
	DXDelete((Object)tab->owner);
}

static struct _table *
getTable()
{
    struct _table *table;
    Private pht = (Private)DXGetCacheEntry("__HASH_DXLInputNAMED", 0, 0);
    if (! pht)
    {
	table = (struct _table *)DXAllocate(sizeof(struct _table));
	if (! table)
	    goto error;

	table->hash = DXCreateHash(sizeof(struct _entry), hash, cmp);
	if (! table->hash)
	{
	    DXFree((Pointer)table);
	    goto error;
	}

	table->strings = NULL;
	table->owner   = DXNewPrivate((Pointer)table, delete_table);
	if (! table->owner)
	{
	    DXDestroyHash(table->hash);
	    DXFree((Pointer)table);
	    goto error;
	}

	if (! DXSetCacheEntry((Object)table->owner, CACHE_PERMANENT, "__HASH_DXLInputNAMED", 0, 0))
	{
	    DXDelete((Object)table->owner);
	    goto error;
	}

	pht = (Private)DXReference((Object)table->owner);
    }

    return (struct _table *)DXGetPrivateData(pht);

error:
    return NULL;
}


static struct _entry *
getEntry(struct _table *table, char *name)
{
    struct _entry key, *entry;

    key.name = name;
    entry = (struct _entry *)DXQueryHashElement(table->hash, (Key)&key);
    if (! entry)
    {
	struct _string *str = newString(table, name);
	if (! str)
	    return NULL;
	
	key.name = str->string;
	key.modids = NULL;

	if (! DXInsertHashElement(table->hash, (void *)&key))
	    return NULL;

	entry = (struct _entry *)DXQueryHashElement(table->hash, (Key)&key);
    }

    return entry;
}

static int
isModidRegistered(struct _entry *entry, Pointer modid)
{
    struct _string *str;

    for (str = entry->modids; str; str = str->link)
	if (! strcmp((char *)modid, str->string))
	    return 1;
    
    return 0;
}

static Error
registerModid(struct _table *table, struct _entry *entry, Pointer modid, int flag)
{
    struct _string *str = newString(table, (char *)modid);
    if (! str)
	return ERROR;
    
    str->link = entry->modids;
    entry->modids = str;
    str->executeFlag = flag;

    return OK;
}
  
Error
DXRegisterForNotification(char *name, Pointer modid)
{
    struct _entry *entry;
    struct _table *table = getTable();
    if (! table)
	return ERROR;

    entry = getEntry(table, name);
    if (! entry)
	goto error;

    if (! isModidRegistered(entry, modid))
	if (! registerModid(table, entry, modid, NOTIFY_READYTORUN))
	    goto error;
    
    freeTable(table);
    return OK;

error:
    freeTable(table);
    return ERROR;
}

Error
DXRegisterForNotificationNoExecute(char *name, Pointer modid)
{
    struct _entry *entry;
    struct _table *table = getTable();
    if (! table)
	return ERROR;

    entry = getEntry(table, name);
    if (! entry)
	goto error;

    if (! isModidRegistered(entry, modid))
	if (! registerModid(table, entry, modid, NOTIFY_READYTORUN_NOEXECUTE))
	    goto error;
    
    freeTable(table);
    return OK;

error:
    freeTable(table);
    return ERROR;
}

Error
DXNotifyRegistered(char *name)
{
    struct _entry *entry;
    struct _string *str;
    struct _table *table = getTable();
    if (! table)
	return ERROR;

    entry = getEntry(table, name);
    if (! entry)
	goto error;
    
    for (str = entry->modids; str; str = str->link)
	if (str->executeFlag == NOTIFY_READYTORUN)
	    DXReadyToRun((Pointer)str->string);
	else
	    DXReadyToRunNoExecute((Pointer)str->string);
    
    freeTable(table);
    return OK;

error:
    freeTable(table);
    return ERROR;
}

Error
DXNotifyRegisteredNoExecute(char *name)
{
    struct _entry *entry;
    struct _string *str;
    struct _table *table = getTable();
    if (! table)
	return ERROR;

    entry = getEntry(table, name);
    if (! entry)
	goto error;
    
    for (str = entry->modids; str; str = str->link)
	DXReadyToRunNoExecute((Pointer)str->string);
    
    freeTable(table);
    return OK;

error:
    freeTable(table);
    return ERROR;
}
