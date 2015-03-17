#include "node.h"
#include "node_buffer.h"
#include "nan.h"

#include <dlfcn.h>

#include <limits.h>
#include <stdint.h>
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

  if (!args[0]->IsNull() && !args[0]->IsString()) {
    return NanThrowTypeError("a string filename, or null must be passed as the first argument");
  }

  v8::Local<v8::Object> buf = args[1].As<v8::Object>();
  lib_t *lib = reinterpret_cast<lib_t *>(node::Buffer::Data(buf));

  dlerror(); /* Reset error status. */

  lib->errmsg = NULL;

  /* figure out which RTLD "mode" to use */
  int mode;
  if (args[2]->IsNumber()) {
    int64_t orig = args[2]->IntegerValue();
    if (orig > INT_MAX || orig < INT_MIN)
      return NanThrowRangeError("Mode exceeds the range of C int");
    mode = orig;
  } else {
    mode = RTLD_LAZY;
  }

  if (args[0]->IsNull()) {
    lib->handle = dlopen(NULL, mode);
  } else if (args[0]->IsString()) {
    v8::String::Utf8Value name(args[0]);
    lib->handle = dlopen(*name, mode);
  }

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
  NanScope();

  NanCallback* callback = reinterpret_cast<NanCallback *>(hint);
  if (callback) {
    callback->Call(0, NULL);
    delete callback;
  }
}

/**
 * dlsym()
 */

NAN_METHOD(Dlsym) {
  NanEscapableScope();

  size_t size = 0;
  NanCallback* callback = NULL;
  v8::Local<v8::Value> rtn;

  v8::Local<v8::Object> buf = args[0].As<v8::Object>();
  lib_t *lib = reinterpret_cast<lib_t *>(node::Buffer::Data(buf));

  v8::String::Utf8Value name(args[1]);

  dlerror(); /* Reset error status. */

  void *ptr = dlsym(lib->handle, *name);

  int r = set_dlerror(lib);
  if (r == 0) {

    if (args[2]->IsNumber()) {
      /* explicit byte length was given. */
      size = static_cast<size_t>(args[2]->IntegerValue());
    }

    if (args[3]->IsFunction()) {
      /* GC callback function was given. */
      callback = new NanCallback(args[3].As<v8::Function>());
    }

    rtn = NanNewBufferHandle(reinterpret_cast<char *>(ptr), size, dlsym_cb, callback);
  } else {
    rtn = NanNew<v8::Integer>(r);
  }

  NanReturnValue(rtn);
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

/**
 * Callback function invoked when a dlsym() Buffer instance
 * gets garbage collected from JavaScript.
 */

void rtld_buffer_cb(char *data, void *hint) {
  /* this shouldn't really ever happen... */
}

} // anonymous namespace

void init (v8::Handle<v8::Object> target) {
  NanScope();

  target->Set(NanNew<v8::String>("sizeof_lib_t"), NanNew<v8::Integer>(static_cast<uint32_t>(sizeof(lib_t))));

#define CONST_INT(value) \
  target->ForceSet(NanNew<v8::String>(#value), \
      NanNew<v8::Integer>(static_cast<uint32_t>(value)), \
      static_cast<v8::PropertyAttribute>(v8::ReadOnly|v8::DontDelete));

#ifdef RTLD_LAZY
  CONST_INT(RTLD_LAZY);
#endif

#ifdef RTLD_NOW
  CONST_INT(RTLD_NOW);
#endif

#ifdef RTLD_LOCAL
  CONST_INT(RTLD_LOCAL);
#endif

#ifdef RTLD_GLOBAL
  CONST_INT(RTLD_GLOBAL);
#endif

#ifdef RTLD_NOLOAD
  CONST_INT(RTLD_NOLOAD);
#endif

#ifdef RTLD_NODELETE
  CONST_INT(RTLD_NODELETE);
#endif

#ifdef RTLD_FIRST
  CONST_INT(RTLD_FIRST); /* Mac OS X 10.5 and later */
#endif

#undef CONST_INT


#define CONST_BUFFER(value) \
  target->ForceSet(NanNew<v8::String>(#value), \
      NanNewBufferHandle(reinterpret_cast<char *>(value), 0, rtld_buffer_cb, NULL), \
      static_cast<v8::PropertyAttribute>(v8::ReadOnly|v8::DontDelete));

/*
 * Special handle arguments for dlsym().
 */
#ifdef RTLD_NEXT
  CONST_BUFFER(RTLD_NEXT); /* Search subsequent objects. */
#endif

#ifdef RTLD_DEFAULT
  CONST_BUFFER(RTLD_DEFAULT); /* Use default search algorithm. */
#endif

#ifdef RTLD_SELF
  CONST_BUFFER(RTLD_SELF); /* Search this and subsequent objects (Mac OS X 10.5 and later) */
#endif

#ifdef RTLD_MAIN_ONLY
  CONST_BUFFER(RTLD_MAIN_ONLY); /* Search main executable only (Mac OS X 10.5 and later) */
#endif

#undef CONST_BUFFER


  NODE_SET_METHOD(target, "dlopen", Dlopen);
  NODE_SET_METHOD(target, "dlclose", Dlclose);
  NODE_SET_METHOD(target, "dlsym", Dlsym);
  NODE_SET_METHOD(target, "dlerror", Dlerror);
}
NODE_MODULE(binding, init);
