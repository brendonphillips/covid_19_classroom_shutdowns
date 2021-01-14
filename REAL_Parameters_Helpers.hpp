#ifndef REAL_PARAMETERS_HELPERS_HPP_
#define REAL_PARAMETERS_HELPERS_HPP_

#include <iostream>
#include <string>
#include <vector>
#include <set>
#include <sstream>
#include <tuple>
#include "prettyprint.hpp"
#include <fstream>
#include <filesystem>
#include <chrono>

typedef std::chrono::high_resolution_clock hr_clock;

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

// fixed backgrounnd rate of infection
const float Background_Infection_Rate_H = 1.16e-4;
const float Background_Infection_Not_in_School = 2.*Background_Infection_Rate_H;
const float Background_Infection_in_School = Background_Infection_Rate_H;

// cohort 0 goes to the school every week
const int Teacher_Cohort = 0;

const int Ensemble_Size = 10000;
const int Number_of_Classrooms = 5;

const std::vector<std::string> Class_Assignments_set ({"siblings", "random"}); // , "random"
const std::vector<bool> Reduced_Hours_set ({true, false});

const std::string Data_Folder = "/mnt/c/Users/User/Documents/Most_Recent_COVID_Sim_Data/";

/*
	(edited) Vincenzo Pii's answer to
	"Parse (split) a string in C++ using string delimiter (standard C++)"
	https://stackoverflow.com/questions/14265581/parse-split-a-string-in-c-using-string-delimiter-standard-c
	Accessed: 20 December 2020
*/
const std::vector<std::string> split_the_string(const std::string the_string, const std::string delimiter)
{
    std::string input (the_string);
    size_t pos = 0;
    std::vector<std::string> accumulator {};
    std::string token;
    while ((pos = input.find(delimiter)) != std::string::npos) {
        token = input.substr(0, pos);
        accumulator.push_back(token);
        input.erase(0, pos + delimiter.length());
    }
    accumulator.push_back(input);
    return accumulator;
}

// unique filename for each parameter tuple
const std::string get_filename(const float A0, const float AC, const float BH, const float LAM, const float RI, const std::string Arrangement, const int Num_Child, const int Num_Teacher, const int Num_Cohorts, const bool Reduced)
{
	char file_name_buffer [500];
	sprintf(
		file_name_buffer,
		"COVID_model_A0_%.6f_AC_%.3f_BH_%.5f_Back_%.7f_Rinit_%.3f_Arr_%s_Child_%i_Teach_%i_Cohort_%i_RedHrs_%i.csv",
		A0, AC, BH, LAM, RI, Arrangement.c_str(), Num_Child, Num_Teacher, Num_Cohorts, Reduced
	);
	return std::string(file_name_buffer);
};

// vectors for parameter values
std::vector<float> B_H_set {};
std::vector<float> R_init_set {};
std::vector<float> Alpha_0_set {};
std::vector<float> Alpha_C_set {};
std::vector<float> Background_Infection_Rate_set {};

// holds the random seeds assigned to each indvidual parameter tuple. explained further below.
std::vector<int> Seed_Vector {};

std::vector<std::tuple<float ,float, float, float, float, std::string, int, int, int, bool>> Parameter_Tuples {};

auto Get_Parameter_Combinations = []() -> void
{
	// // parameter baseline values
	const float Alpha_0_base = 0.0025;
	const float B_H_base = 0.109;
	const float R_init_base = 0.1;
	const float Background_Infection_Rate_base = 1.16e-4;

	// for comparison, we vary each parameter by +/- 50% of its baseline value
	for(float mult=0.5; mult<1.51; mult+=0.5)
	{
		B_H_set.push_back(mult*B_H_base);
		Alpha_0_set.push_back(mult*Alpha_0_base);
		Background_Infection_Rate_set.push_back(mult*Background_Infection_Rate_base);
		R_init_set.push_back(mult*R_init_base);
	}

	Alpha_C_set.push_back(0.25);
	Alpha_C_set.push_back(0.75);

	// the set of all parameter combinations desired
	for(float BH : B_H_set){
	for(float AC : Alpha_C_set){
	for(float A0 : Alpha_0_set){
	for(float LAM : Background_Infection_Rate_set){
	for(float RI : R_init_set){
	for(std::string Class_Arrange : std::set<std::string>({"siblings", "random"})){
	for(bool Reduced : std::set<bool>({true, false})){
	for(int num_child=2; num_child<=30; ++num_child){
	for(int num_teacher=1; num_teacher<=3; ++num_teacher){
	for(int num_cohorts=1; num_cohorts<=2; ++num_cohorts)
	{
		// get the name of the output file
		const std::string file_stem = get_filename(A0, AC, BH, LAM, RI, Class_Arrange, num_child, num_teacher, num_cohorts, Reduced);
		bool trial_already_done = false;
		// if the file exists, don't add the parameter tuple to the list
		for (const auto & entry : std::filesystem::directory_iterator(Data_Folder))
		{
			if(entry.path().string().find(file_stem) != std::string::npos){ trial_already_done = true; break; }
		}
		if(trial_already_done){ continue; }
		// add the parameter tuples that haven't been run yet
		Parameter_Tuples.push_back(std::make_tuple( A0, AC, BH, LAM, RI, Class_Arrange, num_child, num_teacher, num_cohorts, Reduced ));

	}}}}}}}}}}

	/*
		For each trial, we generate a unique seed for the random generator. For that trial number, each parameter combination
			is run with the same set of pseudo-random numbers to separate the effects of the changes in parameter values from those
			differences caused by the sequences of random numbers generated.
	*/
	hr_clock::time_point beginning = hr_clock::now();
	for(int i=0; i<Ensemble_Size; ++i)
	{
		hr_clock::duration d = hr_clock::now() - beginning;
		Seed_Vector.push_back(d.count());
	}
};

#endif
