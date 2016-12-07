/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>

#include <string.h>
#include <stdio.h>
#include <dx/dx.h>

#undef STATS

typedef struct hashElement *HashElement;
typedef struct leaf *Leaf;
typedef struct directory Directory;
typedef struct pageTable PageTable;
typedef struct leafBlock LeafBlock;
typedef struct leafCache LeafCache;

#define MAX_D  24
#define LEAF_LEN 8
#define LEAF_WRAP 0x07
#define LEAF_SHIFT 3

#define MAGIC_PSEUDOKEY 0xffffffff

/*
 * Want to make sure pages come from large arena.  Each element must
 * contain the pseudokey, so is no smaller than 4 bytes. 
 */
#define ELTS_PER_PAGE ((1024/sizeof(PseudoKey)) + 1)

/*
 * Want to make sure the leaf blocks are allocated from the large arena.
 */
#define LEAVES_PER_BLOCK ((1024/sizeof(struct leaf)) + 1)

struct hashElement
{
    union
    {
        HashElement next;
        PseudoKey pseudoKey;
    } u;
};

struct leaf
{
    int depth;
    int reference;
    HashElement elements[ LEAF_LEN ];
};

struct leafBlock
{
    LeafBlock *next;
    struct leaf leaves[ LEAVES_PER_BLOCK ];
};

struct leafCache
{
    LeafBlock *blocks;
    Leaf freeList;
    int nextInBlock;
#ifdef STATS
    int currLeaves, maxLeaves;
#endif
};

struct directory
{
    int depth;
    int mask;
    Leaf *leaves;
    LeafCache leafCache;
};

struct pageTable
{
    int nPages;  /* Number of pages allocated  */
    int nPageSlots; /* Current length of pageTable   */
    int nextEltNum; /* index of next available element */
    int eltsPerPage; /* number of elements per page  */
    int pageSize; /* bytes per page   */
    int eltSize; /* bytes per element   */
    HashElement nextElt; /* pointer to next available element */
    HashElement *pagePtrs; /* page pointers   */
    HashElement freeList; /* free list     */
#ifdef STATS
    int currElts, maxElts;
#endif
};

struct hashTable
{
    Directory directory;
    PageTable pageTable;

    int dataSize;

    long directoryMask;
    int directoryLength;

    int getNextPage;
    int getNextElement;

    PseudoKey ( *hash ) ();
    int ( *cmp ) ();

#ifdef STATS
    int nInserts, nCollisions;
    int nSearches, nSuccesses, nMismatches;
#endif

};

#define LEAF_INDEX(pseudoKey)    ((pseudoKey) & LEAF_WRAP)
#define LEAF_INCREMENT(leafIndex) ((leafIndex+1) & LEAF_WRAP)

#define DATA_PTR(elt) (Element)(((char *)elt)+sizeof(struct hashElement))

static int maskBits[] =
    {
        0x00000000,
        0x00000001, 0x00000003, 0x00000007, 0x0000000F,
        0x0000001F, 0x0000003F, 0x0000007F, 0x000000FF,
        0x000001FF, 0x000003FF, 0x000007FF, 0x00000FFF,
        0x00001FFF, 0x00003FFF, 0x00007FFF, 0x0000FFFF,
        0x0001FFFF, 0x0003FFFF, 0x0007FFFF, 0x000FFFFF,
        0x001FFFFF, 0x003FFFFF, 0x007FFFFF, 0x00FFFFFF,
        0x01FFFFFF, 0x03FFFFFF, 0x07FFFFFF, 0x0FFFFFFF
    };

static int powers_of_2[] =
    {
        0x00000001, 0x00000002, 0x00000004, 0x00000008,
        0x00000010, 0x00000020, 0x00000040, 0x00000080,
        0x00000100, 0x00000200, 0x00000400, 0x00000800,
        0x00001000, 0x00002000, 0x00004000, 0x00008000,
        0x00010000, 0x00020000, 0x00040000, 0x00080000,
        0x00100000, 0x00200000, 0x00400000, 0x00800000,
        0x01000000, 0x02000000, 0x04000000, 0x08000000
    };

