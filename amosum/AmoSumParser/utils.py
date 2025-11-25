import sys
import re
from lark import *


# DEBUGGING VARIABLE
DEBUG = False

def debug(*message: str, end ="\n", force_print = False, file = sys.stderr):
    if force_print or DEBUG:
        print(message, end=end, file=file)

def is_leaf(node) -> bool:
        return isinstance(node, Token)

def pick_n_subtree(root:Tree, data: str, n) -> Tree:
    '''
    returns: n-th tree with tree.data=data, following the pretty-like order
    '''
    count = 0
    for node in root.iter_subtrees_topdown():
        if node.data == data:
            count += 1
            if count == n:
                return node
    return None

def remove_adjacents_comma(tree: Tree, precedent_comma = False):

    new_tree = Tree()
    new_tree.data = tree.data
    children = []
    for child in tree.children:
        if isinstance(child, Tree):
            pass
    

def concatenate_nodes(*nodes):
        
        # nodesList = [n for n in nodes if not n is None and (isinstance(n, Token) or not (n.data == "body" and len(n.children) == 0))]
        nodesList = list(nodes)
        res = []
        for n in nodesList[:len(nodesList)-1]:
            res += [n, Token("COMMA", ",")] if not n is None else []
        res += [nodes[-1]] if not [nodes[-1]] is None else []

        return res

def toString(parsed) -> str:

    output = ""

    def visit_tree(tree: Tree):
        nonlocal output
        if is_leaf(tree):
            string = str(tree)
            end = "" if string != "." and string != "not" else "\n" if string == "." else " "
            output += f"{tree}{end}"
        else:
            if isinstance(tree, list):
                for child in tree:
                    visit_tree(child)
            else:
                for child in tree.children:
                    visit_tree(child)
    
    def visit_tree_iterative(tree: Tree):
        nonlocal output
        stack = [tree]
          # Start with the root tree in the stack
        while stack:
            current = stack.pop()
            # print(f"current: {current}")
            if is_leaf(current):
                string = str(current)
                end = "" if string != "." and string != "not" else "\n" if string == "." else " "
                output += f"{current}{end}"
            else:
                if isinstance(current, list):  # If the current node is a list of children
                    stack.extend(reversed(current))  # Add children to the stack in reverse order to maintain left-to-right traversal
                else:
                    stack.extend(reversed(current.children))  # Add children to the stack in reverse order to maintain left-to-right traversal


    visit_tree_iterative(tree=parsed)

    output = re.sub(r",+", ",", output)
    output = re.sub(":-,", ":-", output)
    output = re.sub(r"\(\s*,", "(", output)
    output = re.sub(r",\s*\)", ")", output)

    output = re.sub(",", ", ", output)
    return output