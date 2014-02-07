{
  'link_settings': {
  'library_dirs': [
    '/usr/local/lib/',
    '/usr/lib/'
   ],
  },
  'targets': [
    {
      'target_name': 'curler',
      'include_dirs': [
          '/usr/local/include/curl/',
          '/usr/include/curl/'
      ],
      'libraries': ['-lcurl'],
      'sources': ['src/curl_client.cc',
                  'src/curler.cc'],
      'cflags!': [ '-fno-exceptions' ],
      'cflags_cc!': [ '-fno-exceptions' ],
      'conditions': [
        ['OS=="mac"', {
          'xcode_settings': {
            'GCC_ENABLE_CPP_EXCEPTIONS': 'YES',
            'MACOSX_DEPLOYMENT_TARGET': '10.5',
          }
        }]
      ]
    }
  ]
}