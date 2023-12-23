TEST=tests

PROPAGATOR_E=propagator_opt_e
PROPAGATOR_LE=propagator_opt_le
# PROPAGATOR_DIR=/home/s.fiorentino/Wasp_instafiore/propagator_opt_dir
PROPAGATOR_DIR=.

# p = problem
# n = number of instances
# lb = lower bound
# l = light
run_tests:
	$(TEST)/run_tests.py $(p) $(lb) $(l) $(n)

# p = problem
# i = instance
# lb = lower bound
# l = light
# t = timestamp
run_test:
	$(TEST)/run_test.py $(p) $(i) $(lb) $(l) $(t)

# s=start lb
# e=end lb
# i=instance
# l=boolean for light (1 = light)
run_test_lb:
	$(TEST)/run_test_lb.sh $(s) $(e) $(i) $(l)

# p = problem
# i = instance
run_solver_e:
	gringo $(TEST)/$(i).asp \
		./tests/benchmarks/$(p)/weights.asp   \
	    ./tests/benchmarks/$(p)/encoding_with_group_e.asp \
		./tests/benchmarks/$(p)/lb.asp   \
		--output=smodels \
		| wasp  --silent=2 --interpreter=python \
		--script-directory=$(PROPAGATOR_DIR) --plugins-file=$(PROPAGATOR_E) -n0

# p = problem
# i = instance
run_solver_le:
	gringo $(TEST)/$(i).asp \
		./tests/benchmarks/$(p)/weights.asp   \
	    ./tests/benchmarks/$(p)/encoding_with_group_le.asp \
		./tests/benchmarks/$(p)/lb.asp   \
		--output=smodels \
		| wasp  --silent=2 --interpreter=python \
		--script-directory=$(PROPAGATOR_DIR) --plugins-file=$(PROPAGATOR_LE) -n0

# p = problem
# i = instance
run_solver_aggr:
	gringo tests/temp/$(i).asp \
	./tests/benchmarks/$(p)/weights.asp   \
	./tests/benchmarks/$(p)/encoding_with_group_e.asp \
	./tests/benchmarks/$(p)/lb.asp   \
	--output=smodels | wasp_python --silent=2  -n0