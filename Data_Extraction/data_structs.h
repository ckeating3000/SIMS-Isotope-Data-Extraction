#ifndef DATA_STRUCTS_H
#define DATA_STRUCTS_H

#include <string>
#include <vector>

struct revised_file_data_to_write{
    std::vector<std::string> stat_results_Ri;
    std::vector<std::vector<std::string>> rawData;
    std::vector<std::vector<double>> finaldata;
    std::vector<std::string> xypos;
    std::vector<std::string> detector_types;
    std::vector<std::vector<std::string>> detector_values;
    std::vector<std::vector<std::string>> beam_times;
    std::vector<std::string> field_app_xy;
    std::vector<std::vector<int>> keep_or_reject;
    std::vector<std::vector<std::vector<double>>> final_corrected;
    std::string output_filename;
    std::vector<double> slopes;
    std::vector<double> r_squs;
    std::vector<double> c_t;
    std::vector<double> r_std_measured;
    std::vector<double> r_std_true;
};

struct revised_global_user_inputs{
    std::vector<int> columns_to_compare;
    std::vector<std::string> column_headers;
    int points_per_column;
    int number_of_columns;
    std::vector<double> standard;
    std::vector<double> ct;
    std::vector<double> true_std;
    std::vector<double> international_std;
    double chauvenet;
    double primary_beam_current; // units of picoamps // check main method to make sure this doesnt mess up stuff there
    double primary_beam_flux;
    std::string output_location;
    bool apply_sampling_correction;
    bool apply_qsa_dt_fc;
    std::string standard_name; // for standard path only
    
    revised_global_user_inputs() : columns_to_compare(0), column_headers(0), points_per_column(0), number_of_columns(0),
    standard(0), ct(0), true_std(0), international_std(0), chauvenet(0), primary_beam_current(0), primary_beam_flux(0){}
};

#endif
