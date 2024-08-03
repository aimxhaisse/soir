#!/usr/bin/env python3

import inspect

def x():
    print('1')

print(f'{inspect.getsource(x)}')

def x():
    print('1')

print(f'{inspect.getsource(x)}')

def x():
    """Hello
    """
    print('2')

print(f'{inspect.getsource(x)}')

def x():
    """Hello
    """
    # OK
    print('2')

print(f'{inspect.getsource(x)}')
