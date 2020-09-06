#ifndef REAL_TOWN_HPP_
#define REAL_TOWN_HPP_

/*
	I put *lots* of assertions in the code to make sure I didn't misspell or misroute anything.
	Turn all the asserts off when gathering data. I reckon it's ~20 times faster without the assertions.
*/
// #define NDEBUG

#include "REAL_Person.hpp"
#include "prettyprint.hpp" // convenient for printing the contents of the STL containers without constantly writing loops
#include <numeric>
#include <cassert> // assertions to check the inputs to some of the functions
#include <vector>
#include <map>
#include <set>
#include <random>
#include <sstream> // used in the "print to the terminal", since C++ can't concatenate strings on the fly

std::random_device rd;
std::mt19937 generator(rd());

template<typename TK, typename TV> std::set<TK> extract_keys(std::map<TK, TV> const& input_map)
{
	std::set<TK> retval;
	for(auto const& element : input_map) retval.insert(element.first);
	return retval;
}
template<typename TK, typename TV> std::set<TV> set_of_values(std::map<TK, TV> const& input_map)
{
	std::set<TV> retval;
	for(std::pair<TK, TV> const& element : input_map) retval.insert(element.second);
	return retval;
}
template<typename TV> std::set<TV> set_of_values(std::set<std::set<TV>> const& input_map)
{
	std::set<TV> retval;
	for(std::set<TV> const& element : input_map) retval.insert(element.begin(), element.end());
	return retval;
}
template<typename TV> std::set<TV> set_of_values(std::vector<std::vector<TV>> const& input_map)
{
	std::set<TV> retval;
	for(std::vector<TV> const& element : input_map) retval.insert(element.begin(), element.end());
	return retval;
}


// template<typename TK, typename TV> std::vector<TV> vector_of_values(std::map<TK, TV> const& input_map)
// {
// 	std::vector<TV> retval;
// 	for(std::pair<TK, TV> const& element : input_map) retval.push_back(element.second);
// 	return retval;
// }
template<typename TK, typename TV> int number_of_values(std::map<TK, TV> const& input_map)
{
	int retval=0;
	for(std::pair<TK, TV> const& element : input_map) retval += element.second.size();
	return retval;
}
template<typename TV> int number_of_values(std::vector<std::vector<TV>> const& input_vec)
{
	int retval=0;
	for(std::vector<TV> const& element : input_vec) retval += element.size();
	return retval;
}

class Town
{
	private:

		// Child-Adult contact matrices built using the Canada-specific household and school contact matrices given be Prem et. al. (2017)
		std::map<std::pair<char, char>, float> _school_contact_matrix = { {{'C','C'}, 1.2355}, {{'C','A'}, 0.0589}, {{'A', 'C'}, 0.1176}, {{'A','A'}, 0.0451} };
		std::map<std::pair<char, char>, float> _home_contact_matrix = { {{'C','C'}, 0.5378}, {{'C', 'A'}, 0.3916}, {{'A','C'}, 0.3632}, {{'A','A'}, 0.3335} };

		// vector to hold all the persons in the simulation
		std::vector<Person> _Population;
		// vector to hold the numbers of all the agents in the simulation. used for iteration in loops
		std::vector<int> _agent_IDs;

		// these are the households; the key is the number of the household, and the value is the set of IDs of the agents in that flat
		// ex. household number 1: agents: 2, 5, 76, 9
		std::map<int, std::set<int>> _households;

		// this is the single school in the population; each classroom will have an assigned number and a set of student IDs
		std::map<int, std::set<int>> _school;

		/*
			Their plan is, if there's an outbreak in a class, shut it down for 14 days.

			Key - the number of the classroom
			Value - the number of school days for which it has been shut down so far

			1) This structure will track the number of days for which the classroom was closed per closure.
			2) Trying to print a shut down classroom will just give a "stay away if you want to live" message.
			3) There will be a getter function that tells whether a particular room is shut down
		*/
		std::map<int, int> _classroom_num_days_shut_down_due_to_illness;

		/*
			Scenario: a teacher originally assigned to some room becomes ill and is sent home. They must be replaced.

			Key - the original teacher for that classroom.
			Value - the substitute teacher hired to replace the original.

			When a teacher becomes symptomatically infected, they go home for 14 days and are replaced by a new teacher.
			When the original teacher recovers (14 days in isolation is up), then they return and the substitute is sacked.
			This sick list keeps track of all the teachers that are out sick and the replacements hired to cover for them.
			This is a hang-over from a previous iteration of the model (with no class shutdowns, where it was possible for
				substitutes to get sick and also need replacement), but helps us to remember who the "original" teacher was
				for each particular class so they can be brought back to their original class after recovery.
			Every time step, check the isolation time of each OG teacher (each key) and if longer than 14 days, reinstate
				them to their classroom number, sack their replacement (that is, remove them from the current classroom and set their
				classroom characteristic to 0)
		*/
		std::map<int, int> _substitute_list_OGs_first;

		/*
			set of agents for every disease state - this makes it easier to find specifically the infected nodes, for example

			Key - disease state (one of S, E, P, A, I, R)
			Value - set of IDs of the agents currently in that stage of disease progression

			this way, we don't need to keep traversing the list of all agents each time we're looking for a specific subgroup.

			Statuses:
				S - susceptible, they get infected
				E - exposed, useless fodder
				P - presymptomatic, they're infectious but we wouldn't reasonably expect the development of symptoms at this stage
				A - asymptomatically infectious, can spread the disease but appear hale and hearty - this is why we need widespread masking
				I - symptomatically infectious, they're grounded for 14 days until their COVID-19 infection has learnt its lesson
				R - removed/recuperating/recovered - they spend 14 days in isolation, then recover and go back to school/work like normal
				    ("What? COVID-19? I don't know her."...)
		*/
		std::map<char, std::set<int>> _disease_compartments;

		/*
			The set of allowable disease statuses in the model. For error checks, I'll assert that disease statuses in the sim must be
				a member of this set. Else, fail.
		*/
		const std::set<char> _disease_statuses = {'S', 'E', 'P', 'I', 'A', 'R'};

