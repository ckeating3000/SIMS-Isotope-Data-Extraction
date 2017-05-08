#include "stdafx.h"
#include "main.h"
#include<windows.h>

//revised global variable declarations, to be used for infinite comparisons
std::vector<std::vector<double>> $final_data;
std::vector<std::vector<int>> global_keep_or_reject;
std::vector<std::vector<std::vector<double>>> global_corrected;
std::vector<std::vector<std::vector<double>>> global_final_corrected;
std::vector<std::vector<std::vector<double>>> global_corrected_sampling_times;
std::vector<std::string> global_write_data;
std::vector<std::vector<std::vector<double>>> global_all_vals;

class Revised_User_Parameters{ //object that stores the user-specific parameters
public:
    //int number_of_rows;
};

/*
 * Entry point of the program
 */
int main(int num_args, char* arg_strings[])
{
    // check to see if number of args correct
    if (num_args != EXPECTED_NUM_ARGS) {
        return usage(arg_strings[PROGRAM_NAME]);
    }
    //If correcting a standard... (needs debugging)
    std::cout << std::endl;
    if (yes_no_question("Are you correcting a standard? (y/n)"))
    {
        std::cout << "Standard Path" << std::endl;
        revised_global_user_inputs rgui_s;
        ask_for_int(rgui_s.number_of_columns, 1, "Enter number of data columns in files", "    e.g. enter 2 for Heart data");
        ask_for_int(rgui_s.points_per_column, 5, "Enter number of cycles", "    e.g. enter '90' for Heart data");
        
        //get chauvenet's criterion and check that # points is valid; if not, exit
        int response = ask_for_chauvenet(rgui_s.points_per_column, rgui_s.chauvenet);
        if (response == status::ERROR_BAD_NUM_POINTS_CHAUV){
            std::cout << "Status Error: " << response << std::endl;
            return response;
        }
        
        ask_for_string("enter the name of the standard; e.g. Balmat", rgui_s.standard_name);
        //get primary beam current
        ask_for_double(rgui_s.primary_beam_current, 0.0, "Enter primary beam current in picoamps" ,"    e.g. for 1500 picoamps enter '1500'");
        std::cout << "Primary Beam Current: " << rgui_s.primary_beam_current << " picoamps" << std::endl;
        rgui_s.primary_beam_flux = rgui_s.primary_beam_current*(6.2415*pow(10, 6));
        rgui_s.columns_to_compare = infinite_ask_for_cols(rgui_s.column_headers);
        
        //NEW, automated WAY (changed 12/15/16)
        // assemble list of input filenames and new names
        std::vector<std::string> file_list;
        std::vector<std::string> new_names;
        int num_files = 0;
        DIR *dir;
        struct dirent *ent;
        if ((dir = opendir("C:\\Users\\Lab User\\Documents\\Visual Studio 2013\\Projects\\DataExtractionApril2017\\Debug\\inputs_standards\\")) != NULL) {
            /* print all the files and directories within directory */
            while ((ent = readdir(dir)) != NULL) {
                //printf("%s\n", ent->d_name);
                std::string fullname = ent->d_name;
                //cout << "fullname: " << fullname << std::endl;
                file_list.push_back("inputs_standards\\" + fullname);
                std::string newname = fullname.substr(0, fullname.size() - 4); //chop off .asc extension
                //cout << "newname: " << newname << std::endl;
                new_names.push_back(newname);
                num_files++;
            }
            closedir(dir);
        }
        else{
            std::cout << "inputs_standards directory not found " << std::endl;
            return status::FOLDER_NOT_FOUND;
        }
        // erase the first two elements "." and ".." in file_list
        file_list.erase(file_list.begin(), file_list.begin() + 2);
        new_names.erase(new_names.begin(), new_names.begin() + 2);
        num_files = num_files - 2;
        
        std::vector<double> all_Rstd_values;
        std::vector<double> all_CT_values;
        
        //iterate through each file
        for (int ii = 0; ii < num_files; ii++){
            
            revised_file_data_to_write f;
            f.output_filename = new_names[ii];
            f.stat_results_Ri = get_someData(file_list[ii]);
            f.xypos = find_xy_pos(file_list[ii]);
            f.detector_types = find_detector_type(file_list[ii]);
            f.detector_values = find_detector_values(file_list[ii]);
            
            int num_rows = rgui_s.points_per_column;
            std::vector<std::vector< std::string>> raw_unparsed = get_rawData(file_list[ii], rgui_s.number_of_columns, num_rows); // raw_unparsed stores all raw data as strings
            f.rawData = infinite_parse_raw_data(raw_unparsed, rgui_s.columns_to_compare); // reorganize columns of rawData so columns to be compared are next to each other
            
            // declare data structures for holding file-specific data
            std::vector<std::vector<std::vector< std::string>>> parsed_rawData;
            std::vector<std::vector<std::vector<double>>> rawDoubles;
            
            //iterate through each set of columns
            for (unsigned int z = 0; z < rgui_s.columns_to_compare.size() / 2; z++){
                //get true d(std) (ex: balmat pyrite , 15.1 permil
                double ts = 0;
                ask_for_true_d_std(ts);
                rgui_s.true_std.push_back(ts);
                //get international std R (ex: VCDT or BDP)
                double is = 0;
                ask_for_international_std(is);
                rgui_s.international_std.push_back(is);
                
                std::vector<std::vector< std::string>> parsed_rawData1 = {};
                std::vector<std::vector<double>> rawDoubles1 = {};
                // put first set of data in new std::vector
                parsed_rawData1.resize(f.rawData.size(), std::vector< std::string>(4, ""));
                for (unsigned int i = 0; i < f.rawData.size(); i++){
                    for (unsigned int j = 0; j < 4; j++){
                        parsed_rawData1[i][j] = f.rawData[i][j + (4 * z)];
                    }
                }
                rawDoubles1 = string_to_double(parsed_rawData1);
                //correct raw data for EM/qsa, FC1 and FC2
                //iterate through each column
                for (unsigned int i = 0; i < rawDoubles1.size(); i++){
                    for (unsigned int j = 0; j < 2; j++){
                        // convert data types
                        double EM_yield = atof(f.detector_values[0][0].c_str());
                        double EM_deadtime = atof(f.detector_values[0][1].c_str()) * .000000001;
                        double FC1_yield = atof(f.detector_values[1][0].c_str());
                        double FC1_background = atof(f.detector_values[1][1].c_str());
                        double FC2_yield = atof(f.detector_values[2][0].c_str());
                        double FC2_background = atof(f.detector_values[2][1].c_str());
                        //see which things to correct for
                        if (f.detector_types[rgui_s.columns_to_compare[2 * z + j]] == "EM "){
                            rawDoubles1[i][j] = correct_for_deadtime(rawDoubles1[i][j], EM_deadtime, EM_yield);
                            rawDoubles1[i][j] = correct_for_qsa(rawDoubles1[i][j], rgui_s.primary_beam_flux);
                        }
                        else if (f.detector_types[rgui_s.columns_to_compare[2 * z + j]] == "FC1"){
                            rawDoubles1[i][j] = correct_for_fc(rawDoubles1[i][j], FC1_background, FC1_yield);
                        }
                        else if (f.detector_types[rgui_s.columns_to_compare[2 * z + j]] == "FC2"){
                            rawDoubles1[i][j] = correct_for_fc(rawDoubles1[i][j], FC2_background, FC2_yield);
                        }
                    }
                }
                parsed_rawData.push_back(parsed_rawData1);
                rawDoubles.push_back(rawDoubles1); // should be corrected for EM and FC1 and FC2
                
                // run correct data
                std::cout << "***PARAMETERS RELATED TO COMPARISON " << z << ": ***" << std::endl;
                //applies chauvenet 2x, rejects points as appropriate
                std::vector<double> temp_corrected_data = correct_data(rawDoubles1, rgui_s.chauvenet, .01, global_keep_or_reject, 1.0); // * rawDoubles1 is now deadtime corrected"   * 1.0 is used as placeholder for Ct correction
                //cout << "passes temp_corrected_data" << std::endl;                                // ^ (above) this value of .01 is an arbitrary placeholder; we don't have daily standards for this type of run (and we don't care about delta values) but the method needs some value in order to run
                f.finaldata.push_back(temp_corrected_data); // isnt passing correct_data
                double delt = calculate_delta(f.finaldata[z][0], rgui_s.international_std[z]);
                f.r_std_measured.push_back(f.finaldata[z][0]); //r_std_meas = (mean for profile) 34 counts / 32 counts
                // old way f.r_std_true.push_back(calculate_r_std_true(rgui_s.true_std[z], rgui_s.international_std[z])); //calculate true standard ratio and save
                f.r_std_true.push_back(calculate_r_std_true(rgui_s.true_std[ii*z + ii], rgui_s.international_std[ii*z + ii])); //calculate true standard ratio and save
                std::cout << " line 148 Rstd: " << calculate_r_std_true(rgui_s.true_std[ii*z + ii], rgui_s.international_std[ii*z + ii]) << std::endl;
                all_Rstd_values.push_back(f.r_std_true[z]);
                f.c_t.push_back(calculate_ct(f.r_std_true[z], f.r_std_measured[z])); //calculate Ct and save
                all_CT_values.push_back(f.c_t[z]);
            }
            write_standard_data(f, file_list[ii], rgui_s);   // write to text file
            //write_to_csv(f, rawDoubles, global_keep_or_reject, rgui_s); // write to csv
            //write_all_raw_to_csv(f, raw_unparsed, rgui_s);
            new_write_csv(f, rawDoubles, global_keep_or_reject, rgui_s, ii);
            f = {}; //reset all variables
        }
        
        write_CT_Rstd_to_csv(rgui_s, all_CT_values, all_Rstd_values);
        // see program_work_log notes from 12/15 about what needs to be done
        // see lines 71/72 for new std::vectors
        std::cout << "STANDARD DONE ALL" << std::endl;
        return status::SUCCESS;
    }
    
    else{ //Normal (non-standard) runs:
        // make instance of global user inputs struct
        revised_global_user_inputs rgui;
        ask_for_int(rgui.number_of_columns, 1, "Enter number of data columns in files", "    e.g. enter 2 for Heart data");
        ask_for_int(rgui.points_per_column, 5, "Enter number of cycles", "	 e.g. enter '90' for Heart data");
        
        int response = ask_for_chauvenet(rgui.points_per_column, rgui.chauvenet);
        if (response == status::ERROR_BAD_NUM_POINTS_CHAUV){
            std::cout << "Status Error: " << response << std::endl;
            return response;
        }
        ask_for_double(rgui.primary_beam_current, 0.0, "Enter primary beam current in picoamps", "    e.g. for 1500 picoamps enter '1500'");
        std::cout << "Primary Beam Current: " << rgui.primary_beam_current << " picoamps" << std::endl;
        
        rgui.primary_beam_flux = rgui.primary_beam_current*(6.2415*pow(10, 6));
        rgui.columns_to_compare = infinite_ask_for_cols(rgui.column_headers); // asks for how many column comparisons the user wants to make
        rgui.apply_sampling_correction = yes_no_question("Apply sampling time difference interpolation correction? (y/n)");
        rgui.apply_qsa_dt_fc = yes_no_question("Apply deadtime, QSA, and FC corrections? (y/n)");
        
        //assemble vectors of deadtimes and standards for each set of comparisons
        for (unsigned int z = 0; z < rgui.columns_to_compare.size() / 2; z++){
            std::cout << "Parameters related to comparison " << z << ":" << std::endl;
            std::cout << std::endl;
            double standard;
            ask_for_std(standard);
            rgui.standard.push_back(standard);
            double ct;
            ask_for_ct(ct);
            rgui.ct.push_back(ct);
        }
        
        // assemble list of input filenames and new names
        std::vector<std::string> file_list;
        std::vector<std::string> new_names;
        int num_files = 0;
        
        DIR *dir;
        struct dirent *ent;
        if ((dir = opendir("C:\\Users\\Lab User\\Documents\\Visual Studio 2013\\Projects\\DataExtractionApril2017\\Debug\\inputs\\")) != NULL) {
            /* print all the files and directories within directory */
            while ((ent = readdir(dir)) != NULL) {
                //printf("%s\n", ent->d_name);
                std::string fullname = ent->d_name;
                //cout << "fullname: " << fullname << std::endl;
                file_list.push_back("inputs\\" + fullname);
                std::string newname = fullname.substr(0, fullname.size() - 4); //chop off .asc extension
                //cout << "newname: " << newname << std::endl;
                new_names.push_back(newname);
                num_files++;
            }
            closedir(dir);
        }
        else{
            std::cout << "dir not found " << std::endl;
        }
        // erase the first two elements "." and ".." in file_list
        file_list.erase(file_list.begin(), file_list.begin() + 2);
        new_names.erase(new_names.begin(), new_names.begin() + 2);
        num_files = num_files - 2;
        //iterate through each file using the object
        for (int ii = 0; ii < num_files; ii++){
            //instance of file_data_to_write
            revised_file_data_to_write f;
            f.output_filename = new_names[ii];
            std::cout << std::endl;
            std::cout << "Now Parsing: " << file_list[ii] << std::endl;
            f.stat_results_Ri = get_someData(file_list[ii]);
            f.xypos = find_xy_pos(file_list[ii]);
            f.detector_types = find_detector_type(file_list[ii]);
            f.detector_values = find_detector_values(file_list[ii]);
            f.beam_times = find_beam_times(file_list[ii], rgui.number_of_columns);
            
            //field app & entrance slit
            std::vector<std::string> field_and_slit;
            std::string field_app_x = get_field_x(file_list[ii]);
            field_and_slit.push_back(field_app_x);
            std::string field_app_y = get_field_y(file_list[ii]);
            field_and_slit.push_back(field_app_y);
            std::string slit_x = get_slit_x(file_list[ii]);
            field_and_slit.push_back(slit_x);
            f.field_app_xy = field_and_slit;
            
            //get raw data
            std::vector<int> columns = rgui.columns_to_compare;
            int num_rows = rgui.points_per_column;
            std::vector<std::vector<std::string>> raw_unparsed = get_rawData(file_list[ii], rgui.number_of_columns, num_rows); // raw_unparsed stores all raw data as strings
            f.rawData = infinite_parse_raw_data(raw_unparsed, columns); // reorganize columns of rawData so columns to be compared are next to each other
            
            //split parsed_rawData into pairs of columns to be corrected individually
            std::vector<std::vector<std::vector<std::string>>> parsed_rawData;
            std::vector<std::vector<std::vector<double>>> rawDoubles;
            std::vector<std::vector<double>> file_corrected_sampling_times;
            //iterate once for each pairwise comparison:
            for (unsigned int z = 0; z < rgui.columns_to_compare.size() / 2; z++){
                std::vector<std::vector<std::string>> parsed_rawData1 = {};
                std::vector<std::vector<double>> rawDoubles1 = {};
                std::vector<std::vector<double>> rawDoubles1_temp = {};
                std::vector<double> corrected_sampling_times;
                
                // put first set of data in new std::vector
                parsed_rawData1.resize(f.rawData.size(), std::vector<std::string>(4, ""));
                for (unsigned int i = 0; i < f.rawData.size(); i++){
                    for (unsigned int j = 0; j < 4; j++){
                        parsed_rawData1[i][j] = f.rawData[i][j + (4 * z)];
                    }
                }
                rawDoubles1 = string_to_double(parsed_rawData1);
                rawDoubles1_temp = string_to_double(parsed_rawData1);
                
                //correct raw data for EM, FC1 and FC2, sampling time
                //iterate through each column
                for (unsigned int i = 0; i < rawDoubles1.size(); i++){
                    //correct for sampling time
                    if (rgui.apply_sampling_correction){
                        // if we are not at the last row (since next row is needed for calculation):
                        if (i != rawDoubles1.size() - 1){
                            double temp_corrected_sampling_time;
                            //						 correct_for_sampling_time(double datapoint, double next_datapoint, double cycle_number, double datapoint_column_number, double corresponding_datapoint_column_number, std::vector<std::vector<string>>beam_times, double &corrected_sampling_time);
                            rawDoubles1_temp[i][0] = correct_for_sampling_time(rawDoubles1[i][0], rawDoubles1[i + 1][0], i, rgui.columns_to_compare[2 * z], rgui.columns_to_compare[2 * z + 1], f.beam_times, temp_corrected_sampling_time);
                            corrected_sampling_times.push_back(temp_corrected_sampling_time);
                        }
                        //EDGE CASE: include last datapoint "uncorrected;" input the last datapoint for the current datapoint as well as the following datapoint
                        else if (i == rawDoubles1.size() - 1){
                            double temp_corrected_sampling_time;               // below, using the same datapoint for both sides so no correction is made
                            rawDoubles1_temp[i][0] = correct_for_sampling_time(rawDoubles1[i][0], rawDoubles1[i][0], i, rgui.columns_to_compare[2 * z], rgui.columns_to_compare[2 * z + 1], f.beam_times, temp_corrected_sampling_time);
                            corrected_sampling_times.push_back(temp_corrected_sampling_time);
                        }
                    }
                    rawDoubles1 = rawDoubles1_temp;
                    //check if correct deadtime/ qsa/ fc
                    if (rgui.apply_qsa_dt_fc){
                        //correct for deadtime, qsa, and fc as applicable
                        for (unsigned int j = 0; j < 2; j++){
                            // convert data types
                            double EM_yield = atof(f.detector_values[0][0].c_str());
                            //cout << "EM_yield: " << EM_yield << std::endl;
                            double EM_deadtime = atof(f.detector_values[0][1].c_str()) * .000000001;
                            //cout << "em deadtime: " << EM_deadtime << std::endl;
                            double FC1_yield = atof(f.detector_values[1][0].c_str());
                            double FC1_background = atof(f.detector_values[1][1].c_str());
                            double FC2_yield = atof(f.detector_values[2][0].c_str());
                            double FC2_background = atof(f.detector_values[2][1].c_str());
                            
                            if (f.detector_types[rgui.columns_to_compare[2 * z + j]] == "EM "){
                                //cout << "applying deadtime and qsa corrections to data column " << rgui.columns_to_compare[2 * z + j] << std::endl;
                                //cout << "before any:     EM: i: " << i << "  j:" << j << " rawDoubles1[i][j]= " << rawDoubles1[i][j] << std::endl;
                                rawDoubles1[i][j] = correct_for_deadtime(rawDoubles1[i][j], EM_deadtime, EM_yield);
                                //cout << "after deadtime: EM: i: " << i << "  j:" << j << " rawDoubles1[i][j]= " << rawDoubles1[i][j] << std::endl;
                                rawDoubles1[i][j] = correct_for_qsa(rawDoubles1[i][j], rgui.primary_beam_flux);
                                //cout << "after qsa:      EM: i: " << i << "  j:" << j << " rawDoubles1[i][j]= " << rawDoubles1[i][j] << std::endl;
                            }
                            else if (f.detector_types[rgui.columns_to_compare[2 * z + j]] == "FC1"){
                                rawDoubles1[i][j] = correct_for_fc(rawDoubles1[i][j], FC1_background, FC1_yield);
                                //cout << "FC1: i: " << i << "  j:" << j << " rawDoubles1[i][j]= " << rawDoubles1[i][j] << std::endl;
                                //cout << "applying FC1 correction to data column " << rgui.columns_to_compare[2 * z + j] << std::endl;
                            }
                            else if (f.detector_types[rgui.columns_to_compare[2 * z + j]] == "FC2"){
                                rawDoubles1[i][j] = correct_for_fc(rawDoubles1[i][j], FC2_background, FC2_yield);
                                //cout << "FC2: i: " << i << "  j:" << j << " rawDoubles1[i][j]= " << rawDoubles1[i][j] << std::endl;
                                //cout << "applying FC2 correction to data column " << rgui.columns_to_compare[2 * z + j] << std::endl;
                            }
                        }
                    }
                }
                parsed_rawData.push_back(parsed_rawData1);
                rawDoubles.push_back(rawDoubles1); // (if all corrections are confirmed by user, now corrected for EM and FC1 and FC2 and sampling time)
                global_final_corrected.push_back(rawDoubles1);
                // run correct data
                std::cout << "***PARAMETERS RELATED TO COMPARISON " << z << ": ***" << std::endl;
                
                std::vector<double> temp_corrected_data = correct_data(rawDoubles1, rgui.chauvenet, rgui.standard[z], global_keep_or_reject, rgui.ct[z]); // note that "rawDoubles1" is now deadtime corrected"
                f.finaldata.push_back(temp_corrected_data);
                file_corrected_sampling_times.push_back(corrected_sampling_times);
                std::vector<double> r_squ_slope_1 = linear_regression(global_corrected[z], global_keep_or_reject[z], rgui.points_per_column, f.finaldata[z][1]);
                f.r_squs.push_back(r_squ_slope_1[1]);
                f.slopes.push_back(r_squ_slope_1[0]);
                //std::cout << "SLOPE " << z << ": " << f.slopes[z] << std::endl;
                //std::cout << "R_SQU " << z << ": " << f.r_squs[z] << std::endl;
            }
            global_corrected_sampling_times.push_back(file_corrected_sampling_times);
            write_data(f, file_list[ii], rgui);   // write to text file
            write_to_csv(f, rawDoubles, global_keep_or_reject, rgui, ii); // write to csv
            write_all_raw_to_csv(f, raw_unparsed, rgui);
            new_write_csv(f, rawDoubles, global_keep_or_reject, rgui, ii);
            f = {}; //reset all variables
        }
        //write_all_runs_to_csv();
        write_all_runs_to_csv_updated();
        std::cout << "DONE ALL" << std::endl;
        return status::SUCCESS;
    }
}

