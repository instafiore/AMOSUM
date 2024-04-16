TEST=tests

PROPAGATOR_E=propagator_opt_e
PROPAGATOR_LE=propagator_opt_le
# PROPAGATOR_DIR=/home/s.fiorentino/Wasp_instafiore/propagator_opt_dir
PROPAGATOR_DIR=.

# p = problem
# n = number of instances
# lb = lower bound
# l = light
# enc = encoding type [ge_amo, ge_eo, le_eo]
run_tests:
	$(TEST)/run_tests.py $(p) $(lb) $(l) $(enc) $(n)

# p = problem
# i = instance
# lb = lower bound
# l = light
# enc = encoding type [ge_amo, ge_eo, le_eo]
# t = timestamp
# example make run_test p=graph_colouring i=0001-graph_colouring-125-0.asp lb=40 l=1 enc=ge_amo
run_test:
	$(TEST)/run_test.py $(p) $(i) $(lb) $(l) $(enc) $(t)

# s=start lb 
# e=end lb
# i=instance
# l=boolean for light (1 = light)
# enc = encoding type [ge_amo, ge_eo, le_eo]
# NOTE: s >= e, step size -2
run_test_lb:
	$(TEST)/run_test_lb.sh $(s) $(e) $(i) $(l) $(enc)