#include "node.h"
#include "node_buffer.h"
#include "nan.h"

#include <dlfcn.h>

#include <stdlib.h>
#include <string.h>

namespace {

typedef struct {
  void* handle;
  char* errmsg;
} lib_t;

/**
 * This code is heavily based off of the `src/unix/dl.c` file in libuv.
 */

static int set_dlerror(lib_t* lib);

/**
 * dlopen()
 */

NAN_METHOD(Dlopen) {
  NanEscapableScope();

  const char *filename;
  if (args[0]->IsNull()) {
    filename = NULL;
  } else if (args[0]->IsString()) {
    v8::String::Utf8Value name(args[0]);
    filename = *name;
  } else {
    return NanThrowTypeError("a string filename, or null must be passed as the first argument");
  }

  v8::Local<v8::Object> buf = args[1].As<v8::Object>();
  lib_t *lib = reinterpret_cast<lib_t *>(node::Buffer::Data(buf));

  dlerror(); /* Reset error status. */

  lib->errmsg = NULL;

  // TODO: make RTLD_LAZY configurable
  lib->handle = dlopen(filename, RTLD_LAZY);

  NanReturnValue(NanNew<v8::Integer>(lib->handle ? 0 : set_dlerror(lib)));
}

/**
 * dlclose()
 */

NAN_METHOD(Dlclose) {
  NanEscapableScope();

  v8::Local<v8::Object> buf = args[0].As<v8::Object>();
  lib_t *lib = reinterpret_cast<lib_t *>(node::Buffer::Data(buf));

  if (lib->errmsg) {
    free(lib->errmsg);
    lib->errmsg = NULL;
  }

  if (lib->handle) {
    /* Ignore errors. No good way to signal them without leaking memory. */
    dlclose(lib->handle);
    lib->handle = NULL;
  }

  NanReturnUndefined();
}

/**
 * Callback function invoked when a dlsym() Buffer instance
 * gets garbage collected from JavaScript.
 */

void dlsym_cb(char *data, void *hint) {
}

/**
 * dlsym()
 */

NAN_METHOD(Dlsym) {
  NanEscapableScope();

  v8::Local<v8::Object> buf = args[0].As<v8::Object>();
  lib_t *lib = reinterpret_cast<lib_t *>(node::Buffer::Data(buf));

  v8::String::Utf8Value name(args[1]);

  dlerror(); /* Reset error status. */

  void *ptr = dlsym(lib->handle, *name);
  v8::Local<v8::Object> rtn = NanNewBufferHandle(reinterpret_cast<char *>(ptr), sizeof(void *), dlsym_cb, NULL);

  int r = set_dlerror(lib);
  if (r == 0) {
    NanReturnValue(rtn);
  } else {
    NanReturnValue(NanNew<v8::Integer>(r));
  }
}

/**
 * dlerror()
 */

NAN_METHOD(Dlerror) {
  NanEscapableScope();

  v8::Local<v8::Object> buf = args[0].As<v8::Object>();
  lib_t *lib = reinterpret_cast<lib_t *>(node::Buffer::Data(buf));

  NanReturnValue(NanNew<v8::String>(lib->errmsg ? lib->errmsg : "no error"));
}

/**
 * Sets the `errmsg` field of `lib` via dlerror().
 */

static int set_dlerror(lib_t* lib) {
  const char* errmsg;

  if (lib->errmsg)
    free(lib->errmsg);

  errmsg = dlerror();

  if (errmsg) {
    lib->errmsg = strdup(errmsg);
    return -1;
  }
  else {
    lib->errmsg = NULL;
    return 0;
  }
}

} // anonymous namespace

void init (v8::Handle<v8::Object> target) {
  NanScope();

  target->Set(NanNew<v8::String>("sizeof_lib_t"), NanNew<v8::Integer>(sizeof(lib_t)));
  target->Set(NanNew<v8::String>("sizeof_void_ptr"), NanNew<v8::Integer>(sizeof(void *)));

  NODE_SET_METHOD(target, "dlopen", Dlopen);
  NODE_SET_METHOD(target, "dlclose", Dlclose);
  NODE_SET_METHOD(target, "dlsym", Dlsym);
  NODE_SET_METHOD(target, "dlerror", Dlerror);
}
NODE_MODULE(binding, init);
