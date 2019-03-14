//
// Created by morten on 27.10.17.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "tree.h"

/*
 * Notice:
 *
 *      Root is used a lot as parameter, this is necessary to allow for forests (several tree roots)
 *
 *      The function Search is also heavily used, this is due to requirements to parameters in signatures-
 *      ordinarily I would recommend supplying node as argument, rather than keys (would also lessen root params).
 */

// Swap node pointers
static void Swap(NODE **x, NODE **y) {
    // XOR swapping would require more computation- memory usage VS processing
    NODE *tmp = *x;
    *x = *y;
    *y = tmp;
}

// Bubble sort node keys
static void BubbleSort(NODE **parent, const int numChildren) {
    for (int i = 1; i < numChildren; ++i) {
        // Compare string arguments- if string 2 less than string 1
        if (strcmp((*parent)->children[i - 1]->key, (*parent)->children[i]->key) > 0) {
            // Then swap string pointers
            Swap(&(*parent)->children[i - 1], &(*parent)->children[i]);
            // Rerun- increments to 1 when loop finishes and avoids error
            i = 0;
        }
    }
}

// Handle reallocation (if increase in heap)
static NODE **ReallocHandling(NODE **oldBuff, const size_t size) {
    NODE **newBuff = realloc (oldBuff, size);
    if (!newBuff) {
        fprintf(stderr, "\nERROR: Reallocating memory failed!\n");
        free(oldBuff);  // Avoid leakage
    }
    return newBuff;     // If null, operations should be terminated
}

// Split full key path into end key (excl. "*")
int SplitEndKey(char *key, const char *fullKey) {
    char *token;

    memcpy(key, fullKey, sizeof(char) * strlen(fullKey));

    token = strtok(key, ".*");       // Initialize tokens

    while (token) {     // Until end of tokens
        strcpy(key, token);    // Grab end key
        token = strtok(NULL, ".*");
    }
    return OK;
}

// Split full key path into parent of end key
int SplitParentKey(char *key, const char *fullKey) {
    char *token;

    memcpy(key, fullKey, sizeof(char) * strlen(fullKey));
    token = strtok(key, ".*");       // Initialize tokens

    while (token) {         // Until end of tokens
        strtok(NULL, ".*");
        token = strtok(NULL, ".*");
        if (token) strcpy(key, token); // Grab parent key
    }
    return OK;
}

// Create node (no rules- usage is AddNode())
static NODE *CreateNode(const char *key) {
    // Allocate memory for node
    NODE *newNode = calloc (1, sizeof(NODE));

    if (newNode) {
        // Allocate memory for key
        newNode->key = calloc (strlen(key) + 1, sizeof(char));
        strcpy (newNode->key, key);

        // Initialize children of node to zero
        newNode->numChildren = 0;
        newNode->children = NULL;
    }
    else {
        fprintf(stderr, "\nERROR: Allocating memory failed!\n");
    }
    // Return node
    return newNode;
}

