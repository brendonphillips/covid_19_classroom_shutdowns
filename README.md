# Model-based projections for COVID-19 outbreak size and student-days lost to closure in Ontario childcare centers and primary schools

This project models the spread of infections and the number of student days missed due to shutdowns; find the report [here](https://doi.org/10.1101/2020.08.07.20170407), currently awaiting peer-review.

Short description: a total of 21 scenarios were modelled, including different student-teacher ratios in classrooms, low and high modes of disease transmission, different classroom allocations, and student cohorting. This is the C++ code for an agent-based model; each agent represents an individual, either a child, (non-teacher) adult, or teacher. 

Results we're interested in (for each parameter combination):
1) Infections occurring in each model location (household infection, school common area infection, classroom infection, community transmission, total infections).
2) Student days missed: a classroom is closed for 14 days when a student/teacher in that room shows symptoms of the disease; all children/teachers assigned to that classroom are sent home until that classroom reopens. A time step in the model is considered to be a missed student day for that agent when an asymptomatic child is kept home on a school day because their classroom was shut down due to infection. Weekends are not counted, neither are any days when the student is symptomatic or in a cohort not due in class that week; they wouldn't have been in school anyway.
3) Length of the simulation: the simulation stops when there are no active infections in the entire population. As such, the length of the run tells us how long this period of disease lasted.
4) Secondary infections: an index case is infected at random. The number of infections produced by this index case gives an estimation of the effective reproductive ratio of the disease. We're also interested in the number of runs with secondary spread; the disease doesn't 'catch on' in every simulation. Because of parameter values, some infections don't spread widely. 
5) Time between the index case and the first secondary case.
6) Evolution of the health of the population (how many people are sick at each time step, for example). We're also interested in seeing when the population infection rate peaks within the first month of disease spread.
7) Number of classrooms closed at each time step. 

There are three code files (``` REAL_* ```) used for the simulation: ``` REAL_Person.hpp ``` describes each agent, ``` REAL_Town.hpp ``` describes the environment (the population, school, households, etc) and main file ``` REAL_Simulation_parallel.cpp ``` runs the simulation. Model output (of 2000 trials) is printed to a separate CSV for each unique combination of parameter values.

## INDIVIDUALS

### ``` REAL_Person.hpp ``` (object name Person)
The characteristics of each agent are :
1. Age - a categorical age, either child (C) or adult (A),
2. Household - the number of the household that the agent belongs to,
3. Classroom - the number of the classroom the child/teacher is assigned to,
4. Disease status - the agent's stage of progression through the SEPAIR disease model,
5. Identity - a number used to refer to the agent,
6. Days since first symptoms - used to decide isolation and for the agent,
7. Infection location - tells where in the model the agent was infected (at home, in the classroom, in the school common area or in the community),
8. Cohort - if we're dividing students into cohorts by week, this is either 1 or 2 (to see which week the agent will be in class),
9. Time step infected - tells on which day the agent was infected.

Also in the file are constructors, getters and an overload for the stream insertion operator. Setters are protected, to be used only by the Town class; many of the agents are collected (e.g. all Susceptible agents, all agents in House #2, agents assigned to Cohort 1 in Classroom #2, etc). To make sure that agent characteristics aren't updated without corresponding changes to the containers in the Town class, the only interface is with Town. All necessary changes are made there.

### ``` UNIT_TEST_Person_humourless.cpp ```

Comment the access specifier ``` protected: ``` in REAL_Person.hpp and compile with ``` g++ UNIT_TEST_Person_humourless.cpp -o test ```.

Test of the Person class: changing disease status, household number, classroom number, constructors and printing.

## POPULATION AND ENVIRONMENT

### ``` REAL_Town.hpp ``` (object name TOWN)

The characteristics of each Town are:
1) Contact matrices (school and home) - Canada-specific contact rates between children and adults in classrooms and households respectively,
2) Population, IDs - a vector of Person object that make up the population, and a list of their numbers (respectively),
3) Households - the key is the number of the household, the value is a set of integers representing household members,
4) School - the key is the number of the classroom, the value is a set of integers representing the teachers and children assigned to that room,
5) Number of days shutdown due to illness - the key is the number of the classroom, the value is an integer representing the number of calendar days since the class has been shut down
6) Substitute list - if a teacher gets sick, they must be replaced with another adult drawn from the population; this keeps track of which substitute teacher is covering for which teacher. the key is the number of the symptomatic teacher, the value is the number of the substitute,
7) Disease compartments - the key is the infection state (for example, 'S' for susceptible), the value is the set of all agents currently in that stage of the infection,
8) Run time - the number of calendar days (time steps) for which the simulation has been running,
9) Cohorts - the key is the number of the cohort, the value is a set of all children assigned to that cohort (not separated by classroom). Teachers are placed in Cohort 0 since they go to the school every day; the same with children in a single-cohort scenario. If there are two cohorts, they're numbered 1 and 2.
10) Places infected - the key is a string representing the place of infection (household, class, common area, community), the value is the set of all agents infected in that location.

All member functions ``` check_* ``` are used in assertions to check function inputs, and functions ``` print_* ``` output to the terminal. The stream insertion operator prints a summary of the Town object.

Notable functions:
- ``` replace_sick_teacher ```: when a teacher falls ill and does not recover in time for the start of class, a substitute must be chosen from a household with no-one attending the educational institution in any capacity. 'Extra households' are made in the main simulation for this reason. If, for some reason, a substitute can't be found with that constraint, the trial will print an error message and quit. 
- ``` agents ``` vs. ``` agents_in_school ```: the ``` agents ``` function returns a set of all individuals in the population with the desired status, while ``` agents_in_school ``` returns a set of only students and teachers. The same applied to the functions ``` *_proportion ```.
- ``` set_classroom ```: cohort number -1 represents anyone not attending the school in any capacity, cohort 0 represents those individuals who go to class every day during the school week (all teachers, and students in a single cohort scenario), and cohorts 1 and 2 represent the sets of students that alternate based on week (even/odd).

### ``` UNIT_Test_Town_general.cpp ```

Compile with ``` g++ UNIT_TEST_Town_general.cpp -o test ```.





