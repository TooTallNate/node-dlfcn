
0.0.2 / 2014-06-11
==================

  * deps: update dlfcn-win32 to revision 38
  * package: update "ref" to v0.3.0
  * index: default the `size` var to 0 always
  * index: use %o for debug() now that we're on v1.0
  * README: add Travis-CI badge
  * src: ensure the V8 Utf8Value remains valid memory
  * package: update "debug" to v1.0.1
  * binding: add "libraries" array to link to -ldl
  * appveyor: disable PowerShell `npm test` for now
  * binding: force the product extension to dylib, dll, so
  * test: use "glob" to find the shared library file

0.0.1 / 2014-06-07
==================

  * add `.travis.yml` file
  * add `appveyor.yml` file for Windows testing
  * test: initial test cases
  * make `dlsym()` accept a byte length and callback
  * make `dlsym()` return a Buffer instance directly
  * add README.md file
  * initial commit
