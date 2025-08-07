# Smart-Grid-Energy-Management
A simple C++ console-based group project that demonstrates algorithmic problem-solving using three core approaches: Dynamic Programming, Divide and Conquer, and Greedy Algorithms, applied to realistic smart grid energy management tasks.
. Developed as part of a course assignment to demonstrate understanding of these fundamental techniques. Future improvements may be added based on feedback or extended requirements.

---

## Algorithms Implemented

| Strategy              | Function              | Algorithm Used                      | Purpose                                                                 |
|-----------------------|-----------------------|--------------------------------------|-------------------------------------------------------------------------|
| Divide and Conquer    | `forecastDemand`      | Recursive Averaging + Heuristic      | Forecasts hourly energy demand based on historical data                |
| Dynamic Programming   | `optimizeEnergyDispatch` | Cost-Minimizing Dispatch             | Optimally schedules energy dispatch to minimize cost and balance load  |
| Greedy Algorithm      | `scheduleMaintenance` | Uptime-Based Greedy Selection        | Schedules maintenance while preserving minimum grid capacity           |


## Features

- Load forecasting using time-adjusted recursive smoothing
- Cost-optimized energy dispatch planning with battery usage
- Maintenance scheduling based on generator uptime
- Emergency rescheduling to restore capacity
- Modular, maintainable code structure using C++17 features


## Technologies Used

- Language: **C++**
- Standard: **C++17**
- Input: Predefined data arrays
- Output: Console-based output with detailed steps and final results


## Requirements to Run

- A C++ compiler supporting *C++17* standard or higher (e.g., GCC 7+, Clang 5+, MSVC 2017+)
- Any modern IDE or code editor such as *Code::Blocks*, *Visual Studio Code*, or *CLion*
- Command-line access to compile and run the program (optional but recommended)



## Team Members and Contributions

- **Nusrath Jahan Shawon**  
  GitHub: *Shawon-NusrathJahan*  
  Responsibility: Divide and Conquer

- **Suraia Tabassoom Ruhi**  
  GitHub: *std1305*  
  Responsibility: Dynamic Programming

- **Sumaia Tarannoom Mahi**  
  GitHub: *std2003*  
  Responsibility: Greedy Algorithm
  

## Future Enhancements

* Visualize energy dispatch using charts
* Implement additional algorithms for each strategy
* Extend to real-time monitoring and adjustments
* Allow real-time input via the console or config file
* Optional: Add a GUI (Graphical User Interface) for visualization


> Notes
>* This project was developed as a group coursework assignment.
>* It serves as a demonstration of understanding algorithmic problem-solving approaches.
