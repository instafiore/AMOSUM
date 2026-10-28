"""Small helpers of the parser (the old Python propagator that lived here was replaced by amosum.core)."""

ERROR_CODE = 1


def read_file(path):
    with open(path, "r") as file:
        return file.read()


def write_file(path, string):
    with open(path, "w") as file:
        file.write(string)