static Error _initPageTable( PageTable *, int );
static void _deletePageTable( PageTable * );
static HashElement _getFreeElement( PageTable * );
static Error _initDirectory( Directory * );
static void _deleteDirectory( Directory * );
static Error _InsertHash( HashTable, void *, PseudoKey );
static int _InsertHashLeaf( HashTable, PseudoKey, Leaf, void * );
static Error _expandHash( HashTable );
static Error _splitLeaf( HashTable, int, Leaf );
static HashElement _QueryHashElement( HashTable, Key );
static void _initLeafCache( LeafCache * );
static void _freeLeaf( LeafCache *, Leaf );
static Leaf _getFreeLeaf( LeafCache * );
static void _deleteLeafCache( LeafCache * );

#if 0
static Error _hashCheck( HashTable );
static void _freeElement( PageTable *, HashElement );
#endif


HashTable
DXCreateHash( int dataSize, PseudoKey ( *hash ) (), int ( *cmp ) () )
{
    HashTable hashTable;

    hashTable = ( HashTable ) DXAllocate( sizeof( struct hashTable ) );

    if ( ! hashTable )
        return NULL;

    /*
     * Round to word boundary
     */
    if ( dataSize & 0x3 )
        hashTable->dataSize = ( dataSize & 0xffffffc ) + 0x0000004;
    else
        hashTable->dataSize = dataSize;

    hashTable->hash = hash;
    hashTable->cmp = cmp;

    if ( ! _initDirectory( &hashTable->directory ) ) {
        DXFree( ( Pointer ) hashTable );
        return NULL;
    }

    hashTable->directoryMask = maskBits[ hashTable->directory.depth ];
    hashTable->directoryLength = powers_of_2[ hashTable->directory.depth ];

    if ( ! _initPageTable( &hashTable->pageTable, dataSize ) ) {
        _deleteDirectory( &hashTable->directory );
        DXFree( ( Pointer ) hashTable );
        return NULL;
    }

#ifdef STATS
    hashTable->nInserts = 0;
    hashTable->nCollisions = 0;
    hashTable->nSearches = 0;
    hashTable->nSuccesses = 0;
    hashTable->nMismatches = 0;
#endif

    return hashTable;
}


Error
DXDestroyHash( HashTable hashTable )
{
    if ( ! hashTable )
        return OK;

    _deleteDirectory( &hashTable->directory );
    _deletePageTable( &hashTable->pageTable );

#ifdef STATS
    DXMessage( "%d insertions, %d collisions",
               hashTable->nInserts, hashTable->nCollisions );
    DXMessage( "%d searches, %d successes, %d mismatches",
               hashTable->nSearches, hashTable->nSuccesses, hashTable->nMismatches );
#endif

    DXFree( ( Pointer ) hashTable );
    return OK;
}


Element
DXQueryHashElement( HashTable hashTable, Key key )
{
    HashElement elt;

#ifdef STATS
    hashTable->nSearches ++;
#endif

    if ( NULL == ( elt = _QueryHashElement( hashTable, key ) ) )
        return NULL;
    else {
#ifdef STATS
        hashTable->nSuccesses ++;
#endif
        return DATA_PTR( elt );
    }
}

Error
DXDeleteHashElement( HashTable hashTable, Key key )
{
    HashElement elt;

    if ( NULL != ( elt = _QueryHashElement( hashTable, key ) ) )
        elt->u.pseudoKey = 0;

    return OK;
}


Error
DXInsertHashElement( HashTable hashTable, void *data )
{
    PseudoKey pseudoKey;
    int status;
    PseudoKey ( *hash ) ();

#ifdef STATS
    hashTable->nInserts ++;
#endif

    if ( NULL == ( hash = hashTable->hash ) )
        pseudoKey = *( PseudoKey * ) data;
    else
        pseudoKey = ( *hash ) ( ( Key ) data );

    /*
     * pseudoKey 0 is illegal.  It implies that an entry is empty.
     * So, we increment pseudokeys by 1, unless its -1.
     */
    if ( pseudoKey == ( PseudoKey ) - 1 ) {
        DXSetError( ERROR_INTERNAL, "invalid hash key (0xffffffff)" );
        return ERROR;
    }

    pseudoKey += 1;
    status = _InsertHash( hashTable, data, pseudoKey );
    return status;
}