		/*
			A counter of the number of the days in the simulation

			Previously, I kept track of this in the main sim file, but since we have weekends and alternating cohorts, we need to track it here
			1 calendar week = 7 days
			1 school week = 5 days
			We assume that Day 0 (0 the top of the sim) is Monday

			If students alternate, then it's one week on, one week off for them as expected.
			Weekends are not counted as missed student days.
			Symptomatic infections over the weekend still shut the class down on the following Monday
				(we assume that parents report their child's illness and the class is shut preemptively).
		*/
		int _run_time;

		/*
			Lists of the IDs of students in each cohort in the school

			Key - number of the cohort
			Value - set of IDs of the students in that cohort

			If there's a single cohort (1), then obviously they don't alternate; we just see them every week.
			With two cohorts (1, 2), then coh 1 spends a week in class while coh 2 is at home, and then it's reversed the next week, etc.

			Cohort 0 is the group of agents that are in class every week without alternation, even if students are placed in cohorts. Since
				teachers are assigned to the class, then both cohorts in the same classroom will share the same teacher. So the teacher will be
				in class every week without alternation (unless replaced by a substitute when they fall ill)

			Ex. First week: 	Teacher: Comrade Ogilvy,	Students: cohort 1
				Second week: 	Teacher: Comrade Ogilvy,	Students: cohort 2
				Third week:		Teacher: Comrade Ogilvy,	Students: cohort 1
				und so weiter...
		*/
		std::map<int, std::set<int>> _the_cohorts;

		/*
			This structure keeps track of where exactly each node got infected; it'd be nice to know which places are the most dangerous

			Key: the specific location in the model: household ("home"), classroom ("class"), common areas ("commons"), community infection ("background")
			Value: a set giving the IDs of all the agents infected at that site

			Agents are entered in here when they become exposed (status E).
			The index case is given status P, so we don't care about them; that just happened.
		*/
		std::map<std::string, std::set<int>> _places_infected;

		// list of all the valid infection locations in the model
		const std::set<std::string> _infection_places = {"background", "home", "class", "commons"};

		// check functions for assert statements - making sure I didn't do anything stupid
		bool check_agent_number(const int index) const { return (index >= 0) & (index <= _Population.size()); } // checks that the agent with that number exists
		bool check_disease_status(const char state) const { return (_disease_statuses.count(state) != 0); } // checks that the agent has a SEPAIR disease status
		bool check_infection_locale(const std::string place) const { return (_infection_places.count(place) != 0); } // checks that the place infected is one of the allowed options
		bool check_cohort_number(const int person) const { return _Population[person].cohort() != -1; } // allowed cohort number
		bool check_classroom_status(const int classr){ return true; }

		/*
			If a teacher from the school falls ill, they must be replaced. This functions goes through the entire network searching
				teachers that match the following criteria:

			1) There can only be one teacher per household,
			2) A substitute cannot be accepted from a house with children enrolled at the school,
			3) The substitute must be adult,
			4) The substitute must either be asymptomatic or fully recovered.
		*/
		void replace_sick_teacher(const int sick_teacher)
		{
			assert(check_agent_number(sick_teacher));

			// sanity checks to see that the original teacher is an adult, and currently assigned to a classroom
			assert(_Population[sick_teacher].age() == 'A');
			assert(_Population[sick_teacher].classroom() != -1);

			// check that the teacher has actually been "sent home"
			assert(is_in_isolation(sick_teacher));

			/*
				This variable holds the candidate for substitute teacher. We'll pull an agent and then check the criteria
					one by one. If any fail, we pull another agent, rinse and repeat. If enough households were not generated,
					it's possible that no substitutes can be found that fit the given criteria. So, if this number is still -1
					at the end of the loop, print a big ass warning and rage quit the entire simulation like a true legend.
			*/
			int substitute_teacher = -1;

			// find teacher households - in a suitable house, everyone should have classroom -1
			for(std::pair<int, std::set<int>> the_pair : _households)
			{
				// if anyone in this house is assigned a spot in the school, move on to another house
				bool skip_to_next_house = false;
				for(int person : the_pair.second)
				{
					if(_Population[person].classroom() != -1)
					{
						skip_to_next_house = true;
						break;
					}
				}
				if(skip_to_next_house) continue;

				/*
					Now that we've found a viable house to exploit, find each adult in the house and see if they're isolating.
					If we find a single asymptomatic adult, hallelujah, accept the candidate substitute and move on.
				*/
				for(int adult : adults_in_household(the_pair.first))
				{
					if(not is_in_isolation(adult))
					{
						substitute_teacher = adult;
						break;
					}
				}

				// if we haven't found a substitute yet, skip to the next house
				if(substitute_teacher == -1){ continue; }
				else { break; }
			}

			// If we've gone through all the houses and still haven't found a substitute, quit.
			if(substitute_teacher == -1)
			{
				// fail fast. fail hard. fail loud. give receipts.
				std::cerr << "\n###### REPLACEMENT TEACHER NOT FOUND. No households to draw from (receipts below). ######" << std::endl;
				print_households();
				std::cerr << "\n###### REPLACEMENT TEACHER NOT FOUND. Receipts above (no households to draw from). ######" << std::endl;
				std::exit(EXIT_FAILURE);
			}

			// get the classroom that the original (now sick) teacher was assigned to. we'll remove and replace them.
			const int classroom_needing_a_new_teacher = _Population[sick_teacher].classroom();

			// if they themselves were a substitute, find the original teacher that they were subbing for and mark the new sub as temping for the original one
			if( set_of_values(_substitute_list_OGs_first).count(sick_teacher) )
			{
				int OG_teacher = -1;

				// find who they were subbing for
				for(std::pair<int, int> the_pair : _substitute_list_OGs_first)
				{
					if(the_pair.second==sick_teacher)
					{
						OG_teacher = the_pair.first;
						break;
					}
				}
				// mark them as a replacement
				_substitute_list_OGs_first[OG_teacher] = substitute_teacher;
				_Population[sick_teacher].set_classroom(-1);
			}
			else // they themselves *are* the original teacher
			{
				assert(not extract_keys(_substitute_list_OGs_first).count(sick_teacher));
				assert(not set_of_values(_substitute_list_OGs_first).count(sick_teacher));

				// note their substitute
				_substitute_list_OGs_first[sick_teacher] = substitute_teacher;
			}

			// sent the sick teacher home by taking them out of their classes
			_school[classroom_needing_a_new_teacher].erase(sick_teacher);
			_the_cohorts[0].erase(sick_teacher);

			// hire and onboard the substitute
			_school[classroom_needing_a_new_teacher].insert(substitute_teacher); // put them in the classroom
			_the_cohorts[0].insert(substitute_teacher); // they'll report to school every day, so put them on cohort 0

			// set their individual characteristics
			_Population[substitute_teacher].set_classroom(classroom_needing_a_new_teacher);
			_Population[substitute_teacher].set_cohort(0);

			return;
		}