// Find node(s) by depth first traversal (root is required for forest)
int DephtFirst(NODE **root, SEARCHRESULT **result, char *targetKey, const enum searchMode search) {
    /*
     *  Depth First Traversal:
     *      As recursion may put huge overhead on large trees (ie, exponentially increasing calls on stack)
     *      an iterative preorder traversal for scalability is implemented.
     *
     *      The overhead incurred cannot be reasonably justified, especially for database queries.
     *          - However, it might be reasonable to use inorder or postorder recursion to delete nodes.
     *
     *      How recursion would work:
     *          A recursive function would send the stack back to itself, minus the current processed node.
     *          Breaking out of it would be caused by finding target- or if stack and current equals empty.
     */

    // If no root
    if (!root) {
        fprintf(stderr, "\nSearch node error: root is null.\n");
        return ERROR;
    }

    // Initialize LIFO-stack to root
    int     stackSize = 1,
            memLeftStack = MEMLIMIT,
            memLeftNodes = 0,
            i;

    short int iRc = OK;

    NODE **stack = calloc ((size_t) stackSize + memLeftStack, sizeof(NODE *)),
            *current;

    // If calloc failed
    if (!stack) {
        iRc = ERROR;
    }

    if (iRc == OK) {
        // Start stack with root
        memcpy(stack, root, sizeof(NODE *) * stackSize);

        // If current is null and stack is empty, no node was found
        while (stackSize) {
            // Set current to top of stack
            current = stack[stackSize - 1];

            // Result must be freed when searches are completed, this also goes for result nodes if more than one target.
            if (search != targetNode) {
                // Allocate memory for result nodes
                memLeftNodes--;
                if (memLeftNodes < 0) {
                    memLeftNodes = MEMLIMIT;
                    (*result)->nodes = ReallocHandling((*result)->nodes,
                                                       sizeof(NODE *) * ((*result)->numNodes + memLeftNodes + 1));
                    if (!(*result)->nodes) {
                        iRc = ERROR;
                        break;
                    }
                }
                // Add to result
                (*result)->nodes[(*result)->numNodes] = current;
                (*result)->numNodes++;
            }

            // If current is target and search not full tree, return node(s)
            if (strcmp(current->key, targetKey) == 0 && search != fullTree) {
                if (search == targetNode) {
                    (*result)->node = current;
                    (*result)->numNodes++;
                }
                break;
            }

            // Push currents children to stack
            if (current->numChildren != 0) {
                // Do we need to allocate more memory?
                memLeftStack -= current->numChildren;
                if (memLeftStack < 0) {
                    // Re-allocate memory to match stack + number of children + memlimit
                    memLeftStack = MEMLIMIT;
                    stack = ReallocHandling(stack,
                                            (sizeof(NODE *) * (stackSize + memLeftStack) * current->numChildren));
                    if (!stack) {
                        iRc = ERROR;
                        break;
                    }
                }

                for (i = 0; i < current->numChildren; ++i) {
                    // Increment stack size and add children to stack
                    stack[stackSize - 1] = current->children[i];
                    stackSize++;
                }
            }
            stackSize--;        // Decrement stack
            memLeftStack++;     // Make space available
        }
    }
    else {
        fprintf(stderr, "\nSearch node error: Allocating memory failed!\n");
    }
    // Free
    if (stack) {
        free (stack);
    }

    return iRc;
}

// Search (handle search types)
int Search(NODE **root, SEARCHRESULT **result, char *targetKey, const enum searchMode search) {
    /*
     *  Allow parents to hold same key name:
     *      Requires iterative walk through each node in path (out of the scope for this assignment).
     *
     *  Allow leafs to hold duplicate key names:
     *      Requires node paths (parent.key): finds parent and searches children.
     *
     *  Unique leafs:
     *      Only requires child (key): finds child, but does not allow for duplicate key names in leafs.
     */

    // Receive path or key, return target according to specification and search type

    char *key = NULL;

    // Find parent node, search children and return target (duplicate leaf names)
    // I've mistakenly implemented a key value database that only allows unique keys.
    // This is incomplete as I didn't have time to fix the code for non-unique leaf nodes (therefore appending lang)
    if (UNIQUEKEYS == FALSE && search != fullTree) {
        // Split key path into target end key if it contains '.'
        if (strstr(targetKey, ".") != NULL) {
            key = calloc (strlen(targetKey), sizeof(char));
            SplitParentKey(key, targetKey);

            // if calloc failed
            if (!key) {
                return ERROR;
            }
        }
        else {
            fprintf(stderr, "Search requires path.key!");
            return ERROR;
        }
    }

    // Find child node, return target (unique leafs)
    else if (UNIQUEKEYS == TRUE) {
        // Split key path into target end key if it contains '.'
        if (search != fullTree && strstr(targetKey, ".") != NULL) {
            key = calloc (strlen(targetKey), sizeof(char));
            SplitEndKey(key, targetKey);

            // if calloc failed
            if (!key) {
                return ERROR;
            }
        }
    }

    // Search types
    DephtFirst(root, result, (key) ? (key) : (targetKey), search);

    if (key) {
        free (key);
    }
    return OK;
}