Error
DXInitGetNextHashElement( HashTable hashtab )
{
    if ( hashtab == NULL )
        return ERROR;

    hashtab->getNextPage = 0;
    hashtab->getNextElement = 0;

    return OK;
}


Element
DXGetNextHashElement( HashTable hashtab )
{
    PageTable * pagetab;
    HashElement element, nextelt;
    HashElement *pages;
    int pageNum, eltNum;
    int eltSize, eltsInPage;
    int nPages;

    if ( ! hashtab )
        return ERROR;

    pagetab = &hashtab->pageTable;

    if ( ( pageNum = hashtab->getNextPage ) >= ( nPages = pagetab->nPages ) )
        return NULL;

    eltNum = hashtab->getNextElement;
    pages = pagetab->pagePtrs;
    eltSize = pagetab->eltSize;

    if ( pageNum == nPages - 1 )
        eltsInPage = pagetab->nextEltNum;
    else
        eltsInPage = ELTS_PER_PAGE;

    nextelt = ( HashElement ) ( ( ( char * ) pages[ pageNum ] ) + eltNum * eltSize );

    while ( pageNum < nPages ) {
        element = nextelt;
        eltNum ++;

        if ( eltNum >= eltsInPage ) {
            eltNum = 0;

            if ( ++pageNum < nPages ) {
                if ( pageNum == nPages - 1 )
                    eltsInPage = pagetab->nextEltNum;
                else
                    eltsInPage = ELTS_PER_PAGE;

                nextelt = pages[ pageNum ];
            }
        } else
            nextelt = ( HashElement ) ( ( char * ) element + eltSize );

        if ( element->u.pseudoKey ) {
            hashtab->getNextPage = pageNum;
            hashtab->getNextElement = eltNum;
            return DATA_PTR( element );
        }
    }
    return NULL;
}


static HashElement
_QueryHashElement( HashTable hashTable, Key key )
{
    PseudoKey pseudoKey;
    int bucketNum;
    Leaf leaf;
    int slot, start, found;
    HashElement element;
    PseudoKey ( *hash ) ();
    int ( *cmp ) ();

    cmp = hashTable->cmp;
    hash = hashTable->hash;

    if ( hash )
        pseudoKey = ( *hash ) ( key );
    else
        pseudoKey = *( PseudoKey * ) key;

    pseudoKey += 1;

    bucketNum = ( pseudoKey >> LEAF_SHIFT ) & hashTable->directoryMask;
    leaf = hashTable->directory.leaves[ bucketNum ];
    start = LEAF_INDEX( pseudoKey );
    found = 0; slot = start;

    do {

        if ( ( element = leaf->elements[ slot ] ) == NULL )
            break;

        if ( ( element->u.pseudoKey == pseudoKey )
                && ( !cmp || !( *cmp ) ( key, DATA_PTR( element ) ) ) ) {
            found = 1;
            break;
        }
        
#ifdef STATS
        hashTable->nMismatches ++;
#endif
        slot = LEAF_INCREMENT( slot );
    }

    while ( slot != start )
        ;

    if ( ! found )
        return NULL;
    else
        return element;
}


static Error
_initPageTable( PageTable *pagetab, int dataSize )
{
    int eltSize;

    eltSize = sizeof( struct hashElement ) + dataSize;
    eltSize = ( eltSize + ( sizeof( long ) - 1 ) ) & ~( sizeof( long ) - 1 );

    pagetab->nPages = 0;
    pagetab->nPageSlots = 0;
    pagetab->eltsPerPage = ELTS_PER_PAGE;
    pagetab->nextEltNum = pagetab->eltsPerPage; /* force alloc of first page */
    pagetab->pageSize = pagetab->eltsPerPage * eltSize;
    pagetab->eltSize = eltSize;
    pagetab->pagePtrs = NULL;
    pagetab->freeList = NULL;

#ifdef STATS
    pagetab->currElts = 0;
    pagetab->maxElts = 0;
#endif

    return OK;
}


