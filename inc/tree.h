/*********************************************************************
 * Filename:    tree.h
 * Author:      Morten P. Wilsgård (morten.wilsgaard AT gmail.com)
 * Copyright:   Automatic by norwegian law
 * Credits:     Hans Aspenberg for code snippets and lectures
 * Disclaimer:  Code is presented "as is" without any guarantees
 * Details:     Defines API for the corresponding implementation
*********************************************************************/

#ifndef N_TREE
#define N_TREE

/*************************** HEADER FILES ***************************/

/************************* MACROS & DEFINES *************************/
#define OK      0
#define ERROR   1

#define FALSE   0
#define TRUE    1

// Defines amount of nodes to pre-allocate in memory for search (optimize access speeds)
#define MEMLIMIT 10

// Defines if leaf nodes may hold duplicate key names or unique keys only (incomplete)
#define UNIQUEKEYS TRUE

/**************************** DATA TYPES ****************************/
/*
 *  Considered using union- calling a null value gets sigsegv error:
 *      If we must retain type, the point of using a union is lost (space used would equal retained type)
 *      for this assignment, if there were more than two data types, a union would make sense.
 *
 *  Structures
 *      Low-coupling of data makes our program more versatile and easy to maintain.
 *      It allows for quickly adding new metadata about search results, retained data and returned data.
 */

// Node types
enum nodeType { errorUndefinedNode, noSuchNode, emptyNode, parentNode, stringNode, integerNode };

// Search modes
enum searchMode { targetNode, traversedTarget, fullTree };

// Search results
typedef struct _SEARCHRESULT {
    unsigned long   numNodes;           // Number of elements
    struct  _NODE   *node;              // Single node search
    struct  _NODE   **nodes;            // Multiple node search
} SEARCHRESULT;

// Data in node (also used to return pointers to data)
typedef struct _DATA {
    unsigned long   integer;            // If no children and no string; leaf holds integer value (incl. 0).
    char            *string;
} DATA;

// Tree node
typedef struct _NODE {
    char            *key;               // Name of node
    struct  _DATA   value;              // Data a node may hold   (named value for KV-database term.)
    unsigned int    numChildren;        // Number of children
    struct  _NODE   **children;         // Children               (if none, leaf = true)
} NODE;

/********************** GLOBAL EXTERN VARIABLES *********************/

/*********************** FUNCTION DECLARATIONS **********************/
NODE *InitTree ();

int DeinitTree (NODE **root);

int DeserializeTextFile (NODE **root, const char *fileName);

int AddNode (NODE **root, char *targetKey, char *key);

char *GetText (NODE **root, char *targetKey, char *language);

int Delete (NODE **root, char *targetKey);

int Enumerate (NODE **root, char *targetKey);

int EnumKeyValue (const char *targetKey, const DATA *data);

int PrintValue (const DATA *data);

int SetValue (NODE **root, char *targetKey, char *format, ...);

DATA *GetValue (NODE **root, char *targetKey);

char *GetString (NODE **root, char *targetKey);

unsigned long GetInt (NODE **root, char *targetKey);

int SetString (NODE **root, char *targetKey, const char *valueString);

int SetInt (NODE **root, char *targetKey, unsigned long valueInteger);

enum nodeType GetType (NODE **root, char *targetKey);

enum nodeType NodeType (NODE *node);

int Search (NODE **root, SEARCHRESULT **result, char *targetKey, enum searchMode search);

int DephtFirst (NODE **root, SEARCHRESULT **result, char *targetKey, enum searchMode search);

#endif   // N_TREE