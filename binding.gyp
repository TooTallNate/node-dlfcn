{
  'targets': [
    {
      'target_name': 'binding',
      'sources': [ 'src/binding.cc' ],
      'include_dirs': [
        '<!(node -e "require(\'nan\')")'
      ],
      'conditions': [
        ['OS=="win"', {
          'dependencies': [
            'deps/dlfcn-win32/dlfcn.gyp:dlfcn'
          ],
        }, { # OS!="win"
          'libraries': [
            '-ldl'
          ]
        }]
      ]
    }
  ]
}