static HashElement
_getFreeElement( PageTable *pagetab )
{
    HashElement element;

    if ( pagetab->freeList ) {
        element = pagetab->freeList;
        pagetab->freeList = pagetab->freeList->u.next;
    } else {
        if ( pagetab->nextEltNum == pagetab->eltsPerPage ) {
            /*
             * If no page table slot is available, must make room for more.
             */

            if ( pagetab->nPages == pagetab->nPageSlots || !pagetab->pagePtrs ) {
                /*
                 * If this is the first, allocate pagePtrs table.  Otherwise,
                 * increase the size
                 */

                if ( ! pagetab->pagePtrs ) {
                    pagetab->nPageSlots = 1;
                    pagetab->pagePtrs = ( HashElement * ) DXAllocate
                                        ( pagetab->nPageSlots * sizeof( HashElement ) );
                } else {
                    pagetab->nPageSlots *= 2;
                    pagetab->pagePtrs = ( HashElement * ) DXReAllocate(
                                            ( Pointer ) pagetab->pagePtrs,
                                            pagetab->nPageSlots * sizeof( HashElement ) );
                }

                if ( pagetab->pagePtrs == NULL )
                    return NULL;
            }

            pagetab->pagePtrs[ pagetab->nPages ] =
                ( HashElement ) DXAllocate( pagetab->pageSize );

            if ( ! pagetab->pagePtrs[ pagetab->nPages ] )
                return NULL;

            pagetab->nextElt = pagetab->pagePtrs[ pagetab->nPages ];
            pagetab->nextEltNum = 0;
            pagetab->nPages++;

        }

        element = pagetab->nextElt;

        pagetab->nextEltNum ++;
        pagetab->nextElt = ( HashElement ) ( ( char * ) pagetab->nextElt
                                             + pagetab->eltSize );
    }

    element->u.pseudoKey = 0;

#ifdef STATS

    if ( ( ++( pagetab->currElts ) ) > pagetab->maxElts )
        pagetab->maxElts = pagetab->currElts;

#endif

    return element;
}


static void
_deletePageTable( PageTable *pagetab )
{
    int i;

    if ( pagetab->pagePtrs ) {
        for ( i = 0; i < pagetab->nPages; i++ )
            if ( pagetab->pagePtrs[ i ] ) {
                DXFree( ( Pointer ) pagetab->pagePtrs[ i ] );
                pagetab->pagePtrs[ i ] = NULL;
            }

        DXFree( ( Pointer ) pagetab->pagePtrs );
        pagetab->pagePtrs = NULL;
    }

#ifdef STATS
    DXMessage( "elements: %d current, %d max",
               pagetab->currElts, pagetab->maxElts );

#endif

}

static Error
_initDirectory( Directory *directory )
{
    int nBuckets;

    _initLeafCache( &( directory->leafCache ) );

    nBuckets = 1;

    directory->depth = 0;
    directory->mask = maskBits[ 0 ];
    directory->leaves = ( Leaf * ) DXAllocate( nBuckets * sizeof( Leaf ) );

    if ( ! directory->leaves )
        return ERROR;

    directory->leaves[ 0 ] = _getFreeLeaf( &( directory->leafCache ) );
    directory->leaves[ 0 ] ->depth = 0;
    directory->leaves[ 0 ] ->reference = 1;

    return OK;
}

static void
_deleteDirectory( Directory *directory )
{
    _deleteLeafCache( &( directory->leafCache ) );

    if ( directory->leaves )
        DXFree( ( Pointer ) directory->leaves );

#ifdef STATS
    DXMessage( "Directory length: %d", powers_of_2[ directory->depth ] );

#endif
}


