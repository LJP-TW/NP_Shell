#!/usr/bin/env python3

with open('./testcase2', 'wb+') as f:
    f.write(b'echo "aa" ')
    for _ in range(1500):
        f.write(b'| cat ')
    f.write(b'\n')
    f.write(b'whoami')