//*****************************************************
// Usage function to instruct user how to format inputs
//*****************************************************
int usage(char* program_name){
    std::cout << "Usage: " << program_name << std::endl;
    return status::ERROR_INCORRECT_ARGS;
}

//**************
// Error message
//**************
int unable_to_open(std::string filename){
    std::cout << "Unable to open '" << filename << "'" << std::endl;
    std::cout << "input file must be in the same directory as the program itself" << std::endl;
    return status::ERROR_UNABLE_TO_OPEN_FILE;
}

//**************************************
// ask for a string with a custom prompt
//**************************************
int ask_for_string(std::string input_message, std::string &result){
    std::string str = "";
    std::string user_str;
    bool valid = false;
    
    while (!valid){
        valid = true;
        std::cout << input_message << std::endl;
        std::cin >> user_str;
        if (std::cin.fail()){
            std::cin.clear();
            std::cin.ignore();
            std::cout << "invalid input" << std::endl;
            valid = false;
        }
        else if (user_str.length() > 0){
            str = user_str;
            valid = true;
        }
        else{
            std::cout << "invalid input" << std::endl;
            valid = false;
        }
    }
    std::cout << "Using: " << str << "\n" << std::endl;
    result = str;
    return status::SUCCESS;
}

//************************
// finds x and y positions
//************************
std::vector<std::string> find_xy_pos(std::string input_file_name){
    std::vector<std::string> xy_pos;
    
    std::ifstream ifs(input_file_name.c_str());
    if (!ifs) {
        unable_to_open(input_file_name);  //reminder: input file must be in the same directory as the program itself
    }
    else
    {
        std::string data = "";
        const std::string needle = "X POSITION";
        std::string xpos = "";
        std::string ypos = "";
        while (std::getline(ifs, data)) {
            if (data.find("X POSITION") != std::string::npos){
                // get x position and y position
                size_t found_x = data.find("X POSITION");
                size_t found_y = data.find("Y POSITION");
                // calculate length of each string to pull out
                int x_length = found_y - found_x - 12;
                int y_length = data.size() - found_y - 12;
                std::cout << "x_length: " << x_length << std::endl;
                std::cout << "y_length: " << x_length << std::endl;
                // get x position and y position
                xpos = data.substr(found_x + 12, x_length);
                ypos = data.substr(found_y + 12, y_length);
                break;
            }
            else{
                xpos = "xpos not found";
                ypos = "ypos not found";
            }
        }
        xy_pos.push_back(xpos);
        xy_pos.push_back(ypos);
    }
    ifs.close();
    return xy_pos;
}

//********************
// find detector types
//********************
std::vector<std::string> find_detector_type(std::string input_file_name){
    std::vector<std::string> detector_types;
    
    std::ifstream ifs(input_file_name.c_str());
    if (!ifs) {
        //unable_to_open(input_file_name);  //reminder: input file must be in the same directory as the program itself
        std::cout << "unable to open file" << std::endl;
    }
    else{
        std::string data = "";
        //char delimiter = (char)"Em_Drift";
        while (std::getline(ifs, data)) {
            if (data.find("Detector") != std::string::npos){
                for (unsigned int i = 1; i < data.size() / 11; i++){
                    if ((data.find("EM") != std::string::npos) || (data.find("FC") != std::string::npos)){
                        detector_types.push_back(data.substr(11 * i, 3));
                        
                    }
                }
                break;
            }
        }
    }
    ifs.close();
    return detector_types;
}

