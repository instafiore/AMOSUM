import random
import os

def generate_instance(
    num_units: int,
    num_tasks: int,
    num_slots: int,
    workload_range=(5, 15),
    duration_range=(1, 3),
    efficiency_range=(1, 100),
    capability_prob=0.7,
    dependency_prob=0.3,
    seed=None
) -> str:
    """
    Generate ONE ASP instance for the Operator-Constrained Task Scheduling Problem.
    """

    if seed is not None:
        random.seed(seed)

    asp = []
    # ---------- Time slots ----------
    for s in range(1, num_slots + 1):
        asp.append(f"time_slot(s{s}).")

    # ---------- Units ----------
    for unit_id in range(1, num_units + 1):
        maxw = random.randint(*workload_range)
        asp.append(f"unit(u{unit_id},{maxw}).")


    # ---------- Tasks ----------
    for t in range(1, num_tasks + 1):
        duration = random.randint(*duration_range)
        asp.append(f"task(t{t},{duration}).")

    # ---------- Capabilities ----------
    for u in range(1, num_units + 1):
        for t in range(1, num_tasks + 1):
            if random.random() < capability_prob:
                eff = random.randint(*efficiency_range)
                asp.append(f"capable(u{u},t{t},{eff}).")

    # ---------- Dependencies (acyclic by construction) ----------
    # for t in range(2, num_tasks + 1):
    #     if random.random() < dependency_prob:
    #         before = random.randint(1, t - 1)
    #         asp.append(f"dependent(t{t},t{before}).")

    tasks = list(range(1,num_tasks+1))
    start = random.choice(tasks)
    queue = [start]
    tasks.remove(start)
    branching_factor = 3
    while len(queue) > 0:
        task = queue.pop(0)
        n_children = random.randint(0,min(branching_factor,len(tasks)))

        for i in range(n_children):
            child = random.choice(tasks)
            queue.append(child)
            tasks.remove(child)
            asp.append(f"dependent(t{child},t{task}).")


    return "\n".join(asp)


def generate_N_instances(
    N: int,
    output_dir="instances",
    units_range=(2,100),
    num_tasks_range=(2,500),
    num_slots_range=(5,100),
    seed = 13
):
    """
    Generate N ASP instances and save them to files.
    """

    random.seed(seed)

    os.makedirs(output_dir, exist_ok=True)

    for i in range(1, N + 1):
        num_units= random.randint(*units_range)
        num_tasks= random.randint(*num_tasks_range)
        num_slots= random.randint(*num_slots_range)
        instance = generate_instance(
            num_units=num_units,
            num_tasks=num_tasks,
            num_slots=num_slots
        )

        filename = os.path.join(output_dir, f"{i:03}-wod-u{num_units}-t{num_tasks}-s{num_slots}.asp")
        with open(filename, "w") as f:
            f.write(instance)

        print(f"Generated {filename}")



if __name__ == "__main__":
    generate_N_instances(
        N=100,
        units_range=(2,10),
        num_tasks_range=(2,100),
        num_slots_range=(1,24)
    )
