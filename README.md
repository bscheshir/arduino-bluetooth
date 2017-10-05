# arduino-bluetooth
Scatch for connect BC-06 to Arduino Nano v3

For baby bicycle... V2

Also you can use [bscheshir/volume-control](https://github.com/bscheshir/volume-control)
```ino
#include <VolumeControl.h>
#define VOL A0
void setVolume(int volume) {
  Serial.println((String)"Volume set to " + volume);
  mp3_set_volume(volume);
}
VolumeControl vc(VOL, 500, &setVolume);
...
void loop() {
  vc.update(millis());
  ...
}
```