// Add node
int AddNode(NODE **root, char *targetKey, char *key) {
    short int iRc = ERROR;
    char error[51];     // Max 50 chars error message

    // If no root
    if (!root) {
        strcpy(error, "root is null");
    }

        // If missing key
    else if (!key) {
        strcpy(error, "missing key.");
    }

        // If missing target key
    else if (!targetKey) {
        strcpy(error, "missing target key.");
    }

    else {
        // Check if key already exists in tree (unique keys only)
        SEARCHRESULT *result = calloc(1, sizeof(SEARCHRESULT));
        if (!result) {
            strcpy(error, "allocating memory for search failed!");
        }
        else {
            Search(root, &result, key, targetNode);

            if (result->node) {
                strcpy(error, "key already exists in tree.");
            } else {
                // Find target
                Search(root, &result, targetKey, targetNode);

                // If target key wasn't found
                if (!result->node) {
                    strcpy(error, "target key doesn't exist in tree.");
                } else {
                    // Create child
                    NODE *newNode = CreateNode(key);
                    if (!newNode) {
                        strcpy(error, "problem creating new node.");

                    } else {
                        // Init string value to NULL
                        newNode->value.string = NULL;

                        // Remove any values held by parent
                        result->node->value.integer = 0;
                        if (result->node->value.string != NULL) {
                            free(result->node->value.string);
                            result->node->value.string = NULL;
                        }

                        // Re-allocate memory to match number of children
                        result->node->children = ReallocHandling(result->node->children,
                                                                 sizeof(NODE *) * (result->node->numChildren + 1));

                        if (result->node->children != NULL) {
                            // Increment number of children
                            result->node->numChildren++;

                            // Add child to parents children
                            result->node->children[result->node->numChildren - 1] = newNode;

                            // Sort children's keys
                            BubbleSort(&result->node, result->node->numChildren);

                            iRc = OK;
                        }
                        else {
                            strcpy(error, "allocating memory for children failed!");
                        }
                    }
                }
            }
            free(result);
        }
    }
    if (iRc != OK) {
        fprintf(stderr, "\nAdd node '%s' error: %s", key, error);
    }
    return iRc;
}

// Get node type by node
enum nodeType NodeType(NODE *node) {
    enum nodeType type;

    if (!node) {
        fprintf(stderr, "\nGet type error: node is null.\n");
        type = noSuchNode;
    }

    else if (node->numChildren != 0) {
        type = parentNode;
    }

    else if (node->value.string) {
        type = stringNode;
    }

    else {
        type = integerNode;
    }

    return type;
}

// Get node type by key
enum nodeType GetType(NODE **root, char *targetKey) {
    // If no root
    if (!root) {
        fprintf(stderr, "\nGet type error: root is null.\n");
        return errorUndefinedNode;
    }

    SEARCHRESULT *result = calloc(1, sizeof(SEARCHRESULT));
    if (!result) {
        fprintf(stderr, "\nGet type error: allocating memory for search failed.\n");
        return errorUndefinedNode;
    }

    Search(root, &result, targetKey, targetNode);
    enum nodeType type = noSuchNode;

    if (!result->node) {
        fprintf(stderr, "\nGet type error: no such key in tree.\n");
    }
    else {
        type = NodeType(result->node);
    }
    free (result);
    return type;
}

// Set node integer
int SetInt(NODE **root, char *targetKey, const unsigned long valueInteger) {
    // If no root
    if (!root) {
        fprintf(stderr, "\nSet int error: root is null.\n");
        return ERROR;
    }

    short int iRc = OK;
    SEARCHRESULT *result = calloc(1, sizeof(SEARCHRESULT));
    if (!result) {
        fprintf(stderr, "\nSet int error: allocating memory for search failed.\n");
        return ERROR;
    }

    Search(root, &result, targetKey, targetNode);

    if (result->node) {
        // Get type
        enum nodeType targetNode = NodeType(result->node);

        if (targetNode != parentNode) {
            if (targetNode == integerNode) {
                result->node->value.integer = valueInteger;
            }

            else {
                fprintf(stderr, "\nSet int error: node contains string value.\n");
                iRc = ERROR;
            }
        }

            // If target is parent
        else {
            fprintf(stderr, "\nSet int error: node is a parent node.\n");
            iRc = ERROR;
        }
    }
    else {
        fprintf(stderr, "\nSet int error: no such target key.\n");
        iRc = ERROR;
    }
    free (result);
    return iRc;
}

