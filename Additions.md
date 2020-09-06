### Symbolic Tree 

Showing a Execution Tree for Probabilistic Symbolic Execution example modifying PTREE generation.

### Starting Example 

```cpp
#include <stdio.h>
#include <assert.h>
#include <random>
#include <klee/klee.h>
std::default_random_engine generator;

/**
 * switch => door_switch
*/
bool montyhall(bool door_switch)
{
    std::uniform_int_distribution<int> distribution(1, 3);

    int host_door = 0;
    int car_door = distribution(generator);
    int choice = distribution(generator);

    klee_make_symbolic(&choice, sizeof(choice), "choice_sym");
    klee_make_symbolic(&car_door, sizeof(car_door), "car_door_sym");
    klee_make_symbolic(&host_door, sizeof(host_door), "host_door_sym");

    /**
     * Based on car door and choice, choose a host door. 
    */
    if (car_door != 1 && choice != 1)
    {
        host_door = 1;
    }
    else if (car_door != 2 && choice != 2)
    {
        host_door = 2;
    }
    else
    {
        host_door = 3;
    }

    /**
     * Based door_switch and host_door, change choices. 
    */
    if (door_switch)
    {
        if (host_door == 1)
        {
            if (choice == 2)
            {
                choice = 3;
            }
            else
            {
                choice = 2;
            }
        }
        else if (host_door == 2)
        {
            if (choice == 1)
            {
                choice = 3;
            }
            else
            {
                choice = 1;
            }
        }
        else
        {
            if (choice == 1)
            {
                choice = 2;
            }
            else
            {
                choice = 1;
            }
        }
    }

    if (choice == car_door)
    {
        return true;
    }
    else
    {
        return false;
    }

    return true;
}

int main()
{
    int choice = 0;
    bool door_switch = false;

    klee_make_symbolic(&door_switch, sizeof(door_switch), "door_switch_sym");
    return montyhall(choice, door_switch);
}
```