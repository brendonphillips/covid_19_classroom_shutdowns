#include "REAL_Town.hpp"
#include "Timing.hpp"
#include <execution>

int main(int argc, char *argv[])
{
TIC(0);

	// didn't want to cram everything into one single file, so here's a pretty jank solution
	Get_Parameter_Combinations();

	std::cout << "Number of combinations: " << Parameter_Tuples.size() << std::endl;

	// // for a small initial test run
	// Parameter_Tuples = {Parameter_Tuples[0]};

	std::for_each(std::execution::par_unseq, Parameter_Tuples.begin(), Parameter_Tuples.end(), [&](auto&& parameter_tuple) // for the actual run
	// std::for_each(std::execution::seq, Parameter_Tuples.begin(), Parameter_Tuples.end(), [&](auto&& parameter_tuple) // for testing
	{
		// take a parameter tuple and get the individual parameter values
		const float Alpha_0 =  						std::get<0>(parameter_tuple);
		const float Alpha_C =  						std::get<1>(parameter_tuple);
		const float B_H = 							std::get<2>(parameter_tuple);
		const float Background_Infection_Rate = 	std::get<3>(parameter_tuple);
		const float R_init =  						std::get<4>(parameter_tuple);
		const std::string Classroom_Arrangement =	std::get<5>(parameter_tuple);
		const int Num_Children_per_Classroom = 		std::get<6>(parameter_tuple);
		const int Num_Teachers_per_Classroom =  	std::get<7>(parameter_tuple);
		const int Num_Child_Cohorts = 				std::get<8>(parameter_tuple);
		const bool Reduced_Hours = 					std::get<9>(parameter_tuple);

		// calculate B_C, B_0 according to the draft
		const float B_C = Alpha_C*B_H;
		const float B_0 = Alpha_0*B_C;

		// get the file stem, and if the run has already been completed, exit
		const std::string File_Stem = get_filename(Alpha_0, Alpha_C, B_H, Background_Infection_Rate, R_init, Classroom_Arrangement, Num_Children_per_Classroom, Num_Teachers_per_Classroom, Num_Child_Cohorts, Reduced_Hours);

		for (const auto & entry : std::filesystem::directory_iterator(Data_Folder))
		{
			if(entry.path().string().find(File_Stem) != std::string::npos){ return; }
		}

		// Toy output statements
		//
		// std::cout << "B_H: " << B_H << std::endl;
		// std::cout << "Alpha_0: " << Alpha_0 << std::endl;
		// std::cout << "Alpha_C: " << Alpha_C << std::endl;
		// std::cout << "Lambda: " << Background_Infection_Rate << std::endl;
		// std::cout << "R_init: " << R_init << std::endl;
		// std::cout << "Classroom arrangement: " << Classroom_Arrangement << std::endl;
		// std::cout << "TCRatio: " << Num_Children_per_Classroom << ":" << Num_Teachers_per_Classroom << std::endl;
		// std::cout << "\tTeachers: " << Num_Teachers_per_Classroom << std::endl;
		// std::cout << "\tChildren: " << Num_Children_per_Classroom << std::endl;
		// std::cout << "\tCohorts: " << Num_Child_Cohorts << std::endl;
		//
		// std::exit(EXIT_SUCCESS);

		// stream store the output data
		std::stringstream no_secondary_infections_output {};
		std::stringstream yes_secondary_infections_output {};

		for(int Instance=0; Instance<Ensemble_Size; ++Instance)
		{
			/*
				Printing the data from all the instances (with and without secondary infections), and then separating them in R for
				data analysis is tedious and time wasting. So we'll declare a temp output stringstream to hold the results for this instance.
				After the simulation is finished, then we can look and see whether there were any secondary infections or not, and then we'll
				copy that data to either the secondary_infection stringstream, or the no_secondaries one. Then we'll clear the temp buffer
				and start over with the next one. When we're finished all the simulations, then we'll save the two data sets separately in
				appropriately named CSV files
			*/
			std::stringstream local_output_buffer {};

			//  get the seed corresponding to this Instance number from the given file
			int Random_Seed = Seed_Vector.at(Instance);

			// test output
			// std::cout << "A0 " << Alpha_0 << ", AC " << Alpha_C << ", BH " << B_H << ", Rinit " << R_init << ", instance " << Instance << ", seed " << Random_Seed << ", T_C_R " << Num_Teachers_per_Classroom << ":" << Num_Children_per_Classroom << ":" << Num_Child_Cohorts << ", reduced hours " << Reduced_Hours << ", ensemble size " << Ensemble_Size << ", random seed " << Random_Seed << ", seed file name: " << Seeds_File << std::endl;
			// return 0;

			// beginning each instance with the same seed to generate the same series of numbers
			// this will help tease out the true effects of the different arrangements and ratios
			std::mt19937 generator(Random_Seed);
			std::uniform_real_distribution<float> randfloat(0,1);

			Town NorthShore;

			/*
				we'll be using this counter for both the do-while loops
				creating households for (children and parents) and (teachers)
			*/
			int household_number = 0;


			// generating the households not hosting the teachers
			for(int cohort_number=1; cohort_number <= Num_Child_Cohorts; ++cohort_number)
			{
				/*
					store the children we accept for enrollment - here we keep siblings together
						for convenience. later, we can just break them up for random allocation.

					Looks like { {children from household 1}, {children from house 2}, {children from house 3}, ... }
				*/
				std::vector<std::vector<int>> children_accepted_to_centre {};
				children_accepted_to_centre.push_back({});

				bool centre_is_full = false;

				while( not centre_is_full )
				{
					// get the household size and distribution
					std::pair<int, int> children_first_parents_second = NorthShore.children_and_adults_household_size_distribution(randfloat(generator));
					// add the parents to the model - they don't attend the school
					for(int num_adults = 0; num_adults < children_first_parents_second.second; ++num_adults)
					{
						NorthShore.add_agent(Person('A', household_number, 'S', -1));
					}
					// adding their children
					for(int num_childr = 0; num_childr < children_first_parents_second.first;  ++num_childr)
					{
						// add the child to the population
						NorthShore.add_agent(Person('C', household_number, 'S', -1));

						/*
							if this is the first child from that specific household that we're looking at, then we just accept
								them while there's still space. whereas, for the second (or further) child form the same house,
								remember that there's a probability that the sibling may not attend the same school, hence the
								if-else here
						*/
						if(num_childr == 0)
						{
							// if the centre is not full, accept the child
							if( number_of_values(children_accepted_to_centre) < Num_Children_per_Classroom*Number_of_Classrooms )
							{
								children_accepted_to_centre.back().push_back( NorthShore.num_agents()-1);
							}
							// else mark the centre as full
							else { centre_is_full = true; }
						}
						else // if it's a sibling of the first child
						{
							// the probability of the sibling going to the same childcare centre
							if( randfloat(generator) < Probability_Going_Same_Childcare_Centre )
							{
								// if the centre isn't full, take them
								if( number_of_values(children_accepted_to_centre) < Num_Children_per_Classroom*Number_of_Classrooms )
								{
									children_accepted_to_centre.back().push_back( NorthShore.num_agents()-1);
								}
								// else mark the centre as full
								else { centre_is_full = true; }
							}
						}
					}
					// push that sibling group to the enrollment list
					if(not centre_is_full){ children_accepted_to_centre.push_back({}); }
					// move on to the next household
					++ household_number;
				}

				/*
					In this model, there are two ways to allocate the children to classrooms:
						1) keep siblings together
						2) randomly assign children
				*/

				if(Classroom_Arrangement == "siblings") // keeping siblings together
				{
					// we'll assign the largest groups first, and then smaller ones after
					std::set<int> available_classes {};
					std::vector<std::vector<int>> whole_family_couldnt_fit_into_a_single_classroom {};
					std::vector<std::vector<int>> children_enrolled_by_house = children_accepted_to_centre;

					// sort the households by size - biggest last and we"ll pop from the back
					std::sort(
						children_enrolled_by_house.begin(),
						children_enrolled_by_house.end(),
						[](const std::vector<int> first, const std::vector<int> second){ return first.size() < second.size(); }
					);

					// get a list of available classes that aren't full yet - right now, that's all of them
					for(int i = 0; i < Number_of_Classrooms; ++i){ available_classes.insert(i); }

					// while there are still classrooms with room available:
					do
					{
						// fetch a group of siblings, and remove it from the list
						std::vector<int> this_family = children_enrolled_by_house.back();

						children_enrolled_by_house.pop_back();

						// we scan each not-full class for empty chairs
						// for(std::set<int>::iterator iter=available_classes.begin(); iter != available_classes.end();)
						for(const int class_number : available_classes)
						{
							// get the number of seats available
							const int Seats_Available = Num_Children_per_Classroom - NorthShore.classroom(class_number).size();
							// if there's no space left, move to the next classroom
							if(not Seats_Available) { continue; }
							// if we can't fit this sibling group into this class, move on to the next one
							if(this_family.size() > Seats_Available) { continue; }
							// else, if there's room, shove them in there
							for(int child : this_family){ NorthShore.set_classroom(child, class_number, cohort_number); }
							// if there's no space left in this class, remove this classroom from the availability list
							if((Num_Children_per_Classroom - NorthShore.classroom(class_number).size()) == 0)
							{
								// this one works fine, apparently
								available_classes.erase(class_number);
							}
							// we're done with this family
							this_family.clear();
							// we've put them in, so no need to keep searching classrooms - move on to the next group
							break;
						}

						// if we're keeping all the siblings together, we may need to break a single family
						if(not this_family.empty()){ whole_family_couldnt_fit_into_a_single_classroom.push_back(this_family); }
					}
					while(children_enrolled_by_house.size()); // keep going while we still have unenrolled children left

					// if we couldn't fit the entire sibling group into a classroom, break them up among the classrooms with space in them
					for(std::vector<int> family : whole_family_couldnt_fit_into_a_single_classroom)
					{
						for(int child : family) // for every child, find a classroom with space and put them in there
						{
							/*
								For anyone reading this, please help me out with something.

								Notice that the iteration in the following loop is different from the range iteration above (inside the do-while loop, iterating over available_classes). This is because, even though range iteration in the above do-while loop works fine (even with erasing the number of full classes, the line "available_classes.erase(class_number)"), range iteration in the following loop will find values of class_number that are NOT IN available_classes. Specifically, class numbers 0 and 65536 in the list {3,4} (of available classes).

								This style of iteration avoids "finding" these weirdly specific non-member values (during serial runs) and segfaults (during parallel execution). My question: WHY? What have I done wrong, or is the compiler just taking the piss? This std::set<int> isn't shared between instances, so why does it make a difference if it's parallel (it segfaults while running 12 instances in parallel, not in serial)? Is std::set<int> being corrupted somehow (that still doesn't explain the very calmly chosen nonsense values, which happens in all cases in this loop but not the one above)? I'm nowhere near RAM capacity, and GDB and Valgrind weren't particularly helpful.

								This may be elementary (I *have* been staring at this for a while now), but I've got nothing...

								Compiling with GCC 9.3.0, C++ 17 with -msse2 -O3 and TBB.

								Brendon Phillips, 24 December 2020. b2philli@uwaterloo.ca
							*/
							// for(int class_number : available_classes)
							for(std::set<int>::iterator iter=available_classes.begin(); iter != available_classes.end();)
							{
								const int class_number = *iter;
								if(not available_classes.count(class_number))
								{
									std::cout << "selected class number " << class_number << " from the class list " << available_classes << std::endl;
								}
								if((Num_Children_per_Classroom - NorthShore.classroom(class_number).size())!=0)
								{
									NorthShore.set_classroom(child, class_number, cohort_number);
									++iter;
									break;
								}
								// if there's isn't a free seat, mark that classroom as full and move on
								else
								{
									// available_classes.erase(class_number);
									iter = available_classes.erase(iter);
								}
							}
						}
					}
				}
				else if(Classroom_Arrangement == "random") // randomly assigning them - arguably easier
				{
					// unpack the family groups to get a flat vector with all the children there
					std::vector<int> children_in_centre({});
					for(const std::vector<int> subvec : children_accepted_to_centre) for(int critter : subvec) { children_in_centre.push_back(critter); }
					// random shuffle the list of children
					std::shuffle(children_in_centre.begin(), children_in_centre.end(), generator);
					int classroom_number = 0;
					// throw them into classes until each room is full
					for(int index = 0; index < children_in_centre.size(); ++index)
					{
						if((index%Num_Children_per_Classroom==0) & (index!=0)) ++classroom_number;
						NorthShore.set_classroom( children_in_centre[index], classroom_number, cohort_number );
					}
				}
				else // we're not studying any other classroom arrangements (grouping by age, etc.)
				{
					std::cerr << "\nERROR: CLASSROOM ARRANGEMENT NOT FOUND." << std::endl;
					std::exit(EXIT_FAILURE);
				}
			}

			assert(set_of_values(set_of_values( NorthShore.classrooms() )).size() == Num_Children_per_Classroom*Number_of_Classrooms);

			/*
				Creation of the teacher households, using the same household counter from before
				36% of teachers live in houses with 1+ children, using the same distribution as before
				the other teachers live with only adults
				We generate twice/thrice the number of necessary households so that we have houses to pull
					substitute teachers from without violating the original assumption of only one teacher per
					household at this centre, with no cohabiting children and teachers attending the same centre
			*/
			const int Substitute_Teacher_Factor = 3;
			std::vector<int> teacher_households;
			do
			{
				// if it's a house with just adults
				if(randfloat(generator) < Percentage_Teachers_With_No_Children)
				{
					// get the house size from the distribution
					const int Number_of_Adults = NorthShore.just_adults_household_size_distribution(randfloat(generator));
					// throw them all in the same flat
					for(int num_adults=0; num_adults < Number_of_Adults; ++num_adults)
					{
						NorthShore.add_agent(Person('A', household_number, 'S', -1));
					}
				}
				else  // there are children in the house too
				{
					// get the house size from the distribution
					const std::pair<int, int> Children_First_Adults_Second = NorthShore.children_and_adults_household_size_distribution(randfloat(generator));
					// throw the children in there
					for(int num_children=0; num_children < Children_First_Adults_Second.first; ++num_children)
					{
						NorthShore.add_agent(Person('C', household_number, 'S', -1));
					}
					// aaaand the adults...
					for(int num_adults=0; num_adults < Children_First_Adults_Second.second; ++num_adults)
					{
						NorthShore.add_agent(Person('A', household_number, 'S', -1));
					}
				}
				// move on to the next house
				teacher_households.push_back(household_number);
				++ household_number;
			}
			while(teacher_households.size() < Substitute_Teacher_Factor*Num_Teachers_per_Classroom*Number_of_Classrooms);

			// classrooms are filled - now to pick teachers - no two from the same household
			int running_classroom_number = 0;
			for(int index=0; index < Num_Teachers_per_Classroom*Number_of_Classrooms; ++index)
			{
				// increment the classroom number when we fill one room
				if((index%Num_Teachers_per_Classroom==0) & (index!=0)) ++running_classroom_number;
				// get the adults in the teacher household
				std::vector<int> adults = NorthShore.adults_in_household( teacher_households[index] );
				// assign the first one to the classroom at hand
				NorthShore.set_classroom(adults[0], running_classroom_number, Teacher_Cohort);
			}

			/*
				we kick off the infection by changing one of the susceptible school attendees to the presymptomatic disease status,
					and then we just let their health evolve as normal. most of these index cases will not produce secondary infections.
				Since we are ultimately interested in what happens at the school, starting the infection directly with the school
					population wastes less computational time waiting for the disease to "spread there eventually".
			*/

			// get the susceptible school attendees, put them in an unsorted container, shuffle them, get the first person
			std::set<int> S_agents( NorthShore.agents_in_school({'S'}) );
			std::vector<int> School_Susceptibles(S_agents.begin(), S_agents.end());
			std::random_shuffle(School_Susceptibles.begin(), School_Susceptibles.end());
			// infect this index case
			const int Index_Case = School_Susceptibles.front();
			NorthShore.set_status(Index_Case, 'P', "initial");

			// setting the initial proportion of recovered agents
			for(int person : NorthShore)
			{
				if(NorthShore.Agent(person).status() != 'S'){ continue; }
				if(randfloat(generator) < R_init){ NorthShore.set_status(person, 'R', "initial"); }
			}

			/*
			 	We'll calculate the R_e value of the infection by counting the number of secondary infections due to this first randomly
				infected agent. It also gives us the time to the first infection, by getting the first time when this number moves above zero.
			*/
			int number_of_secondary_infections = 0;

			// keeping track of the time steps here in the sim
			int run_counter = 0;

			// lambda for writing the results to file
			auto write_results = [&]() -> void
			{
				local_output_buffer
					<< Alpha_0 << ","
					<< Alpha_C << ","
					<< B_H << ","
					<< B_C << ","
					<< B_0 << ","
					<< Probability_Going_Same_Childcare_Centre << ","
					<< Percentage_Teachers_With_No_Children << ","
					<< Background_Infection_Rate_H << ","
					<< R_init << ","
					<< Probability_of_Child_Developing_Symptoms << ","
					<< Probability_of_Adult_Developing_Symptoms << ","
					<< NorthShore.num_households() << ","
					<< NorthShore.num_classrooms() << ","
					<< NorthShore.num_agents() << ","
					<< NorthShore.num_age('A') << ","
					<< NorthShore.num_age('C') << ","
					<< Num_Children_per_Classroom << ","
					<< Num_Teachers_per_Classroom << ","
					<< Num_Child_Cohorts << ","
					<< Classroom_Arrangement << ","
					<< Reduced_Hours << ","
					<< E_to_P_rate << ","
					<< P_to_Inf_rate << ","
					<< I_to_R_rate << ","
					<< A_to_R_rate << ","
					<< Instance << ","
					<< run_counter << ","
					<< NorthShore.currently_the_weekend() << ","
					<< NorthShore.closed_classrooms().size() << ","
					<< NorthShore.child_closure_days() << ",";
					// must be done as vectors so that the states stay in predictable order, rather than being sorted
					for(char status : std::vector<int>({'S','E','P','A','I','R'})){ local_output_buffer << NorthShore.agents_proportion(status) << ","; }
					for(char status : std::vector<int>({'S','E','P','A','I','R'})){ local_output_buffer << NorthShore.agents_in_school_proportion(status) << ","; }
				local_output_buffer
					<< NorthShore.locale_infections("background") << ","
					<< NorthShore.locale_infections("home") << ","
					<< NorthShore.locale_infections("class") << ","
					<< NorthShore.locale_infections("commons") << ","
					<< number_of_secondary_infections
				<< '\n';
			};

			// record the initial state of the network
			write_results();

			// in that case, intentionally infect someone in the school see what happens
			do
			{
				// background infection (coffee shops, Walmart, Grindr hookups, you know the drill...)
				for(int susceptible : NorthShore.agents('S'))
				{
					// double the rate for individuals who do not go to the school
					if(NorthShore.Agent(susceptible).classroom() == -1)
					{
						if(randfloat(generator) < Background_Infection_Not_in_School){ NorthShore.set_status(susceptible, 'E', "background"); }
					}
					else
					{
						// just the plain old exposure rate
						if(randfloat(generator) < Background_Infection_in_School){ NorthShore.set_status(susceptible, 'E', "background"); }
					}
				}

				// spreading the infection to everyone living in the flat
				for(int infectious : NorthShore.agents(Infectious_Statuses))
				{
					/*
						right now, we're assuming that children and teachers sent home after outbreaks in their classroom can and will
							effectively self-isolate from other members of the household - a very conservative assumption
						*/
					if(NorthShore.is_in_isolation(infectious)){ continue; }

					// for each infectious person in the simulation, get their flat
					std::vector<int> the_house = NorthShore.household(NorthShore.Agent(infectious).household());
					for(int flatmate : the_house)
					{
						// try to infect all the susceptibles in the flat
						if(flatmate == infectious) { continue; } // can't be the same person
						if(NorthShore.Agent(flatmate).status() != 'S') { continue; } // must be susceptible to the infection
						char sick_age = NorthShore.Agent(infectious).age();
						char mate_age = NorthShore.Agent(flatmate).age();
						// boost B^H by 50% on weekends because of presumably increased interaction
						// boost B_H during reduced hours to represent the increased amount of time spent at home with the flatmates
						// currently_the_weekend is true/false, or 0/1
						// so is the Reduced class time variable
						if(randfloat(generator) <= (1 + 0.5*(!!NorthShore.currently_the_weekend()) + (!!Reduced_Hours))*B_H*NorthShore.home_contact_rate(sick_age, mate_age))
						{
							NorthShore.set_status(flatmate, 'E', "home");
							// if the infection was produced by the index case, mark it as such
							if(infectious == Index_Case){ ++ number_of_secondary_infections; }
						}
					}
				}

				// spreading the infection in the classroom
				for(std::pair<int, std::set<int>> the_class : NorthShore.classrooms())
				{
					// obvious, but we retrieve these lists of infectious and susceptible members here since they vary by classroom
					std::vector<int> infectious_members, susceptible_members;
					std::copy_if(
						the_class.second.begin(),
						the_class.second.end(),
						std::back_inserter(infectious_members),
						[&](int person){ return Infectious_Statuses.count(NorthShore.Agent(person).status()); }
					);
					std::copy_if(
						the_class.second.begin(),
						the_class.second.end(),
						std::back_inserter(susceptible_members),
						[&](int person){ return (NorthShore.Agent(person).status() == 'S'); }
					);

					// actually spread the infection
					for(int inf : infectious_members){ for(int sus : susceptible_members){
						const char Inf_Age = NorthShore.Agent(inf).age();
						const char Sus_Age = NorthShore.Agent(sus).age();
						// halve the in-school transmissions in the reduced hours scenario
						if(randfloat(generator) < (1 - 0.5*(!!Reduced_Hours))*B_C*NorthShore.school_contact_rate(Inf_Age, Sus_Age))
						{
							NorthShore.set_status(sus, 'E', "class");
							// if exposed to the index case, mark it as such
							if(inf == Index_Case){ ++ number_of_secondary_infections; }
						}
					}}
				}

				// infection in the common area - so all agents just crawling all over each other
				for(int inf : NorthShore.agents_in_school(Infectious_Statuses))
				{
					for(int sus : NorthShore.agents_in_school({'S'}))
					{
						// again using the age- and locale-specific contact rates
						const char Inf_Age = NorthShore.Agent(inf).age();
						const char Sus_Age = NorthShore.Agent(sus).age();
						// halve the in-school transmissions in the reduced hours scenario
						if(randfloat(generator) < (1 - 0.5*(!!Reduced_Hours))*B_0*NorthShore.school_contact_rate(Inf_Age, Sus_Age))
						{
							NorthShore.set_status(sus, 'E', "commons");
							// if exposed to the index case, mark it as such
							if(inf == Index_Case){ ++ number_of_secondary_infections; }
						}
					}
				}

				// increment the run time of the sim and record the results
				++run_counter;
				write_results();

				/*
					Increment the number of days, decide which classrooms to shut down and which to reopen, determine whether it's a
						weekend or not, replace or rehire teachers, swap out cohorts for the week, bring recovered people back
						to the classroom, etc. all that good stuff.
				*/
				NorthShore.advance_the_time();

				/*
					we store all the compartments first to make sure that agents aren't processed more than once,
						that is, E->P->A all in one go because of how the transition operations are structured
					alternately, the order of the transitions could just be reversed with the same effect
				*/
				const std::set<int> E_Agents = NorthShore.agents('E');
				const std::set<int> P_Agents = NorthShore.agents('P');
				const std::set<int> I_Agents = NorthShore.agents('I');
				const std::set<int> A_Agents = NorthShore.agents('A');

				// exposed (E) agents become presymptomatic (P)
				for(int exposed : E_Agents){ if(randfloat(generator) < E_to_P_rate){ NorthShore.set_status(exposed, 'P'); } }
				// presymptomatic (P) agents become either symptomatic (I) or asymptomatic (A)
				for(int no_symp	: P_Agents)
				{
					if(randfloat(generator) < P_to_Inf_rate)
					{
						// children and adults have different probabilities of developing symptoms
						if(NorthShore.Agent(no_symp).age() == 'C')
						{
							if(randfloat(generator) < Probability_of_Child_Developing_Symptoms){ NorthShore.set_status(no_symp, 'I'); }
							else { NorthShore.set_status(no_symp, 'A'); }
						}
						else if(NorthShore.Agent(no_symp).age() == 'A')
						{
							if(randfloat(generator) < Probability_of_Adult_Developing_Symptoms){ NorthShore.set_status(no_symp, 'I'); }
							else { NorthShore.set_status(no_symp, 'A'); }
						}
					}
				}
				// symptomatically and asymptomatically infected agents recover/isolate at the given rates
				for(int coughing : I_Agents){ if(randfloat(generator) < I_to_R_rate){ NorthShore.set_status(coughing, 'R'); } }
				for(int fakewell : A_Agents){ if(randfloat(generator) < A_to_R_rate){ NorthShore.set_status(fakewell, 'R'); } }

			}
			while((NorthShore.agents({'P', 'I', 'A', 'E'}).size() != 0) or (NorthShore.closed_classrooms().size() != 0));
			// stopping criteria: All classrooms are open, and there is no possible infection spread in the population

			// get the final state of the sim at the end
			++run_counter;
			write_results();

			// copy the results of the local buffer to either of the two outside the loop depending on whether there were secondaries of not
			if(number_of_secondary_infections == 0){ no_secondary_infections_output << local_output_buffer.str(); }
			else { yes_secondary_infections_output << local_output_buffer.str(); }

			// clear the local buffer for preparation for the next instance
			std::stringstream().swap(local_output_buffer);

			// reset the network before running the next parameter combination
			NorthShore.reset();
		}

		// assemble the title line of the output CSV
		std::stringstream title_line;
		title_line
			<< "Alpha_0" << ","
			<< "Alpha_C" << ","
			<< "B_H" << ","
			<< "B_C" << ","
			<< "B_0" << ","
			<< "prob_same_school" << ","
			<< "prop_childless_teachers" << ","
			<< "background_inf" << ","
			<< "R_init" << ","
			<< "prob_child_symptomatic" << ","
			<< "prob_adult_symptomatic" << ","
			<< "num_houses" << ","
			<< "num_classes" << ","
			<< "size" << ","
			<< "num_adults" << ","
			<< "num_children" << ","
			<< "students_per_class" << ","
			<< "teachers_per_class" << ","
			<< "num_cohorts" << ","
			<< "class_grouping" << ","
			<< "reduced_hours" << ","
			<< "E_to_P" << ","
			<< "P_to_Infected" << ","
			<< "I_to_R" << ","
			<< "A_to_R" << ","
			<< "instance" << ","
			<< "time_step" << ","
			<< "is_weekend" << ","
			<< "num_classes_closed" << ","
			<< "num_student_days_missed" << ",";
			// must be done as vectors so that the states stay in predictable order, rather than being sorted
			for(char status : std::vector<int>({'S','E','P','A','I','R'})){ title_line << "prop_"<< status << ","; }
			for(char status : std::vector<int>({'S','E','P','A','I','R'})){ title_line << "prop_"<< status<<"_in_school" << ","; }
		title_line
			<< "inf_background" << ","
			<< "inf_home" << ","
			<< "inf_class" << ","
			<< "inf_commons" << ","
			<< "secondary_infections"
		<< '\n';

		// write the two data files of the instances where there were/were not secondary infections stemming from the initial case
		if(not yes_secondary_infections_output.str().empty())
		{
			std::ofstream yes_secondary_outFile(Data_Folder + "With_Secondary_Spread_" + File_Stem);
			yes_secondary_outFile << title_line.str() << yes_secondary_infections_output.str();
			yes_secondary_outFile.close();
		}
		if(not no_secondary_infections_output.str().empty())
		{
			std::ofstream no_secondary_outFile(Data_Folder + "No_Secondary_Spread_" + File_Stem);
			no_secondary_outFile << title_line.str() << no_secondary_infections_output.str();
			no_secondary_outFile.close();
		}
	});

TOC(0, true);

	std::exit(EXIT_SUCCESS);

}
