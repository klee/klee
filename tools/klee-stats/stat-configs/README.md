#KLEE-Stats Config Files

Note: The values in the following sections are expected to be lists seperated with ', ': Groups and Calculations

Key Value pairs in the config file are stored as 

```
key: value 
```

and appear in each of the headers/sections of the config file.

##Info
This header is used by the klee-stats script to store version information. The version option specifies which output version of klee is supported. Klee output version numbers will change as the format of the outputted values changes. Therefore a new config file is needed for each new outout version.

##Input
This is where the format of the input data is defined. In this section there must be a defintion for Record. In the default config, the definitino for Record is as follows
```
Record: I, BFull, BPart, BTot, T, St, Mem, QTot, QCon,\
        _, Treal, SCov, SUnc, _, Ts, Tcex, Tf, Tr
```

Which is meant to match the pattern of the CSVs in the run.stats file. This is also where the variables used in the Calculations section are defined.

##Groups
This section can be used to define a group of variables which can be used in the calculation section. In the defualt config file there is a group definition as follows:
```
time: T, Ts, Tcex, Tf, Tr
```

This group in the calculation section can be used to apply a mapping onto the values in the group For example in the default config there is the following line:

```
 rel-time = 100 * time / Treal
```

This expands into **rel_T = 100 * T / Treal, rel_Ts = 100 * Ts / Treal**, and so on. Note that this is done using replace on a string in python, and therefore can cause some unexpected renaming. The dash is used to delimit the group name in the variable name and is not legal python syntax and is therefore changed into underscores after expanding.

##Calculations
This section contains the calculations performed by klee-stats. The calculations are stored as key value pairs where the values are a list of equations using the variables defined in the input as well as newly defined variables within the equations. Entries can be referred to by other entries using the notation @key. For example, in the defualt config file, there is a key **cov** with the value **cov = 100 * SCov / (SCov + SUnc), bcov = 100 * (2 * BFull + BPart) / (2 * BTot), icount = SCov + SUnc**. This is used by both the _All_ and _More_ key by having @cov in their list of equations. Furthermore an equation can be a mapping onto a group. To read more about this, look at the group section.

##Output
This section specifies the data that appears in the outputted rows. The row is specified as a tuble containing variables. The variables referenced in this section can be derived variables from the calculation section or the raw inputs themselves.

##Labels
This section contains the corresponding text label for a value. The value in this section must match the corresponding output value. They will appear at the top of the column for the corresponding values.

##Help
This section defines what appears in the help menu when klee-stats is run with this config file.

##Default
When no flags are given to klee-stats, the defaults are assumed. These are denoted in the config sections using the default field.