static Error
_InsertHash( HashTable hashTable, void *data, PseudoKey pseudoKey )
{
    int bucketNum;
    Leaf leaf;
    int found;

    bucketNum = ( pseudoKey >> LEAF_SHIFT ) & hashTable->directoryMask;
    leaf = hashTable->directory.leaves[ bucketNum ];

    found = _InsertHashLeaf( hashTable, pseudoKey, leaf, data );

    if ( found == 1 ) {
        return OK;
    } else if ( found == 0 ) {
        /*
         * If leaf isn't shared, expand the directory.
         */

        if ( leaf->depth == hashTable->directory.depth )
            if ( ! _expandHash( hashTable ) )
                return ERROR;

        /*
         * Now expand the leaf
         */
        if ( ! _splitLeaf( hashTable, bucketNum, leaf ) )
            return ERROR;

        /*
         * Now try insertion again recursively
         */ 
        return _InsertHash( hashTable, data, pseudoKey );
    } else {
        return ERROR;
    }
}


static Error
_expandHash( HashTable hashTable )
{
    int oldNBuckets;
    int oldLength;
    int i;
    Directory *dir;

    dir = &hashTable->directory;

    oldNBuckets = powers_of_2[ dir->depth ];
    oldLength = oldNBuckets * sizeof( Leaf * );

    for ( i = 0; i < oldNBuckets; i++ )
        dir->leaves[ i ] ->reference ++;

    dir->leaves = ( Leaf * ) DXReAllocate( ( Pointer ) dir->leaves, 2 * oldLength );

    if ( ! dir->leaves )
        return ERROR;

    memcpy( ( char * ) dir->leaves + oldLength, ( char * ) dir->leaves, oldLength );

    dir->depth ++;

    hashTable->directoryMask = maskBits[ dir->depth ];
    hashTable->directoryLength = powers_of_2[ dir->depth ];

    return OK;
}


static int
_InsertHashLeaf( HashTable hashTable, PseudoKey pseudoKey,
                 Leaf leaf, void *data )
{
    int slot, start;
    int found;
    HashElement element;
    int ( *cmp ) ();

    cmp = hashTable->cmp;
    start = LEAF_INDEX( pseudoKey );

    /*
     * Look for either an empty slot in the leaf or one that
     * matches the key.
     */
    found = 0; slot = start;

    do {

        element = leaf->elements[ slot ];

        if ( element == NULL || ! element->u.pseudoKey ) {
            found = 1;
            break;
        }

        if ( ( element->u.pseudoKey == pseudoKey )
                && ( !cmp || !( *cmp ) ( ( Key ) data, DATA_PTR( element ) ) ) ) {
            found = 1;
            break;
        }

#ifdef STATS
        hashTable->nCollisions ++;
#endif
        slot = LEAF_INCREMENT( slot );
    }

    while ( slot != start )
        ;

    if ( found ) {
        if ( ! element ) {
            element = _getFreeElement( &hashTable->pageTable );

            if ( ! element )
                return -1;
        }

        if ( ! element->u.pseudoKey )
            element->u.pseudoKey = pseudoKey;

        if ( hashTable->dataSize )
            memcpy( DATA_PTR( element ), data, hashTable->dataSize );

        leaf->elements[ slot ] = element;
    }

    return found;
}