		/*
			Say one of the teachers originally assigned to the school recovered from their illness and is now eligible for
				return to class. This function brings them back and tosses the sub out.
		*/
		void rehire_recovered_OG_teacher(const int recovered_teacher)
		{
			// should be adult, have a classroom number and be out of isolation
			assert(_Population[recovered_teacher].classroom() != -1);
			assert(_Population[recovered_teacher].age() == 'A');
			assert(not is_in_isolation(recovered_teacher));
			// assert(_Population[recovered_teacher].status() == 'R');

			// sanity check - if the teacher isn't one of the original ones, don';t try to *re*hire them - that makes no sense
			if(not extract_keys(_substitute_list_OGs_first).count(recovered_teacher)){ return; };
			// sanity check - the one we're trying to *re*hire isn't a substitute
			assert(not set_of_values(_substitute_list_OGs_first).count(recovered_teacher));
			assert(_substitute_list_OGs_first.count(recovered_teacher));
			// assert(_Population[recovered_teacher].days_since_first_symptoms() >= 14);

			// find the teacher currently subbing for them
			const int teacher_substituting_for_them = _substitute_list_OGs_first[recovered_teacher];
			const int classroom_number = _Population[recovered_teacher].classroom();

			// the classroom numbers of the recovered teacher and the temp should be the same
			assert(classroom_number == _Population[teacher_substituting_for_them].classroom());

			// sack the sub, and change their individual characteristics
			_school[classroom_number].erase(teacher_substituting_for_them);
			_Population[teacher_substituting_for_them].set_classroom(-1);
			_Population[teacher_substituting_for_them].set_cohort(-1);
			_the_cohorts[0].erase(teacher_substituting_for_them);

			// rehire the recovered teacher
			_school[classroom_number].insert(recovered_teacher);
			_substitute_list_OGs_first.erase(recovered_teacher);
			_the_cohorts[0].insert(recovered_teacher);
		}

	public:

		/*
			the only functions represented here are the ones that directly involve changing variables in this class
			any function requiring simple information about the agent in question can be accessed by <Town object>.Agent(<agent number>).<getter function>
			I think it rather clarifies matters to have the class handle only things that relate to it
		*/

		// iterating over the network in a loop is more convenient
		std::vector<int>::iterator begin() noexcept { return _agent_IDs.begin(); }
		std::vector<int>::iterator end() { return _agent_IDs.end(); }

		// Canada-specific school contact matrix for individual interactions
		const float school_contact_rate(const char state_A, const char state_B /* this order matters, so don't change this */)
		{
			assert(std::set<char>({'A','C'}).count(state_A));
			assert(std::set<char>({'A','C'}).count(state_B));
			return _school_contact_matrix[{state_A, state_B}];
		}

		// Canada-specific household contact matrix for individual interactions
		const float home_contact_rate(const char state_A, const char state_B /* this order matter, so don't change this */)
		{
			assert(std::set<char>({'A','C'}).count(state_A));
			assert(std::set<char>({'A','C'}).count(state_B));
			return _home_contact_matrix[{state_A, state_B}];
		}

		// number of children and adults in a household, using 2018 StatCan census data
		const std::pair<int, int> children_and_adults_household_size_distribution(const float pick_a_float__any_float)
		{
			/*
				these are the cumulative sums of the probabilities given for family size and adult:child distribution
				we don't need else statements. the first one of these that's true, the function immediately exits
				trivially the other conditions below it will not be checked.

			*/
			if( pick_a_float__any_float < 0.169 ) { return {1,1}; }
			if( pick_a_float__any_float < 0.284 ) { return {2,1}; }
			if( pick_a_float__any_float < 0.267 ) { return {3,1}; }
			if( pick_a_float__any_float < 0.274 ) { return {4,1}; }
			if( pick_a_float__any_float < 0.277 ) { return {5,1}; }
			if( pick_a_float__any_float < 0.561 ) { return {1,2}; }
			if( pick_a_float__any_float < 0.868 ) { return {2,2}; }
			if( pick_a_float__any_float < 0.954 ) { return {3,2}; }
			if( pick_a_float__any_float < 0.978 ) { return {4,2}; }
												  { return {5,2}; }
		}

		// number of adults in childless households, based on 2018 StatCan census data
		const int just_adults_household_size_distribution(const float pick_a_float__any_float)
		{
			if( pick_a_float__any_float < 0.282 ) { return 1; }
			if( pick_a_float__any_float < 0.627 ) { return 2; }
			if( pick_a_float__any_float < 0.779 ) { return 3; }
			if( pick_a_float__any_float < 0.917 ) { return 4; }
			if( pick_a_float__any_float < 0.972 ) { return 5; }
			if( pick_a_float__any_float < 0.993 ) { return 6; }
												  { return 7; }
		}

		void reset()
		{{ // double brace for code folding
			_Population = {};
			_agent_IDs = {};
			_households = {};
			_school = {};
			_disease_compartments = {};
		}}

