#!/home/s.fiorentino/miniconda3/bin/python
import re
import os
import sys
import random
from typing import List

import numpy as np

class InstanceFactory:

    map_roles = {
        "full_professor": (70, 0.17),
        "associate_professor": (50, 0.64),
        "researcher": (30, 0.19),
    }

    SIGMA_P = 0.001
    max_hours_working_per_month = 125
    months = 12

    def __init__(self, num_projects, num_people):
        self.num_projects = num_projects
        self.num_people = num_people

    def __create_roles(self):
        self.map_person_role = dict()

        start_index_partition = 1
        end_index_partition : int 
        comulative_percentage = 0
        for role in self.map_roles:
            p = self.map_roles[role][1]
            comulative_percentage += p
            quantile = round(self.num_people * comulative_percentage)
            end_index_partition = quantile 
            for person in range(start_index_partition, end_index_partition+1):
                self.map_person_role[person] = role
                self.instance.append(f"role({person}, {role}).")
            start_index_partition = end_index_partition+1
        # print(f"[__create_roles] self.map_person_role: {self.map_person_role}")

    def __create_projects_duration(self):
        self.map_project_duration = dict()
        for pro in range(1, self.num_projects+1):
            value = random.randint(1, 100)
            if value <= 25:
                duration = 3
            elif value <= 60:
                duration = 6
            else:
                duration = 12
            self.map_project_duration[pro] = duration
            max_start = InstanceFactory.months - duration + 1
            sm = random.randint(1, max_start)
            self.instance.append(f"project_month({pro}, {sm}, {sm+duration-1}).")

    def create_instance(self):
        self.instance = []
        self.__create_projects_duration()
        self.__create_roles()
        self.__create_project_bounds()
        return self.instance
    
    def __create_mps(self, pro):
        mps = 0
        for per in range(1, self.num_people+1):
            role = self.map_person_role[per]
            salary = self.map_roles[role][0]
            for _ in range(self.map_project_duration[pro]):
                mps += salary *  InstanceFactory.max_hours_working_per_month
        
        return mps    

    def __create_project_bounds(self):
        for pro in self.map_project_duration:
            mps = self.__create_mps(pro=pro)
            lb = round(self._create_bound(mps))
            ub = lb + round(lb*0.2)
            self.instance.append(f"project({pro}, {lb}, {ub}).")

    def __repr__(self):
        return "middle"
    
    def __str__(self):
        return self.__repr__()
    
    def _create_bound(self, mps):
        mu = mps * 1 / self.num_projects
        sigma = mu * InstanceFactory.SIGMA_P
        return  np.random.normal(mu, sigma)

class SatInstanceFactory(InstanceFactory):      
    def _create_bound(self, mps):
        mu = mps * 1 / self.num_projects * 1 / 4
        sigma = mu * InstanceFactory.SIGMA_P
        return np.random.normal(mu, sigma)  
    
    def __repr__(self):
        return "sat"
    
class PossiblyUnsatInstanceFactory(InstanceFactory):
    def _create_bound(self, mps):
        mu = mps * 0.9
        sigma = mu * InstanceFactory.SIGMA_P
        return np.random.normal(mu, sigma)  
    
    def __repr__(self):
        return "punsat"
    
class UnsatInstanceFactory(InstanceFactory):
    def _create_bound(self, mps):
        mu = mps + 1
        sigma = mu * InstanceFactory.SIGMA_P
        return np.random.normal(mu, sigma)  
    
    def __repr__(self):
        return "unsat"
    

class BenchmarkCreator():

    instances_type_distribution = {
        "sat": (SatInstanceFactory, 3),
        "possibly_unsat": (PossiblyUnsatInstanceFactory, 3),
        "middle": (InstanceFactory, 3),
        "unsat": (UnsatInstanceFactory, 1),
    }

    def __init__(self, project_configurations, people_configurations):
        self.people_configurations = people_configurations
        self.project_configurations = project_configurations
        self.instances = []

    def print_instance(self, factory: InstanceFactory, id):
        common_lines = []
        common_lines.append(f"person(1..{factory.num_people}).")
        instance = factory.create_instance()
        self.instances.extend(instance)
        file_name = f"{id:>03}-group-assignment-{factory.num_people}-{factory.num_projects}-{factory}.asp"
        with open(f"instances/{file_name}", "w") as file:
            file.write("\n".join(instance))
            file.write("\n")
            file.write("\n".join(common_lines))

    def create_benchmark(self):
        self.instances = []
        id = 0
        for nproj in self.project_configurations:
            for nper in self.people_configurations:
                for instance_type in BenchmarkCreator.instances_type_distribution:
                    Factory, n = BenchmarkCreator.instances_type_distribution[instance_type]
                    factory = Factory(num_projects=nproj, num_people=nper)
                    for _ in range(n):
                        self.print_instance(factory, id)
                        id += 1

def main(argv):
    
    # Possible configurations
	# 	- projects 5-10-15
	# 	- people 30-40-50-60
    project_configurations = (10, 15, 20)
    people_configurations = (30, 40, 50, 60)
    benchmarkCreator = BenchmarkCreator(project_configurations, people_configurations)
    
    # benchmarkCreator.create_benchmark()
    benchmarkCreator.print_instance(PossiblyUnsatInstanceFactory(num_projects=5, num_people=20), "")
    # benchmarkCreator.print_instance(UnsatInstanceFactory(num_projects=10, num_people=30), "")
    # benchmarkCreator.print_instance(SatInstanceFactory(num_projects=10, num_people=20), "")
    # benchmarkCreator.print_instance(InstanceFactory(num_projects=10, num_people=30), "")

if __name__ == "__main__":
    main(sys.argv)