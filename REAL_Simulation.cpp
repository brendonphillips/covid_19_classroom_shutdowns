// g++-9 -g -Wfatal-errors -std=c++17 REAL_Simulation.cpp -o test -ltbb -msse2 -O3

#include "REAL_Town.hpp"
#include <fstream>
#include <chrono>
#include <execution>
#include <filesystem>
#include <tuple>

typedef std::chrono::high_resolution_clock hr_clock;

int main(int argc, char *argv[])
{
	std::srand (std::time(nullptr));

	// disease statuses capable of spreading infection
	const std::set<char> Infectious_Statuses = {'P', 'I', 'A'};
	// potential infection locales in the model
	const std::set<std::string> Infection_Places({"initial", "background", "house", "classroom", "commons"});

	// probability of siblings attending the same childcare centre/school
	const float Probability_Going_Same_Childcare_Centre = 0.8;
	// ratio of teachers living in houses without children
	const float Percentage_Teachers_With_No_Children = 0.36;

	// transition probabilities between disease states (per day)
	const float E_to_P_rate = 0.5;
	const float P_to_Inf_rate = 0.5;
	const float I_to_R_rate = 1;
	const float A_to_R_rate = 0.25;

	// probabilities of agents developing symptoms after exposure
	const float Probability_of_Adult_Developing_Symptoms = 0.6;
	const float Probability_of_Child_Developing_Symptoms = 0.4;

	/*
		to compensate for activities not modelled, we view non-school attendees as having
		twice the probability of community infection
	*/
	const float Background_Multiplier_Rate_For_Those_Who_Dont_Attend_The_Centre = 2;

	// number of classrooms in the model
	const int Number_of_Classrooms = 5;

	// parameter baseline values
	const float Alpha_0_base = 0.0025;
	const float B_H_base = 0.109;
	const float R_init_base = 0.1;
	const float Background_Infection_Rate_base = 1.16e-4;

	// cohort 0 goes to the school every week
	const int Teacher_Cohort = 0;

	// a_0 values for low- and high-transmission scenarios (respectively)
	const std::vector<float> Alpha_C_set = {0.25, 0.75};

	// vectors for parameter values
	std::vector<float> B_H_set {};
 	std::vector<float> Alpha_0_set {};
	std::vector<float> R_init_set {};
	std::vector<float> Background_Infection_Rate_set {};

	// for comparison, we vary each parameter by 50% of its baseline value
	for(float mult=1; mult<1.5; mult+=1)
	// for(float mult=0.5; mult<1.51; mult+=0.5)
	{
		B_H_set.push_back(mult*B_H_base);
		Alpha_0_set.push_back(mult*Alpha_0_base);
		Background_Infection_Rate_set.push_back(mult*Background_Infection_Rate_base);
		R_init_set.push_back(mult*R_init_base);
	}

	// unique filename for each parameter tuple
	auto get_filename = [](const float A0, const float AC, const float BH, const float LAM, const float RI) -> const std::string
	{
		char file_name_buffer [200];
		sprintf(
			file_name_buffer,
			"COVID_Ontario_schools_model_alpha0_%.6f_alphaC_%.3f_BH_%.5f_lambda_%.6f_Rinit_%.3f.csv",
			A0, AC, BH, LAM, RI
		);
		return std::string(file_name_buffer);
	};

	std::vector<std::tuple<float ,float, float, float, float>> Parameter_Tuples {};

	// the set of all parameter combinations desired
	for(float BH : B_H_set){
	for(float AC : Alpha_C_set){
	for(float A0 : Alpha_0_set){
	for(float LAM : Background_Infection_Rate_set){
	for(float RI : R_init_set)
	{
		const std::string file_name = get_filename(A0, AC, BH, LAM, RI);
		// if the parameter combination has already been run, skip it
		// if( std::filesystem::exists(file_name) ){ continue; }
		// else, add it to the tuples
		Parameter_Tuples.push_back(std::make_tuple(BH, AC, A0, LAM, RI) );
	}}}}}

	// children can be assigned to classes randomly, or by keeping siblings together
	const std::set<std::string> Class_Groupings({"random", "siblings"});

	// number teachers, number students per class, number of cohorts, in that order
	const std::set<std::tuple<int, int, int>> Teacher_Student_Ratios_Cohorts({
		std::make_tuple(1,30,1),
		std::make_tuple(1,15,2),
		std::make_tuple(1,15,1),
		std::make_tuple(2,15,1),
		std::make_tuple(2,8,1),
		std::make_tuple(1,8,1),
		std::make_tuple(1,8,2),
		std::make_tuple(3,7,1)
	});

	// 2000 trials per parameter combination; 60% of them are lost due to no spread, so go big or go home
	const int Ensemble_Size = 2000;

	/*
		For each trial, we generate a unique seed for the random generator. For that trial number, each parameter combination
			is run with the same set of pseudo-random numbers to separate the effects of the changes in parameter values from those
			differences caused by the sequences of random numbers generated.
	*/
	std::vector<int> Seeds {};
	hr_clock::time_point beginning = hr_clock::now();
	for(int i=0; i<Ensemble_Size; ++i)
	{
		hr_clock::duration d = hr_clock::now() - beginning;
		Seeds.push_back(d.count());
	}

	std::for_each(std::execution::seq, Parameter_Tuples.begin(), Parameter_Tuples.end(), [&](auto&& parameter_tuple) // for testing
	// std::for_each(std::execution::par_unseq, Parameter_Tuples.begin(), Parameter_Tuples.end(), [&](auto&& parameter_tuple) // for the actual run
	{
		// take a parameter tuple and get the individual parameter values
		const float B_H = 							std::get<0>(parameter_tuple);
		const float Alpha_C =  						std::get<1>(parameter_tuple);
		const float Alpha_0 =  						std::get<2>(parameter_tuple);
		const float Background_Infection_Rate = 	std::get<3>(parameter_tuple);
		const float R_init =  						std::get<4>(parameter_tuple);

		// calculate B_C, B_0 according to the draft
		const float B_C = Alpha_C*B_H;
		const float B_0 = Alpha_0*B_C;

		// make the file name
		const std::string Data_File_Name = get_filename(Alpha_0, Alpha_C, B_H, Background_Infection_Rate, R_init);

		// stream store the output data
		std::stringstream output {};

		for(int inst = 0; inst < Ensemble_Size; ++inst)
		{
			// beginning each instance with the same seed to generate the same series of numbers
			// this will help tease out the true effects of the different arrangements and ratios
			std::mt19937 generator(Seeds[inst]);
			std::uniform_real_distribution<float> randfloat(0,1);

		for(const std::string Classroom_Arrangement : Class_Groupings){
		for(const std::tuple<int, int, int> T_S_Ratio_Coh : Teacher_Student_Ratios_Cohorts)
		{
			const int Num_Teachers_per_Classroom = std::get<0>(T_S_Ratio_Coh);
			const int Num_Children_per_Classroom = std::get<1>(T_S_Ratio_Coh);
			const int Num_Child_Cohorts = std::get<2>(T_S_Ratio_Coh);

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

				// keeping siblings together
				if(Classroom_Arrangement == "siblings")
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
						for(int class_number : available_classes)
						{
							// get the number of seats available
							const int Seats_Available = Num_Children_per_Classroom - NorthShore.classroom(class_number).size();
							// if there's no space left, move to the next classroom
							if(not Seats_Available)
							{
								continue;
							}
							// if we can't fit this sibling group into this class, move on to the next one
							if(this_family.size() > Seats_Available)
							{
								continue;
							}
							// else, if there's room, shove them in there
							for(int child : this_family){ NorthShore.set_classroom(child, class_number, cohort_number); }
							// if there's no space left in this class, remove this classroom from the availability list
							if((Num_Children_per_Classroom - NorthShore.classroom(class_number).size()) == 0)
							{
								available_classes.erase(class_number);
							}
							// we're done with this family
							this_family.clear();
							// we've put them in, so no need to keep searching classrooms - move on to the next group
							break;
						}

						if(not this_family.empty()){ whole_family_couldnt_fit_into_a_single_classroom.push_back(this_family); }
					}
					while(children_enrolled_by_house.size()); // keep going while we still have unenrolled children left

					// if we couldn't fit the entire sibling group into a classroom, break them up among the classrooms with space in them
					for(std::vector<int> family : whole_family_couldnt_fit_into_a_single_classroom)
					{
						for(int child : family) // for every child, find a classroom with space and put them in there
						{
							for(int class_number : available_classes)
							{
								if((Num_Children_per_Classroom - NorthShore.classroom(class_number).size())!=0){ NorthShore.set_classroom(child, class_number, cohort_number); break; }
								// if there's isn't a free table, mark that classroom as full and move on
								else { available_classes.erase(class_number); }

							}
						}
					}
				}

				// randomly assigning them - arguably easier
				else if(Classroom_Arrangement == "random")
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

				// we're not studying any other classroom arrangements (grouping by age, etc.)
				else
				{
					std::cout << "\nClassroom arrangement not found" << std::endl;
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
			std::vector<int> Susceptibles(S_agents.begin(), S_agents.end());
			std::random_shuffle(Susceptibles.begin(), Susceptibles.end());
			// infect this index case
			const int Index_Case = Susceptibles.front();
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
				output
					<< Alpha_0 << ","
					<< Alpha_C << ","
					<< B_H << ","
					<< B_C << ","
					<< B_0 << ","
					<< Probability_Going_Same_Childcare_Centre << ","
					<< Percentage_Teachers_With_No_Children << ","
					<< Background_Infection_Rate << ","
					<< R_init << ","
					<< Probability_of_Child_Developing_Symptoms << ","
					<< Probability_of_Adult_Developing_Symptoms << ","
					<< NorthShore.num_households() << ","
					<< NorthShore.num_classrooms() << ","
					<< NorthShore.num_agents() << ","
					<< NorthShore.num_age('A') << ","
					<< NorthShore.num_age('C') << ","
					<< Num_Children_per_Classroom << ":" << Num_Teachers_per_Classroom << ","
					<< Num_Child_Cohorts << ","
					<< Classroom_Arrangement << ","
					<< E_to_P_rate << ","
					<< P_to_Inf_rate << ","
					<< I_to_R_rate << ","
					<< A_to_R_rate << ","
					<< inst << ","
					<< run_counter << ","
					<< NorthShore.currently_the_weekend() << ","
					<< NorthShore.closed_classrooms().size() << ","
					<< NorthShore.child_closure_days() << ",";
					// must be done as vectors so that the states stay in predictable order, rather than being sorted
					for(char status : std::vector<int>({'S','E','P','A','I','R'})){ output << NorthShore.agents_proportion(status) << ","; }
					for(char status : std::vector<int>({'S','E','P','A','I','R'})){ output << NorthShore.agents_in_school_proportion(status) << ","; }
				output
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
						if(randfloat(generator) < Background_Multiplier_Rate_For_Those_Who_Dont_Attend_The_Centre*Background_Infection_Rate)
						{
							NorthShore.set_status(susceptible, 'E', "background");
						}
					}
					else
					{
						// just the plain old exposure rate
						if(randfloat(generator) < Background_Infection_Rate){ NorthShore.set_status(susceptible, 'E', "background"); }
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
						// currently_the_weekend is true/false, or 0/1
						if(randfloat(generator) <= (1 + 0.5*NorthShore.currently_the_weekend())*B_H*NorthShore.home_contact_rate(sick_age, mate_age))
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
						if(randfloat(generator) < B_C*NorthShore.school_contact_rate(Inf_Age, Sus_Age))
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
						if(randfloat(generator) < B_0*NorthShore.school_contact_rate(Inf_Age, Sus_Age))
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
				const std::set<int> E_Agents  = NorthShore.agents('E');
				const std::set<int> P_Agents  = NorthShore.agents('P');
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

			// reset the network before running the next parameter combination
			NorthShore.reset();

		}}}

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
			<< "child_teacher_ratio" << ","
			<< "num_cohorts" << ","
			<< "class_grouping" << ","
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

		// write the data file to the given folder
		std::ofstream outFile(Data_File_Name);
		outFile << title_line.rdbuf() << output.rdbuf();
		outFile.close();
	});

	std::exit(EXIT_SUCCESS);
}
