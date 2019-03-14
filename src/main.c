#include <stdio.h>
#include "tree.h"

int main(void) {
    // Initialize tree and deserialize text into kv-database
    // (mistakenly though keys were supposed to be unique- added appending hack to comply with assignment)
    NODE *root = InitTree();
    DeserializeTextFile(&root, "dataToDeserialize.txt");

    // Show full tree
    Enumerate(&root, "root.*");

    // Test strings
    printf("\nTest strings:");
    SetString(&root, "button_cancel", "Changed cancel string value");
    printf("\nTest get string: \"%s\"\n", GetString(&root, "button_cancel"));

    // Show config.update
    Enumerate(&root, "config.update.*");

    Enumerate(&root, "en");

    // Test Set Value string
    printf("\nTest set value string:");
    SetValue(&root, "button_cancel", "%s", "Testing SetValue string");
    Enumerate(&root, "strings");

    // Test int
    printf("\nTest int:");
    SetInt(&root, "loglevel", 42);
    printf("\nTest get int: %li\n", GetInt(&root, "loglevel"));

    // Test Set Value integer
    printf("\nTest set value integer:");
    AddNode(&root, "config", "integerTest");
    SetValue(&root, "integerTest", "%d", 73);

    Enumerate(&root, "config.*");

    // Test Get Value
    printf("\nTest get value:");
    printf("\nTest get value returning ");
    PrintValue(GetValue(&root, "button_cancel"));
    printf("\n\nThen integerTest only using keys: ");
    PrintValue(GetValue(&root, "integerTest"));

    Enumerate(&root, "config");

    // Test delete config
    printf("\nTest delete config:");
    Delete(&root, "config");
    Enumerate(&root, "root.*");

    // Test type
    printf("\nTest get type:");
    enum nodeType type = GetType(&root, "text");
    if (type == stringNode) {
        printf("\nNode is stringNode holding string value = \"%s\"\n", GetString(&root, "text"));
    }

    // Test Get Text
    printf("\nTest Get text");
    printf("\nGet text: \"%s\"\n", GetText(&root, "button_cancel", "no"));

    AddNode(&root, "en", "NotOnNorsk");
    SetString(&root, "NotOnNorsk", "Only in english");

    printf("\nGet text: \"%s\"\n", GetText(&root, "NotOnNorsk", "no"));

    // Test more delete
    printf("\nTest more delete (delete 'no):");
    Delete(&root, "no");
    Enumerate(&root, "root");

    // Test even more delete (now, strings should also be removed from root)
    printf("\nTest even more delete (delete 'en'):");
    Delete(&root, "en");
    Enumerate(&root, "root");

    printf("\nString should be gone and return no such target key:\n");
    SetString(&root, "string", "Should return (somewhere in terminal): \"Set string error: no such target key.\"");

    // Cleanup
    DeinitTree(&root);
}