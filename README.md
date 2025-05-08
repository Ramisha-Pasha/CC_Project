# CC_Project
ChoreoLang -- Compiler Construction Project


# ChoreoLang

A choreography-inspired domain-specific language (DSL) for control-flow programming. ChoreoLang abstracts traditional programming constructs into intuitive, theatrical keywords—like `ENTER`, `SPIN`, `MOVE TO`, and `ECHO`—making scripts feel like designing a performance. Ideal for teaching basic programming, simulating structured logic, or simply having fun with code flow :contentReference[oaicite:0]{index=0}:contentReference[oaicite:1]{index=1}.

## Key Features

- **Human-readable, theatrical syntax**  
- **Control flow** via labels and unconditional jumps  
- **Conditional logic** with `SPIN … THEN MOVE TO …`  
- **Loops**: `REPEAT n TIMES:` and `WHILE …:`  
- **Input/Output**: print strings or variables with `ECHO`  
- **Modular, flowchart-style thinking**
- **Initializing Arrays**

## Example Program

```choreo
ENTER x = 2
ENTER y = 3
ENTER s=9


ECCO "Initial values: "
ECCO_D x
ECCO_D y


x = x + y * 2
ECCO "After x = x + y * 2: "
ECCO_D x


SPIN x > 10 THEN MOVE TO high
 SPIN s > 4 THEN MOVE TO loop
ECCO "x is not greater than 10"
ECCO "s is not greater than 4 "
MOVE TO after_spin

high:
  ECCO "x exceeded 10!"



loop:
    ECCO "s exceeded 4 "
    ENTER i=0
    REPEAT 4 TIMES
        REPEAT 3 TIMES
          i = i+1
          ECCO "hello g, i am inside nested loop with iteration "
          ECCO_D i
          ENDREPEAT

    ECCO "Loop iteration: "
    ECCO_D x
    x = x + 1
    ENDREPEAT

after_spin:
ECCO "Final x: "
ECCO_D x

ENSEMBLE arr[5]
ENTER i = 0
REPEAT 5 TIMES
    arr[i] = 10 + 2 * i
    ECCO_D arr[i]
    i = i + 1
ENDREPEAT
EXIT
```


## Syntax Reference

| Syntax                          | Description                               |
|---------------------------------|-------------------------------------------|
| `ENTER x = 5`                   | Declare and initialize a variable         |
| `SPIN x > y THEN MOVE TO lbl`   | Conditional jump if the condition is true |
| `MOVE TO lbl`                   | Unconditional jump to a label             |
| `ECHO "message"`                | Print a string                            |
| `ECHO x`                        | Print the value of a variable             |
| `label:`                        | Define a jump label                       |
| `REPEAT n TIMES:`               | Repeat the enclosed block _n_ times       |
| `WHILE x > 0:`                  | Repeat block while the condition holds    |
| `EXIT`                          | End the program                           | :contentReference[oaicite:6]{index=6}:contentReference[oaicite:7]{index=7}

## Installation

```bash
# Build the compiler
make
```

```bash
# Build and run on your .choreo script:
make run input=your_script.choreo
```

## MVP
As written in the proposal, we implemented **basic if-else, loops, assignment and binary operations.**

Additional work includes **initializing arrays, support for nested conditionals and nested loops!**

# Contributors:
- Ramisha Kamal Pasha -- Lead
- Muhammad Aneeb ur Rehman (22L-6808)
- Muhammad Hussain Zafar (22L-6940) 
