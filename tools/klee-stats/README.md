#KLEE-Stats

##Info
klee-stats is a script to help interpret data outputted by klee. It is used with a corresponding config file and heavily uses Python's eval and exec capabilities. The config file follows the Windows INI structure however in the key-value pairs, the values are stored as segments of Python code.

To use a custom config file, it must be named override.config and must be in the current working directory from where klee-stats was run.

##Prerequisites
As this script uses python, python must be installed. Furthermore, it uses the tabulate library therefore it depends on tabulate being installed. This can be done by installing tabulate from the pip manager, as the root user. The following line should install tabulate if tabulate is not installed:

```
sudo pip install tabulate
```

#How to create a new config

### 1. Add Input information about the output of KLEE 
First thing we need to do is add the version information for this config file. Currently the KLEE output version is 1 so we must add this under the Info header under the **Version** field, as follows: 
```
[Info]
Version: 1
```

The next thing the config file must have is information about the input provided by Klee. This requires that we add in a **Record** field and a **Stats** field under out Input header. The corresponding section for the current output of Klee and the one we will be using is as follows: 

```
[Input]  
Record: I, BFull, BPart, BTot, T, St, Mem, QTot, QCon,\
        _, Treal, SCov, SUnc, _, Ts, Tcex, Tf, Tr
Stats: maxMem, avgMem, maxStates, avgStates
```
We should also add the version of klee-stats currently being run. This need

### 2. Add Output Labels

A specific configuration must have corresponding values in the Output and Labels headers. A configuration will be identified by the flags which will be used to run it during execution. 
For example, to create a configuration for relative times we will identify it by Reltime and its corresponding flag will be `--print-reltime`. This identifier will be used as the field value in many headers.
In the Labels header, the corresponding value is stored as identifier`_labels`. So for Reltime, it would be **Reltime_labels**. The value for this section is a tuple corresponding to the output labels that this configuration will produce. These labels appear at the top of the table outputted by klee-stats. The following Labels section for Reltime is as follows:

```
[Labels]
Reltime_labels: ('Path', 'Time(s)', 'TUser(%)', 'TSolver(%)',
                  'Tcex(%)', 'Tfork(%)', 'TResolve(%)')
```

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

