
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
 * @param {RTLD} mode - RTLD constant "mode" to open the library as (`RTLD_LAZY` by default)
 * @class
 * @public
 */

function Library (name, mode) {
  if (!(this instanceof Library)) return new Library(name, mode);

  if (name) {
    // append the `ext` if necessary
    var ext = exports.ext[process.platform];
    if (name.substring(name.length - ext.length) !== ext) {
      debug('appending dynamic lib suffix (%o) to %o', ext, name);
      name += ext;
    }
  } else {
    // if no name was passed in then pass `null` to open the current process
    name = null;
  }

  debug('library name: %o, mode: %o', name, mode);
  this.name = name;

  // create the `lib_t` data space
  this.lib_t = new Buffer(bindings.sizeof_lib_t);

  // do the `dlopen()` dance
  var r = bindings.dlopen(name, this.lib_t, mode);
  debug('dlopen(): %o', r);
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
  // allow passing `cb` as the second argument, with 0 byte-length
  if ('function' === typeof size) {
    cb = size;
    size = 0;
  } else if ('number' !== typeof size) {
    size = 0;
  }

  debug('get(%o, %o, %o)', name, size, cb);

  var sym = bindings.dlsym(this.lib_t, name, size, cb);
  debug('dlsym(): %o', sym);

  if ('number' === typeof sym) {
    // error
    throw new Error(bindings.dlerror(this.lib_t));
  }

  // add some debugging info
  sym.name = name;
  sym.lib = this;

  return sym;
};
