import random
from pathlib import Path

def generate_instances(
    num_cities: int,
    revenue_range=(1, 100),
    density=0.5,
    seed=42,
    distribution_mps = [0.25, 0.5, 0.75, 1, 1.25]
):

    random.seed(seed)

    cities = [f"c{i}" for i in range(num_cities)]
    start = cities[random.choice(range(num_cities))]

    lines = []

    # Cities
    for c in cities:
        lines.append(f"city({c}).")

    # Start city
    lines.append(f"start({start}).")

    mps = 0

    revenue_map = {}

    # Ensure basic reachability: build a spanning tree from start
    addedPairs = set()
    for i in range(1, num_cities):
        r = random.randint(*revenue_range)
        lines.append(f"revenue({cities[i-1]},{cities[i]},{r}).")
        revenue_map.setdefault(cities[i-1], [])
        revenue_map[cities[i-1]].append(r)
        pair = f"{i-1}-{i}"
        assert pair not in addedPairs
        addedPairs.add(pair)

    # Add extra edges to increase choice (controlled by density)
    for i in range(num_cities):
        x = cities[i]
        for j in range(num_cities):
            y = cities[j]
            if i != j and i != j - 1 and random.random() <= density:
                r = random.randint(*revenue_range)
                lines.append(f"revenue({x},{y},{r}).")
                pair = f"{i}-{j}"
                assert pair not in addedPairs
                addedPairs.add(pair)
                revenue_map.setdefault(x, [])
                revenue_map[x].append(r)

    for city in revenue_map:
        mps += max(revenue_map[city])

    asp_instances = []
    asp_instance = "\n".join(lines)
    for percentage in distribution_mps:
        bound = int(mps * percentage)
        bound_fact = f"lb({bound})."
        asp_instances.append("\n".join([asp_instance, bound_fact]))

    return asp_instances, distribution_mps


def generate_n_instances(
    n_instances: int,
    output_dir="instances",
    **kwargs
):
    """
    Generate N ASP instances and write them to files.
    """
    output_dir = Path(output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)

    num_cities: int = 10
    inc = 5
    for i in range(n_instances):
        asp_instances, percentages_mps = generate_instances(
            num_cities=num_cities,
            seed=i,
            **kwargs
        )
        num_cities += inc
        for asp, percentage_mps in zip(asp_instances, percentages_mps):
            filename = output_dir / f"{i:03}-{num_cities}-{str(percentage_mps).replace('.',':')}-tsp.asp"
            filename.write_text(asp)

generate_n_instances(60)