//*******************************************
// finds yield and deadtime/background values
//*******************************************
std::vector<std::vector<std::string>> find_detector_values(std::string input_file_name){
    std::vector<std::vector<std::string>> detector_values;								// how the array is always stored:
    std::vector<std::string> EM_values;											//EMyield:   detector_values[0][0] EMdeadtime:     detector_values[0][1]
    std::vector<std::string> FC1_values;											//FC1 yield: detector_values[1][0] FC1 background: detector_values[1][1]
    std::vector<std::string> FC2_values;											//FC1 yield: detector_values[2][0] FC2 background: detector_values[2][1]
    std::ifstream ifs(input_file_name.c_str());
    if (!ifs) {
        //unable_to_open(input_file_name);  //reminder: input file must be in the same directory as the program itself
        std::cout << "unable to open file" << std::endl;
    }
    else{
        std::string data = "";
        //char delimiter = (char)"Em_Drift";
        while (std::getline(ifs, data)) {
            if (data.find("DETECTOR PARAMETERS:") != std::string::npos){
                // go down 4 more lines
                std::getline(ifs, data);
                std::getline(ifs, data);
                std::getline(ifs, data);
                std::getline(ifs, data);
                //LINE 1 of 3
                //unsure of ordering here so on each line have to check what type of detector it is
                if (data.find("FC1") != std::string::npos){
                    FC1_values.push_back(data.substr(11, 8)); // Yield
                    FC1_values.push_back(data.substr(22, 6));  // Bkg(c/s)
                    //cout << FC1_values[0] << std::endl;
                    //cout << FC1_values[1] << std::endl;
                }
                else if (data.find("FC2") != std::string::npos){
                    FC2_values.push_back(data.substr(11, 8)); // Yield
                    FC2_values.push_back(data.substr(22, 6));  // Bkg(c/s)
                    //cout << FC2_values[0] << std::endl;
                    //cout << FC2_values[1] << std::endl;
                }
                else if (data.find("EM") != std::string::npos){
                    EM_values.push_back(data.substr(11, 8)); // Yield
                    EM_values.push_back(data.substr(33, 4)); // Deadtime
                    //cout << EM_values[0] << std::endl;
                }
                //LINE 2 of 3
                //check for detector type on next line and act accordingly
                getline(ifs, data);
                if (data.find("FC1") != std::string::npos){
                    FC1_values.push_back(data.substr(11, 8)); // Yield
                    FC1_values.push_back(data.substr(22, 6));  // Bkg(c/s)
                    //cout << FC1_values[0] << std::endl;
                    //cout << FC1_values[1] << std::endl;
                }
                else if (data.find("FC2") != std::string::npos){
                    FC2_values.push_back(data.substr(11, 8)); // Yield
                    FC2_values.push_back(data.substr(22, 6));  // Bkg(c/s)
                    //cout << FC2_values[0] << std::endl;
                    //cout << FC2_values[1] << std::endl;
                }
                else if (data.find("EM") != std::string::npos){
                    EM_values.push_back(data.substr(11, 8)); // Yield
                    EM_values.push_back(data.substr(33, 4)); // Deadtime
                    //cout << EM_values[0] << std::endl;
                }
                //LINE 3 of 3
                //check for detector type on next line and act accordingly
                getline(ifs, data);
                if (data.find("FC1") != std::string::npos){
                    FC1_values.push_back(data.substr(11, 8)); // Yield
                    FC1_values.push_back(data.substr(22, 6));  // Bkg(c/s)
                    //cout << FC1_values[0] << std::endl;
                    //cout << FC1_values[1] << std::endl;
                }
                else if (data.find("FC2") != std::string::npos){
                    FC2_values.push_back(data.substr(11, 8)); // Yield
                    FC2_values.push_back(data.substr(22, 6));  // Bkg(c/s)
                    //cout << FC2_values[0] << std::endl;
                    //cout << FC2_values[1] << std::endl;
                }
                else if (data.find("EM") != std::string::npos){
                    EM_values.push_back(data.substr(11, 8)); // Yield
                    EM_values.push_back(data.substr(33, 4)); // Deadtime
                    //cout << EM_values[0] << std::endl;
                }
                break;
            }
        }												// how the array is always stored:
        detector_values.push_back(EM_values);			//detector_values[0][0] detector_values[0][1]
        detector_values.push_back(FC1_values);			//detector_values[1][0] detector_values[1][1]
        detector_values.push_back(FC2_values);			//detector_values[2][0] detector_values[2][1]
        
        //verified
        std::cout << "em yield:  " << detector_values[0][0] << " EM deadtime:    " << detector_values[0][1] << std::endl;
        std::cout << "fc1 yield: " << detector_values[1][0] << " fc1 background: " << detector_values[1][1] << std::endl;
        std::cout << "fc2 yield: " << detector_values[2][0] << " fc2 background: " << detector_values[2][1] << std::endl;
    }
    ifs.close();
    return detector_values;
}

//*******************************************
// finds beam times
//*******************************************
std::vector<std::vector<std::string>> find_beam_times(std::string input_file_name, int number_of_columns){
    std::vector<std::vector<std::string>> beam_times;								// full array composed of:
    std::vector<std::string> c_times;											// c_times: variable in length
    std::vector<std::string> w_times;											// w_times: variable in length
    std::ifstream ifs(input_file_name.c_str());
    if (!ifs) {
        //unable_to_open(input_file_name);  //reminder: input file must be in the same directory as the program itself
        std::cout << "unable to open file" << std::endl;
    }
    else{
        std::string data = "";
        //char delimiter = (char)"Em_Drift";
        while (std::getline(ifs, data)) {
            //find correct header
            if (data.find("ACQUISITION PARAMETERS:") != std::string::npos){
                //if (data.find("ACQUISITION PARAMETERS:") != std::string::npos){  alternate with no need to go down 7 lines
                // go down 7 more lines
                std::getline(ifs, data);
                std::getline(ifs, data);
                std::getline(ifs, data);
                std::getline(ifs, data);
                std::getline(ifs, data);
                std::getline(ifs, data);
                std::getline(ifs, data);
                
                //iterate through two lines, check for C.Time(s) and W.Time(s) on each line (unsure of order)
                for (size_t j = 0; j < 2; j++){
                    //C.Time(s)
                    if (data.find("C.Time(s)") != std::string::npos){
                        for (int i = 1; i <= number_of_columns; i++){
                            c_times.push_back(data.substr(11 * i, 4));
                        }
                    }
                    //W.Time(s)
                    else if (data.find("W.Time(s)") != std::string::npos){
                        for (int i = 1; i <= number_of_columns; i++){
                            w_times.push_back(data.substr(11 * i, 4));
                        }
                    }
                    getline(ifs, data);
                }
                
                break;
            }
        }												// how the array is always stored:
        beam_times.push_back(c_times);			//detector_values[0][0] detector_values[0][1]
        beam_times.push_back(w_times);			//detector_values[1][0] detector_values[1][1]
        
        //to verify:
        /*for (size_t i = 0; i < beam_times.size(); i++){
         for (size_t j = 0; j < beam_times[0].size(); j++){
         std::cout << "beam_times " << i << ", " << j << ": " << beam_times[i][j] << std::endl;
         }
         std::cout << std::endl;
         }*/
    }
    ifs.close();
    return beam_times;
}

//***************
//get Field App x
//***************
std::string get_field_x(std::string input_file_name){
    std::string trimmed_field_x = "field_x not found";
    
    std::ifstream ifs(input_file_name.c_str());
    if (!ifs) {
        unable_to_open(input_file_name);  //reminder: input file must be in the same directory as the program itself
    }
    else
    {
        std::string data = "";
        char delimiter = (char)"Entrance";
        std::string field_x = "";
        std::string trimmed_field_x = "";
        while (std::getline(ifs, data, delimiter)) {
            // ***********
            // get field_x
            // ***********
            size_t found_field_app = data.find("Field App");
            if (data.find("Field App") != std::string::npos){
                field_x = data.substr(found_field_app + 44, 7);      // second paramenter is length
                //std::cout << "untrimmed field_x is " << field_x << std::endl;
                size_t begin_x = field_x.find_first_not_of(' ');     // finds position in string where number starts
                //cout << "begin_x is " << begin_x << std::endl;
                trimmed_field_x = field_x.substr(begin_x, field_x.size() - begin_x); // cut out spaces from front of untrimmed string
                //cout << "front trimmed: " << trimmed_field_x << "END" << std::endl;
                size_t end_x = trimmed_field_x.find_first_of(' ');
                //cout << "end_x is " << end_x << std::endl;
                trimmed_field_x = trimmed_field_x.substr(0, end_x);
                //cout << "final trimmed x is_" << trimmed_field_x <<"_END"<< std::endl;
                return trimmed_field_x;
            }
            else{
                trimmed_field_x = "field_x not found";
                std::cout << "trimmed x is" << trimmed_field_x << std::endl;
            }
            break;
        }
    }
    ifs.close();
    return trimmed_field_x;
}

//***************
//get Field App y
//***************
std::string get_field_y(std::string input_file_name){
    std::string trimmed_field_y = "nothing";
    std::ifstream ifs(input_file_name.c_str());
    if (!ifs) {
        unable_to_open(input_file_name);  //reminder: input file must be in the same directory as the program itself
    }
    else
    {
        std::string data = "";
        char delimiter = (char)"Entrance";
        std::string field_y = "";
        while (std::getline(ifs, data, delimiter)) {
            // ***********
            // get field_y
            // ***********
            size_t found_field_app = data.find("Field App");
            if (data.find("Field App") != std::string::npos){
                field_y = data.substr(found_field_app + 56, 8);      // second paramenter is length
                //std::cout << "untrimmed field_y is " << field_y << std::endl;
                size_t begin_y = field_y.find_first_not_of(' ');
                //cout << "begin_y is " << begin_y << std::endl;
                trimmed_field_y = field_y.substr(begin_y, field_y.size() - begin_y); // cut out spaces from front of untrimmed string
                size_t end_y = trimmed_field_y.find_first_of(' ');
                //cout << "end_y is " << end_y << std::endl;
                trimmed_field_y = trimmed_field_y.substr(0, end_y);  // cut out spaces from end
                //cout << "final trimmed y is_" << trimmed_field_y << "_END" << std::endl;
                return trimmed_field_y;
            }
            else{
                trimmed_field_y = "field_y not found";
            }
            break;
        }
    }
    ifs.close();
    return trimmed_field_y;
}

//*******************
//get Entrance Slit X
//*******************
//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
// currently it can't find the word "entrance" in the data file, there seems to be a bug in std::ifstream because this also happened in get-field-x and get-field-y
//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
std::string get_slit_x(std::string input_file_name){
    std::string trimmed_slit_x = "nothing";
    std::ifstream ifs(input_file_name.c_str());
    if (!ifs) {
        unable_to_open(input_file_name);  //reminder: input file must be in the same directory as the program itself
    }
    else
    {
        std::string data = "";
        char delimiter = (char)"Contrast";
        std::string slit_x = "";
        while (std::getline(ifs, data, delimiter)) {
            // **********
            // get slit_x
            // **********
            size_t found_slit_x = data.find("Entrance Slits");
            if (data.find("Entrance Slits") != std::string::npos){
                slit_x = data.substr(found_slit_x + 44, 8);      // second paramenter is length
                
                size_t begin_x = slit_x.find_first_not_of(' ');
                //cout << "begin_y is " << begin_y << std::endl;
                trimmed_slit_x = slit_x.substr(begin_x, slit_x.size() - begin_x); // cut out spaces from front of untrimmed string
                size_t end_y = trimmed_slit_x.find_first_of(' ');
                //cout << "end_y is " << end_y << std::endl;
                trimmed_slit_x = trimmed_slit_x.substr(0, end_y);  // cut out spaces from end
                //cout << "final trimmed y is_" << trimmed_field_y << "_END" << std::endl;
                return trimmed_slit_x;
            }
            else{
                std::cout << "slit x not found" << std::endl;
                trimmed_slit_x = "slit_x not found";
            }
            break;
        }
    }
    ifs.close();
    return trimmed_slit_x;
}

