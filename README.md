# SIMS-Isotope-Data-ExtractionREADME

DataExtractionMay2017.exe

author: Colin Keating

last modified: 5/8/17

OBJECTIVE: 	Intended for use with the Cameca SIMS 7f-GEO mass spectrometer belonging to Washington 
		University in St. Louis.  Extracts raw data from Cameca output file and applies statistical
		corrections.  Saves all corrected data to CSV file for manipulation in Excel or MATLAB and saves
		parameters including delta and standard deviation (in parts per thousand, or permil) to a text file.

USAGE:		
		Regular (non-isotopic-standard) path:
		Inputting of files: 
			All files inside the "inputs" folder 
			C:\Users\Lab User\Documents\Visual Studio 2013\Projects\DataExtractionApril2017\Debug\inputs\
			will be parsed automatically.
			NOTE: do not put anything besides the .asc versions of the Cameca output files in this folder
		
		Outputting of files:
			All files have an automatic output beginning with their corresponding input name.  All output 
			files are automatically saved in the "outputs" folder at
			C:\Users\Lab User\Documents\Visual Studio 2013\Projects\DataExtractionApril2017\Debug\outputs\

		Isotopic Standard path:
			Same, except you must place all input files in the folder called "inputs_standards"
			Outputs are still placed in "outputs" folder

COMMAND LINE PROMPTS:

		"Are you correcting a standard? (y/n)"
			Enter "y" if you wish to correct data from an output file or files containing non-sample 
			isotopic standard.   			

		"Enter number of data columns in files"
			Enter an integer value corresponding to the number of data columns (isotopes analyzed).  E.g. 
			enter "2" for a file containing 32S and 34S.
			
			Note: Entering an integer n that is smaller than the total number of data columns will result in the 
			analysis of only the first n data columns.)

		"Enter number of cycles"
			Enter an integer value (e.g. 45) corresponding to the total number of cycles per column 
			in each run.

		"From given # data points, calculated Chauvenet's Criterion to be ___ sigma
		Use this value? (y/n)"
			Enter "y" to use the suggested criterion or "n" to enter your own manually.
			
			Note: Chauvenet's Criterion determines which outlier data points to reject.  A value 
			of 2.5 would reject any data points >2.5 standard deviations from the mean.  In the 
			program, two rounds of rejections are performed. The first pass is intended to discard
			any egregious outliers that grossly skew the mean and standard deviations.  The second 
			pass recalculates the mean and standard deviation on remaining points and rejects any new
			outliers.
			See CALCULATIONS section for full description of Chauvenet's criterion.

		"Enter primary beam current in picoamps"
			Generally this value will be between 0-5000 picoamps.
			Note that this value is not necessarily the same as the beam current value found in the 
			output file. Consult Clive if you're unsure of this value...
		
		"How many sets of comparisons/ratios would you like to make in each file?"
			Enter an integer value corresponding to the number of different ratios you wish 
			to make. For example, enter "2" to compare both 34S/32S as well as 33S/32S

		"Enter the first of two columns to compare (the lighter isotope)"
			Refer to your output file to determine which column to enter first.
			Note: this program recognizes the first column in your Cameca file as "0," the second as 
			"1," and so forth. 

		"Enter the corresponding isotope for that column"
			This entry will be used as the column header in the ouput file titled "...new_data.csv" 
		
		"Enter daily standard ratio (Rstd) as decimal (e.g. .0434877) for this comparison"
			Enter the standard ratio as a decminal for the corresponding comparison (the two columns just 
			previously entered) to be used for delta calculations.

		"Enter Ct correction factor (Rstd true / Rstd measured) as decimal for this comparison"
			Enter the applicable correction factor as a decimal or enter "1" to apply no correction 
			
					
TECHNICAL: 	
		
		main.h holds the main function declaractions and enumerations for the SIMS Data Extraction and 
		Correction program
		and extracts the relevant data: 
			the X and Y stage coordinates of the sample analysis site,
			the Isotopic Ratio table data,
			the Beam Centering Results--resultX and resultY from the 
				Field App (DT1), resultX from the Entrace Slits, and the 
				Contrast Apperture resultY, and
			the raw data in a matrix format (# of cycles x # size of set of
				aquisition parameters)
		then corrects the raw data based on the Ri values and formulas, and 
		then writes all data to two matrices stored in a csv file. The csv file
		is meant to be used and analyzed in MatLab. The first matrix will include
		only header information and will be in a vector format. The second matrix
		will contain only the raw data with no headers.

CALCULATIONS:

		Primary beam flux = primary beam current (in picoamps) * 6.2415*10^6
		
		Chauvenet's Criterion = suggested criterion is a based on a linear approximation from given number 
		of cycles compared to values in this lookup table:
			
				#cycles				criterion
			chauv_lookup[0][0] = 5;     	chauv_lookup[1][0] = 1.65;
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
		
			//code snippet:
			double weighted_lower = 1.0 - (num_points - chauv_lookup[0][index - 1]) / (chauv_lookup[0][index] - chauv_lookup[0][index - 1]);
			double weighted_upper = 1.0 - weighted_lower;
			double crit = weighted_lower*chauv_lookup[1][index - 1] + weighted_upper*chauv_lookup[1][index];

		Corrections:
			deadtime (EM detector only):     I(cor) = I(raw)/(1-I(raw)*DT)  *  1/yield
			QSA (EM only): 			 I(cor) = (Iraw^-1 - B/J) ^ -1    B=.66 , J=primary beam flux = primary beam current*6.2415*e6 
			FC (fc1 and fc2 detectors only): I(cor) = (I(raw)-background)/yield
				

		Standard path:
			delta = ( (r_measured/r_vcdt) -1 ) * 1000  where R_vcdt = R_international_std
			
			R std true = (delta_std/1000+1)*R_international_std
			Ct = Rstd true / mean Rstd_measured
			
			
