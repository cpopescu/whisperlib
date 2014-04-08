#!/usr/bin/python

import unicodedata

crt = 0
sz = 0
line = 0
s = '    '
numbers = 0
spaces = []
for c in xrange(0, 0x10000):
    cat = unicodedata.category(unichr(c))
    if (cat[0] == 'L'):
        crt |= 1 << sz;
    elif (cat[0] == 'N'):
        crt |= 1 << sz;
        numbers += 1
    elif (cat[0] == 'Z'):
        spaces.append(unichr(c))
    sz += 1
    if sz == 8:
        s += '0x%02x, ' % crt
        sz = 0
        crt = 0
        line += 1
        if line == 16:
            line = 0
            print s
            s = '    '

print "NUMBERS: ", numbers
print "SPACES: ", len(spaces)
print spaces
