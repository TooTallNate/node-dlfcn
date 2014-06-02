
/**
 * Module dependencies.
 */

var debug = require('debug')('dlfcn');
var bindings = require('bindings')('binding');

/**
 * Module exports.
 */

exports = module.exports = Library;

/**
 * Re-export the *raw* dl bindings.
 */

Object.keys(bindings).forEach(function (key) {
  exports[key] = bindings[key];
});

/**
 * Map of `process.platform` values to their corresponding
 * "dynamic library" file name extension.
 */

exports.ext = {
  linux:   '.so',
  linux2:  '.so',
  sunos:   '.so',
  solaris: '.so',
  freebsd: '.so',
  openbsd: '.so',
  darwin:  '.dylib',
  mac:     '.dylib',
  win32:   '.dll'
};

/**
 * The `Library` class is an object-oriented wrapper around the
 * dlopen(), dlclose(), dlsym() and dlerror() functions.
 *
 * @param {String} name - Library name or full filepath
 * @api private
 */

function Library (name) {
  if (!(this instanceof Library)) return new Library(name);

  if (name) {
    // append the `ext` if necessary
    var ext = exports.ext[process.platform];
    if (name.substring(name.length - ext.length) !== ext) {
      debug('appending dynamic lib suffix (%s)', ext, name);
      name += ext;
    }
  } else {
    // if no name was passed in then pass `null` to open the current process
    name = null;
  }

  debug('library name', name);
  this.name = name;

  // create the `lib_t` data space
  this.lib_t = new Buffer(bindings.sizeof_lib_t);

  // do the `dlopen()` dance
  var r = bindings.dlopen(name, this.lib_t);
  debug('dlopen() result', r);
  if (0 !== r) {
    // error
    throw new Error(bindings.dlerror(this.lib_t));
  }
}

/**
 * Calls `dlclose()` on this Library instance.
 *
 * @api public
 */

Library.prototype.close = function () {
  debug('close()');
  if (this.lib_t) {
    bindings.dlclose(this.lib_t);
    this._lib_t = null;
  }
};

/**
 * Calls `dlsym()` on this Library instance.
 *
 * A Node.js `Buffer` instance is returned which points to the
 * memory location of the requested "symbol".
 *
 * @param {String} name - Symbol name to attempt to retrieve
 * @param {Number} [size] - Optional byte length of the returned Buffer instance. Defaults to 0.
 * @param {Function} [cb] - Optional callback function to invoke when the Buffer instance is garbage collected from JavaScript land.
 * @return {Buffer} a Buffer instance pointing to the memory address of the symbol
 * @api public
 */

Library.prototype.get = function (name, size, cb) {
  debug('get()', name);

  // allow passing `cb` as the second argument, with 0 byte-length
  if ('function' === typeof size) {
    cb = size;
    size = 0;
  }

  var sym = bindings.dlsym(this.lib_t, name, size, cb);
  debug('dlsym() result', sym);

  if ('number' === typeof sym) {
    // error
    throw new Error(bindings.dlerror(this.lib_t));
  }

  // add some debugging info
  sym.name = name;
  sym.lib = this;

  return sym;
};
