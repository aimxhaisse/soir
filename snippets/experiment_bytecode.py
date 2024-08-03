#!/usr/bin/env python3

import hashlib


def _hash(func: callable) -> str:
    h = hashlib.sha1()
    h.update(func.__code__.co_code)
    print(func.__code__.co_consts)
    #h.update(func.__code__.co_consts)
    return h.hexdigest()


def x():
    print('hello_world')


print(f'{_hash(x)}')

    
def x():
    print('hello_world')

print(f'{_hash(x)}')

def x():
    """A comment.
    """
    print('hello_world')

print(f'{_hash(x)}')

def x():
    # Another comment
    print('hello_world')

print(f'{_hash(x)}')

def x():
    print('for some reason bytecode is the same even if raw parameter differs')

print(f'??? {_hash(x)}')

def x():
    print('hello_world_2')
    print('hello_world')

print(f'{_hash(x)}')

print(f'{x.__dir__()}')
print(f'{x.__hash__()}')
print(f'{x.__hash__()}')
print(f'{x.__hash__()}')

def x():
    print('hello_world_2')
    print('hello_world')

print(f'{x.__hash__()}')

def x():
    print('hello_world_3')
    print('hello_world')

print(f'{x.__hash__()}')
