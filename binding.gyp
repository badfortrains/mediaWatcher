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
          '../../Platinum',
          '../../Core', 
          '../../../../Neptune/Source/Core',
          '../../Devices/MediaServer',
          '../../Devices/MediaRenderer',
          '../../Devices/MediaConnect',
          '../../Extras'
      ],
    }
  ]
}