		/* GETTERS */

		// in the case of alternating cohorts, see which one is in class this week
		const int this_weeks_cohort()
		{
			std::set<int> all_the_cohorts = extract_keys(_the_cohorts);
			all_the_cohorts.erase(0);
			all_the_cohorts.erase(-1);
			if(all_the_cohorts.empty()){ return 0; }
			return (_run_time/7)%all_the_cohorts.size()+1;
		}

		// get the day of the week 0-6, where 0 is Monday
		const int day_of_the_week()
		{
			return _run_time%7;
		}

		// get the time stamp of the simulation
		const int days_elapsed()
		{
			return _run_time;
		}

		// check whether it's currently the weekend (Sat. or Sun., days 5 or 6)
		const bool currently_the_weekend()
		{
			return std::set<int>({5,6}).count(day_of_the_week());
		}

		// function tells whether the agent is isolating or not, i.e. symptomatic, with less than 14 days since the onset of symptoms
		const bool is_in_isolation(const int agent)
		{
			// shorter handle for the agent we want
			Person* them = &_Population[agent];

			// the isolation starts the day *after* the first symptoms, so day 1 and forward
			if(them->days_since_first_symptoms() < 1) return false;
			// it they're past the isolation time then they can venture out into the world again
			if(them->days_since_first_symptoms() >= 14) return false;
			return true;

		}

		// size of the network
		const int num_agents() const { return _Population.size(); }

		// returns the number of agents with the categorical age specified
		const int num_age(const char the_age) const
		{
			assert((the_age == 'A') or (the_age == 'C'));
			int count = 0;
			for(Person agent : _Population){ count += (agent.age() == the_age); }
			return count;
		}

		// returns the number of days (not counting weekends) for which the specified classroom has remained shut down due to infection
		const int classroom_closure_time(const int classr)
		{
			assert( check_classroom_status(classr) );
			if(not _classroom_num_days_shut_down_due_to_illness.count(classr)){ return 0; }
			return _classroom_num_days_shut_down_due_to_illness[classr];
		}

		// TRUE/FALSE whether the specified classroom is closed due to infection
		const bool classroom_closed_due_to_infection(const int classr)
		{
			assert( check_classroom_status(classr) );
			return _classroom_num_days_shut_down_due_to_illness.count(classr);
		}

		// TRUE/FALSE whether every class in the school is shut for infection
		const bool school_closed_due_to_infection()
		{
			for(const int classr : extract_keys(_school))
			{
				assert( check_classroom_status(classr) );
				if(not _classroom_num_days_shut_down_due_to_illness.count(classr)){ return false; }
				if(_classroom_num_days_shut_down_due_to_illness[classr] == 0){ return false; }
			}
			return true;
		}

		// returns the numbers of the classrooms that are closed
		const std::set<int> closed_classrooms()
		{
			return extract_keys(_classroom_num_days_shut_down_due_to_illness);
		}

		// returns the number of missed student-days in this tine step due to classroom closures
		const int child_closure_days()
		{
			// if it's the weekend, children wouldn't have been in school anyway
			if(currently_the_weekend()) return 0;
			// counter for the number of days
			int child_wasted_days_count = 0;
			// checking every agent
			for(Person child : _Population)
			{
				// must be a child
				if(child.age() != 'C'){ continue; }
				// must be assigned a classroom
				if(child.classroom() == -1){ continue; }
				// not isolating **due to illness** - eligible for return to class
				if(/*coronacated*/ is_in_isolation(child.ID()) and /*actually sick*/ std::set<char>({'I','R'}).count(child.status()) ){ continue; }
				// their classroom should be closed
				if(not classroom_closed_due_to_infection(child.classroom())){ continue; }
				// it also doesn't count as a wasted day if their cohort isn't the one in class this week anyway
				if(child.cohort() != this_weeks_cohort()){ continue; }

				++ child_wasted_days_count;
			}
			return child_wasted_days_count;
		}

		// takes an ID number and returns a Person object - handy for getting the individual characteristics in the main sim file
		Person Agent(const int index)
		{
			return _Population[index];
		}
		// the total number of households in the model
		const int num_households()
		{
			return _households.size();
		}

		// the number of agents in a given household
		const int household_size(const int index)
		{
			assert(_households.count(index));
			return _households[index].size();
		}

		// returns the IDs of the agents in a given household
		const std::vector<int> household(const int index)
		{
			assert(_households.count(index));
			return std::vector<int>(_households[index].begin(), _households[index].end());
		}

		// return all households. Key - house number. Value - set of all the agents in that house
		const std::map<int, std::set<int>> households(){ return _households; }

		const std::vector<int> adults_in_household(const int index)
		{
			assert(_households.count(index));
			std::vector<int> the_adults;
			std::copy_if(
				_households[index].begin(),
				_households[index].end(),
				std::back_inserter(the_adults),
				[=](int person){ return (_Population[person].age()=='A'); }
			);
			return the_adults;
		}

		// return the IDs of all the children in a given house
		const std::vector<int> children_in_household(const int index)
		{
			assert(_households.count(index));
			std::vector<int> the_children;
			std::copy_if(
				_households[index].begin(),
				_households[index].end(),
				std::back_inserter(the_children),
				[=](int person){ return (_Population[person].age()=='C'); }
			);
			return the_children;
		}

		// a set of all the house numbers in the population
		const std::set<int> home_addresses() { return extract_keys(_households); }

		// number of valid classrooms in the school
		const int num_classrooms() const { return _school.size()-_school.count(-1); }

		// number of attendees actually in a class at the moment (so not counting students that are out sick)
		const int class_size(const int class_number) { return _school[class_number].size(); }

		// return a set of the IDs of agents in a given class
		const std::set<int> classroom(const int class_number) { return _school[class_number]; }

		// all the classrooms in the school. Key-  class number. Value - set of IDs of agents sitting in that class at the moment
		const std::map<int, std::set<int>> classrooms()
		{
			std::map<int, std::set<int>> the_classes = _school;
			the_classes.erase(-1);
			return the_classes;
		}

