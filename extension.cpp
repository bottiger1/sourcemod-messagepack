/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Sample Extension
 * Copyright (C) 2004-2008 AlliedModders LLC.  All rights reserved.
 * =============================================================================
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 3.0, as published by the
 * Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * As a special exception, AlliedModders LLC gives you permission to link the
 * code of this program (as well as its derivative works) to "Half-Life 2," the
 * "Source Engine," the "SourcePawn JIT," and any Game MODs that run on software
 * by the Valve Corporation.  You must obey the GNU General Public License in
 * all respects for all other code used.  Additionally, AlliedModders LLC grants
 * this exception to all derivative works.  AlliedModders LLC defines further
 * exceptions, found in LICENSE.txt (as of this writing, version JULY-31-2007),
 * or <http://www.sourcemod.net/license.php>.
 *
 * Version: $Id$
 */

#include "extension.h"
#include <msgpack.h>

#define DEBUG 0

Sample g_Sample;

SMEXT_LINK(&g_Sample);

HandleType_t
    g_type_msgpack_object,
    g_type_packer;

void print_string(char* s, size_t length) {
    putchar('\"');

    for(size_t i=0;i<length;i++) {
        unsigned cp = (unsigned char)*s++;
        printf("\\x%.2x", cp);
    }

    putchar('\"');
}

struct Buffer {
    int     refs;
    char*   buffer;
    size_t  length;

    Buffer(char* raw_data, size_t raw_length) {
        refs = 0;
        length = raw_length;
        buffer = (char*)malloc(raw_length);
        memcpy(buffer, raw_data, raw_length);
    }

    ~Buffer() {
        #if DEBUG
        printf("buffer %x destroyed\n", (uint)this);
        #endif        
        free(buffer);
    }

    void AddRef() {
        refs++;
    }

    void RemoveRef() {
        refs--;
        if(refs <= 0) {
            delete this;
        }
    }
};

struct Unpacker {
    int                 refs;
    Buffer*             buffer;
    char*               original_buffer;
    size_t              buffer_length;
    size_t              offset;
    msgpack_unpacked    unpacked;
        
    Unpacker(bool copy, char* raw_data, size_t raw_length, size_t* user_offset, int& success) {
        refs = 0;
        buffer = 0;
        original_buffer = 0;
        buffer_length = raw_length;
        offset = 0;
        msgpack_unpacked_init(&unpacked);

        if(copy) {
            buffer = new Buffer(raw_data, buffer_length);
            buffer->AddRef();
            success = msgpack_unpack_next(&unpacked, buffer->buffer, buffer_length, &offset);
        } else {
            original_buffer = raw_data;
            success = msgpack_unpack_next(&unpacked, original_buffer, buffer_length, &offset);
        }

        #if DEBUG
        printf("success %i offset %i length %i\n", success, offset, buffer_length);
        #endif

        if(user_offset)
            *user_offset = offset;
    }

    Unpacker() {
        refs = 0;
        buffer = 0;
        original_buffer = 0;
        buffer_length = 0;
        offset = 0;
        msgpack_unpacked_init(&unpacked);
    }

    ~Unpacker() {
        #if DEBUG
        printf("unpacker %x destroyed\n", (uint)this);
        #endif
        msgpack_unpacked_destroy(&unpacked);
        if(buffer)
            buffer->RemoveRef();
    }

    Unpacker* Next() {
        auto o = new Unpacker();
        o->original_buffer = original_buffer;
        o->buffer_length = buffer_length;
        o->offset = offset;
        
        int success;
        if(buffer) {
            buffer->AddRef();
            o->buffer = buffer;
            success = msgpack_unpack_next(&o->unpacked, buffer->buffer, buffer_length, &o->offset);
        } else {
            success = msgpack_unpack_next(&o->unpacked, original_buffer, buffer_length, &o->offset);
        }

        #if DEBUG
        printf("next success %i offset %i\n", success, o->offset);
        #endif

        if(success < 1) {
            delete o;
            return NULL;
        }

        return o;
    }

