
/**
 * Module dependencies.
 */

var dl = require('../');
var ref = require('ref');
var glob = require('glob');
var path = require('path');
var assert = require('assert');

describe('dlfcn', function () {

  describe('Library', function () {

    it('should be exported directly', function () {
      assert('function' === typeof dl);
      assert('Library' === dl.name);
    });

    it('should return a `Library` instance', function () {
      var lib = new dl();
      assert(lib instanceof dl);
    });

    it('should not require the `new` operator', function () {
      var lib = dl();
      assert(lib instanceof dl);
    });

    describe('"current process" mode', function () {

      it('should dynamically open the "current process" address space', function () {
        var lib = dl();
        assert(lib instanceof dl);
        assert(null === lib.name);
      });

      it('should load the "strlen" function pointer symbol', function () {
        // strlen() is used in Node.js source code (node.cc),
        // so it should always be present.
        var lib = dl();
        var sym = lib.get('strlen');
        assert(!!sym);
        assert(Buffer.isBuffer(sym));
        assert(!ref.isNull(sym));
        assert(sym.length === 0);
      });

    });

    describe('libtest.c', function () {
      // find the built .so, .dylib, .dll shared library file.
      // historically, node-gyp tends to build the file in different places.
      var pattern = path.resolve(__dirname, '**', 'libtest.{so,dylib,dll}');
      var libtestPath = glob.sync(pattern)[0];

      it('should dynamically open the `libtest.c` shared library', function () {
        var lib = dl(libtestPath);
        assert(lib instanceof dl);
      });

      it('should load the "six" `int` symbol', function () {
        var lib = dl(libtestPath);
        var sym = lib.get('six', ref.sizeof.int);
        assert(!!sym);
        assert(Buffer.isBuffer(sym));
        assert(sym.length === ref.sizeof.int);

        // now dereference the buffer and assert the actual value
        sym.type = ref.types.int;
        assert(ref.deref(sym) === 6);
      });

      it('should load the "n" `void *` symbol', function () {
        var lib = dl(libtestPath);
        var sym = lib.get('n');
        assert(!!sym);
        assert(Buffer.isBuffer(sym));
        assert(sym.length === 0);

        // now dereference the buffer and assert that it is NULL
        sym.type = 'void*'
        var ptr = ref.deref(sym);
        assert(!!ptr);
        assert(Buffer.isBuffer(ptr));
        assert(ptr.length === 0);
        assert(ref.isNull(ptr));
      });

      it('should load the "str" `char []` symbol', function () {
        var lib = dl(libtestPath);
        var sym = lib.get('str');
        assert(!!sym);
        assert(Buffer.isBuffer(sym));
        assert(sym.length === 0);

        // since it's a char[], null-terminated string, then we need
        // to reinterpret the 0-length buffer until the null byte.
        var str = ref.reinterpretUntilZeros(sym, 1).toString('ascii');
        assert(str === 'hello world');
      });

      it('should load the "factorial" function pointer symbol', function () {
        var lib = dl(libtestPath);
        var sym = lib.get('factorial');
        assert(!!sym);
        assert(Buffer.isBuffer(sym));
        assert(!ref.isNull(sym));
        assert(sym.length === 0);
      });

      it('should load the "factorial_addr" `intptr_t` symbol', function () {
        var lib = dl(libtestPath);
        var sym = lib.get('factorial_addr', ref.sizeof.size_t);
        assert(!!sym);
        assert(Buffer.isBuffer(sym));
        assert(!ref.isNull(sym));
        assert(sym.length === ref.sizeof.size_t);

        // it should point to the value of the `factorial` function
        // location in memory
        sym.type = 'size_t';
        assert(ref.deref(sym) === ref.address(lib.get('factorial')));
      });

    });

  });

});
