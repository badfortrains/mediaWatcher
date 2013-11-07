{
  "targets": [
    {
      "target_name": "media_watcher",
      "sources": [ "media_watcher.cc" ],
      'link_settings': {
        'libraries': [
              '../libZlib.a','../libMicro.a','../libPlatinum.a','../libNeptune.a','-lpthread','../libPltMediaRenderer.a','../libPltMediaConnect.a','../libPltMediaServer.a','../libaxTLS.a'
          ]
      },
      'include_dirs': [
          '/',
          '../',
          './node_modules/Platinum/Platinum/Source/Platinum',
          './node_modules/Platinum/Platinum/Source/Core', 
          './node_modules/Platinum/Platinum/Source/../../Neptune/Source/Core',
          './node_modules/Platinum/Platinum/Source/Devices/MediaServer',
          './node_modules/Platinum/Platinum/Source/Devices/MediaRenderer',
          './node_modules/Platinum/Platinum/Source/Devices/MediaConnect',
          './node_modules/Platinum/Platinum/Source/Extras'
      ],
    }
  ]
}