    void AddRef() {
        refs++;
    }
    
    void RemoveRef() {
        refs--;
        if(refs <= 0) {
            delete this;
        }
    }
    
};

struct MsgPackObject {
    Unpacker* unpacker;
    msgpack_object obj;
    
    MsgPackObject(Unpacker* u) {
        unpacker = u;
        obj = unpacker->unpacked.data;
        unpacker->AddRef();
    }
    
    MsgPackObject(MsgPackObject* parent, msgpack_object o) {
        unpacker = parent->unpacker;
        obj = o;
        unpacker->AddRef();
    }
    
    ~MsgPackObject() {
        unpacker->RemoveRef();
    }

    MsgPackObject* Next() {
        auto u = unpacker->Next();
        if(u == NULL)
            return NULL;
        return new MsgPackObject(u);
    }
};

struct Packer {
    msgpack_sbuffer buffer;
    msgpack_packer packer;
    
    Packer() {
        msgpack_sbuffer_init(&buffer);
        msgpack_packer_init(&packer, &buffer, msgpack_sbuffer_write);
    }
    
    ~Packer() {
        msgpack_sbuffer_destroy(&buffer);
    }
};

struct HandleHandler : public IHandleTypeDispatch {
	void OnHandleDestroy(HandleType_t type, void *object) {
        if(type == g_type_msgpack_object)
            delete static_cast<MsgPackObject*>(object);
        else if(type == g_type_packer)
            delete static_cast<Packer*>(object);
	}
} g_handle_handler;

static cell_t NewPack(IPluginContext *pContext, const cell_t *params) {
    Packer* p = new Packer();
    return g_pHandleSys->CreateHandle(g_type_packer, (void*)p, pContext->GetIdentity(), myself->GetIdentity(), NULL);
}

#define PACKER_HANDLE_MACRO \
    HandleError err;\
    HandleSecurity sec;\
    sec.pOwner = NULL;\
    sec.pIdentity = myself->GetIdentity();\
    Handle_t handle = static_cast<Handle_t>(params[1]);\
    Packer* p;\
	if ((err = g_pHandleSys->ReadHandle(handle, g_type_packer, &sec, (void **)&p)) != HandleError_None) {\
		return pContext->ThrowNativeError("Invalid Packer handle %x (error %d)", handle, err);\
	}

static cell_t PackInt(IPluginContext *pContext, const cell_t *params) {
    PACKER_HANDLE_MACRO
    return msgpack_pack_int32(&p->packer, params[2]);
}

static cell_t PackFloat(IPluginContext *pContext, const cell_t *params) {
    PACKER_HANDLE_MACRO
    return msgpack_pack_float(&p->packer, sp_ctof(params[2]));
}

static cell_t PackNil(IPluginContext *pContext, const cell_t *params) {
    PACKER_HANDLE_MACRO
    return msgpack_pack_nil(&p->packer);
}

static cell_t PackTrue(IPluginContext *pContext, const cell_t *params) {
    PACKER_HANDLE_MACRO
    return msgpack_pack_true(&p->packer);
}

static cell_t PackFalse(IPluginContext *pContext, const cell_t *params) {
    PACKER_HANDLE_MACRO
    return msgpack_pack_false(&p->packer);
}

static cell_t PackArray(IPluginContext *pContext, const cell_t *params) {
    PACKER_HANDLE_MACRO
    return msgpack_pack_array(&p->packer, params[2]);
}

static cell_t PackMap(IPluginContext *pContext, const cell_t *params) {
    PACKER_HANDLE_MACRO
    return msgpack_pack_map(&p->packer, params[2]);
}

static cell_t PackRaw(IPluginContext *pContext, const cell_t *params) {
    PACKER_HANDLE_MACRO
    msgpack_pack_bin(&p->packer, params[3]);
    
    char* buffer;
    pContext->LocalToPhysAddr(params[2], (cell_t**)&buffer);
    
    return msgpack_pack_bin_body(&p->packer, buffer, params[3]);
}

