#include "REAL_Town.hpp"

int main()
{
	Town Barbados;

	// the first agent is numbered 0, to keep with the 0-indexing of C++
	// age, household, childcare, illness, classr

	Barbados.add_agent(Person( /* age */ 'C', /* household*/ 3,  /* disease */ 'E', /* classroom */ 3, /* cohort */ 1)); // 0
	Barbados.add_agent(Person( /* age */ 'C', /* household*/ 3,  /* disease */ 'A', /* classroom */ 2, /* cohort */ 1)); // 1
	Barbados.add_agent(Person( /* age */ 'C', /* household*/ 2,  /* disease */ 'S', /* classroom */ 3, /* cohort */ 1)); // 2
	Barbados.add_agent(Person( /* age */ 'C', /* household*/ 2,  /* disease */ 'P', /* classroom */ 1, /* cohort */ 1)); // 3
	Barbados.add_agent(Person( /* age */ 'C', /* household*/ 3,  /* disease */ 'E', /* classroom */ 2, /* cohort */ 1)); // 4
	Barbados.add_agent(Person( /* age */ 'C', /* household*/ 1,  /* disease */ 'S', /* classroom */ 2, /* cohort */ 1)); // 5

	// should square with all the stuff above
	std::cout << "Printing the network stats\n\n" << Barbados;
	std::cout << "################################################################################" << std::endl;

	// testing state change - state change agent 2 to "F" should give an error
	// Barbados.set_status(2, 'F');

	// state change 2 from "I" to 'R' should show in the list
	std::cout << "\nchanging agent 1 from A to E. before the change\n";
	Barbados.print_compartments();
	Barbados.set_status(1, 'E', "commons");
	std::cout << "\nagent 1 should now be in the E compartment and *NOT* in the A compartment.\n";
	Barbados.print_compartments();
	std::cout << "the agent 1 herself now thinks her status is "<< Barbados.Agent(1).status() << std::endl;
	std::cout << "\nsame thing with # 0, from E to S\n";
	Barbados.set_status(0, 'S');
	std::cout << "agent 0 should now be in the S compartment and *not* in the E compartment.\n";
	Barbados.print_compartments();
	std::cout << "the agent 0 herself now thinks her status is "<< Barbados.Agent(0).status() << std::endl;

	std::cout << "\n################################################################################" << std::endl;

	// checking the household code
	// will change households to make sure that everything works as planned
	std::cout << "\nchanging agent 2 from household 2 to 4. before the change\n";
	Barbados.print_households();
	Barbados.set_household(2, 4);
	std::cout << "\nagent 2 should now be the sole member of household 4, and *NOT* in the household 2\n";
	Barbados.print_households();
	std::cout << "\nagent 2 thinks they're in household " << Barbados.Agent(2).household() << std::endl;

	// trying to create an empty household - it should be pruned
	Barbados.set_household(2, 1);
	Barbados.set_household(3, 1);
	std::cout << "moved agents 2 and 3 to household 1. household 2 shouldn't be there anymore, since it's empty now.\n";
	Barbados.print_households();
	std::cout << "agent 2 thinks they're in household " << Barbados.Agent(2).household() << std::endl;
	std::cout << "agent 3 thinks he's in household " << Barbados.Agent(3).household() << std::endl;

	std::cout << "\n################################################################################" << std::endl;

	// checking the classroom code
	// will change classrooms to make sure that everything works as planned
	// this one creates a new classroom
	std::cout << "\nTEST: changing agent 5 from class 2 to 89. before the change\n";
	Barbados.print_classrooms();
	Barbados.set_classroom(5, 89, 1);
	std::cout << "TEST: agent 5 should now be the sole member of class 89, and *NOT* in the class 2\n";
	Barbados.print_classrooms();
	std::cout << "TEST: agent 5 thinks she's in class " << Barbados.Agent(5).classroom() << std::endl;

	// now leaving an empty classroom by moving agent 4 from classroom 1 to 3. it should be pruned
	std::cout << "\nTEST: emptying a classroom to make sure that it gets pruned. moving agent 3 from class 1 to class 2. class 1 should not exist anymore.\n";
	Barbados.set_classroom(3, 2, 1);
	Barbados.print_classrooms();
	std::cout << "TEST: agent 3 thinks she's in class " << Barbados.Agent(3).classroom() << std::endl;

	std::cout << "\n################################################################################" << std::endl;

	return 0;
}
