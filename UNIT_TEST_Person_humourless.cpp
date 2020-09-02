#include <iostream>
#include "REAL_Person.hpp"

/*******************************************************************

TU RUN THIS TEST, COMMENT THE ACCESS SPECIFIER "protected" in "REAL_Town.hpp"

********************************************************************/

int main()
{
	// test of the default constructor
	Person first();

	// should just show a single 1
	std::cout << "TEST: first agent: " << first << std::endl << std::endl;

	// constructor with parameters given
	Person second('A', 5, 'A', 7);
	std::cout << "TEST: second agent: " << second << std::endl;


	// prints the current age of the second object - either adult or child
	std::cout << "\n##################################################\nadult or child right now: " << second.age() << std::endl;

	// changes the disease status of the person
	std::cout << "\n##################################################\nTEST: change of disease state from A to R.\n";
	std::cout << "epi state right now: " << second.status() << std::endl;
	// should add a day to the age
	second.set_status('R');
	std::cout << "epi state after recovery: " << second.status() << std::endl;


	// see the household that a person is in
	std::cout << "\n##################################################\nTEST: changing the household assigned to: going from 5 to 129.\n";
	std::cout << "household: " << second.household() << std::endl;
	second.set_household(129);
	std::cout << "new household: " << second.household() << std::endl;

	// see and change the childcare centre
	std::cout << "\n##################################################\nTEST: changing the classroom from 7 to 1\n";
	std::cout << "centre: " << second.classroom() << std::endl;
	second.set_classroom(1);
	std::cout << "new class: " << second.classroom() << std::endl;

	// testing the copy constructor
	std::cout << "\n##################################################\nTEST: testing the copy constructor. the two object descriptions should be the same\n";
	std::cout << "description of second:\n" << second << std::endl;
	Person third(second);
	// third should have the same stats as second
	std::cout << "description of third:\n" << third << std::endl;

	// testing the = operator, but let's alter third a bit
	std::cout << "\n##################################################\nTEST: testing the assignment operator. the two descriptions should be the same.\n";
	third.set_age('A'); third.set_household(5); third.set_status('Y');
	std::cout << "description of third:\n" << third << std::endl;
	auto fourth = third;
	std::cout << "description of fourth:\n" << fourth << std::endl;

	// testing the move constructor
	std::cout << "\n##################################################\nTEST: testing the move constructor. Here are two different objects.\n";
	third.set_classroom(6);
	std::cout << second << std::endl;
	std::cout << third << std::endl;
	std::cout << std::endl << "if the move constructor works, the first object here should be the second done from before, and the second one should be bunk.\n\n";
	Person fifth = std::move(third);
	std::cout << fifth << std::endl;
	std::cout << third << std::endl;

	return 0;
}
