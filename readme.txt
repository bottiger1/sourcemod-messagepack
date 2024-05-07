How to use

1. Download the binary by clicking Releases on the right on github.
2. Put it in sourcemod/extensions.
3. Copy sourcepawn/msgpack.inc to your sourcemod/scripting/includes folder.
4. You can see an example of how to include it in sourcepawn/msgpack-tests.sp

How to compile

1. Install ambuild.
2. Edit the build/build file and fix the library paths.
3. Run the build file.

Updating MessagePack

https://github.com/msgpack/msgpack-c/releases

rename folder to msgpack

then do "cmake ." and then do make which is needed to create additional header files

Current msgpack version: https://github.com/msgpack/msgpack-c/releases/tag/c-6.0.1
