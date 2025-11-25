import sys
from typing import List
from lark import Lark, Transformer, Tree, Token, Visitor

from AmoSumParser.ValidatorAmoSum import ValidatorAmoSum
from AmoSumParser.utils import concatenate_nodes
from settings import *


class AspFactory():

    AMOSUM_FUNCTION = "#amosum"
    EOSUM_FUNCTION = "#esum"

    def create_classical_literal(predicate: str, *args: List[Tree | Token]):
        data = Token("RULE","classical_literal") # data
        children = [Token("ID",predicate)] # predicate
        
        if len(args) > 0:
            children += [Token("PAREN_OPEN", "(")]
            children += concatenate_nodes(*args)
            children += [Token("PAREN_CLOSE", ")")]

        atom = Tree(data, children)
        return atom
    
    def create_tuple_terms(*terms: List[Tree | Token]):
        data = Token("RULE","terms") # data
        children = [] 
        
        if len(terms) > 0:
            children += [Token("PAREN_OPEN", "(")]
            children += concatenate_nodes(*terms)
            children += [Token("PAREN_CLOSE", ")")]

        atom = Tree(data, children)
        return atom
    
    def create_fact(classical_literal: Tree):
        fact = [ Tree(Token("RULE","statement"), 
               [ Tree(Token("RULE","head"), [Tree(Token("RULE", "disjunction"),[classical_literal])]),
                 Token("DOT", ".")]) ]
        return fact
    
    def create_head_choice(choice_elements):

        head_choice = Tree(Token("RULE","head"), 
                           [Tree(Token("RULE", "choice"),
                                 [Token("CURLY_OPEN","{"), choice_elements, Token("CURLY_CLOSE","}")])])
        return head_choice
    
    def create_rule(head, body):
        

        rule = [head, Token("CONS",":-") if not body is None else []] if not head is None else [Token("CONS",":-")] 
        rule += [body, Token("DOT", ".")] if not body is None else [Token("DOT", ".")]
        rule = Tree(Token("RULE","statement"), rule)

        return rule
    
    def create_token(type: str, value: str):
        return Token(type, str(value))
    
    def create_terminal_term(type, value):
        return  Tree("term", [Token(type, str(value))])

    
    def create_amosum_rules(bound: Tree, binop: Tree, aggregate_id: Tree, aggregate_function: str, body = None, ):
        assert ValidatorAmoSum.checkIsValidBinop(binop)

        op = binop.children[0]
        predicate_name : str
        
        prop_type, predicate_name = AspFactory.get_pred_name_prop_type(binop=binop, aggregate_function=aggregate_function)
        
        # term_aggregate_id
        head = AspFactory.create_classical_literal(predicate_name, bound, aggregate_id)
        bound_rule =  AspFactory.create_rule(head=head, body=body)

        # amosum_literal = AspFactory.create_classical_literal(AspFactory.AMOSUM, aggregate_id, AspFactory.create_token("STRING", prop_type))
        # amosum_rule = AspFactory.create_rule(amosum_literal, body)

        return {"bound_rule": bound_rule}
    
    def get_pred_name_prop_type(binop, aggregate_function: Tree):
        op = binop.children[0]
        predicate_name : str
        prop_type : str
        amo = str(aggregate_function.children[0]) == AspFactory.AMOSUM_FUNCTION 
        if op.type == "GREATER_OR_EQ":
            predicate_name = PREDICATE_LB
            prop_type = "ge_amo" if amo else "ge_eo"
        elif op.type == "LESS_OR_EQ":
            predicate_name = PREDICATE_UB
            prop_type = "le_eo"
        else:
            assert False
        return prop_type, predicate_name