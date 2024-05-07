#if defined _msgpack_included
#endinput
#endif
#define _msgpack_included

enum MsgPack_Type {
	MSGPACK_OBJECT_NIL	= 0x00,
    MSGPACK_OBJECT_BOOLEAN	= 0x01,
    MSGPACK_OBJECT_POSITIVE_INTEGER	= 0x02,
    MSGPACK_OBJECT_NEGATIVE_INTEGER	= 0x03,
    MSGPACK_OBJECT_DOUBLE	= 0x04,
    MSGPACK_OBJECT_RAW	= 0x05,
    MSGPACK_OBJECT_ARRAY	= 0x06,
    MSGPACK_OBJECT_MAP	= 0x07,
};

/*
*  Packing Functions
*/

// create a pack handle to pack with
native Handle:MsgPack_NewPack();

native MsgPack_PackInt(Handle:pack, value);
native MsgPack_PackFloat(Handle:pack, Float:value);
native MsgPack_PackNil(Handle:pack);
native MsgPack_PackTrue(Handle:pack);
native MsgPack_PackFalse(Handle:pack);
native MsgPack_PackArray(Handle:pack, size);
native MsgPack_PackMap(Handle:pack, size);

// pack some bytes. does not null terminate
native MsgPack_PackRaw(Handle:pack, const String:buffer[], buffer_length);

stock MsgPack_PackString(Handle:pack, const String:null_terminated_string[]) {
    return MsgPack_PackRaw(pack, null_terminated_string, strlen(null_terminated_string));
}

// get the pack length in bytes
native MsgPack_GetPackSize(Handle:pack);

// get the buffer so you can send it somewhere
native MsgPack_GetPackBuffer(Handle:pack, const String:buffer[], buffer_length);

/*
*  Unpacking Functions
*/

// create a messagepack object handle.
// offset is the number of bytes consumed
// enabling save_buffer copies the buffer so objects still work properly when the buffer goes out of scope
// the buffer is shared between all objects that come from here and will be deleted when they are all closed
native Handle:MsgPack_NewUnpack(const String:buffer[], buffer_length, &offset=0, bool:save_buffer=true);


// pretty print object to console for debugging
native MsgPack_UnpackPrint(Handle:obj);

// get the object type
native MsgPack_Type:MsgPack_UnpackType(Handle:obj);

// get the number of elements for maps, arrays, and raws. returns 0 if not one of those types
native MsgPack_UnpackCount(Handle:obj);

native bool:MsgPack_UnpackBool(Handle:obj);
native MsgPack_UnpackInt(Handle:obj);
native Float:MsgPack_UnpackFloat(Handle:obj);
native MsgPack_UnpackRaw(Handle:obj, String:buffer[], buffer_length);
stock MsgPack_UnpackString(Handle:obj, String:buffer[], buffer_length) {
    new written = MsgPack_UnpackRaw(obj, buffer, buffer_length);
    if(written-1 >= buffer_length)
        buffer[buffer_length] = 0;
    else
        buffer[written] = 0;
    
    return written;
}
native Handle:MsgPack_UnpackArray(Handle:obj, index);
native Handle:MsgPack_UnpackKey(Handle:obj, index);
native Handle:MsgPack_UnpackValue(Handle:obj, index);
native Handle:MsgPack_UnpackNext(Handle:obj);
native any:MsgPack_UnpackArrayCell(Handle:obj, index);

////////////////////////////////////////////////// old functions
#pragma deprecated Renamed Object to Unpack 
native MsgPack_ObjectPrint(Handle:obj);

#pragma deprecated Renamed Object to Unpack 
native MsgPack_Type:MsgPack_ObjectType(Handle:obj);

#pragma deprecated Renamed Object to Unpack 
native MsgPack_ObjectCount(Handle:obj);

#pragma deprecated Renamed Object to Unpack
native bool:MsgPack_ObjectBool(Handle:obj);

#pragma deprecated Renamed Object to Unpack
native MsgPack_ObjectInt(Handle:obj);

#pragma deprecated Renamed Object to Unpack
native Float:MsgPack_ObjectFloat(Handle:obj);

#pragma deprecated Renamed Object to Unpack
native MsgPack_ObjectRaw(Handle:obj, String:buffer[], buffer_length);

#pragma deprecated Renamed Object to Unpack
stock MsgPack_ObjectString(Handle:obj, String:buffer[], buffer_length) {
    return MsgPack_UnpackString(obj, buffer, buffer_length);
}

#pragma deprecated Renamed Object to Unpack
native Handle:MsgPack_ObjectArray(Handle:obj, index);

#pragma deprecated Renamed Object to Unpack
native Handle:MsgPack_ObjectKey(Handle:obj, index);

#pragma deprecated Renamed Object to Unpack
native Handle:MsgPack_ObjectValue(Handle:obj, index);

public Extension:__ext_MsgPack = 
{
	name = "msgpack",
	file = "msgpack.ext",
#if defined AUTOLOAD_EXTENSIONS
	autoload = 1,
#else
	autoload = 0,
#endif
#if defined REQUIRE_EXTENSIONS
	required = 1,
#else
	required = 0,
#endif
}