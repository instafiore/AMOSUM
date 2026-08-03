#!/usr/bin/python3

import re
import xml.etree.ElementTree as ET
import sys
import os
from os import system
from amosum.utility import *

def cat(filename):
    with open(filename, 'r') as file:
        return file.read()
    
def main():

    param = parse_args_check()
    checkAmomaximize(param)

if __name__ == "__main__":
    main()