		// get the IDs of teachers in the given classroom - in the model, could be either one or two
		const std::vector<int> teachers_in_classroom(const int index)
		{
			assert(_school.count(index));
			std::vector<int> the_adults;
			std::copy_if(
				_school[index].begin(),
				_school[index].end(),
				std::back_inserter(the_adults),
				[=](int person)
				{
					assert(not is_in_isolation(person));
					return (_Population[person].age()=='A');
				}
			);
			return the_adults;
		}

		// get the IDs of children in the given classroom
		const std::vector<int> children_in_classroom(const int index)
		{
			assert(_school.count(index));
			std::vector<int> the_children;
			std::copy_if(
				_school[index].begin(),
				_school[index].end(),
				std::back_inserter(the_children),
				[=](int person)
				{
					assert(not is_in_isolation(person));
					return (_Population[person].age()=='C');
				}
			);
			return the_children;
		}

		// return a set of all the IDs of agents in the given single stage of the SEPAIR disease progression
		const std::set<int> agents(const char the_status = '\0')
		{
			if(the_status == '\0'){ return std::set<int>(_agent_IDs.begin(), _agent_IDs.end()); }
			assert(check_disease_status(the_status));
			return _disease_compartments[the_status];
		}

		/*
			returns a collection of all the agent IDs in multiple states in the disease progression
			Ex. agents({'S','E'}) will return a collection of susceptible and exposed agents
		*/
		const std::set<int> agents(std::set<char> the_statuses)
		{
			std::set<int> the_collection {};
			for(char one_state : the_statuses)
			{
				if(not _disease_compartments.count(one_state)){ continue; }
				std::set<int> temp = _disease_compartments[one_state];
				std::copy(temp.begin(), temp.end(), std::inserter(the_collection, the_collection.begin()));
			}
			return the_collection;
		}

		// returns the proportion of agents with the given disease status
		const float agents_proportion(const char the_status)
		{
			if(_disease_compartments.count(the_status))
			{
				return _disease_compartments[the_status].size()/(1.*_Population.size());
			}
			return 0;
		}

		/*
			gives a set of IDs of the agents currently in school (that is, not sick teachers or children home from school,
				or children in the cohort not meeting this week) with the given disease statuses
		*/
		const std::set<int> agents_in_school(const std::set<char> the_statuses = {})
		{
			// no one's in school on the weekend
			if(currently_the_weekend()) return {};
			for(char state : the_statuses){ assert(check_disease_status(state)); }

			// get the disease statuses we want the attendees to have
			std::set<char> all_them {};
			if(the_statuses.empty()){ all_them = _disease_statuses; }
			else{ all_them = the_statuses; }

			// get all the people in the school - this is anyone with non-trivial classroom number in an open classroom and isolation counter either 0 or above 14
			// this is sufficient to catch the children out of school, the subs serving at the school while filtering out the OG teachers recovering at home
			// we want an asses-in-the-seats count
			std::set<int> school_attendees_in_state {};
			for(char one_state : all_them)
			{
				std::copy_if(
					_agent_IDs.begin(),
					_agent_IDs.end(),
					std::inserter(school_attendees_in_state, school_attendees_in_state.begin()),
					[=](int agent){
						if(_Population[agent].status() != one_state){ return false; } // in the correct disease state
						if(classroom_closed_due_to_infection(_Population[agent].classroom())){ return false; } // classroom open
						if(_Population[agent].classroom() == -1 ){ return false; } // assigned to a room in the centre
						if(is_in_isolation(agent)){ return false; } // must be healthy and fit to be in class
						if(not std::set<int>({this_weeks_cohort(), 0}).count(_Population[agent].cohort()) ){ return false; } // in this week's cohort
						return true;
					}
				);
			}
			return school_attendees_in_state;
		}

		// proportion of agents currently in school with the given disease status
		const float agents_in_school_proportion(const char the_status)
		{
			return agents_in_school({the_status}).size()/(1.*agents_in_school().size());
		}

		// number of infections occurring in the requested location
		const int locale_infections(const std::string place)
		{
			if(place == ""){ return -1; };
			if(not _places_infected.count(place)){ return 0; }
			return _places_infected[place].size();
		}

		/* SETTERS */

		/*
			add an agent to the simulation, including setting classroom and cohort, etc.
		*/
		void add_agent(Person them)
		{
			assert(check_disease_status(them.status()));

			// the new agent will be pushed to the back of the vector of agents, and will get the next biggest number integer available
			int temp_identity = _Population.size();
			_Population.push_back(them); // add the object to the vector of objects
			_agent_IDs.push_back(temp_identity); // add their ID to the list of IDs

			// we know what the number will be, since we're always pushing at the back
			_households[ them.household() ].insert(temp_identity); // put them in the requested household
			_disease_compartments[them.status()].insert(temp_identity); // add them to the specified disease compartment

			// if we add an exposed agent, we don't care where they were infected - that's a them problem
			// so we're not setting an infection locale for the node here

			//sorting out individual characteristics of the agent
			_Population.back().set_identity(temp_identity); //ID
			_Population.back().set_household(them.household()); // household
			_Population.back().set_age(them.age()); // age
			_Population.back().set_status(them.status()); // stage of disease progression
			// _Population.back().set_classroom(them.classroom()); // classroom they're in
			_Population.back().set_days_since_first_symptoms(them.days_since_first_symptoms()); // time since the first cough

			set_classroom(temp_identity, them.classroom(), them.cohort()); // insert the node into the requested classroom and cohort
		}