static cell_t GetPackSize(IPluginContext *pContext, const cell_t *params) {
    PACKER_HANDLE_MACRO
    
    return p->buffer.size;
}

// pack, buffer, bufferlength
static cell_t GetPackBuffer(IPluginContext *pContext, const cell_t *params) {
    PACKER_HANDLE_MACRO
    
    char* sourcepawn_buffer;
    pContext->LocalToPhysAddr(params[2], (cell_t**)&sourcepawn_buffer);
    
    cell_t buffer_len = (cell_t) p->buffer.size;
    cell_t written = params[3] < buffer_len ? params[3] : buffer_len;
    
    memcpy(sourcepawn_buffer, p->buffer.data, written);
    return written;
}

// buffer, buffer_length, &offset, save_buffer
static cell_t NewUnpack(IPluginContext *pContext, const cell_t *params) {
    char* sourcepawn_buffer;
    cell_t* offset;
    
    pContext->LocalToPhysAddr(params[1], (cell_t**)&sourcepawn_buffer);
    pContext->LocalToPhysAddr(params[3], &offset);
    
    #if DEBUG
    printf("buffer %x length %i offset %x save %i\n", (uint)sourcepawn_buffer, params[2], (uint)offset, params[4]);
    print_string(sourcepawn_buffer, params[2]);
    #endif

    int success;
    Unpacker* unpacker = new Unpacker(params[4], sourcepawn_buffer, params[2], (size_t*)offset, success);

    /*
    MSGPACK_UNPACK_SUCCESS              =  2,
    MSGPACK_UNPACK_EXTRA_BYTES          =  1,
    MSGPACK_UNPACK_CONTINUE             =  0,
    MSGPACK_UNPACK_PARSE_ERROR          = -1,
    MSGPACK_UNPACK_NOMEM_ERROR          = -2
    
    msgpack_unpack_continue means the buffer is too small, it should never happen for us
    */

    if(success < 1) {
        delete unpacker;
        return 0;
    }
    
    MsgPackObject* p = new MsgPackObject(unpacker);
    return g_pHandleSys->CreateHandle(g_type_msgpack_object, (void*)p, pContext->GetIdentity(), myself->GetIdentity(), NULL);
}

#define OBJECT_HANDLE_MACRO \
    HandleError err;\
    HandleSecurity sec;\
    sec.pOwner = NULL;\
    sec.pIdentity = myself->GetIdentity();\
    Handle_t handle = static_cast<Handle_t>(params[1]);\
    MsgPackObject* p;\
	if ((err = g_pHandleSys->ReadHandle(handle, g_type_msgpack_object, &sec, (void **)&p)) != HandleError_None) {\
		return pContext->ThrowNativeError("Invalid MsgPackObject handle %x (error %d)", handle, err);\
	}

static cell_t UnpackNext(IPluginContext *pContext, const cell_t *params) {
    OBJECT_HANDLE_MACRO

    auto n = p->Next();
    if(n == NULL)
        return 0;
    return g_pHandleSys->CreateHandle(g_type_msgpack_object, (void*)n, pContext->GetIdentity(), myself->GetIdentity(), NULL);
}

static cell_t UnpackPrint(IPluginContext *pContext, const cell_t *params) {
    OBJECT_HANDLE_MACRO
    
    msgpack_object_print(stdout, p->obj);
    fprintf(stdout, "\n");
    return 0;
}

static cell_t UnpackType(IPluginContext *pContext, const cell_t *params) {
    OBJECT_HANDLE_MACRO
    
    return p->obj.type;
}

static cell_t UnpackCount(IPluginContext *pContext, const cell_t *params) {
    OBJECT_HANDLE_MACRO
    
    switch (p->obj.type) {
        case MSGPACK_OBJECT_BIN:
            return p->obj.via.bin.size;
        case MSGPACK_OBJECT_ARRAY:
            return p->obj.via.array.size;
        case MSGPACK_OBJECT_MAP:
            return p->obj.via.map.size;
    };
    
    return 0;
}

