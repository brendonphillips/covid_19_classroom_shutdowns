#ifndef REAL_PERSON_HPP_
#define REAL_PERSON_HPP_

#include <iostream>
#include <limits>

class Person
{
	friend class Town;

	private:

		// AGENT CHARACTERISTICS

		char _age; // categorical age of the agent - either child 'C' or adult 'A'
		int _household; // number of the household containing the node
		int _classroom; // classroom that the child/teacher is assigned to
		char _disease_status; // stage of the SEPAIR infection progression
		int _identity; // individual number of the node - assigned when added to the network
		int _days_since_first_symptoms; // number of days since first COVID-19-esque symptoms, used for enforcing isolation upon asymptomatic infection
		std::string _infection_locale; // place where the agent got infected; background, home, class, common area
		int _cohort; // cohort that the student is in (single cohort can be modelled by placing all children in the same cohort)
		int _time_step_infected_at; // tracks the passage of the infection, whether or not the agent is asymptomatic

	public:

		// GETTERS

		const int ID() const { return _identity; }
		const char age() const { return _age; }
		const int household() const { return _household; }
		const char status() const { return _disease_status; }
		const bool is_susceptible() { return (_disease_status == 'S'); }
		const int classroom() const { return _classroom; }
		const int days_since_first_symptoms() const { return _days_since_first_symptoms; }
		const int cohort() const { return _cohort; }
		const std::string infection_locale() const { return _infection_locale;}
		const int weekend_offset() const
		{
			// if the node gets sick over the weekend, notify the school at the beginning of the next week
			if(_disease_status != 'I') return 0;
			// see where we are in the week, throw away Mon-Fri, and count the remainders
			return std::max(_time_step_infected_at%7-4, 0);

		}

		Person& operator = (const Person& other)
		{
			if(this == &other) return *this;
			_age = other._age;
			_household = other._household;
			_classroom = other._classroom;
			_disease_status = other._disease_status;
			_days_since_first_symptoms = other._days_since_first_symptoms;
			_infection_locale = other._infection_locale;
			_cohort = other._cohort;
			_time_step_infected_at = other._time_step_infected_at;
			return *this;
		}

		Person()
		{
		 	_age = '\0';
			_household = std::numeric_limits<int>::quiet_NaN();
			_disease_status = '\0';
			_classroom = std::numeric_limits<int>::quiet_NaN();
			_identity = std::numeric_limits<int>::quiet_NaN();
			_days_since_first_symptoms = -1;
			_infection_locale = "";
			_time_step_infected_at = 0;
			_cohort = -1;
		}

		Person(const char age, const int household, const char illness, const int classr, const int cohort_num=-1, const std::string place_of_infection="")
		{
			_age = age;
			_household = household;
			_disease_status = illness;
			_classroom = classr;
			_identity = std::numeric_limits<int>::quiet_NaN();
			_days_since_first_symptoms = -1;
			_infection_locale = place_of_infection;
			_cohort = cohort_num;
			_time_step_infected_at = 0;
		}

		Person(const Person& other)
		{
			_age = other._age;
			_household = other._household;
			_disease_status = other._disease_status;
			_classroom = other._classroom;
			_identity = other._identity;
			_classroom = other._classroom;
			_days_since_first_symptoms = other._days_since_first_symptoms;
			_infection_locale = other._infection_locale;
			_cohort = other._cohort;
			_time_step_infected_at = other._time_step_infected_at;
		}

		Person(Person&& other)
		{
			_identity = other._identity;
			_age = other._age;
			_household = other._household;
			_disease_status = other._disease_status;
			_classroom = other._classroom;
			_days_since_first_symptoms = other._days_since_first_symptoms;
			_infection_locale = other._infection_locale;
			_cohort = other._cohort;
			_time_step_infected_at = other._time_step_infected_at;

			other._age = '\0';
			other._household = std::numeric_limits<int>::quiet_NaN();
			other._disease_status = '\0';
			other._classroom = std::numeric_limits<int>::quiet_NaN();
			other._identity = std::numeric_limits<int>::quiet_NaN();
			other._days_since_first_symptoms = -1;
			other._infection_locale = "";
			other._cohort = -1;
			other._time_step_infected_at = 0;
		}

	// protected:

		// SETTERS

		/*
			I've hidden them, because all the agent properties (cohort, disease status, etc) are also used by the Town class.
			To avoid a characteristic being updated in one place but not the other, these setter functions are hidden from
			the user, and everything is changed consistently by the Town class interface. Getters are not hidden since
			they have no side effects
		*/

		void set_identity(const int n) { _identity = n; }
		void set_age(const int n) { _age = n; }
		void set_household(const int n) { _household = n; }
		void set_status(const char illness, const char infec_time=0)
		{
			_time_step_infected_at = infec_time;
			_disease_status = illness;
		}
		void set_classroom(const int classr) { _classroom = classr; }
		void set_days_since_first_symptoms(const int num_days, const int weekend_infection=0){ _days_since_first_symptoms = num_days; }
		void add_day_since_first_symptoms(void) { _days_since_first_symptoms += 1; }
		void set_infection_locale(const std::string place) { _infection_locale = place; }
		void set_cohort(const int cohort_number) { _cohort = cohort_number; }
};

std::ostream& operator << (std::ostream& out, Person const& agent)
{
	out << "Agent # " << agent.ID() << ". Age: " << agent.age() << ". Assigned to household #" << agent.household() << ". In class #" << agent.classroom() << ". Disease status " << agent.status() << ". Time spent in isolation " << agent.days_since_first_symptoms() <<  ". Infected at location: " << agent.infection_locale() << ".";
	return out;
}

#endif
