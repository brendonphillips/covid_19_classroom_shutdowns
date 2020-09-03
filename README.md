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

There are three code files (beginning with "REAL_"): REAL_Person.hpp describes each agent, REAL_Town.hpp describes the environment (the population, school, households, etc) and main file REAL_Simulation_parallel.cpp runs the simulation. Model output (of 2000 trials) is printed to a separate CSV for each unique combination of parameter values.

## INDIVIDUALS

### REAL_Person.hpp (object name Person)
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

Also in the file are constructors, getters and an overload for the stream insertion operator. Setters are protected, to be used only by the Town class; many of the agents are grouped (e.g. all Susceptible agents, all agents in House #2, agents assigned to Cohort 1 in Classroom #2, etc). To make sure that agent characteristics aren't update without corresponding changes to the containers in the Town class, the only interface is with the Town class. All necessary changes are made there.
