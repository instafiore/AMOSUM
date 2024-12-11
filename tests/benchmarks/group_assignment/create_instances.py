#!/Users/instafiore/instafiore_env/bin/python
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
        for pro in range(1, self.num_projects+1):
            value = random.randint(1, 100)
            if value <= 25:
                duration = 3
            elif value <= 60:
                duration = 6
            else:
                duration = 12
            max_start = InstanceFactory.months - duration + 1
            sm = random.randint(1, max_start)
            self.instance.append(f"project_month({pro}, {sm}, {sm+duration-1}).")

    def create_instance(self):
        self.instance = []
        self.__create_projects_duration()
        self.__create_roles()
        mps = self.__create_mps()
        self.__create_project_bounds(mps=mps)
        return self.instance
    
    def __create_mps(self):
        mps = 0
        for per in range(1, self.num_people+1):
            role = self.map_person_role[per]
            salary = self.map_roles[role][0]
            mps += salary *  InstanceFactory.max_hours_working_per_month
        
        return mps
    
    def _create_bounds(self, mps):
        mu = mps * 0.5
        sigma = mu * 0.1  
        return  np.random.normal(mu, sigma, self.num_projects)   
        

    def __create_project_bounds(self, mps):
        lower_bounds = self._create_bounds(mps)
        for i, lb in enumerate(lower_bounds):
            lb = round(lb)
            ub = lb + round(lb*0.2)
            self.instance.append(f"project({i+1}, {lb}, {ub}).")

class SatInstanceFactory(InstanceFactory):      
    def _create_bounds(self, mps):
        mu = mps * 0.2
        sigma = mu * 0.1  
        return np.random.normal(mu, sigma, self.num_projects)  

class PossiblyUnsatInstanceFactory(InstanceFactory):
    def _create_bounds(self, mps):
        mu = mps
        sigma = mu * 0.1  
        return np.random.normal(mu, sigma, self.num_projects)  



def main(argv):
    
    # instanceFactory = InstanceFactory(num_projects=10, num_people=30)
    # instanceFactory = SatInstanceFactory(num_projects=10, num_people=30)
    # instanceFactory = PossiblyUnsatInstanceFactory(num_projects=10, num_people=30)

    # Possible configurations
	# 	- projects 5-10-15
	# 	- people 30-40-50-60

    project_configurations = (5, 10, 15)
    people_configurations = (30, 40, 50, 60)
    # project_configurations = [5]
    # people_configurations = [30]
    instances_type_distribution = {
        "sat": (SatInstanceFactory, 3),
        "possibly_unsat": (PossiblyUnsatInstanceFactory, 3),
        "middle": (InstanceFactory, 4),
    }

    instances = []
    id = 0
    for nproj in project_configurations:
        for nper in people_configurations:
            common_lines = []
            common_lines.append(f"person(1..{nper}).")
            for instance_type in instances_type_distribution:
                Factory, n = instances_type_distribution[instance_type]
                factory = Factory(num_projects=nproj, num_people=nper)
                for _ in range(n):
                    instance = factory.create_instance()
                    instances.extend(instance)
                    file_name = f"{id:>03}-group-assignment-{nper}-{nproj}-{instance_type}.asp"
                    with open(f"instances/{file_name}", "w") as file:
                        file.write("\n".join(instance))
                        file.write("\n")
                        file.write("\n".join(common_lines))
                        
                    id += 1


if __name__ == "__main__":
    main(sys.argv)