//********************************************************************************************************
//returns R values pulled from the output file. (In original cases, R0 is mean 32S and R1 is mean 34S/32S)
//********************************************************************************************************
std::vector<std::string> get_someData(std::string input_file_name)
{
    // open input file
    std::ifstream ifs(input_file_name.c_str());
    std::vector<std::string> stat_results_Ri;
    if (!ifs) {
        unable_to_open(input_file_name);  //reminder: input file must be in the same directory as the program itself
    }
    else
    {
        //std::cout << "File opened successfully for get_someData" << std::endl;
        
        // *********************************
        // get raw data from ascii text file
        // *********************************
        std::string data = "";
        std::string delimiter = " ";
        
        std::string R0 = "";
        std::string R1 = "";
        std::string R2 = "";
        std::string R3 = "";
        std::string R4 = "";
        std::string R5 = "";
        std::string R6 = "";
        std::string R7 = "";
        
        int num_R_vals = 0;
        while (getline(ifs, data)) {
            // ****************************************************************************
            // determine number of R values in ISOTOPICS RATIO using getline and a counter
            // not working now for some reason upon implemeting the cheater's R0/R1 value extracter below, but really how valuable is this info?
            // ****************************************************************************
            /*
             if (data.find("Rejected #") != std::string::npos) {
             while (getline(ifs, data)) {
             if (data != "") { // not a blank line
             num_R_vals++;
             }
             if (data.find("RESULTS") != std::string::npos) {
             break;
             }
             }
             num_R_vals--; //do this because the counter counts the "Rejected #" line; there is actually one less R value
             
             std::cout << "num R vals: " << num_R_vals << std::endl;
             }
             */
            
            // *********************************************
            // store values associated with all R values
            // currently looks for up to 8 R values
            // the problem was looking past the first set of R0 and R1 in the file; using "CUMULATED" as keyword to get between first and second set
            // *********************************************
            if (data.find("CUMULATED") != std::string::npos){						// find line containing "CUMULATED"
                for (unsigned int l = 0; l < 10; l++) {							// loop through the next several lines
                    //R0
                    while (std::getline(ifs, data)) {
                        if (data.find("R0") != std::string::npos) {					// find the correct R0
                            size_t found_R0 = data.find("R0");
                            if (found_R0 != std::string::npos) {
                                R0 = data.substr(found_R0 + 2, found_R0 + 14);  // pull R0 value, including scientific notation
                                stat_results_Ri.push_back(R0);					// store it in std::vector
                                break;
                            }
                        }
                    }
                    //R1
                    while (std::getline(ifs, data)) {								// do same thing for R1
                        if (data.find("R1") != std::string::npos) {
                            size_t found_R1 = data.find("R1");
                            if (found_R1 != std::string::npos) {
                                R1 = data.substr(found_R1 + 2, found_R1 + 14);
                                stat_results_Ri.push_back(R1);
                                break;
                            }
                        }
                    }
                    //R2
                    while (std::getline(ifs, data)) {								// do same thing for R2
                        if (data.find("R2") != std::string::npos) {
                            size_t found_R2 = data.find("R2");
                            if (found_R2 != std::string::npos) {
                                R2 = data.substr(found_R2 + 2, found_R2 + 14);
                                stat_results_Ri.push_back(R2);
                                break;
                            }
                        }
                    }
                    //R3
                    while (std::getline(ifs, data)) {								// do same thing for R3
                        if (data.find("R3") != std::string::npos) {
                            size_t found_R3 = data.find("R3");
                            if (found_R3 != std::string::npos) {
                                R3 = data.substr(found_R3 + 2, found_R3 + 14);
                                stat_results_Ri.push_back(R3);
                                break;
                            }
                        }
                    }
                    //R4
                    while (std::getline(ifs, data)) {								// do same thing for R4
                        if (data.find("R4") != std::string::npos) {
                            size_t found_R4 = data.find("R4");
                            if (found_R4 != std::string::npos) {
                                R4 = data.substr(found_R4 + 2, found_R4 + 14);
                                stat_results_Ri.push_back(R4);
                                break;
                            }
                        }
                    }
                    //R5
                    while (std::getline(ifs, data)) {								// do same thing for R5
                        if (data.find("R5") != std::string::npos) {
                            size_t found_R5 = data.find("R5");
                            if (found_R5 != std::string::npos) {
                                R5 = data.substr(found_R5 + 2, found_R5 + 14);
                                stat_results_Ri.push_back(R5);
                                break;
                            }
                        }
                    }
                    //R6
                    while (std::getline(ifs, data)) {								// do same thing for R6
                        if (data.find("R6") != std::string::npos) {
                            size_t found_R6 = data.find("R6");
                            if (found_R6 != std::string::npos) {
                                R6 = data.substr(found_R6 + 2, found_R6 + 14);
                                stat_results_Ri.push_back(R6);
                                break;
                            }
                        }
                    }
                    //R7
                    while (std::getline(ifs, data)) {								// do same thing for R7
                        if (data.find("R7") != std::string::npos) {
                            size_t found_R7 = data.find("R7");
                            if (found_R7 != std::string::npos) {
                                R7 = data.substr(found_R7 + 2, found_R7 + 14);
                                stat_results_Ri.push_back(R7);
                                break;
                            }
                        }
                    }
                }
            }
        }
        ifs.close();
    }
    return stat_results_Ri;
}

//**********************************************************************************************************
//*same as ask_for_cols but can store *infinite* columns for *infinite* comparisons
// currently limited to 20 comparisons for program safety
//
//ask user which isotopes they wish to compare. default is column 0 versus column 1
// compare up to 3 sets of columns
// output example: {0,1,3,4,5,2}  reads as: compare 0 vs 1 (ratio of column0/column1)
//                 compare 3 vs 4, compare 5 vs 2
// also prompts user to enter the appropriate header for each column and stores in referenced std::vector<string>
//**********************************************************************************************************
std::vector<int> infinite_ask_for_cols(std::vector<std::string> &headers){
    int user_num;
    int validated_user_num;
    bool valid_user_num = false;
    while (!valid_user_num){
        valid_user_num = true;
        std::cout << std::endl;
        std::cout << "How many sets of comparisons/ratios would you like to make in each file?" << std::endl;
        std::cout << "    (Example: If only comparing 34S / 32S, enter 1)" << std::endl;
        std::cout << "     If comparing 34S / 32S as well as 18O / 16O, enter 2)" << std::endl;
        std::cin >> user_num;
        if (std::cin.fail()){
            std::cin.clear();
            std::cin.ignore();
            std::cout << "invalid input" << std::endl;
            valid_user_num = false;
        }
        else if (user_num > 0 && user_num <= 20){ // limit to 20 comparisons
            validated_user_num = user_num;
            valid_user_num = true;
        }
        else{
            std::cout << "invalid input" << std::endl;
            valid_user_num = false;
        }
    }
    std::vector<int> response;
    
    // iterate through each comparison, ask user which columns to compare for each comparison
    for (int i = 0; i < validated_user_num; i++)
    {
        bool validint1 = false;
        bool valid_head1 = false;
        bool validint2 = false;
        bool valid_head2 = false;
        int user_col0 = -1;
        std::string user_head1 = "";
        int user_col1 = -1;
        std::string user_head2 = "";
        std::cout << std::endl;
        std::cout << "Comparison Number " << i << ": " << std::endl;
        while (!validint1){
            validint1 = true;
            std::cout << "Enter the first of two columns to compare (the lighter isotope)" << std::endl;
            std::cout << "    (Example: if comparing 34S / 32S, now enter column corresponding to 32S)" << std::endl;
            std::cout << "    e.g. 0" << std::endl;
            std::cout << "    (Note: this program interprets the first column in the Cameca file as column 0, the second as 1, and so on)" << std::endl;
            std::cin >> user_col0;
            if (std::cin.fail()){
                std::cin.clear();
                std::cin.ignore();
                std::cout << "invalid input" << std::endl;
                validint1 = false;
            }
            else if (user_col0 >= 0){
                response.push_back(user_col0);
                std::cout << "first column: " << user_col0 << std::endl;
                validint1 = true;
            }
            else{
                std::cout << "invalid input" << std::endl;
                validint1 = false;
            }
        }
        //header 1
        while (!valid_head1){
            valid_head1 = true;
            std::cout << "Enter the corresponding isotope for that column" << std::endl;
            std::cout << "    (e.g. '32S')" << std::endl;
            std::cin >> user_head1;
            if (std::cin.fail()){
                std::cin.clear();
                std::cin.ignore();
                std::cout << "invalid input" << std::endl;
                valid_head1 = false;
            }
            else {
                headers.push_back(user_head1);
            }
        }
        //int #2
        while (!validint2){
            validint2 = true;
            std::cout << "Enter the second of two columns to compare (heavier isotope)" << std::endl;
            std::cout << "    (example: if comparing 34S / 32S, now enter column corresponding to 34S)" << std::endl;
            std::cout << "    (Note: the second column in the output file is recognized as column 1)" << std::endl;
            std::cin >> user_col1;
            if (std::cin.fail()){
                std::cin.clear();
                std::cin.ignore();
                std::cout << "invalid input" << std::endl;
                validint2 = false;
            }
            else if (user_col0 >= 0){
                response.push_back(user_col1);
                std::cout << "second column: " << user_col1 << std::endl;
                validint2 = true;
            }
            else{
                std::cout << "invalid input" << std::endl;
                validint2 = false;
            }
        }
        //header 2
        while (!valid_head2){
            valid_head2 = true;
            std::cout << "Enter the corresponding isotope for that column" << std::endl;
            std::cout << "    (e.g. '34S')" << std::endl;
            std::cin >> user_head2;
            if (std::cin.fail()){
                std::cin.clear();
                std::cin.ignore();
                std::cout << "invalid input" << std::endl;
                valid_head2 = false;
            }
            else {
                headers.push_back(user_head2);
            }
        }
        //c_c = response;
    }
    return response;
}

//*********************************************************
//ask a yes or no question
//yes returns true; no returns false
//*********************************************************
bool yes_no_question(std::string message){
    bool response = false;
    std::string yn;
    bool validyn = false;
    while (!validyn){
        validyn = true;
        std::cout << message << std::endl;
        std::cin >> yn;
        if (std::cin.fail()){
            std::cin.clear();
            std::cin.ignore();
            std::cout << "invalid input" << std::endl;
            validyn = false;
        }
        if (yn.compare("n") == 0 || yn.compare("N") == 0 || yn.compare("no") == 0 || yn.compare("No") == 0){
            validyn = true;
            response = false;
        }
        else if ((yn.compare("y") == 0 || yn.compare("Y") == 0 || yn.compare("yes") == 0 || yn.compare("Yes") == 0)){
            response = true;
        }
        else{
            std::cout << " invalid input" << std::endl;
            validyn = false;
        }
    }
    return response;
}

//*********************************************************
// asks for an int with a specified lower bound (inclusive)
//*********************************************************
int ask_for_int(int &result, int lower_bound, std::string message_1, std::string message_2){
    int user_int = -1;
    bool validint = false;
    
    while (!validint){
        validint = true;
        std::cout << message_1 << std::endl;
        std::cout << message_2 << std::endl;
        
        std::cin >> user_int;
        if (std::cin.fail()){
            std::cin.clear();
            std::cin.ignore();
            std::cout << "invalid input" << std::endl;
            validint = false;
        }
        else if (user_int >= lower_bound){
            result = user_int;
            validint = true;
        }
        else{
            std::cout << "invalid input" << std::endl;
            validint = false;
        }
    }
    return status::SUCCESS;
}

//*********************************************************
// asks for an int with a specified lower bound (inclusive)
//*********************************************************
int ask_for_double(double &result, double lower_bound, std::string message_1, std::string message_2){
    double user_double = -1.0;
    bool valid_double = false;
    
    while (!valid_double){
        valid_double = true;
        std::cout << message_1 << std::endl;
        std::cout << message_2 << std::endl;
        
        std::cin >> user_double;
        if (std::cin.fail()){
            std::cin.clear();
            std::cin.ignore();
            std::cout << "invalid input" << std::endl;
            valid_double = false;
        }
        else if (user_double >= lower_bound){
            result = user_double;
            valid_double = true;
        }
        else{
            std::cout << "invalid input" << std::endl;
            valid_double = false;
        }
    }
    return status::SUCCESS;
}

//*****************************************************************
//gets raw data from file
// returns 2dimensional std::vector containing raw data from each column
//*****************************************************************
std::vector<std::vector<std::string>> get_rawData(std::string input_file_name, int num_columns, int num_rows)
{
    // open input file
    std::ifstream ifs(input_file_name.c_str());
    std::string data = "";
    std::vector<std::vector<std::string>> rawData;
    
    if (!ifs) {
        unable_to_open(input_file_name);  //reminder: input file must be in the same directory as the program itself
    }
    else {
        //std::cout << "File opened successfully for getRawData" << std::endl;
        
        // ***********************
        // find and store RAW DATA
        // ***********************
        while (getline(ifs, data)){
            if (data.find("RAW") != std::string::npos) {
                getline(ifs, data);
                getline(ifs, data);
                getline(ifs, data);
                getline(ifs, data);
                getline(ifs, data);
                int i = 0;
                while (i < num_rows){		// iterate through lines of raw data
                    //std::cout << "i: " << i << std::endl;
                    std::vector<std::string> temp;
                    getline(ifs, data);
                    std::istringstream iss(data);
                    std::vector<size_t> start_Indices = { 17, 30, 43, 56, 69, 82, 95, 108 };
                    size_t length = 8;           // length, 7 digits plus decimal
                    for (int j = 0; j < num_columns; j++){
                        //check for "-" sign
                        if (data.substr(start_Indices[j], 1).compare("-") == 0){ // check for negative sign
                            length = 9;
                        }
                        temp.push_back(data.substr(start_Indices[j], length));
                        temp.push_back(data.substr(start_Indices[j] + length + 1, 2));
                    }
                    rawData.push_back(temp);
                    temp.clear();
                    i++;
                    // verified that rawData is storing values correctly, including for Exxon data
                }
            }
            else if (data.find("PRIMARY INTENSITY DATA") != std::string::npos) {
                break;
            }
            
        }
        ifs.close();
        //cout << "raw Data accumulated" << std::endl;
    }
    return rawData;
}

