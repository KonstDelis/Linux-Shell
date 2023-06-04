# Linux Shell  
Konstantinos Delis csd4623  

Implementation of a Shell for Linux  
supported:  
  -All default bash commands done through exec sys call(eg ls, pwd, mkdir, echo ....)  
  -Extra commands: cd, exit  
  -Declaration of global variables(eg myvar="Hello World!")  
  -Pipelines of any size(eg cat file.txt|sort)  
  -Signals: ctrl+S(freeze), ctrl+Q(unfreeze) and ctrl+Z/fg  
 
note: the two extra commands, the variables and ctrl+Z/fg signals don't work in a pipeline  
note2: fg doesn't have a second parameter, it continues all child processes  

My goal was to be memory efficient and therefore the only memory I use is the initial buffer
for the input and then the entire sting parsing is done through pointing char pointers at
that initial buffer and making the neccessary alterations when needed.
In the pursuit of that point I have used realloc which I know is not recommended, however I
think is fine since the shell runs smoothly.
