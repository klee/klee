#KLEE-Stats
=============================
##Info
klee-stats is a script to help interpret data outputted by klee. It is used with a corresponding config file and heavily uses Python's eval and exec capabilities. The config file follows the Windows INI structure however in the key-value pairs, the values are stored as segments of Python code.

##How to create a new config

### 1. Add Input information about the output of KLEE 
The first thing the config file must have is information about the input provided by Klee. This requires that we add in a **Record** field and a **Stats** field under out Input header. The corresponding section for the current output of Klee and the one we will be using is as follows: 

```
[Input]  
Record: I, BFull, BPart, BTot, T, St, Mem, QTot, QCon,\
        _, Treal, SCov, SUnc, _, Ts, Tcex, Tf, Tr
Stats: maxMem, avgMem, maxStates, avgStates
```

### 2. Add Output Labels

A specific configuration must have corresponding values in the Output and Labels headers. A configuration will be identified by the flags which will be used to run it during execution. 
For example, to create a configuration for relative times we will identify it by Reltime and its corresponding flag will be `--print-reltime`. This identifier will be used as the field value in many headers.
In the Labels header, the corresponding value is stored as identifier`_labels`. So for Reltime, it would be **Reltime_labels**. The value for this section is a tuple corresponding to the output labels that this configuration will produce. These labels appear at the top of the table outputted by klee-stats. The following Labels section for Reltime is as follows:
`[Labels]
Reltime_labels: ('Path', 'Time(s)', 'TUser(%)', 'TSolver(%)',
                  'Tcex(%)', 'Tfork(%)', 'TResolve(%)')`

### 3. Add Calculation Information (including groups)

In the output section we want to adjust several timings into relative timings. We can use a group to map a declaration to many variables. To create a group, we must declare it as a field under the Groups header. We will call this field **time** and it will contain the variables which we want to be relative. This is declared as follows:

```
[Groups]  
time: T, Ts, Tcex, Tf, Tr
```

Now to use this group, we must include it in one of our equations in the Calculations section. In the calculations section, we must have a corresponding field for our configuration. The value of this field is a list of equations we want to to be evaluated that will manipulate our input data. For **Reltime** we want the mapping to occur on the variables in the group time. We want them all to be multiplied by 100 and then divided by Treal, so we write the following:

```
[Calculations]  
Reltime: rel-time = 100 * time / Treal
```

The name given to these mapped values must include the group name delimitted by '-'. This will expand into rel_T = 100 * T / Treal, rel_Ts = 100 * Ts / Treal, ... If you wnat to know more about equation cabailities, please read the Calculations section below.

### 4. Add Output information

After having our calculations defined, we can now define what variables we want to output. The variables we can chose from are those calculated in the Calculations section or those from the input. We want to output the new relative values, and we want to match the pattern we defined before in our Output Labels section. This section appears as follows: 

```
[Output]  
Reltime: (Treal, rel_T, rel_Ts, rel_Tcex, rel_Tf, rel_Tr)
```

Note, we are using the derived values after the mapping of the group.

### 5. (Optional) Add Help information

It is possible to add help information to appear during klee-stats after it has found this file. This information will appear next to the appropriate flag after running klee-stats with -h. For Reltime we have as follows:

```
[Help]  
Reltime: Print only values of measured times. Values are relative to the measured system execution time.
```

##Config File


Note: The values in the following sections are expected to be lists seperated with ', ': Groups and Calculation
Key Value pairs in the config file are stored as 

key: value 

in each of the sections of the config file.

###Info
This section is used by the klee-stats script to store version information.
##Input
This is where the format of the input data is defined. In this section there must be a defintion for Record. In the default config, the definitino for Record is as follows

Record: I, BFull, BPart, BTot, T, St, Mem, QTot, QCon,\
        _, Treal, SCov, SUnc, _, Ts, Tcex, Tf, Tr

Which is meant to match the pattern of the CSVs in the run.stats file. This is also where the variables used in the Calculations section are defined.

###Groups
This section can be used to define a group of variables which can be used in the calculation section. In the defualt config file there is a group definition as follows:

time: T, Ts, Tcex, Tf, Tr

This group in the calculation section can be used to apply a mapping onto the values in the group For example in the default config there is the following line:

 rel-time = 100 * time / Treal

This expands into **rel_T = 100 * T / Treal, rel_Ts = 100 * Ts / Treal**, and so on. Note that this is done using replace on a string in python, and therefore can cause some unexpected renaming. The dash is used to delimit the group name in the variable name and is not legal python syntax and is therefore changed into underscores after expanding.

###Calculations
This section contains the calculations performed by klee-stats. The calculations are stored as key value pairs where the values are a list of equations using the variables defined in the input as well as newly defined variables within the equations. Entries can be referred to by other entries using the notation @key. For example, in the defualt config file, there is a key **cov** with the value **cov = 100 * SCov / (SCov + SUnc), bcov = 100 * (2 * BFull + BPart) / (2 * BTot), icount = SCov + SUnc**. This is used by both the _All_ and _More_ key by having @cov in their list of equations. Furthermore an equation can be a mapping onto a group. To read more about this, look at the group section.

###Output
This section specifies the data that appears in the outputted rows. The row is specified as a tuble containing variables. The variables referenced in this section can be derived variables from the calculation section or the raw inputs themselves.

###Labels
This section contains the corresponding text label for a value. The value in this section must match the corresponding output value. They will appear at the top of the column for the corresponding values.

###Help
This section defines what appears in the help menu when klee-stats is run with this config file.
