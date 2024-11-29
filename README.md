# AstraSim + NS3 Updated Files

This repository contains updated source files for integrating and modifying the interaction between **AstraSim** and **NS3**, specifically tailored for routing table and flow path generation, as well as collective workload handling. These files are part of a larger platform and are provided **as-is** for reference and potential further development.

## **Included Files**

1. **`entry.h`**
   - Defines the interaction between AstraSim's system layer and the NS3 simulation layer.
   - Implements collective workload registration and management.
   - Facilitates the generation of routing tables and flow paths.

2. **`ASTRASimNetwork.cc`**
   - Implements the `ASTRASimNetwork` class, which bridges AstraSim's `AstraNetworkAPI` with NS3.
   - Updates include:
     - Removal of the NS3 simulation run logic.
     - Support for collective workloads.
     - Simplified time and scheduling functions in the absence of simulation.

3. **`common.h`**
   - Contains common utility functions and structures for network setup, topology configuration, and routing table generation.
   - Updates include:
     - Logic for pre-simulation routing table and flow path generation.
     - Iterative functions for extracting and saving routing details.

## **Key Features**
- **Collective Workload Support**:
  - Workloads can now be registered as a single event instead of segmented events.
  - This simplifies the logic and avoids dependency on continuous NS3 event simulation.

- **Routing Table and Flow Path Generation**:
  - Routing tables and flow paths are generated pre-simulation and stored in separate files (`routing_table.txt` and `flow_paths.txt`).

- **No-Simulation Workflow**:
  - NS3 simulation (`Simulator::Run()`) is entirely removed, making the platform suitable for pre-simulation analysis and testing.

## **Usage**

### **Setup and Integration**
These files are meant to integrate with an existing AstraSim + NS3 platform. Replace the corresponding files in your project directory with the updated versions provided here. 

### **Workflow**
1. Ensure your configuration files (network, workload, etc.) are correctly set up.
2. Compile the project using your platform's build system (e.g., `make`, `CMake`).
3. Run the application, ensuring routing table and flow path files are generated.

### **Important Notes**
- These files are **not standalone** and depend on the rest of the AstraSim + NS3 platform. They are provided for reference and may not compile or function as-is.
- Ensure compatibility with your existing platform before integrating these changes.

## **Acknowledgments**
- **AstraSim**: A simulator for scalable, distributed deep learning systems.
- **NS3**: A discrete-event network simulator for internet systems.
- The modifications here were made to enhance functionality and simplify workflows within this integrated platform.

## **License**
The original platform code is subject to the licensing terms of **AstraSim** and **NS3**. Modifications provided here inherit those terms and are shared for research and educational purposes only.

## **Disclaimer**
This repository contains partial modifications and is not a complete implementation of the AstraSim + NS3 platform. These files are primarily intended for **reference** and **further development**. The provided code is **untested as a standalone entity** and may require adjustments to work with your specific setup.
