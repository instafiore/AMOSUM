from itertools import chain, combinations

def all_subsets(arr):
    """
    Generate all subsets of a given list arr.
    """
    return chain.from_iterable(combinations(arr, r) for r in range(len(arr) + 1))

# Example usage
arr = [1, 2, 3]
subsets = all_subsets(arr)
subsets_list = list(subsets)
print(subsets_list)
