# Deversifi C++ Coding challenge

This repo contains the single header starting point for the C++ coding challenge.

Rather than request participants to spend time setting up a project from scratch complete with libraries for performing REST requests and serialising JSON, the DvfSimulator class provides a simplified trading API with a radically simplified approximation of how an exchange might operate.

```
#include "DvfSimulator.h"

int main()
{
    auto* sim = DvfSimulator::Create();
    
    auto ob = sim->GetOrderBook();
    
    // ... decide price and amount
    sim->PlaceOrder(price, amount);
    sim->CancelOrder(oid);

    return true;
}
```
