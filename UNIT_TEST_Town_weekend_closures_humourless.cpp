// #include "REAL_Town.hpp"
#include "REAL_Town.hpp"


int main()
{
	// replacing teachers - every student in the same cohort, so this should work fine, except for the weekend class closings
	Town Waterloo;

	// these children are in cohort 1
	for(int i = 0; i < 2; ++i){ Waterloo.add_agent(Person('C', 5, 'S', 0, 1)); }
	for(int i = 0; i < 2; ++i){ Waterloo.add_agent(Person('C', 6, 'S', 0, 1)); }

	// these children are in cohort 2
	for(int i = 0; i < 2; ++i){ Waterloo.add_agent(Person('C', 5, 'S', 0, 2)); }
	for(int i = 0; i < 2; ++i){ Waterloo.add_agent(Person('C', 6, 'S', 0, 2)); }

	// teachers in cohort 0 - so every day they teaching
	Waterloo.add_agent(Person('A', 1, 'S', 0, 0));
	Waterloo.add_agent(Person('A', 2, 'S', 0, 0));
	Waterloo.add_agent(Person('C', 3, 'S', 0, 0));

	for(int i = 0; i < 2; ++i){ Waterloo.add_agent(Person('C', 8, 'S', 2, 1)); }
	for(int i = 0; i < 2; ++i){ Waterloo.add_agent(Person('C', 8, 'S', 2, 1)); }
	for(int i = 0; i < 2; ++i){ Waterloo.add_agent(Person('C', 8, 'S', 2, 2)); }
	Waterloo.add_agent(Person('A', 9, 'A', 2, 0));
	Waterloo.add_agent(Person('A', 9, 'A', 2, 0));


	printf("\nTEST: We have lots of susceptible students in classrooms 0 and 2 on day %i.\n", Waterloo.days_elapsed());;
	Waterloo.print_classrooms();

	printf("TEST. This is Monday (day %i). We'll walk it through to Friday and see what it does.\n\n", Waterloo.days_elapsed());
	for(int i=0; i<4; i++) Waterloo.advance_the_time();
	printf("Day %i of the simulation. Weekend: %i. Printing classrooms:\n", Waterloo.days_elapsed(), Waterloo.currently_the_weekend());
	Waterloo.print_classrooms();

	Waterloo.advance_the_time();
	printf("\nTEST: The next two days should be the weekend, so classes closed.");
	printf("\nDay %i: ", Waterloo.days_elapsed());
	Waterloo.print_classrooms();
	Waterloo.advance_the_time();
	printf("Day %i: ", Waterloo.days_elapsed());
	Waterloo.print_classrooms();

	Waterloo.advance_the_time();
	printf("\nIt's Monday again. New week, new class and fresh faces. Cohort %i should be in now for day %i.\n", Waterloo.this_weeks_cohort(), Waterloo.days_elapsed());
	Waterloo.print_classrooms();

	for(int i=0; i<7; i++) Waterloo.advance_the_time();
	printf("\nTEST: rolling through to the next week to see if Cohort 1 comes back to class for Monday next (day %i)", Waterloo.days_elapsed());
	Waterloo.print_classrooms();

	printf("\nTEST: Let's infect child 1 and see what happens. Printing classroom.\n");
	Waterloo.set_status(1, 'I', "class");
	Waterloo.print_classroom(0);

	Waterloo.advance_the_time();
	printf("\nTEST: agent 1 has spent 1 day infected. Class 0 should be shut down now, while Class 2 is still going. The day is %i.\n", Waterloo.days_elapsed());
	Waterloo.print_classrooms();

	for(int i=0; i<7; i++) Waterloo.advance_the_time();
	printf("\nTEST: Advancing the time exactly one week to day %i. Classroom 0 should still be shut, but Cohort 2 should be back in Class 2.", Waterloo.days_elapsed());
	Waterloo.print_classrooms();

	for(int i=0; i<7; i++) Waterloo.advance_the_time();
	printf("\nTEST: Advancing one more week and a day to day %i. Classrooms 0 and 2 should be open, with cohort 1 in both rooms.\n", Waterloo.days_elapsed());
	Waterloo.print_classrooms();

	return 0;
}