//****************************************************************************************
// *Same as parse_raw_data but can have infinite comparisons
//
// reorganizes the columns of rawData so that columns to be compared are next to each other
//
// NOTE: if a column is not being compared, it will not be included in the output
//
//****************************************************************************************
std::vector<std::vector<std::string>> infinite_parse_raw_data(std::vector<std::vector<std::string>> rawData, std::vector<int> columns){
    std::vector<std::vector<std::string>> parsed_Data;
    parsed_Data.resize(rawData.size(), std::vector<std::string>(columns.size() * 2, "")); // resize vector, have to store twice as many across because of exponents
    
    for (size_t i = 0; i < columns.size(); i = i + 2)
    {
        int index1 = columns[i] * 2;
        int index2 = columns[i + 1] * 2;
        for (unsigned int j = 0; j < rawData.size(); j++){
            parsed_Data[j][2 * i] = rawData[j][index1];
            parsed_Data[j][2 * i + 1] = rawData[j][index1 + 1]; // grab the exponent
            
            parsed_Data[j][2 * i + 2] = rawData[j][index2];
            parsed_Data[j][2 * i + 3] = rawData[j][index2 + 1]; // grab the exponent
        }
    }
    return parsed_Data;
}

//**************************************
//prompt user for deadtime (for EM runs)
//**************************************
double ask_for_deadtime(int column){
    double deadtime = 0.0;                                                    /////PRESET DEADTIME
    double user_deadtime = -1;
    std::string yn;
    bool validyn = false;
    bool validdub = false;
    while (!validyn){
        validyn = true;
        std::cout << "Is there a deadtime (EM) correction for data column " << column << "? (y/n)" << std::endl;
        std::cin >> yn;
        if (std::cin.fail()){
            std::cin.clear();
            std::cin.ignore();
            std::cout << "invalid input" << std::endl;
            validyn = false;
        }
        if (yn.compare("n") == 0 || yn.compare("N") == 0 || yn.compare("no") == 0 || yn.compare("No") == 0){
            validyn = true;
        }
        else if ((yn.compare("y") == 0 || yn.compare("Y") == 0 || yn.compare("yes") == 0 || yn.compare("Yes") == 0)){
            while (!validdub){
                validdub = true;
                std::cout << "enter deadtime correction as decimal (e.g. for 47.7 ns type '.0000000477')" << std::endl;
                std::cin >> user_deadtime;
                if (std::cin.fail()){
                    std::cin.clear();
                    std::cin.ignore();
                    std::cout << "invalid input" << std::endl;
                    validdub = false;
                }
                else if (user_deadtime < 1 && user_deadtime >= 0){
                    deadtime = user_deadtime;
                    std::cout << "using user deadtime of " << user_deadtime << std::endl;
                    validdub = true;
                }
                else{
                    std::cout << "invalid input" << std::endl;
                    validdub = false;
                }
            }
        }
        else{
            std::cout << " invalid input" << std::endl;
            validyn = false;
        }
    }
    std::cout << "deadtime: " << deadtime << std::endl;
    return deadtime;
}

//*****************************************************************************
//prompt user for primary beam current (for EM runs)
//OUTPUT: sets primary beam current passed by reference, returns STATUS:SUCCESS
//*****************************************************************************
int ask_for_primary_beam(double &result){
    double primary_beam_current = 0.0;
    double user_current = -1;
    std::string yn;
    bool validyn = false;
    bool validdub = false;
    
    while (!validdub){
        validdub = true;
        std::cout << "Enter primary beam current in picoamps" << std::endl;
        std::cout << "    e.g. for 1500 picoamps enter '1500'" << std::endl;
        std::cin >> user_current;
        if (std::cin.fail()){
            std::cin.clear();
            std::cin.ignore();
            std::cout << "invalid input" << std::endl;
            validdub = false;
        }
        else if (user_current >= 0){
            primary_beam_current = user_current;
            //std::cout << "using user current of " << user_current << std::endl;
            validdub = true;
        }
        else{
            std::cout << "invalid input" << std::endl;
            validdub = false;
        }
    }
    
    std::cout << "Primary Beam Current: " << primary_beam_current << " picoamps" << std::endl;
    std::cout << std::endl;
    
    result = primary_beam_current;
    return status::SUCCESS;
}

//********************************************************
//corrects an individual column for deadtime (for EM runs)
//********************************************************
double correct_for_deadtime(double input, double deadtime, double yield){ // formula: I(cor) = I(raw)/(1-I(raw)*DT)  *  1/yield
    return input / (1 - input * deadtime) * 1 / yield;
}

//********************************************************
//corrects an individual column for QSA (for EM runs)
//********************************************************
double correct_for_qsa(double input, double primary_beam_flux){ // formula: I(cor) = (Iraw^-1 - B/J) ^ -1    B=.66 , J=primary beam flux = primary beam current*6.2415*e6
    return 1 / (1 / input - .66 / primary_beam_flux);
}

//********************************************************
//corrects an individual column for fc (for fc runs)
//********************************************************
double correct_for_fc(double input, double background, double yield){  // formula: I(cor) = (I(raw)-background)/yield
    return ((input - background) / yield);
}

//************************************************************************
//linear interpolation to correct for difference in sampling time for film
//************************************************************************
double correct_for_sampling_time(double datapoint, double next_datapoint, double cycle_number, double datapoint_column_number, double corresponding_datapoint_column_number, std::vector<std::vector<std::string>>beam_times, double &corrected_sampling_time){
    //linear interpolation to correct for difference in sampling time for film
    // "x"=time and "y"=sample counts
    
    //find total time to complete 1 cycle:
    double one_cycle_time = 0;
    for (size_t i = 0; i < beam_times.size(); i++){
        for (size_t j = 0; j < beam_times[0].size(); j++){
            one_cycle_time += stod(beam_times[i][j]);
        }
    }
    //std::cout << "one_cycle_time: " << one_cycle_time << std::endl;
    
    //find absolute time at datapoint:
    double initial_time = 0.5*stod(beam_times[0][datapoint_column_number]) + stod(beam_times[1][datapoint_column_number]);
    for (size_t i = 0; i < datapoint_column_number; i++){
        initial_time += stod(beam_times[0][i]);
        initial_time += stod(beam_times[1][i]);
    }
    //std::cout << "initial time: " << initial_time << std::endl;
    
    //double time_at_datapoint = cycle_number*one_cycle_time + initial_time;
    //std::cout << "time at datapt: " << time_at_datapoint << std::endl;
    
    //find absolute time at corresponding datapoint:
    double initial_time_corresponding = 0.5*stod(beam_times[0][corresponding_datapoint_column_number]) + stod(beam_times[1][datapoint_column_number]);
    for (size_t i = 0; i < corresponding_datapoint_column_number; i++){
        initial_time_corresponding += stod(beam_times[0][i]);
        initial_time_corresponding += stod(beam_times[1][i]);
    }
    //std::cout << "initial time corresponding: " << initial_time_corresponding << std::endl;
    
    //double time_at_corresponding_datapoint = cycle_number*one_cycle_time + initial_time_corresponding;
    //std::cout << "time at corresp datapt: " << time_at_corresponding_datapoint << std::endl;
    
    // calculate weighted percent (where does comparison datapoint sit between the the datapt and previous datapoint)
    //double front_weight = (time_at_corresponding_datapoint - time_at_datapoint) / one_cycle_time;
    double front_weight = (initial_time_corresponding - initial_time) / one_cycle_time;
    
    //std::cout << "front_weight: " << front_weight << std::endl;
    
    double back_weight = 1 - front_weight;
    //std::cout << "back_weight: " << back_weight << std::endl;
    
    double answer = datapoint*front_weight + next_datapoint*back_weight;
    //std::cout << "next_datapt: " << next_datapoint << "  datapoint: " << datapoint << std::endl;
    
    corrected_sampling_time = cycle_number*one_cycle_time + initial_time_corresponding;
    //std::cout << "corrected_sampling_time: " << corrected_sampling_time << std::endl;
    return answer;
}

//********************************************************
// calculate delta
//********************************************************
double calculate_delta(double r_measured, double r_vcdt){  // formula: d = (r_measured/r_vcdt -1)*1000  where R_vcdt = R_international_std
    return (r_measured / r_vcdt - 1) * 1000;
}

//********************************************************
// calculate R std true
//********************************************************
double calculate_r_std_true(double delta_std, double R_international_std){  // formula: R std true = (delta_std/1000+1)*R_international_std
    return (delta_std / 1000 + 1)*R_international_std;
}

//********************************************************
// calculates Ct
//********************************************************
double calculate_ct(double R_std_true, double R_std_measured){  // formula: Ct = Rstd true / mean Rstd_measured
    return R_std_true / R_std_measured;
}

//********************************************************************
// ask for daily mean std
// note that daily mean is dependent on the day the sample was obtained
// takes a reference to a double, sets to specified std
//********************************************************************
int ask_for_std(double &result){
    double std = 0.0;
    double user_std = -1;
    std::string yn;
    //bool validyn = false;
    bool validdub = false;
    
    while (!validdub){
        validdub = true;
        std::cout << "Enter daily standard ratio (Rstd) as decimal (e.g. .0434877) for this comparison" << std::endl;
        std::cin >> user_std;
        if (std::cin.fail()){
            std::cin.clear();
            std::cin.ignore();
            std::cout << "invalid input" << std::endl;
            validdub = false;
        }
        else if (user_std < 1 && user_std > 0){
            std = user_std;
            validdub = true;
        }
        else{
            std::cout << "invalid input" << std::endl;
            validdub = false;
        }
    }
    
    //std::cout << "Using: " << std << "\n" << std::endl;
    result = std;
    return status::SUCCESS;
}

//****************************************************
//ask for Ct
//takes a reference to a double, sets to specified std
//****************************************************
int ask_for_ct(double &result)
{
    double ct = 0.0;
    double user_ct = -1;
    std::string yn;
    //bool validyn = false;
    bool validdub = false;
    
    while (!validdub){
        validdub = true;
        std::cout << "Enter Ct correction factor (Rstd true / Rstd measured) as decimal for this comparison" << std::endl;
        std::cin >> user_ct;
        if (std::cin.fail()){
            std::cin.clear();
            std::cin.ignore();
            std::cout << "invalid input" << std::endl;
            validdub = false;
        }
        else{
            ct = user_ct;
            validdub = true;
        }
    }
    
    //std::cout << "Using: " << ct << "\n" << std::endl;
    result = ct;
    return status::SUCCESS;
}

//****************************************************
// ask for true d_std
// takes a reference to a double, sets to specified std
//****************************************************
int ask_for_true_d_std(double &result){
    double std = 0.0;
    double user_std = -1;
    std::string yn;
    //bool validyn = false;
    bool validdub = false;
    
    while (!validdub){
        validdub = true;
        std::cout << std::endl;
        std::cout << "Enter true d_standard (e.g. for Balmat pyrite, enter 15.1)" << std::endl;
        std::cin >> user_std;
        if (std::cin.fail()){
            std::cin.clear();
            std::cin.ignore();
            std::cout << "invalid input" << std::endl;
            validdub = false;
        }
        else if (user_std > 0){
            std = user_std;
            validdub = true;
        }
        else{
            std::cout << "invalid input" << std::endl;
            validdub = false;
        }
    }
    
    std::cout << "Using: " << std << "\n" << std::endl;
    result = std;
    return status::SUCCESS;
}

//****************************************************
// ask for international std R
// takes a reference to a double, sets to specified std
//****************************************************
int ask_for_international_std(double &result){
    double std = 0.0;
    double user_std = -1;
    std::string yn;
    //bool validyn = false;
    bool validdub = false;
    
    while (!validdub){
        validdub = true;
        std::cout << "Enter international standard ratio (e.g. for VCDT or BDP)" << std::endl;
        std::cin >> user_std;
        if (std::cin.fail()){
            std::cin.clear();
            std::cin.ignore();
            std::cout << "invalid input" << std::endl;
            validdub = false;
        }
        else if (user_std > 0){
            std = user_std;
            validdub = true;
        }
        else{
            std::cout << "invalid input" << std::endl;
            validdub = false;
        }
    }
    
    std::cout << "Using: " << std << "\n" << std::endl;
    result = std;
    return status::SUCCESS;
}

