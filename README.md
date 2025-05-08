# CC_Project
ChoreoLang -- Compiler Construction Project


# ChoreoLang

A choreography-inspired domain-specific language (DSL) for control-flow programming. ChoreoLang abstracts traditional programming constructs into intuitive, theatrical keywords—like `ENTER`, `SPIN`, `MOVE TO`, and `ECHO`—making scripts feel like designing a performance. Ideal for teaching basic programming, simulating structured logic, or simply having fun with code flow :contentReference[oaicite:0]{index=0}:contentReference[oaicite:1]{index=1}.

## Key Features

- **Human-readable, theatrical syntax**  
- **Control flow** via labels and unconditional jumps  
- **Conditional logic** with `SPIN … THEN MOVE TO …`  
- **Loops**: `REPEAT n TIMES:` and `WHILE …:`  
- **Functions**: define with `DANCE …:`, call with `CALL`, and return with `RETURN`  
- **Input/Output**: print strings or variables with `ECHO`  
- **Modular, flowchart-style thinking** :contentReference[oaicite:2]{index=2}:contentReference[oaicite:3]{index=3}.

## Example Program

```choreo
ENTER x = 5
ENTER y = 10

SPIN x < y THEN MOVE TO act1
ECHO "No spin needed"
MOVE TO finale

act1:
    ECHO "Twirl begins"
    x = x + y
    CALL add_more
    MOVE TO finale

DANCE add_more:
    ECHO "Adding more twirls"
    REPEAT 3 TIMES:
        ECHO x
        x = x + 1
    RETURN

finale:
    WHILE x > 0:
        ECHO x
        x = x - 5
    ECHO "Final pose"
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

Additional work includes **initializing arrays, adding function support, support for nested conditionals and nested loops!**

# Contributors:
- Ramisha Kamal Pasha -- Lead
- Muhammad Aneeb ur Rehman (22L-6808)
- Muhammad Hussain Zafar (22L-6940) 
