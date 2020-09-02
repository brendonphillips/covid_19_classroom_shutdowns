// #include "REAL_Town.hpp"
#include "REAL_Town.hpp"


int main()
{
	printf("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");

	Town Barbados;

	// two households in the first classroom
	for(int i = 0; i < 2; ++i){ Barbados.add_agent(Person('C', 5, 'S', 0, 0)); }
	for(int i = 0; i < 2; ++i){ Barbados.add_agent(Person('C', 6, 'S', 0, 0)); }

	// two teachers for  that classroom (from different houses)
	Barbados.add_agent(Person('A', 1, 'S', 0, 0));
	Barbados.add_agent(Person('A', 2, 'S', 0, 0));

	for(int i = 0; i < 2; ++i){ Barbados.add_agent(Person('C', 7, 'S', 0, 0)); }
	Barbados.add_agent(Person('A', 7, 'S', -1));

	// Houses to pull substitutes from
	for(int i = 0; i < 2; ++i){ Barbados.add_agent(Person('C', 1, 'S', -1)); }
	for(int i = 0; i < 2; ++i){ Barbados.add_agent(Person('C', 2, 'S', -1)); }
	for(int i = 0; i < 2; ++i){ Barbados.add_agent(Person('C', 3, 'S', -1)); }
	for(int i = 0; i < 2; ++i){ Barbados.add_agent(Person('A', 1, 'S', -1)); }
	for(int i = 0; i < 2; ++i){ Barbados.add_agent(Person('A', 2, 'S', -1)); }
	for(int i = 0; i < 2; ++i){ Barbados.add_agent(Person('A', 3, 'S', -1)); }

	for(int i = 0; i < 4; ++i){ Barbados.add_agent(Person('C', 8, 'S', 2, 0)); }
	Barbados.add_agent(Person('A', 9, 'A', 2, 0));
	Barbados.add_agent(Person('A', 9, 'A', -1));

	Barbados.add_agent(Person('A', 10, 'S', -1));
	Barbados.add_agent(Person('A', 10, 'S', -1));
	Barbados.add_agent(Person('A', 10, 'S', -1));
	Barbados.add_agent(Person('A', 10, 'S', -1));

	printf("\nCHECK: There are many susceptible students in classrooms 0 and 2.\n");
	Barbados.print_classrooms();

	printf("\nTEST: Let's infect child 1 and see what happens. Printing classroom 0.\n");
	Barbados.set_status(1, 'I', "class");
	Barbados.print_classroom(0);

	printf("\n\nHit ENTER to continue.\n\n"); getchar();

	Barbados.advance_the_time();
	printf("\nTEST: agent 1 has spent 1 day infected. Class 0 should be shut down now, while Class 2 is still going. The day is %i.\n", Barbados.days_elapsed());
	Barbados.print_classrooms();

	Barbados.advance_the_time();
	printf("\nTEST: agent 1 has spent 2 days infected. Class 0 should be shut down now, while Class 2 is still going. The day is %i.\n", Barbados.days_elapsed());
	Barbados.print_classrooms();

	printf("\nTEST: advance the time through 8 days..\n");
	for(int i=2; i < 10; ++i)
	{
		printf("\t%i.\t",i); Barbados.advance_the_time();
		printf("Number of days closed: %i.\n", Barbados.classroom_closure_time(0));
	}

	printf("\n\nHit ENTER to continue.\n\n"); getchar();

	printf("\nTEST: Infecting the teacher 5, so that when the classroom is reopened, they'll still be infected and need a substitute.\n");
	Barbados.set_status(5, 'I');
	for(int i=10; i < 13; ++i)
	{
		printf("\t%i.\t",i); Barbados.advance_the_time();
		printf("Number of days closed: %i.\n", Barbados.classroom_closure_time(0));
		if(i == 10) Barbados.set_status(5, 'R');
	}
	printf("\nTEST: By algorithm, agent 19 would be the next available substitute to replace 5 (as it stands), so we'll infect them too and see what happens. The infection of the two is staggered, so there's no chance 19 will be accepted as a sub.\n");
	Barbados.set_status(19, 'I');
	Barbados.print_households();
	printf("\nCHECK: printing the susceptible agents in the school currently. It's a weekend: %i, so no one is in school right now.\n", Barbados.currently_the_weekend());
	std::cout << Barbados.agents_in_school({'S'}) << std::endl;

	printf("\n\nHit ENTER to continue.\n\n"); getchar();

	for(int i=13; i < 14; ++i)
	{
		printf("\t%i.\t",i); Barbados.advance_the_time();
		printf("Number of days closed: %i.\n", Barbados.classroom_closure_time(0));
		Barbados.set_status(19, 'R');
	}
	printf("\nTEST: Reprinting the classroom 0. Should not be open yet. Infecting child 7.\n");
	Barbados.print_classroom(0);
	Barbados.set_status(7, 'I');
	printf("\n");

	printf("\nCHECK: Class 0 should be reopened; printing. Teacher 5 should have a substitute 20. Child 7 should still be out of class for a few more time steps.\n");
	Barbados.advance_the_time();
	Barbados.print_classroom(0);

	printf("\n\nHit ENTER to continue.\n\n"); getchar();

	printf("\nCHECK: These are the households as they stand. Notice that 19 is isolating in the same house (#3) as 20 that is a teacher at the centre. Not really a concern right now.");
	Barbados.print_households();

	printf("\n\nHit ENTER to continue.\n\n"); getchar();

	printf("\nTEST: Advancing 8 time steps so that original teacher 5 (house 2) is fully recovered. Then Teacher 5 should reprise her position in Class 0, and we sack agent 20 (house 3). Child 7 should still not be in there.\n");
	Barbados.set_status(5, 'R'); 	Barbados.set_status(1, 'R');
	for(int i=0; i<9; ++i){ Barbados.advance_the_time(); }
	// printf("\nHouseholds:");
	// Barbados.print_households();
	printf("\nClassrooms:\n");
	Barbados.print_classrooms();

	printf("\nCHECK: Proportion of asymptomatic (A) agents in the school: %f. Should be 1/12 now.\n", Barbados.agents_in_school_proportion('A'));

	printf("\n\nHit ENTER to continue.\n\n"); getchar();

	printf("\nTEST: Class 2 is fine. Advancing 7 days, and child 7 should be recovered and back in class 0.\n");
	for(int i=0; i<8; ++i){ Barbados.advance_the_time(); }
	Barbados.print_classrooms();

	printf("\nCHECK: Proportion of A agents in the school: %f. Should be 1/13 this time (denominator increases by 1 since we regained student 7).\n", Barbados.agents_in_school_proportion('A'));

	printf("\n\nHit ENTER to continue.\n\n"); getchar();

	printf("\nTEST: Class 0 is fine. Child 22 is now infected, and class 2 now has an infected child.\n");
	Barbados.set_status(22, 'I');
	Barbados.print_classroom(2);

	Barbados.advance_the_time();
	printf("\nCHECK: Child has had a day to run around and spread SARS-CoV-2. It's a new day now, but it's the weekend: %i, for day %i of the sim. Printing the classrooms:\n", Barbados.currently_the_weekend(), Barbados.days_elapsed());
	Barbados.print_classrooms();

	printf("\n\nHit ENTER to continue.\n\n"); getchar();

	Barbados.advance_the_time(); printf("\tStill the weekend..."); Barbados.print_classrooms();

	Barbados.advance_the_time();
	printf("\nCHECK: Not the weekend anymore: %i. Classroom 2 should be shut.", not Barbados.currently_the_weekend());
	Barbados.print_classrooms();

	printf("\nCHECK: Number of classrooms shut down: %i.\n", (int)Barbados.closed_classrooms().size());

	for(int i=0; i < 14; i++)
	{
		printf("\tAdvanced the time to day %i. Classroom 2 is closed for %i days. It's the weekend (true/false): %i.\n", Barbados.days_elapsed(), Barbados.classroom_closure_time(2), Barbados.currently_the_weekend());
		Barbados.advance_the_time();
	}

	printf("\n\nHit ENTER to continue.\n\n"); getchar();

	printf("\nTEST: We've zoomed ahead 14 days so both classrooms should be opened now on day %i, with child 22 recovered. The line with 14 days repeated twice above because those last 2 days were weekend days (so class would not be open anyway).", Barbados.days_elapsed());
	Barbados.set_status(22, 'R');
	Barbados.print_classrooms();

	return 0;
}
