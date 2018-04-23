#!/usr/bin/env python3

import pydbus

bus = pydbus.SystemBus()
intf = bus.get('org.gpiod')
objs = intf.GetManagedObjects()

for obj in objs.keys():
    chip = bus.get('org.gpiod', obj)
    print('{} [{}] ({} lines)'.format(chip.Name,
                                      chip.Label,
                                      chip.NumLines))