static cell_t UnpackBool(IPluginContext *pContext, const cell_t *params) {
    OBJECT_HANDLE_MACRO
    return p->obj.via.boolean;
}

static cell_t UnpackInt(IPluginContext *pContext, const cell_t *params) {
    OBJECT_HANDLE_MACRO
    switch (p->obj.type) {
        case MSGPACK_OBJECT_POSITIVE_INTEGER:
            return (cell_t)p->obj.via.u64;
        case MSGPACK_OBJECT_NEGATIVE_INTEGER:
            return (cell_t)p->obj.via.i64;
    };
    return 0;
}

static cell_t UnpackFloat(IPluginContext *pContext, const cell_t *params) {
    OBJECT_HANDLE_MACRO
    return sp_ftoc((float)p->obj.via.f64);
}

// obj, buffer, bufferlength
static cell_t UnpackRaw(IPluginContext *pContext, const cell_t *params) {
    OBJECT_HANDLE_MACRO
    
    char* sourcepawn_buffer;
    pContext->LocalToPhysAddr(params[2], (cell_t**)&sourcepawn_buffer);
    
    cell_t buffer_len = (cell_t) p->obj.via.bin.size;
    cell_t written = params[3] < buffer_len ? params[3] : buffer_len;
    
    memcpy(sourcepawn_buffer, p->obj.via.bin.ptr, written);
    return written;
}

static cell_t UnpackArray(IPluginContext *pContext, const cell_t *params) {
    OBJECT_HANDLE_MACRO
    
    // bounds and type checking
    if(p->obj.type != MSGPACK_OBJECT_ARRAY)
        return pContext->ThrowNativeError("Object is not array");
    if(params[2] < 0 || (size_t)params[2] >= p->obj.via.array.size)
        return pContext->ThrowNativeError("Array out of bounds");
    
    MsgPackObject* child = new MsgPackObject(p, p->obj.via.array.ptr[params[2]]);
    return g_pHandleSys->CreateHandle(g_type_msgpack_object, (void*)child, pContext->GetIdentity(), myself->GetIdentity(), NULL);    
}

static cell_t UnpackArrayCell(IPluginContext *pContext, const cell_t *params) {
    OBJECT_HANDLE_MACRO
    
    // bounds and type checking
    if(p->obj.type != MSGPACK_OBJECT_ARRAY)
        return pContext->ThrowNativeError("Object is not array: %i", p->obj.type);
    if(params[2] < 0 || (size_t)params[2] >= p->obj.via.array.size)
        return pContext->ThrowNativeError("Array out of bounds");

    auto target = p->obj.via.array.ptr[params[2]];
    switch(target.type) {
        case MSGPACK_OBJECT_POSITIVE_INTEGER:
            return (cell_t)target.via.u64;
        case MSGPACK_OBJECT_NEGATIVE_INTEGER:
            return (cell_t)target.via.i64;
        case MSGPACK_OBJECT_FLOAT:
        case MSGPACK_OBJECT_FLOAT32:
            return sp_ftoc((float)target.via.f64);
        case MSGPACK_OBJECT_BOOLEAN:
            return target.via.boolean;
        case MSGPACK_OBJECT_NIL:
            return 0;
        default:
            return pContext->ThrowNativeError("Object is not convertible to cell: %i", target.type);
    }
}

// obj, index
static cell_t UnpackKey(IPluginContext *pContext, const cell_t *params) {
    OBJECT_HANDLE_MACRO
    
    // bounds and type checking?
    if(p->obj.type != MSGPACK_OBJECT_MAP)
        return pContext->ThrowNativeError("Object is not map");
    if(params[2] < 0 || (size_t)params[2] >= p->obj.via.map.size)
        return pContext->ThrowNativeError("Map out of bounds");
    
    MsgPackObject* key = new MsgPackObject(p, p->obj.via.map.ptr[params[2]].key);    
    return g_pHandleSys->CreateHandle(g_type_msgpack_object, (void*)key, pContext->GetIdentity(), myself->GetIdentity(), NULL);
}