//***********************************************************************************************************
// ask user to specify chauvenet's criterion, or use preset value if 2.5 sigma (this is for sample size of 90)
//***********************************************************************************************************
int ask_for_chauvenet(double num_points, double &result){
    //assemble lookup table for chauvenet's criterion, from clive's textbook
    std::vector<std::vector<double>> chauv_lookup;
    chauv_lookup.resize(2, std::vector<double>(24));
    chauv_lookup[0][0] = 5;     chauv_lookup[1][0] = 1.65;
    chauv_lookup[0][1] = 6;		chauv_lookup[1][1] = 1.73;
    chauv_lookup[0][2] = 7;		chauv_lookup[1][2] = 1.81;
    chauv_lookup[0][3] = 8;		chauv_lookup[1][3] = 1.86;
    chauv_lookup[0][4] = 9;		chauv_lookup[1][4] = 1.91;
    chauv_lookup[0][5] = 10;	chauv_lookup[1][5] = 1.96;
    chauv_lookup[0][6] = 12;	chauv_lookup[1][6] = 2.04;
    chauv_lookup[0][7] = 14;	chauv_lookup[1][7] = 2.10;
    chauv_lookup[0][8] = 16;	chauv_lookup[1][8] = 2.15;
    chauv_lookup[0][9] = 18;	chauv_lookup[1][9] = 2.20;
    chauv_lookup[0][10] = 20;	chauv_lookup[1][10] = 2.24;
    chauv_lookup[0][11] = 25;	chauv_lookup[1][11] = 2.33;
    chauv_lookup[0][12] = 30;	chauv_lookup[1][12] = 2.39;
    chauv_lookup[0][13] = 40;	chauv_lookup[1][13] = 2.49;
    chauv_lookup[0][14] = 50;	chauv_lookup[1][14] = 2.57;
    chauv_lookup[0][15] = 60;	chauv_lookup[1][15] = 2.64;
    chauv_lookup[0][16] = 80;	chauv_lookup[1][16] = 2.74;
    chauv_lookup[0][17] = 100;	chauv_lookup[1][17] = 2.81;
    chauv_lookup[0][18] = 150;	chauv_lookup[1][18] = 2.93;
    chauv_lookup[0][19] = 200;	chauv_lookup[1][19] = 3.02;
    chauv_lookup[0][20] = 300;	chauv_lookup[1][20] = 3.14;
    chauv_lookup[0][21] = 400;	chauv_lookup[1][21] = 3.23;
    chauv_lookup[0][22] = 500;	chauv_lookup[1][22] = 3.29;
    chauv_lookup[0][23] = 1000;	chauv_lookup[1][23] = 3.48;
    
    if (num_points < 5){
        std::cout << "ERROR minimum # points = 5" << std::endl;
        return status::ERROR_BAD_NUM_POINTS_CHAUV;
    }
    
    int index;
    for (unsigned int i = 0; i < chauv_lookup[0].size(); i++){
        
        if (chauv_lookup[0][i] > num_points){
            index = i;
            break;
        }
        else{
            index = chauv_lookup.size() - 1;
        }
    }
    double weighted_lower;
    double weighted_upper;
    if (index == 0){
        weighted_lower = 1;
        weighted_upper = 0;
    }
    else{
        weighted_lower = 1.0 - (num_points - chauv_lookup[0][index - 1]) / (chauv_lookup[0][index] - chauv_lookup[0][index - 1]);
        weighted_upper = 1.0 - weighted_lower;
    }
    //calculate chauvenet's criterion using linear approximation
    
    double crit = weighted_lower*chauv_lookup[1][index - 1] + weighted_upper*chauv_lookup[1][index];
    double user_crit = -1;
    std::string yn;
    bool validyn = false;
    bool validdub = false;
    while (!validyn){
        validyn = true;
        std::cout << "From given # data points, calculated Chauvenet's Criterion to be " << crit << " sigma" << std::endl;
        std::cout << "Use this value? (y/n)" << std::endl;
        std::cin >> yn;
        if (std::cin.fail()){
            std::cin.clear();
            std::cin.ignore();
            std::cout << "invalid input" << std::endl;
            validyn = false;
        }
        if (yn.compare("y") == 0 || yn.compare("Y") == 0 || yn.compare("yes") == 0 || yn.compare("Yes") == 0){
            validyn = true;
        }
        else if ((yn.compare("n") == 0 || yn.compare("N") == 0 || yn.compare("no") == 0 || yn.compare("No") == 0)){
            while (!validdub){
                validdub = true;
                std::cout << "enter new criterion" << std::endl;
                std::cin >> user_crit;
                if (std::cin.fail()){
                    std::cin.clear();
                    std::cin.ignore();
                    std::cout << "invalid input" << std::endl;
                    validdub = false;
                }
                else if (user_crit > 0){
                    crit = user_crit;
                    std::cout << "using user criterion of " << user_crit << std::endl;
                    validdub = true;
                }
                else{
                    std::cout << "invalid input" << std::endl;
                    validdub = false;
                }
            }
        }
        else{
            std::cout << " invalid input" << std::endl;
            validyn = false;
        }
    }
    std::cout << "Chauvenet's criterion: " << crit << "\n" << std::endl;
    //user_inputs.push_back(crit);
    result = crit;
    return status::SUCCESS;
}

//******************************************
// Convert raw data from strings to doubles
//******************************************
std::vector<std::vector<double>> string_to_double(std::vector<std::vector<std::string>> parsed_rawData){
    std::vector<std::vector<double>> rawDoubles;
    for (unsigned int i = 0; i < parsed_rawData.size(); i++){
        std::vector<double> temp;
        for (unsigned int j = 0; j < parsed_rawData[0].size(); j = j + 2){ // every other is an exponent
            std::setprecision(50);
            //std::cout <<"i: " << i << " j: " << j << " " << parsed_rawData[i][j] << std::endl;
            double tempVal1 = stod(parsed_rawData[i][j]);
            double tempExpVal1 = stod(parsed_rawData[i][j + 1].substr(1, 1));
            
            if (parsed_rawData[i][j + 1].substr(0, 1).compare("+") == 0){       // see if exp has positive sign
                tempVal1 = tempVal1*pow(10, tempExpVal1);
            }
            else if (parsed_rawData[i][j + 1].substr(0, 1).compare("-") == 0){   // see if exp has negative sign
                tempVal1 = tempVal1*pow(10, -tempExpVal1);
            }
            else{
                std::cout << "error: no positive or negative sign detected for tempVal1" << std::endl;
            }
            
            temp.push_back(tempVal1);
        }
        rawDoubles.push_back(temp);
        temp.clear();
    }
    return rawDoubles;
}

//***********************************************************************************************************************************************************************************
// rejects outliers, then calculates standard dev & standard err and stores them in a global variable finaldata
// OUTPUT: finaldata[0]= mean col2/col1  finaldata[1]=points rejected total  finaldata[2]=std dev  finaldata[3]=std err permil  finaldata[4]=d(col2/col1)  finaldata[5]=mean(column 1)
//***********************************************************************************************************************************************************************************
std::vector<double> correct_data(std::vector<std::vector<double>> rawDoubles, double chauvenet, double standard, std::vector<std::vector<int>> &global_keep_or_reject, double ct){ // pass in parsed raw data
    std::vector<double> finaldata;
    std::vector<int> keep_or_reject;
    // *************************************************************
    // (34S and 32S used as inherent example)
    // INPUT (rawDoubles): first column is "32S"; second column is "34S"
    // OUTPUT (dtCorrected): the first column is "32S"; the second column is "34S/32S"
    // *************************************************************
    std::vector<std::vector<double>> dtCorrected;
    for (unsigned int i = 0; i < rawDoubles.size(); i++){
        std::vector<double> temp;
        double tempVal1 = rawDoubles[i][0];
        double tempVal2 = (rawDoubles[i][1] / rawDoubles[i][0]);  // make second column the ratio
        temp.push_back(tempVal1);
        temp.push_back(tempVal2);
        dtCorrected.push_back(temp); // dtCorrected is 2d vector containing dt corrected data as doubles
    }
    global_corrected.push_back(dtCorrected); // store corrected data in global_corrected, a global 3d vector
    //****************************************************************
    // calculate mean 34S/32S value
    //  more generally, calculate mean of second column of dtCorrected
    //****************************************************************
    double sum = 0.0;
    double mean3432;
    for (unsigned int i = 0; i < dtCorrected.size(); i++){ //$$$$$$$$$$$ figure out if there is a problem on this line
        sum += dtCorrected[i][1];
    }
    mean3432 = sum / dtCorrected.size();
    //************************************************************
    //calculate standard deviation
    //************************************************************
    //calculate the squared difference of each data point from the mean and store in a new vector
    std::vector<double> dist_from_mean;
    double sum_dists = 0;
    double std_dev;
    for (unsigned int i = 0; i < dtCorrected.size(); i++){
        double temp_dist = abs(dtCorrected[i][1] - mean3432);
        dist_from_mean.push_back(temp_dist);
        
        sum_dists += temp_dist*temp_dist;
    }
    sum_dists = sum_dists / (dtCorrected.size() - 1);
    std_dev = sqrt(sum_dists);
    //****************************************************************************
    //first pass of chauvenet's criterion. store good points in "two_sig_rejected"
    //****************************************************************************
    std::vector<double> two_sig_rejected; //make new vector to store 2-sigma-processed values from dtCorrected
    double num_rejected = 0;
    for (unsigned int i = 0; i < dist_from_mean.size(); i++){
        double deviation = dist_from_mean[i];
        if (deviation >(chauvenet * std_dev)){
            keep_or_reject.push_back(0);
            two_sig_rejected.push_back(0.0);
            num_rejected++;
        }
        else{
            two_sig_rejected.push_back(dtCorrected[i][1]);
            keep_or_reject.push_back(1);
        }
    }
    std::cout << "# Points rejected by Chauvenet Pass 1: " << num_rejected << std::endl;
    //***********
    //update mean
    //***********
    double sum1 = 0.0;
    double mean1;
    for (unsigned int i = 0; i < two_sig_rejected.size(); i++){
        sum1 += two_sig_rejected[i];
    }
    mean1 = sum1 / (two_sig_rejected.size() - num_rejected);
    //*******************************
    //create new deviations from mean
    //*******************************
    std::vector<double> new_devs;      // dist from mean
    std::vector<double> new_devs_sqrd;
    double sum_sqrd_devs = 0;
    for (unsigned int i = 0; i < two_sig_rejected.size(); i++){
        if (two_sig_rejected[i] == 0.0){
            new_devs.push_back(0.0);
            new_devs_sqrd.push_back(0.0);
        }
        else{
            double dev1 = abs(two_sig_rejected[i] - mean1);
            new_devs.push_back(dev1);
            new_devs_sqrd.push_back(dev1*dev1);
        }
        sum_sqrd_devs += new_devs_sqrd[i];
    }
    //*****************
    //recalculate stdev
    //*****************
    double new_st_dev = sqrt(sum_sqrd_devs / (new_devs_sqrd.size() - num_rejected - 1));    // new stdev mathches excel
    
    //*************************************************************************************
    //Chauvenet Pass #2; rejected any additional outliers, store good points in dub_two_sig
    //*************************************************************************************
    std::vector<double> dub_two_sig; // new vector for second round of values that pass chauvenet's inspection
    double additional_rejected = 0;
    //std::cout << "Applying chauvenet's criterion of " << chauvenet << std::endl;
    std::cout << std::endl;
    for (unsigned int i = 0; i < two_sig_rejected.size(); i++){
        double deviation1 = new_devs[i];
        //if it's already rejected by first pass
        if (two_sig_rejected[i] == 0.0){
            dub_two_sig.push_back(0);
        }
        //if it's rejected by 2nd chauvenet pass
        else if (deviation1 > chauvenet * new_st_dev){
            dub_two_sig.push_back(0);
            additional_rejected++;
            keep_or_reject[i] = 0;
        }
        else{
            dub_two_sig.push_back(two_sig_rejected[i]);
        }
    }
    std::cout << "# Points rejected by Chauvenet Pass 2: " << additional_rejected << std::endl;
    std::cout << "# Points rejected total: " << num_rejected + additional_rejected << std::endl;
    //************************************************
    //after chauvenet, correct for Ct with dub_two_sig
    //************************************************
    for (unsigned int i = 0; i < dub_two_sig.size(); i++){
        dub_two_sig[i] = dub_two_sig[i] * ct;
    }
    //************************************************************
    //calculate final sum and mean 34S/32S (2nd Column/1st Column)
    //************************************************************
    double final_sum = 0;
    double final_mean;
    for (unsigned int i = 0; i < dub_two_sig.size(); i++){
        final_sum += dub_two_sig[i];
    }
    final_mean = final_sum / (dub_two_sig.size() - num_rejected - additional_rejected);
    //std::cout << "Final mean ratio of second column/first column: " << final_mean << std::endl;
    finaldata.push_back(final_mean);  // access at finaldata[0]
    //************************************************
    //calculate final deviations, stdev, and std error
    //************************************************
    std::vector<double> final_devs;
    std::vector<double> final_devs_sqrd;
    double final_sum_sqrd_devs = 0;
    double final_std_dev;
    double std_err;
    for (unsigned int i = 0; i < dub_two_sig.size(); i++){
        if (dub_two_sig[i] == 0.0){
            final_devs.push_back(0.0);
            final_devs_sqrd.push_back(0.0);
        }
        else{
            double dev2 = abs(dub_two_sig[i] - mean1);
            final_devs.push_back(dev2);
            final_devs_sqrd.push_back(dev2*dev2);
            final_sum_sqrd_devs += dev2*dev2;
        }
    }
    
    final_std_dev = sqrt(final_sum_sqrd_devs / (dub_two_sig.size() - num_rejected - additional_rejected - 1));
    //std::cout << setprecision(10) << "final stdev is " << final_std_dev << std::endl;
    std_err = final_std_dev * 1000 / (final_mean*sqrt(dub_two_sig.size() - num_rejected - additional_rejected));
    //std::cout << setprecision(10) << "std err is " << std_err << " permil" << std::endl;
    
    finaldata.push_back(num_rejected + additional_rejected); // access at finaldata[1]
    finaldata.push_back(final_std_dev);			// access at finaldata[2]
    finaldata.push_back(std_err);				// access at finaldata[3]
    
    //*********************************************
    //calculate d34S/32S (2nd col/1st col) (permil)
    //*********************************************
    double daily_std = standard;
    double d34S_32S = ((final_mean - daily_std) / daily_std) * 1000;
    finaldata.push_back(d34S_32S);				// access at finaldata[4]
    
    //*******************************
    // calculate mean of first column
    //*******************************
    double sum_first_col = 0.0;
    double mean_first_col;
    double good_points = 0;
    
    for (unsigned int i = 0; i < rawDoubles.size(); i++){
        sum_first_col += rawDoubles[i][0];
    }
    mean_first_col = sum_first_col / rawDoubles.size();
    global_keep_or_reject.push_back(keep_or_reject);
    
    finaldata.push_back(mean_first_col);		// access at finaldata[5]
    finaldata.push_back(additional_rejected);	// access at finaldata[6]
    return finaldata;
}

