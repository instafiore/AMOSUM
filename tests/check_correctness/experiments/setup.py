import re
import xml.etree.ElementTree as ET
import sys

REGEX_INSTANCE = r"\('(?P<folder>.*)\/(?P<instance>[^\/]+)\.asp"
REGEX_SAT = r"Model 1: {(?P<answerset>.*)}"
REGEX_FINDALL_ANSWERSET = r"'(\w+\(?[\w\"\-,]*\)?)'"