		/*
			this function wraps up all the events at the end of each time step, including closing and reopening classrooms,
				emptying the school on the weekend, hiring substitutes and filling the class with the right cohorts for each week
		*/
		void advance_the_time()
		{
			/*
				I know, I know, this function could have been put together better...
				There's nothing more permanent than a temporary solution that works.
			*/

			_run_time += 1;

			// if it's Friday, all the classes get out - regardless of disease or not - this clears space for the new cohort
			if(day_of_the_week() == 5)
			{
				for(int classr : extract_keys(_school)){ _school[classr].clear(); }
			}

			// if anyone is symptomatic for the second day and they're in a class that hasn't been shut, then shut down the class
			for(const int person : agents({'I', 'R'}))
			{
				assert(check_agent_number(person));

				Person* them = &_Population[person];

				// if they're symptomatic but not recovered yet, mark one more day passed
				if((them->days_since_first_symptoms() > -1) and (them->days_since_first_symptoms() < 14))
				{
					them->add_day_since_first_symptoms();
				}
				// checking to see which classrooms to shut down
				if( (them->classroom() != -1) and (not classroom_closed_due_to_infection(them->classroom())) )
				{
					// sick and currently running amok in the classroom
					if( (them->days_since_first_symptoms() - them->weekend_offset()) == 1)
					{
						// shut the classroom down
						_classroom_num_days_shut_down_due_to_illness[them->classroom()] = 0;
						// no one in the classroom anymore
						_school[them->classroom()].clear();

						//////////////// THIS CAN BE REMOVED IF WE WANT TO ASSUME THAT CHILDREN CAN'T ISOLATE EFFECTIVELY AT HOME ////////////////

						// everyone dismissed from that class isolates (we hope) in our "conservative scenario"
						for(int needs_to_isolate : _school[them->classroom()])
						{
							_Population[needs_to_isolate].set_days_since_first_symptoms(0);
						}

						/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
					}
					// recovered from their coronacation, bring them back to the classroom
					else if( not is_in_isolation(person) )
					{
						if(them->age() == 'C')
						{
							_school[them->classroom()].insert(them->ID());
						}
						else if((them->age() == 'A') and (them->status() == 'R'))
						{
							/*
								they only need to be rehired if they've actually left the class due to actual illness,
									so that's why we check to sere if their state is "R"
								rehire them and sack the substitute
							*/
							rehire_recovered_OG_teacher(person);
						}
					}
				}
			}

			std::set<int> classes_to_reopen {};

			// bit of a unique problem here
			// on Mondays we need to reopen all the classes with the new cohorts, while during the weekday only the recovered classes
			// this seems a bang-on solution
			if(day_of_the_week() == 0){
			for(const std::pair<int, std::set<int>> classr : _school)
			{
				if(not _classroom_num_days_shut_down_due_to_illness.count(classr.first)) classes_to_reopen.insert(classr.first);
			}}

			//  increment the days of class closure. if 14 days have passed, pout down that class for reopening
			for(const std::pair<int, int> classr : _classroom_num_days_shut_down_due_to_illness)
			{
				assert(check_classroom_status(classr.first));

				// don't count a weekend as a closure day if the 14-day counter has already run out
				if(classr.second < 14){ ++ _classroom_num_days_shut_down_due_to_illness[classr.first]; }
				// if the requisite 14 days have passed. reopen the classroom
				else if(not currently_the_weekend())
				{
					// printf("Class %i put on the reopening list.\n", classr.first);
					classes_to_reopen.insert(classr.first);
				}
				// else, mark that another day has passed
			}

			// persons in cohort zero return to class every week
			// these are mostly teachers
			std::set<int> cohorts_to_bring_back({0, this_weeks_cohort()});

			// for every classroom marked for reopening
			for(const int classr : classes_to_reopen)
			{
				// take the class off the shut down list
				_classroom_num_days_shut_down_due_to_illness.erase(classr);

				// bring back all the eligible students
				// for the OG teachers that are sick, hire substitutes *here* and nowhere else
				for(const int cohort_num : cohorts_to_bring_back)
				{
					// to avoid iterating over a changing std::set (the replace_sick_teacher_function mutates stuff)
					std::set<int> the_group = _the_cohorts[cohort_num];
					for(const int person : the_group)
					{
						// spiffy handle for the object
						Person* them = &_Population[person];

						if(them->classroom() == classr)
						{
							// make sure that they're no longer infectious
							if(not is_in_isolation(them->ID()))
							{
								_school[classr].insert(them->ID()); // just add them. nothing special here
							}
							// sick teacher must be replaced
							else if(them->age() == 'A')
							{
								replace_sick_teacher(them->ID()); // hire someone else
							}
						}
					}
				}
			}
			return;
		}

		// changing the disease status of an agent and the place they got exposed
		void set_status(const int getting_their_state_changed, const char new_status, const std::string locale="")
		{
			assert(check_disease_status(new_status));
			assert(check_agent_number(getting_their_state_changed));

			// if someone is exposed, we need to know where. for science.
			if( (new_status=='E') and (locale=="") )
			{
				std::cerr << "PERSON EXPOSED YET LOCATION NOT GIVEN." << std::endl;
				std::exit(EXIT_FAILURE);
			}

			Person* them = &_Population[getting_their_state_changed];
			char old_status = them->status();

			// if there's no change, return. else, change the status
			if(old_status == new_status){ return; }
			them->set_status(new_status);
			// move them from the old compartment to the new one
			_disease_compartments[old_status].erase(getting_their_state_changed);
			_disease_compartments[new_status].insert(getting_their_state_changed);

			if(new_status == 'I') // if symptomatic
			{
				// no way in hell you're getting back into class like that, young man!
				them->set_days_since_first_symptoms(0, _run_time);
			}
			else if(new_status == 'E') // simple exposure to infection
			{
				// make sure that I've typed the location name correctly
				assert(check_infection_locale(locale));
				// sanity check - if they're not attending the school, then they can't have been infected in a school area
				assert( (them->classroom() != -1) or ((locale !="class") and (locale != "commons")) ); //
				// mark their exposure at that place
				_places_infected[locale].insert(them->ID());
				them->set_infection_locale(locale);
			}
			else if((new_status == 'R') and (locale != ""))
			{
				// used in the unit tests to just speed things up instead of waiting 15 time steps. doesn't happen in the sim
				them->set_days_since_first_symptoms(15);
			}
		}

