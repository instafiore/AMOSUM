

from typing import List

from lark import Token, Tree

from amosum_parser.AmoSumGrammar import *
from amosum_parser.utils import debug


class ValidatorAmoSum:
    
    def thereIsAmoSum(children: List[Tree | Token]) -> bool:

        for child in children:
            if isinstance(child, Token):
                continue
            amosum_aggregate_list = list(child.find_data(AMOSUM))
            if len(amosum_aggregate_list) > 0:
                return True
        return False
    
    def thereIsAmoMaximize(children: List[Tree | Token]) -> bool:

        for child in children:
            if isinstance(child, Token):
                continue
            amosum_aggregate_list = list(child.find_data(AMOMAXIMIZE))
            if len(amosum_aggregate_list) > 0:
                return True
        return False

    def isConstraint(children: List[Tree | Token]) -> bool:
        if len(children) != 3:
            return False
        try:
            isAConstraint = children[0].type == CONS and children[1].data == BODY and children[2].type == DOT
            return isAConstraint
        except Exception as e:
            debug(e)
            return False
        
    def checkNode(node: Tree | Token, valid_values: List):
        if isinstance(node, Token):
            return node.type in valid_values
        else:
            return node.data in valid_values
        
    def checkIsValidBinop(binop: Tree):
        assert isinstance(binop, Tree)
        token = binop.children[0]
        return ValidatorAmoSum.checkNode(token, valid_values=["GREATER_OR_EQ", "LESS_OR_EQ"])
    
    def isABuiltInAtom(node: Tree):
        if isinstance(node, Token):
            return False
        return node.data == "builtin_atom"

    

class ValidatorGeAmo(ValidatorAmoSum):
    pass