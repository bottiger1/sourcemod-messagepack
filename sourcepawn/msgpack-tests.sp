/*
This is a plugin to test the C++ extension. You don't need this use the extension.
*/

public Plugin:myinfo = 
{
	name        = "Message Pack Tests",
	author      = "Bottiger",
	description = "",
	version     = "1.0",
	url         = "http://skial.com"
};

#include <msgpack>

public OnPluginStart() {
    TestPack1();
    TestPack2();
}

stock bool:BufferEqual(length, const String:b1[], const String:b2[]) {
    for(new i=0;i<length;i++) {
        if(b1[i] != b2[i])
            return false;
    }
    return true;
}

TestPack1() {
    PrintToServer("Test 1 start");
    
    new Handle:pack = MsgPack_NewPack();
    
    MsgPack_PackArray(pack, 4);
    MsgPack_PackInt(pack, 1);
    MsgPack_PackInt(pack, -1);
    MsgPack_PackInt(pack, 65536);
    MsgPack_PackString(pack, "foobar");
    
    new size = MsgPack_GetPackSize(pack);
    PrintToServer("Size Good: %i", size == 15);
    
    decl String:dumped[size];
    MsgPack_GetPackBuffer(pack, dumped, size);
    CloseHandle(pack);
    PrintToServer("Content Good: %i", BufferEqual(size, dumped, "\x94\x01\xff\xce\x00\x01\x00\x00\xa6foobar"));
    
    new offset;
    new Handle:root = MsgPack_NewUnpack(dumped, size, offset);
    // zero out buffer test
    for(new i=0;i<size;i++) {
        dumped[i] = 255;
    }
    PrintToServer("Offset Good: %i", offset == size);
    
    // debug print test
    MsgPack_UnpackPrint(root);
    
    // Test Unpacking
    PrintToServer("root Type Good: %i Size Good %i", 
        MsgPack_UnpackType(root) == MSGPACK_OBJECT_ARRAY, 
        MsgPack_UnpackCount(root) == 4);
    
    new Handle:e = MsgPack_UnpackArray(root, 0);
    PrintToServer("0 Type Good: %i Size Good %i", 
        MsgPack_UnpackType(e) == MSGPACK_OBJECT_POSITIVE_INTEGER, 
        MsgPack_UnpackInt(e) == 1);
    CloseHandle(e);
    
    e = MsgPack_UnpackArray(root, 1);
    PrintToServer("1 Type Good: %i Size Good %i", 
        MsgPack_UnpackType(e) == MSGPACK_OBJECT_NEGATIVE_INTEGER, 
        MsgPack_UnpackInt(e) == -1);
    CloseHandle(e);
    
    e = MsgPack_UnpackArray(root, 2);
    PrintToServer("2 Type Good: %i Size Good %i", 
        MsgPack_UnpackType(e) == MSGPACK_OBJECT_POSITIVE_INTEGER, 
        MsgPack_UnpackInt(e) == 65536);
    CloseHandle(e);
    
    
    e = MsgPack_UnpackArray(root, 3);
    decl String:s[64];
    MsgPack_UnpackString(e, s, sizeof(s));
    PrintToServer("3 Type Good: %i Size Good %i", 
        MsgPack_UnpackType(e) == MSGPACK_OBJECT_RAW, 
        StrEqual(s, "foobar"));
    CloseHandle(e);
    
    CloseHandle(root);
    PrintToServer("Test 1 done");
}

TestPack2() {
    PrintToServer("Test 2 start");
    new Handle:pack = MsgPack_NewPack();
    MsgPack_PackArray(pack, 4);
    MsgPack_PackNil(pack);
    MsgPack_PackFalse(pack);
    MsgPack_PackTrue(pack);
    MsgPack_PackMap(pack, 2);
    MsgPack_PackTrue(pack);
    MsgPack_PackFalse(pack);
    MsgPack_PackString(pack, "foo");
    MsgPack_PackFloat(pack, 1.2342);
    
    new size = MsgPack_GetPackSize(pack);
    decl String:dumped[size];
    MsgPack_GetPackBuffer(pack, dumped, size);
    CloseHandle(pack);
    
    new Handle:root = MsgPack_NewUnpack(dumped, size);
    MsgPack_UnpackPrint(root);
    
    new Handle:map = MsgPack_UnpackArray(root, 3);
    new Handle:k = MsgPack_UnpackKey(map, 1);
    new Handle:v = MsgPack_UnpackValue(map, 1);
    
    decl String:key[64];
    MsgPack_UnpackString(k, key, sizeof(key));
    new Float:value = MsgPack_UnpackFloat(v);
    PrintToServer("Key foo=%s Value 1.2342=%f", key, value);
    
    CloseHandle(root);
    CloseHandle(map);
    CloseHandle(k);
    CloseHandle(v);
    
    PrintToServer("Test 2 done");
}