		// change the classroom that the agent is assigned to
		void set_classroom(const int agent_number, const int new_classroom, const int new_cohort)
		{
			assert(check_agent_number(agent_number));
			assert(check_disease_status(_Population[agent_number].status()));

			// only three possible cohorts in this model; it's every day, bro (0), week 1 (1) and week 2 (2)
			assert( std::set<int>({-1,0,1,2}).count(new_cohort) );

			Person* them = &_Population[agent_number];

			const int old_classroom = them->classroom();
			const int old_cohort_number = them->cohort();

			/*
				if the old classroom exists, take them out of it and change their individual characteristic
				we need to check of the old one exists, since some of the objects will be initialised with
					classroom Nan or -1
			*/
			if(_school.count(old_classroom))
			{
				_school[old_classroom].erase(them->ID());
				if(_school[old_classroom].empty()){ _school.erase(old_classroom); }
			}

			// set the new classroom and cohort characteristics
			them->set_classroom(new_classroom);
			them->set_cohort(new_cohort);

			// mode from the old cohort to the new one
			if(_the_cohorts.count(old_cohort_number)){ _the_cohorts[old_cohort_number].erase(agent_number); }
			_the_cohorts[ them->cohort() ].insert( them->ID() );

			// if they're eligible to be back in class this week, put them in the requested classroom
			if(new_cohort != -1){ // they're on a cohort
			if(not is_in_isolation(them->ID())){ // they're not isolating
			if((them->cohort() == this_weeks_cohort()) or (them->cohort() == 0)) // either their cohort is in school this week, or they need to show up every day
			{
				_school[ them->classroom() ].insert(them->ID());
			}}}

		}

		// place the agent in a house
		void set_household(const int agent_number, const int new_household)
		{
			assert(check_agent_number(agent_number));
			// get the old house number
			const int old_household = _Population[agent_number].household();
			// if it's not a default value, take them out of the old house
			if(_households.count(old_household))
			{
				_households[old_household].erase(agent_number);
				if(_households[old_household].empty()){ _households.erase(old_household); }
			}
			// put them in the new one
			_households[new_household].insert(agent_number);
			_Population[agent_number].set_household(new_household);
		}



		// print the number of adult, children and other in the population
		void print_age_distribution()
		{
			std::cout << "Age distribution of the population:\n";
			std::cout << "\tNumber of children: " << std::accumulate(_Population.begin(), _Population.end(), 0, [&](int accumulator, Person agent){ return accumulator+(agent.age()=='C'); } ) << '\n';
			std::cout << "\tNumber of adults: " << std::accumulate(_Population.begin(), _Population.end(), 0, [&](int accumulator, Person agent){ return accumulator+(agent.age()=='A'); } ) << '\n';
			std::cout << "\tNumber of other: " << std::accumulate(
				_Population.begin(),
				_Population.end(), 0,
				[&](int accumulator, Person agent){ return accumulator+((agent.age()!='C') & (agent.age()!='A')); }
			) << '\n';
		}

		// print each household group in the population, and the characteristics of its members
		void print_households()
		{
			std::cout << "\nNumber of households: " << num_households() << std::endl;
			for(std::pair<int, std::set<int>> house : households())
			{
				if(house.first == -1) continue;

				std::cout << "\tHouse #" << house.first << " :\n";
				std::cout << "\t\tChildren: ";

				for(int child : house.second){ if(_Population[child].age() == 'C')
				{
					const Person* them = &_Population[child];
					if(them->classroom() == -1){ printf("[%i, %c], ", them->ID(), them->status()); }
					else
					{
						std::stringstream blurb;
						blurb << "[" << them->ID() << ", status " << them->status() << ", class " <<  them->classroom();
						if(
							std::set<char>({'I', 'R'}).count(them->status()) and
							(is_in_isolation(them->ID()))
						)
						{ blurb << ", out sick]"; }
						else { blurb << "]"; }
						printf("%s, ", blurb.str().c_str());
					}
				}}
				std::cout << "\n\t\tAdults: ";
				for(int adult : house.second){ if(_Population[adult].age() == 'A')
				{
					const Person* them = &_Population[adult];

					if(them->classroom() == -1){ printf("[%i, %c], ", them->ID(), them->status()); }
					else
					{
						std::stringstream blurb;
						blurb << "[" << them->ID() << ", status " << them->status() << ", class " <<  them->classroom();
						if(
							std::set<char>({'I', 'R'}).count(them->status()) &
							(is_in_isolation(them->ID()))
						)
						{ blurb << ", out sick]"; }
						else if(set_of_values(_substitute_list_OGs_first).count(them->ID())){ blurb << ", substitute]"; }
						else { blurb << "]"; }
						printf("%s, ", blurb.str().c_str());
					}
				}}
				std::cout << "\n";
			}
		}

		// print complete information about the classroom in question, and the characteristics of all the nodes
		void print_classroom(const int room_number)
		{
			if(currently_the_weekend())
			{
				std::cout << "\nIT'S THE WEEKEND: SCHOOL CLOSED.\n" << std::endl;
				return;
			}

			std::pair<int, std::set<int>> classr({room_number, classroom(room_number)});
			if(classr.first == -1) return;
			if(classroom_closed_due_to_infection(room_number)){ printf("Classroom shut down.\n"); return; }

			std::cout << "\tClassroom #" << classr.first << " :\n";

			std::cout << "\t\t # children: " << children_in_classroom(classr.first).size() << ", # teachers: " << teachers_in_classroom(classr.first).size();
			std::cout << "\n\t\tChildren: ";

			for(int child : classr.second){ if(_Population[child].age() == 'C')
			{
				std::stringstream blurb;

				blurb << "[" << child << ", status " << _Population[child].status() << ", house " <<  _Population[child].household() << ", cohort " << _Population[child].cohort();
				if( std::set<char>({'I', 'R'}).count(_Population[child].status()) )
				{
					if(_Population[child].days_since_first_symptoms() >= 14){ blurb << ", recovered"; }
					else if(is_in_isolation(child)){ blurb << ", out sick"; }
				}
				blurb << "]";
				printf("\n\t\t\t%s, ", blurb.str().c_str());
			}}

			std::cout << "\n\t\tTeachers: ";

			for(int adult : classr.second){ if(_Population[adult].age() == 'A')
			{
				std::stringstream blurb;

				blurb << "[" << adult << ", status " << _Population[adult].status() << ", house " <<  _Population[adult].household() << ", cohort " << _Population[adult].cohort();
				if( std::set<char>({'I', 'R'}).count(_Population[adult].status()) )
				{
					if(_Population[adult].days_since_first_symptoms() >= 14){ blurb << ", recovered"; }
					else if(is_in_isolation(adult)){ blurb << ", out sick"; }
				}
				else if(set_of_values(_substitute_list_OGs_first).count(adult)){ blurb << ", substitute"; }
				blurb << "]";
				printf("\n\t\t\t%s, ", blurb.str().c_str());
			}}
			std::cout << "\n";
		}