// Set node string
int SetString(NODE **root, char *targetKey, const char *valueString) {
    // If no root
    if (!root) {
        fprintf(stderr, "\nSet string error: root is null.\n");
        return ERROR;
    }

    SEARCHRESULT *result = calloc(1, sizeof(SEARCHRESULT));
    if (!result) {
        fprintf(stderr, "\nSet string error: allocating memory for search failed.\n");
        return ERROR;
    }

    short int iRc = OK;
    Search(root, &result, targetKey, targetNode);

    if (result->node) {
        // Get type
        enum nodeType targetNode = NodeType(result->node);

        if (targetNode != parentNode) {
            // If stringNode- or if string is null and integer is 0
            // (we allow setting string if integer is 0)
            if (targetNode == stringNode || result->node->value.integer == 0) {
                char *temp = realloc(result->node->value.string,
                                     sizeof(char) * (strlen(valueString) + 1));

                if (temp) {
                    strcpy(temp, valueString);
                    result->node->value.string = temp;
                }
                else {
                    // We don't free old memory held by node string (if it fails, we'll keep the old data)
                    fprintf(stderr, "\nSet string error: reallocating memory failed.\n");
                    iRc = ERROR;
                }
            }

            else {
                fprintf(stderr, "\nSet string error: node contains integer value.\n");
                iRc = ERROR;
            }
        }

            // If target is parent
        else {
            fprintf(stderr, "\nSet string error: node is a parent node.\n");
            iRc = ERROR;
        }
    }

    else {
        fprintf(stderr, "\nSet string error: no such target key.\n");
        iRc = ERROR;
    }
    free (result);
    return iRc;
}

// Get node integer
unsigned long GetInt(NODE **root, char *targetKey) {
    // Chosen to return 0 if error due to unsigned value easily getting mistaken for real values.
    // Returning a data structure with value and error code would be less prone to erroneous mistakes

    // If no root
    if (!root) {
        fprintf(stderr, "\nGet int error: root is null.\n");
        return 0;
    }

    SEARCHRESULT *result = calloc(1, sizeof(SEARCHRESULT));
    if (!result) {
        fprintf(stderr, "\nGet int error: allocating memory for search failed.\n");
        return 0;
    }

    Search(root, &result, targetKey, targetNode);
    unsigned long value = 0;

    if (result->node) {
        // Get type
        enum nodeType targetNode = NodeType(result->node);

        if (targetNode == integerNode) {
            value = result->node->value.integer;
        }

        else {
            fprintf(stderr, "\nGet int error: wrong node type.\n");
        }
    }

    else {
        fprintf(stderr, "\nGet int error: no such key in tree.\n");
    }
    free (result);
    return value;
}

// Get node string
char *GetString(NODE **root, char *targetKey) {
    char *value = NULL;   // Trying to printf a null will crash

    // If no root
    if (!root) {
        fprintf(stderr, "\nGet string error: root is null.\n");
        return value;
    }

    SEARCHRESULT *result = calloc(1, sizeof(SEARCHRESULT));
    if (!result) {
        fprintf(stderr, "\nGet string error: allocating memory for search failed.\n");
        return value;
    }

    Search(root, &result, targetKey, targetNode);

    if (result->node) {
        // Get type
        enum nodeType targetNode = NodeType(result->node);

        if (targetNode == stringNode) {
            value = result->node->value.string;
        }

        else {
            fprintf(stderr, "\nGet string error: wrong node type.\n");
        }
    }

    else {
        fprintf(stderr, "\nGet string error: no such key in tree.\n");
    }
    free (result);
    return value;
}

// String / integer accessor (if string = null, then integer value)
DATA *GetValue(NODE **root, char *targetKey) {
    // If no root
    if (!root) {
        fprintf(stderr, "\nGet value error: root is null.\n");
        return NULL;
    }

    SEARCHRESULT *result = calloc(1, sizeof(SEARCHRESULT));
    if (!result) {
        fprintf(stderr, "\nGet value error: allocating memory for search failed.\n");
        return NULL;
    }

    DATA *data = NULL;
    Search(root, &result, targetKey, targetNode);
    if (result->node) {
        enum nodeType type = NodeType(result->node);

        if (type == stringNode || type == integerNode) {
            data = &result->node->value;
        }
    }
    free (result);
    return data;
}

// String / integer mutator (sets argument to corresponding format- %s for string, %d for int)
int SetValue(NODE **root, char *targetKey, char *format, ...) {
    // If no root
    if (!root) {
        fprintf(stderr, "\nSet value error: root is null.\n");
        return ERROR;
    }

    // Points to each unnamed arg in turn
    va_list ap;
    char *p;

    // Make ap point to 1st unnamed arg
    va_start(ap, format);

    // Iterate over args
    for (p = format; *p; p++) {
        if (*p != '%') {
            fprintf(stderr, "Set value error: valid formats are '%%s' for string and '%%d' for integer.");
            return ERROR;
        }
        switch (*++p) {
            case 'd':
                SetInt(root, targetKey, va_arg(ap, unsigned long));
                break;
            case 's':
                SetString(root, targetKey, va_arg(ap, char *));
                break;
            default:
                break;
        }
    }
    // Clean up when done
    va_end(ap);

    return OK;
}

