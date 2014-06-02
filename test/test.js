
/**
 * Module dependencies.
 */

var dl = require('../');
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

  });

});
