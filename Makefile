TEST=tests

PROPAGATOR_NAME=propagator_opt
PROPAGATOR_DIR=/home/s.fiorentino/Wasp_instafiore/propagator_opt_dir

# p = problem
# n = number of instances
# lb = lower bound
# l = light
run_tests:
	$(TEST)/run_tests.py $(p) $(n) $(lb) $(l)

# p = problem
# i = instance
# lb = lower bound
# l = light
# t = timestamp
run_test:
	$(TEST)/run_test.py $(p) $(i) $(lb) $(l) $(t)

# i = instance
run_solver_group:
	gringo tests/temp/$(i).asp --output=smodels | wasp_python  --silent=2 --interpreter=python --script-directory=$(PROPAGATOR_DIR) --plugins-file=$(PROPAGATOR_NAME) -n0

# i = instance
run_solver_aggr:
	gringo tests/temp/$(i).asp --output=smodels | wasp_python --silent=2  -n0