// Print value of data
int PrintValue(const DATA *data) {
    if (!data) {
        return ERROR;
    }
    else if (!data->string) {
        printf("integer value '%li'", data->integer);
    }
    else {
        printf("string value \"%s\"", data->string);
    }
    return OK;
}

// Print key name and value of data
int EnumKeyValue(const char *targetKey, const DATA *data) {
    printf("\n\t '%s'   \t :: \t", targetKey);
    PrintValue(data);
    return OK;
}

// Enumerate all child nodes with values from given node
int Enumerate(NODE **root, char *targetKey) {
    short int iRc = ERROR;

    // If no root
    if (!root) {
        fprintf(stderr, "\nEnumerate error: root is null.\n");
        return iRc;
    }

    // Get target
    SEARCHRESULT *resultNode = calloc(1, sizeof(SEARCHRESULT));
    if (!resultNode) {
        fprintf(stderr, "\nEnumerate error: allocating memory for search failed.\n");
        return iRc;
    }

    Search(root, &resultNode, targetKey, targetNode);

    if (resultNode->node) {
        // Get targets children
        SEARCHRESULT *resultNodeChildren = calloc(1, sizeof(SEARCHRESULT));
        if (!resultNodeChildren) {
            fprintf(stderr, "\nEnumerate error: allocating memory for search failed.\n");
        }
        else {
            Search(&resultNode->node, &resultNodeChildren, "dummy", fullTree);
            if (resultNodeChildren->numNodes > 1) {
                printf("\nValue holding node(s) enumerated from '%s': ", targetKey);
                // Function pointer (callback) to KeyValue
                int (*KeyValuePtr)(const char *, const DATA *);
                KeyValuePtr = &EnumKeyValue;    // Would be less code and overhead to call method directly

                enum nodeType type;
                int cnt = 1;    // 0 is target node: 1 to go below target
                while (cnt < resultNodeChildren->numNodes) {
                    type = NodeType(resultNodeChildren->nodes[cnt]);
                    // If holding value, callback
                    if (type == stringNode || type == integerNode) {
                        KeyValuePtr(resultNodeChildren->nodes[cnt]->key, &resultNodeChildren->nodes[cnt]->value);
                    }
                    cnt++;
                }
                printf("\n");
            }
            else {
                printf("\nNo value holding nodes found under '%s'.", targetKey);
            }
            free(resultNodeChildren->nodes);
            free(resultNodeChildren);
            iRc = OK;
        }
    }
    free (resultNode);
    return iRc;
}

