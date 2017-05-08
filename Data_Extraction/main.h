// main.h
//
// author: Colin Keating
// last modified: 5/1/17
// for use with the Cameca SIMS 7f-GEO mass spectrometer
//		belonging to Washington University in St. Louis
//
// purpose: main.h holds the main function declaractions and enumerations for the
//		SIMS Data Extraction and Correction program. See README_DataExtractionNovember2016 for full program description

#ifndef MAIN_H
#define MAIN_H

#include "data_structs.h"
#include "dirent.h" //for iterating through files in a directory

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cstdlib>
#include <iomanip>
#include <cstring>
#include <numeric>
#include <filesystem>
#include <ctime>

//#include <stdlib.h>

// **********************
// Enumerations
// **********************

enum status : int {
    SUCCESS = 0,
    ERROR_INCORRECT_ARGS = -1,
    ERROR_FILE_NOTE_FOUND = -2,
    ERROR_UNABLE_TO_OPEN_FILE = -3,
    ERROR_BAD_NUM_POINTS_CHAUV = -4,
    WRONG_COMPARISON_NUMBER = -5,
    FOLDER_NOT_FOUND = -6
};

enum args : int {
    PROGRAM_NAME = 0,
    EXPECTED_NUM_ARGS = 1
};

// **********************
// Function Definitions
// **********************

int usage(char* program_name);
int unable_to_open(std::string filename);

std::vector<std::string> find_xy_pos(std::string input_file_name);
std::vector<std::string> find_detector_type(std::string input_file_name);
std::vector<std::vector<std::string>> find_detector_values(std::string input_file_name);
std::vector<std::vector<std::string>> find_beam_times(std::string input_file_name, int number_of_columns);
std::string get_field_x(std::string input_file_name);
std::string get_field_y(std::string input_file_name);
std::string get_slit_x(std::string input_file_name);

std::vector<std::string> get_someData(std::string input_file_name);
std::vector<std::vector<std::string>> get_rawData(std::string input_file_name, int num_columns, int num_rows);

std::vector<int> infinite_ask_for_cols(std::vector<std::string> &column_headers);
std::vector<std::vector<std::string>> infinite_parse_raw_data(std::vector<std::vector<std::string>> rawData, std::vector<int> columns);

void trim(std::string *s);

bool yes_no_question(std::string message);
int ask_for_string(std::string input_message, std::string &result);
int ask_for_int(int &result, int lower_bound, std::string message_1, std::string message_2);
int ask_for_double(double &result, double lower_bound, std::string message_1, std::string message_2);

double ask_for_deadtime(int column);
int ask_for_primary_beam(double &result);
int ask_for_std(double &result);
int ask_for_ct(double &result);
int ask_for_true_d_std(double &result);
int ask_for_international_std(double &result);
int ask_for_chauvenet(double num_points, double &result);

std::string get_time();
std::vector<std::vector<double>> string_to_double(std::vector<std::vector<std::string>> parsed_rawData);
std::vector<double> correct_data(std::vector<std::vector<double>> rawDoubles, double chauvenet, double standard, std::vector<std::vector<int>> &keep_or_reject, double ct);
double correct_for_deadtime(double input, double deadtime, double yield);
double correct_for_qsa(double input, double primary_beam_flux);
double correct_for_fc(double input, double background, double yield);
double correct_for_sampling_time(double datapoint, double prev_datapoint, double cycle_number, double datapoint_column_number, double corresponding_datapoint_column_number, std::vector<std::vector<std::string>>beam_times, double &corrected_sampling_time);
double calculate_delta(double r_measured, double r_vcdt); //r_vcdt for R_international_std
double calculate_r_std_true(double delta_std, double R_international_std);
double calculate_ct(double R_std_true, double R_std_measured);
std::vector<double> linear_regression(std::vector<std::vector<double>> final_data, std::vector<int> keep_reject, double raw_data_size, double rejects);

void write_data(revised_file_data_to_write f, std::string input_file_name, revised_global_user_inputs rgui);
void write_standard_data(revised_file_data_to_write f, std::string input_file_name, revised_global_user_inputs rgui);
void write_to_csv(revised_file_data_to_write f, std::vector<std::vector<std::vector<double>>> rawDoubles, std::vector<std::vector<int>> binary_rejected, revised_global_user_inputs rgui, int file_number);
void write_all_raw_to_csv(revised_file_data_to_write f, std::vector<std::vector<std::string>> all_raw_strings, revised_global_user_inputs rgui);
void new_write_csv(revised_file_data_to_write f, std::vector<std::vector<std::vector<double>>> rawDoubles, std::vector<std::vector<int>> binary_rejected, revised_global_user_inputs rgui, int file_number);
void write_all_runs_to_csv();
void write_all_runs_to_csv_updated();
//void write_CT_Rstd_to_csv(revised_file_data_to_write f, revised_global_user_inputs rgui, std::vector<double> ct_vals, std::vector<double> Rstd_vals);
void write_CT_Rstd_to_csv(revised_global_user_inputs rgui, std::vector<double> ct_vals, std::vector<double> Rstd_vals);

void write_data(revised_file_data_to_write f, std::string input_file_name, revised_global_user_inputs rgui);
void write_standard_data(revised_file_data_to_write f, std::string input_file_name, revised_global_user_inputs rgui);
void write_to_csv(revised_file_data_to_write f, std::vector<std::vector<std::vector<double>>> rawDoubles, std::vector<std::vector<int>> binary_rejected, revised_global_user_inputs rgui, int file_number);
void write_all_raw_to_csv(revised_file_data_to_write f, std::vector<std::vector<std::string>> all_raw_strings, revised_global_user_inputs rgui);
void new_write_csv(revised_file_data_to_write f, std::vector<std::vector<std::vector<double>>> rawDoubles, std::vector<std::vector<int>> binary_rejected, revised_global_user_inputs rgui, int file_number);
void write_all_runs_to_csv();
void write_all_runs_to_csv_updated();
//void write_CT_Rstd_to_csv(revised_file_data_to_write f, revised_global_user_inputs rgui, std::vector<double> ct_vals, std::vector<double> Rstd_vals);
void write_CT_Rstd_to_csv(revised_global_user_inputs rgui, std::vector<double> ct_vals, std::vector<double> Rstd_vals);

#endif /* MAIN_H */