//*****************************************************
// calculate slope and R^2 using least-square algorithm
//*****************************************************
std::vector<double> linear_regression(std::vector<std::vector<double>> final_data, std::vector<int> keep_reject, double raw_data_size, double rejects){
    double r_squ = 0;
    double slope = 0;
    //std::cout << "linear regression rejected points: " << rejects << std::endl;
    //std::cout << "linear regression raw_data points: " << raw_data_size << std::endl;
    
    double sumX = 0;  //sum x vals
    double sumY = 0;  //sum y vals
    double sumXY = 0; //sum x*y valsf.r_squs
    double sumXX = 0; //sum x*x vals
    double sum_res = 0; // sum of squared residuals
    double avgX = 0;
    double avgY = 0;
    double y_int;
    
    double sum_squares_tot = 0;
    double sum_squares_res = 0;
    
    // calculate sums
    for (unsigned int i = 0; i < final_data.size(); ++i){
        if (keep_reject[i] == 1){
            sumX += i;
            sumY += final_data[i][0];
            sumXY += i * final_data[i][0];
            sumXX += i*i;
        }
    }
    avgX = sumX / (raw_data_size - rejects);
    
    avgY = sumY / (raw_data_size - rejects);
    
    //cout << "slope just before calculation: " << slope << std::endl;
    slope = ((raw_data_size - rejects)*sumXY - sumX*sumY) / ((raw_data_size - rejects)*sumXX - sumX*sumX);
    //cout << "slope just after calculation: " << slope << std::endl;
    y_int = avgY - slope*avgX;
    
    for (unsigned int i = 0; i < final_data.size(); ++i){
        if (keep_reject[i] == 1){
            sum_squares_tot += (final_data[i][0] - avgY)*(final_data[i][0] - avgY);
            sum_squares_res += (final_data[i][0] - (slope*i + y_int))*(final_data[i][0] - (slope*i + y_int)); // calculate using slope
        }
    }
    //cout << "r_squ just before calculation: " << r_squ << std::endl;
    r_squ = 1 - (sum_squares_res / sum_squares_tot);
    //cout << "r_squ just after calculation: " << r_squ << std::endl;
    
    std::vector <double> result;
    result.push_back(slope);
    result.push_back(r_squ);
    return result;
}

//*********************************************************************************************
// returns current time as a string in YYYY-MM-DD_hh-mm format (year, month, day, hour, minute)
//*********************************************************************************************
std::string get_time(){
    time_t t = time(0);   // get time now
    struct tm * now = localtime(&t);
    
    //add 0 in front of minutes if necessary
    std::string minute;
    if (now->tm_min < 10){
        minute = "0" + std::to_string(now->tm_min);
    }
    else{
        minute = std::to_string(now->tm_min);
    }
    //add 0 in front of hours if necessary
    std::string hour;
    if (now->tm_hour < 10){
        hour = "0" + std::to_string(now->tm_hour);
    }
    else{
        hour = std::to_string(now->tm_hour);
    }
    
    std::string full_time = std::to_string(now->tm_year + 1900) + '-'
    + std::to_string(now->tm_mon + 1) + '-'
    + std::to_string(now->tm_mday) + '_'
    + hour + '-'
    + minute;
    std::cout << "full_time: " << full_time << std::endl;
    return full_time;
}

//********************************************************
// write data to text file to be saved in program directory
//********************************************************
void write_data(revised_file_data_to_write f, std::string input_file_name, revised_global_user_inputs rgui){
    
    //adapted from http://www.cplusplus.com/forum/general/58563/
    // first ensure that a sub-folder has been created
    WCHAR* folder = L".\\outputs";
    WCHAR* file = L"File.ext";
    
    std::wstring path = folder;
    path += L"\\";
    path += file;
    
    std::ofstream fout(path);
    
    if (fout.fail()){ // if fout failed to open the file, then the folder probably isn't there
        CreateDirectory(folder, NULL); // From windows.h, creates a folder
        fout.open(path);
    }
    
    //std::ofstream outPut("C:\\Users\\Lab User\\Documents\\SIMS DATA\\COLIN KEATING\\data_extraction_outputs\\" + f.output_filename + "_results.txt");
    //std::ofstream outPut(gui.output_location + "" + f.output_filename + "_results.txt");
    std::ofstream outPut(".\\outputs\\" + f.output_filename + "_results.txt");
    //std::cout << "writing summary file to the following location:\n  C:\\Users\\Lab User\\Documents\\SIMS DATA\\COLIN KEATING\\data_extraction_outputs\\" + f.output_filename + "_results.txt \n" << std::endl;
    //std::cout << "writing summary file to the following location:\n" + gui.output_location + f.output_filename + "_results.txt \n" << std::endl;
    std::cout << std::endl;
    std::cout << "Writing summary file to the following location:\n" + std::string(".\\outputs\\") + f.output_filename + "_results.txt \n" << std::endl;
    
    outPut << "Input file name: " << input_file_name << std::endl;
    outPut << "Data Summary" << "\n" << std::endl;
    std::vector< std::string> Rnums = { "R0: ", "R1: ", "R2: ", "R3: ", "R4: ", "R5: ", "R6: ", "R7: " };
    for (unsigned int i = 0; i < f.stat_results_Ri.size(); i++){
        outPut << Rnums[i] << f.stat_results_Ri[i] << std::endl;
    }
    outPut << std::endl;
    outPut << "  Data points/column_______________" << rgui.points_per_column << std::endl;
    outPut << "  Field App X______________________" << f.field_app_xy[0] << std::endl;
    outPut << "  Field App Y______________________" << f.field_app_xy[1] << std::endl;
    outPut << "  Entrance Slit X__________________" << f.field_app_xy[2] << std::endl;
    outPut << "  X POSITION_______________________" << f.xypos[0] << std::endl;
    outPut << "  Y POSITION_______________________" << f.xypos[1] << std::endl;
    outPut << std::endl;
    //std::cout << "f.finaldata.size(): " << f.finaldata.size() << std::endl;
    for (unsigned int z = 0; z < f.finaldata.size(); z++)
    {
        outPut << "Summary for comparison " << z << " :" << std::endl;
        
        outPut << "  Standard_________________________________" << rgui.standard[z] << std::endl;
        outPut << "  Chauvenet's criterion____________________" << rgui.chauvenet << std::endl;
        outPut << std::endl;
        
        outPut << "  Mean for column " << rgui.columns_to_compare[z * 2] << "________________________" << f.finaldata[z][5] << " (Including rejected datapoints)" << std::endl;
        outPut << "  Mean ratio of column " << rgui.columns_to_compare[2 * z + 1] << "/column " << rgui.columns_to_compare[2 * z] << "__________" << std::to_string(f.finaldata[z][0]) << " (Discounting rejected datapoints)" << std::endl;
        outPut << "  Points Rejected in total_________________" << std::to_string(f.finaldata[z][1]) << std::endl;
        outPut << "  Points Rejected in 2nd Chauvenet Test____" << std::to_string(f.finaldata[z][6]) << std::endl;
        outPut << std::setprecision(10) << "  STD DEV__________________________________" << std::to_string(f.finaldata[z][2]) << " (Discounting rejected datapoints)" << std::endl;
        outPut << "  STD ERR permil___________________________" << std::to_string(f.finaldata[z][3]) << " (Discounting rejected datapoints)" << std::endl;
        
        outPut << "  d(column " << rgui.columns_to_compare[2 * z + 1] << "/column " << rgui.columns_to_compare[2 * z] << "):____________________" << std::to_string(f.finaldata[z][4]) << " (Discounting rejected datapoints)" << std::endl;
        outPut << "  Slope____________________________________" << f.slopes[z] << std::endl;
        outPut << "  R Squared________________________________" << f.r_squs[z] << std::endl;
        outPut << std::endl;
    }
    outPut << std::endl;
    outPut << "see various CSV files for raw data and corrected data" << std::endl;
    //outPut << "'" << f.output_filename << "_all_raw_data.csv' holds all original data points for every column in the original Cameca file" << std::endl;
    //outPut << "'" << f.output_filename << "_comparison_raw_data.csv' holds the data columns corresponding to each user-specified comparison" << std::endl;
}

//*****************************************************************
// write standard data to text file to be saved in program directory
//*****************************************************************
void write_standard_data(revised_file_data_to_write f, std::string input_file_name, revised_global_user_inputs rgui){
    
    //adapted from http://www.cplusplus.com/forum/general/58563/
    // first ensure that a sub-folder has been created
    WCHAR* folder = L".\\outputs";
    WCHAR* file = L"File.ext";
    std::wstring path = folder;
    path += L"\\";
    path += file;
    std::ofstream fout(path);
    
    if (fout.fail()){ // if fout failed to open the file, then the folder probably isn't there
        CreateDirectory(folder, NULL); // From windows.h, creates a folder
        fout.open(path);
    }
    std::ofstream outPut(".\\outputs\\" + f.output_filename + "_standard_results.txt");
    std::cout << std::endl;
    std::cout << "Writing summary file to the following location:\n" + std::string(".\\outputs\\") + f.output_filename + "_standard_results.txt \n" << std::endl;
    
    outPut << "Input file name: " << input_file_name << std::endl;
    outPut << "Data Summary" << "\n" << std::endl;
    
    outPut << std::endl;
    outPut << "  Data points/column__" << rgui.points_per_column << std::endl;
    outPut << std::endl;
    //std::cout << "f.finaldata.size(): " << f.finaldata.size() << std::endl;
    for (unsigned int z = 0; z < f.finaldata.size(); z++)
    {
        outPut << "Summary for comparison " << z << " :" << std::endl;
        outPut << std::endl;
        outPut << "  R(std measured) of column " << rgui.columns_to_compare[2 * z + 1] << "/column " << rgui.columns_to_compare[2 * z] << "_____" << std::to_string(f.r_std_measured[z]) << " (Discounting rejected datapoints)" << std::endl;
        outPut << "  R(std true)______________________________" << std::to_string(f.r_std_true[z]) << std::endl;
        outPut << "  Ct_______________________________________" << std::to_string(f.c_t[z]) << std::endl;
        outPut << std::endl;
        outPut << "  Chauvenet's criterion____________________" << rgui.chauvenet << std::endl;
        outPut << "  Points Rejected in total_________________" << std::to_string(f.finaldata[z][1]) << std::endl;
        outPut << "  Points Rejected in 2nd Chauvenet Test____" << std::to_string(f.finaldata[z][6]) << std::endl;
        outPut << std::endl;
    }
}