static cell_t UnpackValue(IPluginContext *pContext, const cell_t *params) {
    OBJECT_HANDLE_MACRO
    
    // bounds and type checking?
    if(p->obj.type != MSGPACK_OBJECT_MAP)
        return pContext->ThrowNativeError("Object is not map");
    if(params[2] < 0 || (size_t)params[2] >= p->obj.via.map.size)
        return pContext->ThrowNativeError("Map out of bounds");
    
    MsgPackObject* val = new MsgPackObject(p, p->obj.via.map.ptr[params[2]].val);
    return g_pHandleSys->CreateHandle(g_type_msgpack_object, (void*)val, pContext->GetIdentity(), myself->GetIdentity(), NULL);
}

const sp_nativeinfo_t MyNatives[] = {
	{"MsgPack_NewPack",	        NewPack},    
    {"MsgPack_PackInt",	        PackInt},
    {"MsgPack_PackFloat",	    PackFloat},
    {"MsgPack_PackNil",	        PackNil},
    {"MsgPack_PackTrue",	    PackTrue},
    {"MsgPack_PackFalse",	    PackFalse},
    {"MsgPack_PackArray",	    PackArray},
    {"MsgPack_PackMap",	        PackMap},
    {"MsgPack_PackRaw",	        PackRaw},
    
    {"MsgPack_GetPackSize",	    GetPackSize},
    {"MsgPack_GetPackBuffer",	GetPackBuffer},
    
    {"MsgPack_NewUnpack",       NewUnpack},
    {"MsgPack_UnpackPrint",     UnpackPrint},
    {"MsgPack_UnpackType",      UnpackType},
    {"MsgPack_UnpackCount",     UnpackCount},
    {"MsgPack_UnpackBool",      UnpackBool},
    {"MsgPack_UnpackInt",       UnpackInt},
    {"MsgPack_UnpackFloat",     UnpackFloat},
    {"MsgPack_UnpackRaw",       UnpackRaw},
    {"MsgPack_UnpackArray",     UnpackArray},
    {"MsgPack_UnpackKey",       UnpackKey},
    {"MsgPack_UnpackValue",     UnpackValue},
    {"MsgPack_UnpackNext",      UnpackNext},
    {"MsgPack_UnpackArrayCell", UnpackArrayCell},

    // deprecated names
    {"MsgPack_ObjectPrint",     UnpackPrint},
    {"MsgPack_ObjectType",      UnpackType},
    {"MsgPack_ObjectCount",     UnpackCount},
    {"MsgPack_ObjectBool",      UnpackBool},
    {"MsgPack_ObjectInt",       UnpackInt},
    {"MsgPack_ObjectFloat",     UnpackFloat},
    {"MsgPack_ObjectRaw",       UnpackRaw},
    {"MsgPack_ObjectArray",     UnpackArray},
    {"MsgPack_ObjectKey",       UnpackKey},
    {"MsgPack_ObjectValue",     UnpackValue},

	{NULL,			            NULL},
};

bool Sample::SDK_OnLoad(char *error, size_t maxlength, bool late) {
    sharesys->RegisterLibrary(myself, "msgpack");
    sharesys->AddNatives(myself, MyNatives);

    g_type_msgpack_object = g_pHandleSys->CreateType("MsgPack Object", &g_handle_handler, 0, NULL, NULL, myself->GetIdentity(), NULL);
    g_type_packer         = g_pHandleSys->CreateType("MsgPack Packer", &g_handle_handler, 0, NULL, NULL, myself->GetIdentity(), NULL);

    return true;
}

void Sample::SDK_OnAllLoaded() {
	
}

void Sample::SDK_OnUnload() {
    g_pHandleSys->RemoveType(g_type_msgpack_object, myself->GetIdentity());
    g_pHandleSys->RemoveType(g_type_packer, myself->GetIdentity());
}