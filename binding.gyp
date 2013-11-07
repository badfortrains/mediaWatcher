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
          './Platinum/Platinum/Source/Platinum',
          './Platinum/Platinum/Source/Core', 
          './Platinum/Platinum/Source/../../Neptune/Source/Core',
          './Platinum/Platinum/Source/Devices/MediaServer',
          './Platinum/Platinum/Source/Devices/MediaRenderer',
          './Platinum/Platinum/Source/Devices/MediaConnect',
          './Platinum/Platinum/Source/Extras'
      ],
    }
  ]
}