static Error
_splitLeaf( HashTable hashTable, int bucketNum, Leaf leaf )
{
    Leaf leaf0, leaf1;
    int mask;
    int i, count;
    HashElement elt, elt0;
    Leaf which;
    int slot, start, bit;
    int knt0, knt1;

    elt0 = leaf->elements[ 0 ];

    for ( i = 1; i < LEAF_LEN; i++ ) {
        elt = leaf->elements[ i ];

        if ( elt->u.pseudoKey != elt0->u.pseudoKey )
            break;
    }

    if ( i == LEAF_LEN ) {
        DXSetError( ERROR_INTERNAL, "excessive hash key collisions" );
        return ERROR;
    }

    leaf0 = _getFreeLeaf( &( hashTable->directory.leafCache ) );

    if ( ! leaf0 )
        return ERROR;

    leaf1 = _getFreeLeaf( &( hashTable->directory.leafCache ) );

    if ( ! leaf1 )
        return ERROR;

    leaf0->depth = leaf1->depth = leaf->depth + 1;
    mask = bucketNum & maskBits[ leaf->depth ];
    count = powers_of_2[ hashTable->directory.depth - leaf->depth ];

    for ( i = 0; i < count; i++ ) {
        bucketNum = mask | i << leaf->depth;

        if ( i & 0x01 ) {
            hashTable->directory.leaves[ bucketNum ] = leaf1;
            leaf1->reference ++;
        } else {
            hashTable->directory.leaves[ bucketNum ] = leaf0;
            leaf0->reference ++;
        }
    }

    bit = 1 << ( leaf->depth + LEAF_SHIFT );

    knt0 = knt1 = 0;

    for ( i = 0; i < LEAF_LEN; i++ ) {
        elt = leaf->elements[ i ];

        start = LEAF_INDEX( elt->u.pseudoKey );

        if ( elt->u.pseudoKey & bit ) {
            which = leaf1;
            knt1++;
        } else {
            which = leaf0;
            knt0++;
        }

        for ( slot = start; ; slot = LEAF_INCREMENT( slot ) )
            if ( ! which->elements[ slot ] ) {
                which->elements[ slot ] = elt;
                break;
            }
    }

    _freeLeaf( &( hashTable->directory.leafCache ), leaf );

    return OK;
}


static void
_initLeafCache( LeafCache *lc )
{
    lc->blocks = NULL;
    lc->nextInBlock = -1;
    lc->freeList = NULL;

#ifdef STATS
    lc->currLeaves = 0;
    lc->maxLeaves = 0;
#endif
}


static void
_freeLeaf( LeafCache *lc, Leaf leaf )
{
    *( ( Leaf * ) leaf ) = lc->freeList;
    lc->freeList = leaf;

#ifdef STATS
    lc->currLeaves --;
#endif
}

static Leaf
_getFreeLeaf( LeafCache *lc )
{
    int i;
    Leaf leaf;

    if ( lc->freeList ) {
        leaf = lc->freeList;
        lc->freeList = *( ( Leaf * ) leaf );
    } else {
        if ( lc->blocks == NULL || lc->nextInBlock == LEAVES_PER_BLOCK ) {
            LeafBlock * lb;

            lb = ( LeafBlock * ) DXAllocate( sizeof( struct leafBlock ) );

            if ( !lb )
                return NULL;

            lb->next = lc->blocks;
            lc->blocks = lb;
            lc->nextInBlock = 0;
        }

        leaf = lc->blocks->leaves + lc->nextInBlock++;
    }

    leaf->depth = -1;
    leaf->reference = 0;

    for ( i = 0; i < LEAF_LEN; i++ )
        leaf->elements[ i ] = NULL;

#ifdef STATS
    if ( ( ++( lc->currLeaves ) ) > lc->maxLeaves )
        lc->maxLeaves = lc->currLeaves;

#endif

    return leaf;
}


static void
_deleteLeafCache( LeafCache *lc )
{
    LeafBlock * this, *next;

    this = lc->blocks;

    while ( this != NULL ) {
        next = this->next;
        DXFree( ( Pointer ) this );
        this = next;
    }

#ifdef STATS
    DXMessage( "leaves: %d current, %d max", lc->currLeaves, lc->maxLeaves );

#endif
}



#if 0 
static void
_freeElement( PageTable *pagetab, HashElement elt )
{
    elt->u.next = pagetab->freeList;
    pagetab->freeList = elt;

#ifdef STATS
    pagetab->currElts ++;
#endif
}

static Error
_hashCheck( HashTable hashTable )
{
    int nBuckets;
    int i, j;
    Directory *dir;
    Leaf leaf;

    dir = &hashTable->directory;

    nBuckets = powers_of_2[ dir->depth ];

    for ( i = 0; i < nBuckets; i++ ) {
        leaf = hashTable->directory.leaves[ i ];

        for ( j = 0; j < LEAF_LEN; j++ )
            if ( leaf->elements[ j ] != NULL
                    && ( ( ( ( int ) leaf->elements[ j ] ) < 0x00ffffff )
                         || ( ( ( int ) leaf->elements[ j ] ) > 0x30000000 ) ) )
                return ERROR;
    }

    return OK;
}

#endif /* 0 */