// Delete target node (incl. child nodes and empty parent nodes)
int Delete(NODE **root, char *targetKey) {
    short int iRc = ERROR;

    // If no root
    if (!root) {
        fprintf(stderr, "\nDelete error: root is null.\n");
        return iRc;
    }

    // Get target and all traversed nodes
    SEARCHRESULT *resultToNode = calloc (1, sizeof(SEARCHRESULT));
    if (!resultToNode) {
        fprintf(stderr, "\nDelete error: allocating memory for search failed.\n");
        return iRc;
    }

    Search(root, &resultToNode, targetKey, traversedTarget);

    // If anything- and if return matches target
    if (resultToNode->nodes) {
        if (!strcmp(resultToNode->nodes[resultToNode->numNodes - 1]->key, targetKey)) {
            // Get targets children
            SEARCHRESULT *resultNodeChildren = calloc(1, sizeof(SEARCHRESULT));
            if (!resultNodeChildren) {
                fprintf(stderr, "\nDelete error: allocating memory for search failed.\n");
            } else {
                // Get all children of target (using target as root)
                Search(&resultToNode->nodes[resultToNode->numNodes - 1], &resultNodeChildren, "dummy", fullTree);

                long cnt = 1, queued = 0, i, z, x; // long to match numNodes (cnt 1 avoids target)
                if (resultNodeChildren->nodes) {
                    // Free children
                    while (cnt < resultNodeChildren->numNodes) {
                        free(resultNodeChildren->nodes[cnt]->key);
                        free(resultNodeChildren->nodes[cnt]->children);
                        if (resultNodeChildren->nodes[cnt]->value.string) {
                            free(resultNodeChildren->nodes[cnt]->value.string);
                        }
                        free(resultNodeChildren->nodes[cnt]);
                        cnt++;
                    }
                    // Free search
                    free(resultNodeChildren->nodes);
                }
                free(resultNodeChildren);

                // Check parents backwards
                cnt = resultToNode->numNodes - 2;   // Last is target, thus -2 accounting for element 0

                // Set target as current
                NODE *current = resultToNode->nodes[resultToNode->numNodes - 1],
                     **toFree = calloc(resultToNode->numNodes, sizeof(NODE));

                short int modifiedPrevious = FALSE;

                if (!toFree) {
                    fprintf(stderr, "\nDelete error: allocating memory failed.\n");
                } else {
                    toFree[queued] = current;
                    while (cnt >= 0) {

                        for (i = 0; i < resultToNode->nodes[cnt]->numChildren; ++i) {
                            if (strcmp(resultToNode->nodes[cnt]->children[i]->key, current->key) == 0) {
                                // Set parent as current
                                current = resultToNode->nodes[cnt];

                                // If parent is empty (and not root)
                                if (current->numChildren <= 1 && current != *root) {
                                    toFree[++queued] = current;
                                }

                                // If parent should be modified
                                else if (modifiedPrevious == FALSE) {
                                    // Decrement children
                                    current->numChildren--;

                                    // Remove child from parents children
                                    // Find child and swap
                                    for (z = 0; z <= current->numChildren; z++) {
                                        for (x = 0; x <= queued; x++) {
                                            if (strcmp(toFree[x]->key, current->children[z]->key) == 0) {
                                                // Swap to end
                                                current->children[z] = current->children[current->numChildren];
                                            }
                                        }
                                    }

                                    // Re-allocate memory to match number of children
                                    current->children = ReallocHandling(current->children,
                                                                        sizeof(NODE) * (current->numChildren + 1));

                                    modifiedPrevious = TRUE;
                                }
                            }
                        }
                        cnt--;
                    }
                    // Free empty parents (and target)
                    while (queued >= 0) {
                        free(toFree[queued]->key);
                        free(toFree[queued]->children);
                        if (toFree[queued]->value.string) {
                            free(toFree[queued]->value.string);
                        }
                        free(toFree[queued]);
                        queued--;
                    }
                    free(toFree);
                    iRc = OK;
                }
            }
        }
        else {
            fprintf(stderr, "\nDelete error: target key does not exist.\n");
        }
    }

    // Free search
    free (resultToNode->nodes);
    free (resultToNode);

    return iRc;
}

// Return translation for node string value- or english text if translation is void
char *GetText(NODE **root, char *targetKey, char *language) {
    // If no root
    if (!root) {
        fprintf(stderr, "\nGet text error: root is null.\n");
        return NULL;
    }

    SEARCHRESULT *result = calloc(1, sizeof(SEARCHRESULT));
    if (!result) {
        fprintf(stderr, "\nSet string error: allocating memory for search failed.\n");
        return NULL;
    }

    Search(root, &result, language, targetNode);

    // This is somewhat incomplete- I append language due to unique key values only
    char *path = calloc (strlen(language) + strlen(targetKey), sizeof(char));
    if (!path) {
        fprintf(stderr, "\nSet string error: allocating memory for path failed.\n");
        free (result);
        return NULL;
    }
    strcat(path, language);
    strcat(path, targetKey);

    if (result->node) {
        // Language found
        Search(&result->node, &result, path, targetNode);
        enum nodeType type = NodeType(result->node);

        // If target language doesn't have the target, or no language found, search the EN node
        if (type != stringNode) {
            Search(root, &result, "en", targetNode);
            Search(&result->node, &result, targetKey, targetNode);
        }
    }

    free (path);
    free (result);
    return result->node->value.string;
}

// Get length of line from file
static long GetLineLength (FILE *file) {
    long length = 0;
    int character;

    long position = ftell(file);
    while (!feof(file)) {
        character = fgetc(file);
        length++;
        if (character == '\n') {
            break;
        }
    }
    fseek(file, position, SEEK_SET); // Set file position to start of line.
    return length;
}

