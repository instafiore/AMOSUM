import random
from pathlib import Path

def generate_instance(
    num_cities: int,
    revenue_range=(1, 100),
    density=0.5,
    seed=42
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

    # Ensure basic reachability: build a spanning tree from start
    addedPairs = set()
    for i in range(1, num_cities):
        r = random.randint(*revenue_range)
        lines.append(f"revenue({cities[i-1]},{cities[i]},{r}).")
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

    return "\n".join(lines)


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
        asp = generate_instance(
            num_cities=num_cities,
            seed=i,
            **kwargs
        )
        num_cities += inc
        filename = output_dir / f"{i:03}-{num_cities}-tsp.asp"
        filename.write_text(asp)

generate_n_instances(100)