//**********************************************
// write raw data used in comparisons to CSV file #####need to fix global_keep_or_reject issue
//**********************************************
void write_to_csv(revised_file_data_to_write f, std::vector<std::vector<std::vector<double>>> rawDoubles, std::vector<std::vector<int>> binary_rejected, revised_global_user_inputs rgui, int file_number){
    
    std::ofstream outPut(".\\outputs\\" + f.output_filename + "_comparison_raw_data.csv");
    std::cout << "Writing comparison raw data to the following location:\n" + std::string(".\\outputs\\") + f.output_filename + "_comparison_raw_data.csv \n" << std::endl;
    
    std::vector<std::vector<double>> allVals;
    for (unsigned int z = 0; z < rawDoubles.size(); z++)
    {
        for (unsigned int i = 0; i < rawDoubles[z].size(); i++){
            std::vector<double> temp;
            for (unsigned int j = 0; j < rawDoubles[z][0].size(); j++){
                temp.push_back(rawDoubles[z][i][j]);
            }
            temp.push_back(binary_rejected[z][i]); // additional column indicating 1 for keeper or 0 for rejected point
            allVals.push_back(temp);
            temp.clear();
        }
    }
    for (unsigned int i = 0; i < allVals.size(); i++){
        for (unsigned int j = 0; j < allVals[0].size(); j++){
            outPut << std::setprecision(20) << allVals[i][j] << " , ";
        }
        outPut << ", \n";
    }
    outPut.close();
}

//**************************
// write all raw data to CSV
//**************************
void write_all_raw_to_csv(revised_file_data_to_write f, std::vector<std::vector<std::string>> all_raw_strings, revised_global_user_inputs rgui){
    //std::ofstream outPut("C:\\Users\\Lab User\\Documents\\SIMS DATA\\COLIN KEATING\\data_extraction_outputs\\" + f.output_filename + "_all_raw_data.csv");
    //std::ofstream outPut(gui.output_location + "" + f.output_filename + "_all_raw_data.csv");
    std::ofstream outPut(".\\outputs\\" + f.output_filename + "_all_raw_data.csv");
    //std::cout << "gui.output_location" << rgui.output_location << std::endl;
    //std::cout << "writing all raw data to the following location:\n  C:\\Users\\Lab User\\Documents\\SIMS DATA\\COLIN KEATING\\data_extraction_outputs\\" + f.output_filename + "_all_raw_data.csv \n" << std::endl;
    //std::cout << gui.output_location + f.output_filename + "_all_raw_data.csv \n" << std::endl;
    std::cout << "Writing all raw data to the following location:\n" + std::string(".\\outputs\\") + f.output_filename + "_all_raw_data.csv \n" << std::endl;
    std::vector<std::vector<double>> all_raw_vals = string_to_double(all_raw_strings);
    for (unsigned int i = 0; i < all_raw_vals.size(); i++){
        for (unsigned int j = 0; j < all_raw_vals[0].size(); j++){
            outPut << std::setprecision(20) << all_raw_vals[i][j] << " , ";
        }
        outPut << ", \n";
    }
    outPut.close();
}

//***********************************************
// write raw data used in comparisons to CSV file
// also writes all data into correct format for outputting all files in single CSV via global_all_vals
//***********************************************
void new_write_csv(revised_file_data_to_write f, std::vector<std::vector<std::vector<double>>> rawDoubles, std::vector<std::vector<int>> binary_rejected, revised_global_user_inputs rgui, int file_number){
    std::ofstream outPut(".\\outputs\\" + f.output_filename + "_new_data.csv");
    std::cout << "Writing all corrected comparison data to the following location:\n" + std::string(".\\outputs\\") + f.output_filename + "_new_data.csv\n" << std::endl;
    
    std::vector<std::vector<double>> allVals;
    std::vector<std::vector<int>> binary_rejected_updated;
    int start_index = (file_number * rgui.columns_to_compare.size() / 2);
    //put correct set of binary rejected (1s and 0s) data in the vector to be written
    for (unsigned int i = 0; i < binary_rejected[0].size(); i++){
        std::vector<int> temp;
        for (unsigned int j = start_index; j < (start_index + rgui.columns_to_compare.size() / 2); j++){
            temp.push_back(binary_rejected[j][i]);
        }
        binary_rejected_updated.push_back(temp);
        temp.clear();
    }
    std::cout << "line 2210" << std::endl;
    std::cout << "global_corrected_sampling_times.size(): " << global_corrected_sampling_times.size() << std::endl;
    std::cout << "global_corrected_sampling_times[0].size(): " << global_corrected_sampling_times[0].size() << std::endl;
    std::cout << "global_corrected_sampling_times[0][0].size(): " << global_corrected_sampling_times[0][0].size() << std::endl;
    for (unsigned int i = 0; i < rawDoubles[0].size(); i++){ // i=cycle number
        //std::cout << "i: " << i << std::endl;
        std::vector<double> temp;
        for (unsigned int z = 0; z < rawDoubles.size(); z++){
            //std::cout << "z: " << z << std::endl;
            
            //check if point was rejected by chauvenet; if not, put the two isotopes and their ratio into the vector "temp"
            if (binary_rejected_updated[i][z] != 0){
                temp.push_back(i);
                //check for if global corrected_times vector has data; if so, include corrected times
                if (global_corrected_sampling_times[0][0].size() > 0){
                    temp.push_back(global_corrected_sampling_times[file_number][z][i]);
                    //std::cout << "last entry of temp: " << temp[temp.size()-1] << std::endl;
                }
                
                std::vector<double> ratio_parts;                                     //12/8 edit
                for (unsigned int j = 0; j < rawDoubles[z][0].size(); j++){
                    temp.push_back(rawDoubles[z][i][j]);
                    ratio_parts.push_back(rawDoubles[z][i][j]);				    //12/8 edit
                }
                temp.push_back(ratio_parts[1] / ratio_parts[0]);				//12/8 edit
                ratio_parts.clear();											//12/8 edit
            }
            else{ //otherwise put 0s into "temp"
                
                temp.push_back(0);
                if (global_corrected_sampling_times[0][0].size() > 0){
                    temp.push_back(0);
                }
                for (unsigned int j = 0; j < rawDoubles[z][0].size(); j++){
                    temp.push_back(0);
                }
                temp.push_back(0);                                              //12/8 edit
            }
        }
        allVals.push_back(temp);
        temp.clear();
    }
    //
    std::vector<double> temp_headers; // for appending to allVals_modified
    std::vector<double> temp_x_y;
    // add headers
    for (unsigned int i = 0; i < rgui.column_headers.size() / 2; i++){
        //outPut << "cycle , ";
        outPut << "x: " << f.xypos[0] << " y: " << f.xypos[1] << ",";
        outPut << "Time , ";
        outPut << rgui.column_headers[2 * i] << " , ";
        outPut << rgui.column_headers[2 * i + 1] << " , ";
        outPut << rgui.column_headers[2 * i + 1] << "/" << rgui.column_headers[2 * i] << " , ";
        //global_write_data.push_back("cycle");
        
        global_write_data.push_back("x: " + f.xypos[0] + " y: " + f.xypos[1]);
        global_write_data.push_back(rgui.column_headers[2 * i]);
        global_write_data.push_back(rgui.column_headers[2 * i + 1]);
        temp_headers.push_back(0);
        temp_headers.push_back(0);
        temp_headers.push_back(stod(rgui.column_headers[2 * i]));
        temp_headers.push_back(stod(rgui.column_headers[2 * i + 1]));
        temp_headers.push_back(stod(rgui.column_headers[2 * i + 1]) * 100 + stod(rgui.column_headers[2 * i]));
        
        temp_x_y.push_back(0);
        temp_x_y.push_back(0);
        //check if x position is found; if not, then insert null value (negative infinity)
        if (f.xypos[0].compare("xpos not found")==0){
            temp_x_y.push_back(-DBL_MAX);
        }else{
            temp_x_y.push_back(stod(f.xypos[0]));
        }
        
        //check if y position is found; if not, then insert null value (negative infinity)
        if (f.xypos[1].compare("ypos not found") == 0){
            temp_x_y.push_back(-DBL_MAX);
        }else{
            temp_x_y.push_back(stod(f.xypos[1]));
        }
        
        temp_x_y.push_back(0);						// 12/8 modification, placeholder for header for comparison 
    }
    //need to have headers at front of vector, so create new vector, add headers, then loop through allVals to add it behind the headers 
    std::vector<std::vector<double>> temp_allvals;
    //std::cout << "length tempxy: " << temp_x_y.size() << std::endl;
    //std::cout << "length temp headers: " << temp_headers.size() << std::endl;
    temp_allvals.push_back(temp_x_y);
    temp_allvals.push_back(temp_headers);
    for (unsigned int i = 0; i < allVals.size(); i++)
    {
        temp_allvals.push_back(allVals[i]);
    }
    global_all_vals.push_back(temp_allvals);
    
    outPut << ", \n";
    global_write_data.push_back("\n");
    // add data
    for (unsigned int i = 0; i < allVals.size(); i++){
        for (unsigned int j = 0; j < allVals[0].size(); j++){
            outPut << std::setprecision(10) << allVals[i][j] << " , ";
            global_write_data.push_back(std::to_string(allVals[i][j]));
        }
        outPut << ", \n";
        global_write_data.push_back("\n");
    }
    outPut.close();
}

//**********************************************
//write raw data used in comparisons to CSV file
//**********************************************
void write_all_runs_to_csv(){
    std::string current_time = get_time();
    std::ofstream outPut(".\\outputs\\all_runs_" + current_time + ".csv");
    std::cout << "Writing all corrected comparison data from all files to the following location:\n" + std::string(".\\outputs\\") + "all_runs_" + current_time + ".csv \n" << std::endl;
    
    //for (unsigned int i = 0; i < global_write_data.size(); i++){
    //	outPut << global_write_data[i] << std::endl;
    //}
    //http://stackoverflow.com/questions/1430757/c-vector-to-string
    std::stringstream ss;
    for (size_t i = 0; i < global_write_data.size(); ++i)
    {
        if (global_write_data[i].compare("\n") == 0){
            ss << std::setprecision(10) << global_write_data[i];
        }
        else{
            ss << std::setprecision(10) << global_write_data[i];
            ss << ",";
        }
    }
    std::string temp = ss.str();
    outPut << temp;
    //std::copy(global_write_data.begin(), global_write_data.end() - 1, outPut);
    outPut.close();
}

//*********************************************************************************************************************************************************************************************
// write raw data used in comparisons to CSV file.  Adds headers; top is for x/y position, bottom header is for "34s" (but will only output "34" because it's stored in a vector<double> format)
// Comparisons are positioned next to each other rather than stacked
//*********************************************************************************************************************************************************************************************
void write_all_runs_to_csv_updated(){
    std::string current_time = get_time();
    std::ofstream outPut(".\\outputs\\all_runs_updated_" + current_time + ".csv");
    std::cout << "Writing all corrected comparison data from all files to the following location:\n" + std::string(".\\outputs\\") + "all_runs_updated_" + current_time + ".csv \n" << std::endl;
    
    //for (unsigned int i = 0; i < global_write_data.size(); i++){
    //	outPut << global_write_data[i] << std::endl;
    //}
    //http://stackoverflow.com/questions/1430757/c-vector-to-string
    std::stringstream ss;
    int num_files = global_all_vals.size();
    //std::cout << "global allvals numfiles " << num_files << std::endl;
    int length_x = global_all_vals[0].size();
    //std::cout << "global allvals length x: " << length_x << std::endl;
    int length_y = global_all_vals[0][0].size();
    //std::cout << "global allvals length y: " << length_y << std::endl;
    for (int i = 0; i < length_x; i++)
    {
        for (int h = 0; h < num_files; ++h)
        {
            for (int j = 0; j < length_y; j++)
            {
                ss << std::setprecision(10) << global_all_vals[h][i][j];
                ss << ",";
                //cout << global_all_vals[h][i][j] << std::endl;
            }
        }
        ss << "\n";
    }
    
    std::string temp = ss.str();
    outPut << temp;
    //std::copy(global_write_data.begin(), global_write_data.end() - 1, outPut);
    outPut.close();
}

//***************************************
// write all standard CT/Rstd vals to csv
//***************************************
void write_CT_Rstd_to_csv(revised_global_user_inputs rgui, std::vector<double> ct_vals, std::vector<double> Rstd_vals){
    std::ofstream outPut(".\\outputs\\Standard_CT_Rstd.csv");
    std::cout << "Writing all raw data to the following location:\n" + std::string(".\\outputs\\") + "Standard_CT_Rstd.csv \n" << std::endl;
    
    outPut << rgui.standard_name << std::endl;
    double mean_ct = 0;
    if (ct_vals.size() == Rstd_vals.size()){
        for (size_t i = 0; i < ct_vals.size(); i++){
            outPut << "Ct " << i << "," << ct_vals[i] << std::endl;
            mean_ct += ct_vals[i];
            outPut << "Rstd " << i << "," << Rstd_vals[i] << std::endl;
        }
    }
    else{
        outPut << "error incorrect number of Rstd and Ct values" << std::endl;
    }
    outPut << "Mean Ct,";
    mean_ct = mean_ct / ct_vals.size();
    outPut << mean_ct << std::endl;
    outPut.close();
}