		// see summary information for all the classrooms in the school
		void print_classrooms()
		{
			if(school_closed_due_to_infection())
			{
				printf("\nENTIRE SCHOOL IS SHUT DOWN.\n");
				return;
			}
			if(currently_the_weekend())
			{
				printf("\nIT'S THE WEEKEND: SCHOOL CLOSED.\n");
				return;
			}
			std::cout << "\nNumber of classrooms: " << num_classrooms() << std::endl;
			for(std::pair<int, std::set<int>> classr : classrooms())
			{
				if(classr.first == -1) continue;

				std::cout << "\tClassroom #" << classr.first << " :\n";
				if(classroom_closed_due_to_infection(classr.first)){ printf("\t\tCLASSROOM IS SHUT DOWN.\n"); continue; }

				std::cout << "\t\t# children: " << children_in_classroom(classr.first).size() << ", # teachers: " << teachers_in_classroom(classr.first).size();
				std::cout << "\n\t\tChildren:";
				for(int child : classr.second){ if(_Population[child].age() == 'C')
				{
					std::stringstream blurb;
					blurb << "\n\t\t\t[" << child << ", status " << _Population[child].status() << ", house " <<  _Population[child].household() << ", cohort " << _Population[child].cohort();
					if( std::set<char>({'I', 'R'}).count(_Population[child].status()) )
					{
						if(_Population[child].days_since_first_symptoms() < 14) { blurb << ", out sick]"; }
						else { blurb << ", recovered]"; }
					}
					else { blurb << "]"; }
					printf("%s, ", blurb.str().c_str());
				}}

				std::cout << "\n\t\tTeachers:";
				for(int adult : classr.second){ if(_Population[adult].age() == 'A')
				{
					std::stringstream blurb;
					blurb << "\n\t\t\t[" << adult << ", status " << _Population[adult].status() << ", house " <<  _Population[adult].household() << ", cohort " << _Population[adult].cohort();
					if( std::set<char>({'I', 'R'}).count(_Population[adult].status()) )
					{
						if(_Population[adult].days_since_first_symptoms() < 14) { blurb << ", out sick]"; }
						else { blurb << ", recovered]"; }
					}
					else if(set_of_values(_substitute_list_OGs_first).count(adult)){ blurb << ", substitute]"; }
					else { blurb << "]"; }
					printf("%s, ", blurb.str().c_str());
				}}
				std::cout << "\n";
			}
			return;
		}

		// all the stages of the disease progression
		void print_compartments()
		{
			std::cout << "Disease compartments:\n";
			for(std::pair<char, std::set<int>> pair : _disease_compartments)
			{
				std::cout << "\t" << pair.first << " : ";
				for(int person : pair.second){ std::cout << ", " << _Population[person].ID(); }
				std::cout << "\n";
			}
		}

		// the numbers of nodes in each stage of the disease progression
		void print_compartment_sizes()
		{
			std::cout << "Disease compartment sizes:\n";
			for(std::pair<char, std::set<int>> pair : _disease_compartments)
			{
				std::cout << "\t" << pair.first << ": " << pair.second.size() << std::endl;
			}
			std::cout << "\n";
		}

		Town()
		{
			_Population = {};
			_agent_IDs = {};
			_households = {};
			_school = {};
			_disease_compartments = {};
			_run_time = 0;
		}

		Town(Town& other)
		{
			_Population = other._Population;
			_households = other._households,
			_school = other._school;
			_disease_compartments = other._disease_compartments;
			_run_time = other._run_time;
		}

		auto compartments()
		{
			return _disease_compartments;
		}

		void print_substitute_list()
		{
			std::cout << _substitute_list_OGs_first << std::endl;
		}


};

std::ostream& operator << (std::ostream& out, Town& the_city)
{
	out << "\nNumber of agents: " << the_city.num_agents() << "\n";
	out << "\nNumber of adults: " << the_city.num_age('A');
	out << "\nNumber of children: " << the_city.num_age('C') << "\n";
	out << "\nInfected nodes: " << std::endl;;
	for(auto pair : the_city.compartments())
	{
		out << "\tDisease compartment: " << pair.first << " : ";
		for(auto person : pair.second){ out << " " << the_city.Agent(person).ID(); }
		out << "\n";
	}
	// out << "disease compartment sizes: " << the_city.compartment_sizes() << "\n";
	// out << "\nNumber of households : " << the_city.num_households() << "\n";
	// for(auto house : the_city.households())
	// {
	// 	out << "\tHouse #" << house.first << " : ";
	// 	for(auto person : house.second){ out << " " << the_city.Agent(person).ID(); }
	// 	out << "\n";
	// }
	// out << "\nNumber of childcare centre classrooms: " << the_city.num_classrooms() << "\n";
	// for(auto classr : the_city.classrooms())
	// {
	// 	out << "\tClass #" << classr.first << " : ";
	// 	for(auto person : classr.second){ out << " " << the_city.Agent(person).ID(); }
	// 	 out << "\n";
	// }
	return out;
};


#endif
