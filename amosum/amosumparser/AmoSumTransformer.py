import sys
from typing import List
from lark import Lark, Transformer, Tree, Token, Visitor
from amosum.amosumparser.utils import debug
from amosum.amosumparser import AspFactory
from amosum.amosumparser.ValidatorAmoSum import ValidatorAmoSum
from amosum.amosumparser.AmoSumGrammar import *
from amosum.amosumparser.utils import *
from amosum.amosumparser.AspFactory import AspFactory
from amosum.amosumparser.settings import *

# auxiliary atom of an element: (aggregate id, terms tuple, group), true iff one of the conditions with that tuple in that group is true
PREDICATE_ELEMENT = "__element__"

class AmoSumTransformer(Transformer):

    
    
    def __init__(self, visit_tokens = True):
        super().__init__(visit_tokens=visit_tokens)
        self.aggregate_id_autoincrement = 0


    def element_rules(self, aggregate_id, terms, group_id, element_body):
        '''
        returns: (element_literal, [element_rule])
        element_literal is identified by (aggregate_id, terms tuple, group): elements of a group with the same tuple are one
        atom (ASP set semantics); the grounder (AmosumGrounder) then counts a tuple of several groups once and makes the
        elements of a group with the same weight one atom
        '''
        element_literal = AspFactory.create_classical_literal(PREDICATE_ELEMENT, aggregate_id, AspFactory.create_tuple_terms(terms), group_id)
        element_rule = AspFactory.create_rule(head=element_literal, body=concatenate_nodes(*element_body) if element_body else None)
        debug(f"element_rule: {toString(element_rule)}")
        return element_literal, [element_rule]


    def amosum_statement(self, children: List[Tree | Token]) -> List[Tree | Token]:
        # assert ValidatorAmoSum.isConstraint(children)
        statement_t = Tree(data="statement",children=children)
        amosum_rule = children[0]
        children = amosum_rule.children
        body = children[2] if len(children) > 1 else Tree(data="body", children=[])
        # amosum_list = list(amosum_rule.find_data(AMOSUM))
        body_except_amosum = body
        debug(f"body_except_amosum: {body_except_amosum}")
        body = body_except_amosum if len(body.children) > 0 else None 
        body_without_builtin_atoms = DiscardBuiltInAtoms().transform(body) if body is not None else None
        # aggregate id: (counter, body without builtin atoms), or (counter) when the statement has no body
        aggregate_id_autoincerment_term = AspFactory.create_terminal_term(type="NUMBER", value=self.aggregate_id_autoincrement)
        aggregate_id = AspFactory.create_tuple_terms(aggregate_id_autoincerment_term, *([body_without_builtin_atoms] if body is not None else []))
        debug(f"aggregate_id: {toString(aggregate_id)}")
        result = []
        amosum = children[0]
        amosum_function = amosum.children[0]
        binop = amosum.children[-2]
        bound = amosum.children[-1]
        amosum_aggregate_element_list = amosum.find_data(AMOSUM_ELEMENT)
        for amosum_aggregate_element in amosum_aggregate_element_list:
            debug(f"aggregate element: {amosum_aggregate_element}")
            terms = amosum_aggregate_element.children[0]
            debug(f"terms element: {terms}")

            term_weight = pick_n_subtree(amosum_aggregate_element, "term",1)
            debug(f"term_weight: {term_weight}")

            amosum_identifier = amosum_aggregate_element.children[-1]
            group_id = pick_n_subtree(amosum_identifier, "terms", 1)
            debug(f"group_id before paren: {group_id}")
            group_id = AspFactory.create_tuple_terms(group_id)
            debug(f"group_id: {group_id}")

            # None for the elements without condition (terms [G] and terms : [G])
            naf_literals = pick_n_subtree(amosum_aggregate_element, "naf_literals", 1)
            element_body = [n for n in (naf_literals, body) if n is not None]

            element_literal, element_rules = self.element_rules(aggregate_id, terms, group_id, element_body)
            result += element_rules

            group_classical_literal = AspFactory.create_classical_literal(PREDICATE_GROUP, element_literal, AspFactory.create_terminal_term("STRING", '"+"'), term_weight, group_id, aggregate_id)
            debug(f"group_literal_classical_literal: {group_classical_literal}")

            head_choice_group = AspFactory.create_head_choice(group_classical_literal)
            choice_group = AspFactory.create_rule(head=head_choice_group, body=concatenate_nodes(*element_body) if element_body else None)
            debug(f"choice_group: {choice_group}")
            result.append(choice_group)
            
            debug("")

        amosum_rules = AspFactory.create_amosum_rules(bound, binop, aggregate_id, body = body, aggregate_function = amosum_function)
        bound_rule = amosum_rules["bound_rule"]
        debug(f"bound_rule: {bound_rule}")
        result.append(bound_rule)

        prop_type, predicate_name = AspFactory.get_pred_name_prop_type(binop=binop, aggregate_function=amosum_function)
        aux_literal = AspFactory.create_classical_literal(PREDICATE_AUX, aggregate_id,  AspFactory.create_token("STRING", prop_type))
        aux_rule = AspFactory.create_rule(aux_literal, body=body)
        result.append(aux_rule)

        # the aux element is a group of its own: its id is the aux literal itself, so it can't collide with the ids of the
        # element groups, with or without a body
        group_id_aux = AspFactory.create_tuple_terms(aux_literal)
        group_aux_classical_literal = AspFactory.create_classical_literal(PREDICATE_GROUP, aux_literal,  AspFactory.create_terminal_term("STRING", '"-"'), bound, group_id_aux, aggregate_id)
        head_choice_group_aux = AspFactory.create_head_choice(group_aux_classical_literal)
        group_aux_rule = AspFactory.create_rule(head=head_choice_group_aux, body=body)
        result.append(group_aux_rule)

        # amosum_rule = amosum_rules["amosum_rule"]
        # result.append(amosum_rule)

        debug(f"body: {toString(body) if body is not None else ''}")

        debug("")
        
        self.aggregate_id_autoincrement += 1
        

        # print(toString(result))
        return result
    

    def amomaximize_statement(self, children: List[Tree | Token]) -> List[Tree | Token]:
        # assert ValidatorAmoSum.isConstraint(children)
        statement_t = Tree(data="statement",children=children)
        amomaximize_rule = children[0]
        children = amomaximize_rule.children
        body = children[2] if len(children) > 1 else Tree(data="body", children=[])
        # amosum_list = list(amosum_rule.find_data(AMOSUM))
        body_except_amosum = body
        debug(f"body_except_amosum: {body_except_amosum}")
        body = body_except_amosum if len(body.children) > 0 else body 
        body_without_builtin_atoms = DiscardBuiltInAtoms().transform(body)
        # body_without_builtin_atoms = body
        # aggregate id: the same for every element (an empty body is dropped by toString)
        aggregate_id_autoincerment_term = AspFactory.create_terminal_term(type="NUMBER",value="__amomaximizeid__")
        aggregate_id = AspFactory.create_tuple_terms(aggregate_id_autoincerment_term, body_without_builtin_atoms)
        debug(f"aggregate_id: {aggregate_id}")
        result = []
        amosum = children[0]
        amosum_function = amosum.children[0]
        amosum_aggregate_element_list = amosum.find_data(AMOSUM_ELEMENT)
        
        for amosum_aggregate_element in amosum_aggregate_element_list:
            debug(f"aggregate element: {amosum_aggregate_element}")

            terms = amosum_aggregate_element.children[0]
            debug(f"terms element: {terms}")

            term_weight = pick_n_subtree(amosum_aggregate_element, "term",1)
            debug(f"term_weight: {term_weight}")

            amosum_identifier = amosum_aggregate_element.children[-1]
            group_id = pick_n_subtree(amosum_identifier, "terms", 1)
            debug(f"group_id before paren: {group_id}")
            group_id = AspFactory.create_tuple_terms(group_id)
            debug(f"group_id: {group_id}")

            # None for the elements without condition (terms [G] and terms : [G])
            naf_literals = pick_n_subtree(amosum_aggregate_element, "naf_literals", 1)
            element_body = [naf_literals] if naf_literals is not None else []
            # element_body = [n for n in (naf_literals, body) if n is not None]

            element_literal, element_rules = self.element_rules(aggregate_id, terms, group_id, element_body)
            result += element_rules

            group_classical_literal = AspFactory.create_classical_literal(PREDICATE_GROUP, element_literal, AspFactory.create_terminal_term("STRING", '"+"'), term_weight, group_id, aggregate_id)
            debug(f"group_literal_classical_literal: {group_classical_literal}")

            
            
            head_choice_group = AspFactory.create_head_choice(group_classical_literal)
            choice_group = AspFactory.create_rule(head=head_choice_group, body=concatenate_nodes(*element_body) if element_body else None)
            debug(f"choice_group: {choice_group}")

            
            result.append(choice_group)
            
            debug("")
        
            # binop = amosum.children[-2]
            # bound = amosum.children[-1]

        binop = Tree("binop", [Token("GREATER_OR_EQ", ">=")])
        # bound = Tree("term", [Token("NUMBER", "478671")])
        bound = Tree("term", [Token("NUMBER", "0")])
        # bound = Tree("term", [Token("TERM", "X")])
        amosum_rules = AspFactory.create_amosum_rules(bound, binop, aggregate_id, body = None, aggregate_function = amosum_function)
        bound_rule = amosum_rules["bound_rule"]
        debug(f"bound_rule: {bound_rule}")
        result.append(bound_rule)

        # prop_type, predicate_name = AspFactory.get_pred_name_prop_type(binop=binop, aggregate_function=amosum_function)
        aux_literal = AspFactory.create_classical_literal(PREDICATE_AUX, aggregate_id,  AspFactory.create_token("STRING", AMOMAXIMIZE))
        # aux_literal = AspFactory.create_classical_literal(PREDICATE_AUX, aggregate_id,  AspFactory.create_token("STRING", PROPAGATOR_NAME_ge_amo))
        aux_rule = AspFactory.create_rule(aux_literal, body=None)
        result.append(aux_rule)

        # group_id_aux = AspFactory.create_tuple_terms(body_without_builtin_atoms)
        # group_aux_classical_literal = AspFactory.create_classical_literal(PREDICATE_GROUP, aux_literal,  AspFactory.create_terminal_term("STRING", '"-"'), bound, group_id_aux, aggregate_id)
        # head_choice_group_aux = AspFactory.create_head_choice(group_aux_classical_literal)
        # group_aux_rule = AspFactory.create_rule(head=head_choice_group_aux, body=body)
        # result.append(group_aux_rule)

        # amosum_rule = amosum_rules["amosum_rule"]
        # result.append(amosum_rule)

        # debug(f"body: {toString(body)}")
        # print(toString(result))
    
        debug("")
        
        self.aggregate_id_autoincrement += 1
        
        return result
    
    
    
    

    def statement(self, children: List[Tree | Token]) -> List[Tree | Token]:
        if ValidatorAmoSum.thereIsAmoSum(children):
            return self.amosum_statement(children)
        elif ValidatorAmoSum.thereIsAmoMaximize(children):
            return self.amomaximize_statement(children)
        
        return children
        
    
    
class DiscardAmoSum(Transformer):

    def aggregate(self, children):
        if not ValidatorAmoSum.thereIsAmoSum(children):
            return children
        return Discard
    
class DiscardBuiltInAtoms(Transformer):

    def naf_literal(self, children):
        if not ValidatorAmoSum.isABuiltInAtom(children[0]):
            return children
        return Discard