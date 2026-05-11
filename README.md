## I'm going to rewrite this, there are certain parts of it that needs to be refactored.

### Rewrite progress

In `./kernel/` there is a hwmon driver, that can be used for controlling fan speed. It works with `fancontrol`

If you dont want to use fancontrol you can use motherboard sync by doing:

Set `/sys/class/hwmon/hwmon<some number>/pwm*_enable` to 2 for motherboard sync.

No RGB support rn, will add it back in at some point.

UI wont work currently because i havent gotten to it.
