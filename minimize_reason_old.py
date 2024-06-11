def minimize_reason_old(reason: List[int], trues: List[int]):
    global N,lb, I, weight, aggregate, groups,  mps, group, true_group, global_ord_lit, redundant_lits, reason_trues

    for li in trues:
        redundant_lits_li = []
        gi = group[li]
        wli = weight[li]
        for lj in global_ord_lit:
            gj = group[lj]
            if gi.id == gj.id:
                continue
            if not I[lj] is None:
                if I[lj] == True and not not_(lj) in reason: 
                    continue
                elif I[lj] == False and not lj in reason:
                    continue
                tg = true_group[gj]
                mw_g = max_w(gj)


                if I[lj] == True:  
                    # tg == lj
                    if mps - weight[lj] + weight[mw_g] - wli < lb:
                        redundant_lits_li.append(not_(lj))
                    else:
                        break

                elif I[lj] == False and tg is None and weight[lj] > weight[mw_g]:
                    # debug("mps",mps,"weight[mw_g]",weight[mw_g],"weight[lj]",weight[lj],"gj.id", gj.id)
                    if mps - weight[mw_g] + weight[lj] - wli < lb:
                        redundant_lits_li.append(lj)
                    else:
                        break
                    
                    '''
                    mps - wli >= lb
                    
                    mps' - wli
                    '''

        if len(redundant_lits_li) != 0:
            reason_s = convert_array_to_string(name = "reason", array=reason + reason_trues[li] + [li],atomNames=atomNames)
            debug(convert_array_to_string(f"redundant literals for reason: {reason_s}", array=redundant_lits_li, atomNames=atomNames))
        redundant_lits[li] = redundant_lits_li