// Deserialize database from text file (parsed line by line)
int DeserializeTextFile(NODE **root, const char *fileName) {
    // Notice: this deserialization assumes no quotes '"', white spaces or equal signs '=' are used in keys
    // General format should be: path.key = integer OR path.key = "string"

    // If no root
    if (!root) {
        fprintf(stderr, "\nDeserialize text file error: root is null.\n");
        return ERROR;
    }

    unsigned long
            integer = 0,
            cnt = 0;

    char    *keyPath,
            *targetKey,
            *string = NULL,
            *line = NULL,
            *token,
            *path = NULL;

    short int iRc = OK;

    long lineLength;
    FILE *file;

    if (!(file = fopen(fileName, "r"))) {
        fprintf(stderr, "Deserialize text file error: problem reading file.");
        return ERROR;
    }

    // Not critical if it fails- will be checked by AddNode (we check to avoid warnings of existing keys)
    SEARCHRESULT *result = calloc(1, sizeof(SEARCHRESULT));

    // Read until end of file unless error occurs
    while (iRc == OK && !feof (file)) {
        lineLength = GetLineLength(file);               // Get line length
        line = malloc ((size_t) lineLength);            // Allocate buffer to correct length
        cnt++;                                          // Count iterations / line number

        if (line) {
            if (fscanf(file, "%[^\n] \n", line) == 1) {  // Scan to \n (and add \n to retain format)
                // We got the line, including '\n'
                // Split into tokens (starting within " " to allow white spaces in string values)
                strtok(line, "\"");             // Initialize tokens
                token = strtok(NULL, "\"");     // Jump to second token and assign to token

                // If string
                if (token) {
                    string = token;
                }

                // Or if integer
                else {
                    strtok(line, "\t =");                   // Delimiter by tabs, spaces and equal signs
                    token = strtok(NULL, "\t =");

                    if (token) {
                        integer = strtoul(token, NULL, 10); // Get unsigned long from string in base 10
                    }
                    else {
                        fprintf(stderr, "Deserialize text file error: "
                                "extracting values failed for line %li: '%s'.", cnt, line);
                        iRc = ERROR;
                    }
                }

                // Full key path
                keyPath = strtok(line, "\t =");

                // Split keys (first is parent of root, second is parent of first, etc- last one holds value)
                token = strtok(keyPath, ".");
                targetKey = (*root)->key;
                while (token) {
                    // Check if key exists in tree
                    Search(root, &result, token, targetNode);

                    // If not found, add node
                    if (!result->node) {
                        // To avoid replicate nodes (hack for "no")
                        char no[] = "no";
                        if (strcmp(targetKey, no) == 0) {
                            path = calloc (strlen(no) + strlen(token), sizeof(char));
                            if (!path) {
                                iRc = ERROR;
                                break;
                            }
                            strcat(path, no);
                            strcat(path, token);

                            AddNode(root, targetKey, path);
                        }
                        else {
                            // Generate node, add to respective parent
                            AddNode(root, targetKey, token);
                        }
                    }

                    targetKey = token;
                    token = strtok(NULL, ".");

                    // Reset search
                    result->node = NULL;
                }
                // Set value of last node
                if (string) {
                    if (path) {
                        SetString(root, path, string);
                        free (path);
                        path = NULL;
                    }
                    else {
                        SetString(root, targetKey, string);
                    }
                }

                else {
                    SetInt(root, targetKey, integer);
                }
                // Reset string
                string = NULL;
            }
            free (line);
        }
    }
    fclose(file);
    if (result) {
        free (result);
    }
    return iRc;
}

// Deinit tree root (can be replaced with Delete(&root, "root"))
int DeinitTree(NODE **root) {
    // If no root
    if (!root) {
        fprintf(stderr, "\nDeinit error: root is null.\n");
        return ERROR;
    }

    SEARCHRESULT *result = calloc(1, sizeof(SEARCHRESULT));
    if (!result) {
        fprintf(stderr, "\nDeinit error: allocating memory for search failed.\n");
        return ERROR;
    }

    Search(root, &result, "dummy", fullTree);

    if (!result->nodes) {
        fprintf(stderr, "ERROR: Deinitialization failed!");
        return ERROR;
    }

    else {
        // Free memory of nodes
        int i;
        for (i = 0; i < result->numNodes; ++i) {
            free(result->nodes[i]->key);
            free(result->nodes[i]->children);
            if (result->nodes[i]->value.string) {
                free(result->nodes[i]->value.string);
            }
            free(result->nodes[i]);
        }

        // Free search
        free(result->nodes);
    }
    free (result);

    return OK;
}

// Init tree root
NODE *InitTree() {
    register NODE *root = CreateNode("root");

    // Check if create node was successful
    if (!root) {
        fprintf(stderr, "ERROR: creating root node failed!");
    }
    return root;
}